//compile: g++ rccar.cpp -o rccar -lSDL2 -lpigpio -lpthread -O2

#include <SDL2/SDL.h>
#include <pigpio.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <csignal>
#include <atomic>
#include <algorithm>

//--------------------------------------------------------------------------
//GPIO-pinout 
//
//PWM_LEFT (13): controls front-left (17/27) + rear-left (23/22)
//
//PWM_RIGHT (12): controls front-right (6/5) + rear-right (24/25)
//--------------------------------------------------------------------------

const int LF_IN1 = 17, LF_IN2 = 27; //Front left
const int LR_IN1 = 23, LR_IN2 = 22; //Rear left
const int RF_IN1 = 6, RF_IN2 = 5; //Front right				    				    				   
const int RR_IN1 = 24, RR_IN2 = 25; //Rear right
const int PWM_LEFT = 13;
const int PWM_RIGHT = 12;

// -----------------------------------------
// Control parameters
// -----------------------------------------
const int DEADZONE = 8;
const float STEERING_FACTOR = 0.7f;
const int MIN_PWM = 35;
const float TRIGGER_SCALE = 1.0f;
const int LOOP_DELAY_MS = 10;
const int DASHBOARD_EVERY = 10;

std::atomic<bool> running(true);
void sigHandler(int) { running = false; }


// -----------------------------------------
// Helper functions
// -----------------------------------------

int applyDeadzone(int val, int dz){
	return (std::abs(val) < dz) ? 0 : val;
}


int clamp128(int v) {
	if (v > 128) return 128;
	if (v < -128) return -128;
	return v;
}


int toPWM(int v) {
	if (v == 0) return 0;
	int absV = std::min(std::abs(v), 128);
	return MIN_PWM + (int)((float)absV / 128.0f * (255 - MIN_PWM));
}

void setSide(int speed, int a1, int a2, int b1, int b2) {
	int in1 = 0, in2 = 0;
	if (speed > 0)	{ in1 = 1; in2 = 0; }
	else if (speed < 0) { in1 = 0; in2 = 1; }
	gpioWrite(a1, in1); gpioWrite(a2, in2);
	gpioWrite(b1, in1); gpioWrite(b2, in2);
}


// ------------------------------------------
// Engine control
// leftSpeed / rightSpeed: -128 (max backward) ... +128 (max foreward)
// ------------------------------------------

void drive(int leftSpeed, int rightSpeed) {
	setSide(leftSpeed, LF_IN1, LF_IN2, LR_IN1, LR_IN2);
	setSide(rightSpeed, RF_IN1, RF_IN2, RR_IN1, RR_IN2);
	gpioPWM(PWM_LEFT, toPWM(leftSpeed));
	gpioPWM(PWM_RIGHT, toPWM(rightSpeed));
}


void stopAll() {
	for (int p : { LF_IN1, LF_IN2, LR_IN1, LR_IN2,
			RF_IN1, RF_IN2, RR_IN1, RR_IN2})
		gpioWrite(p, 0);
	gpioPWM(PWM_LEFT, 0);
	gpioPWM(PWM_RIGHT, 0);
}


// ---------------------------------
// Dashboard
// ---------------------------------

void printDashboard(bool connected, int throttle, int steering,
		int leftSpeed, int rightSpeed, bool turbo, bool recording) {
	std::cout << "\033[H";
	std::cout << "=============== RC CAR CONTROLLER ===============\n";
	std::cout << "Controller : "
		<< (connected ? "\033[32mCONNECTED\033[0m" : "\033[31mSEARCHING... \033[0m")
		<< "\n";
	std::cout << "-------------------------------------------------\n";
	std::cout << " Throttle : " << std::setw(5) << throttle
		<< " Steering : " << std::setw(5) << steering << "\n";
	std::cout << " Left PWM : " << std::setw(5) << toPWM(leftSpeed)
		<< " Right PWM : " << std::setw(5) << toPWM(rightSpeed) << "\n";
	std::cout << " Turbo    : " << (turbo ? "\033[33mON \033[0m" : "OFF") << "\n";
	std::cout << " Recording    : " << (recording ? "\033[31mON \033[0m" : "OFF") << "\n";
	std::cout << "-------------------------------------------------\n";
	std::cout << " Left Stick Y -> Foreward / Backward\n";
	std::cout << " Left Stick X -> Steer\n";
	std::cout << " R2	->	Throttle foreward (alternative)\n";
	std::cout << " L2 	->	Throttle backward (alternative)\n";
	std::cout << " R1 hold	-> 	Turbo (100% PWM)\n";
	std::cout << " Options	-> 	Quit\n";
	std::cout << "------------------------------------------------\n";
	std::cout << " Quit: [Options] or Ctrl+C\n" << std::flush;
}


// ---------------------------------
// main
// ---------------------------------

int main(int argc, char* argv[]) {
	if (gpioInitialise() < 0) {
		std::cerr << "pigpio init failed - forgot sudo or pigpiod running?\n";
		return 1;
	}

	//IMPORTTANT: set gpioInitialise() AFTER signal handler,
	//otherwise pigpio overwrites them and stopAll() will never be called when pressing ctrl+c-
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);

	for (int p : {LF_IN1, LF_IN2, LR_IN1, LR_IN2,
			RF_IN1, RF_IN2, RR_IN1, RR_IN2, PWM_LEFT, PWM_RIGHT})
		gpioSetMode(p, PI_OUTPUT);
	stopAll();

	if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
		std::cerr << "SDL init failed: " << SDL_GetError() << "\n";
		gpioTerminate();
		return 1;
	}

	std::cout << "\033[2J\033[?25l"; 	//empty screen, hide cursor
						
	SDL_GameController* controller = nullptr;
	SDL_Event event;

	int axisLX = 0, axisLY = 0;
	int axisL2 = 0, axisR2 = 0;	//Trigger: 0..127
	bool turbo = false;
	bool recording = false;
	unsigned long frame = 0;

	while (running) {
		// Events
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					running = false;
					break;

				case SDL_CONTROLLERDEVICEADDED:
					if (!controller)
						controller = SDL_GameControllerOpen(event.cdevice.which);
					break;

				case SDL_CONTROLLERDEVICEREMOVED:
					if (controller &&
						event.cdevice.which == SDL_JoystickInstanceID(
							SDL_GameControllerGetJoystick(controller))) {
						SDL_GameControllerClose(controller);
					controller = nullptr;
					//reset axis, so no old value is stuck
					
					axisLX = axisLY = axisL2 = axisR2 = 0;
					turbo = false;
					}
					break;

				case SDL_CONTROLLERAXISMOTION:
					switch (event.caxis.axis) {
						case SDL_CONTROLLER_AXIS_LEFTX:
							axisLX = event.caxis.value / 256;	// -128..127
							break;
						case SDL_CONTROLLER_AXIS_LEFTY:
							axisLY = event.caxis.value / 256;
							break;
						case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
							axisL2 = event.caxis.value / 256;
							break;
						case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
							axisR2 = event.caxis.value / 256;
							break;
						default: break;
						}
						break;
				case SDL_CONTROLLERBUTTONDOWN:
						if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
							turbo = true;
						if (event.cbutton.button == SDL_CONTROLLER_BUTTON_START)
							running = false;
						if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
							recording = !recording;
						break;

				case SDL_CONTROLLERBUTTONUP:
						if (event.cbutton.button == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)
							turbo = false;
						break;

				default: break;
				}
		}
		
		// compute steering
		int lx = applyDeadzone(axisLX, DEADZONE);
		int ly = applyDeadzone(axisLY, DEADZONE);

		// SDL Y-axis: up = negaitve -> invert so foreward = positive
		int throttle = -ly;
		int steering = lx; //positive = stick to the right

		// trigger are prioritized when pressed
		int r2 = (int)(axisR2 * TRIGGER_SCALE);
		int l2 = (int)(axisL2 * TRIGGER_SCALE);
		if (r2 > DEADZONE || l2 > DEADZONE)
			throttle = r2 - l2;


		float fL = (float)throttle + (float)steering * STEERING_FACTOR;
		float fR = (float)throttle - (float)steering * STEERING_FACTOR;

		float maxVal = std::max(std::abs(fL), std::abs(fR));
		if (maxVal > 128.0f) {
			fL = fL / maxVal * 128.0f;
			fR = fR / maxVal * 128.0f;
		}

		int leftSpeed = clamp128((int)fL);
		int rightSpeed = clamp128((int)fR);

		// turbo: keep the sign, val to max
		if (turbo) {
			if (leftSpeed != 0) leftSpeed = (leftSpeed > 0) ? 128 : -128;
			if (rightSpeed != 0) rightSpeed = (rightSpeed > 0) ? 128 : -128;
		}

		// engines

		if (controller) {
			drive(leftSpeed, rightSpeed);
		} else {
			stopAll();
		}

		if (frame++ % DASHBOARD_EVERY == 0) {
			printDashboard(controller != nullptr,
					throttle, steering,
					leftSpeed, rightSpeed, turbo, recording);
		}

		SDL_Delay(LOOP_DELAY_MS);
	}


	stopAll();
	if (controller) SDL_GameControllerClose(controller);
	SDL_Quit();
	gpioTerminate();

	std::cout << "\033[?25h\033[2J\033[H"; // show cursor again
	std::cout << "RC car stopped";
	return 0;
}

