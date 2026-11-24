#include <iostream>
#include <pigpio.h>

// Deine verifizierte Belegung
const int LF_IN1 = 17, LF_IN2 = 27; // Vorne Links
const int LR_IN1 = 23, LR_IN2 = 22; // Hinten Links
const int RF_IN1 = 24, RF_IN2 = 25; // Hinten Rechts
const int RR_IN1 = 6,  RR_IN2 = 5;  // Vorne Rechts

const int PWM_FRONT = 12;
const int PWM_REAR  = 13;

int main() {
    if (gpioInitialise() < 0) return 1;

    // Alle Pins als Ausgang konfigurieren
    int pins[] = {LF_IN1, LF_IN2, LR_IN1, LR_IN2, RF_IN1, RF_IN2, RR_IN1, RR_IN2, PWM_FRONT, PWM_REAR};
    for (int p : pins) gpioSetMode(p, PI_OUTPUT);

    std::cout << "🔥 VOLLGAS in 3... 2... 1..." << std::endl;
    gpioDelay(1000000); // 1 Sekunde Vorbereitungszeit

    // --- ALLE MOTOREN AUF VORWÄRTS ---
    gpioWrite(LF_IN1, 1); gpioWrite(LF_IN2, 0);
    gpioWrite(LR_IN1, 1); gpioWrite(LR_IN2, 0);
    gpioWrite(RF_IN1, 1); gpioWrite(RF_IN2, 0);
    gpioWrite(RR_IN1, 1); gpioWrite(RR_IN2, 0);

    // --- MAXIMUM SPEED (255) ---
    gpioPWM(PWM_FRONT, 255);
    gpioPWM(PWM_REAR, 255);

    std::cout << "🚀 RENNEN!" << std::endl;
    gpioDelay(5000000); // 5 Sekunden Vollgas fahren

    // --- STOPP ---
    std::cout << "🛑 Bremsen..." << std::endl;
    gpioPWM(PWM_FRONT, 0);
    gpioPWM(PWM_REAR, 0);
    for (int p : pins) gpioWrite(p, 0);

    gpioTerminate();
    return 0;
}