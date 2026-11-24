// motor_diagnose.cpp
// Testet jeden Motor einzeln – damit wir wissen welcher Pin welche Seite ist
// Kompilieren: g++ motor_diagnose.cpp -o motor_diagnose -lpigpio -lpthread
// Starten:     sudo ./motor_diagnose

#include <iostream>
#include <pigpio.h>

const int LF_IN1 = 17, LF_IN2 = 27;   // "Vorne Links"
const int LR_IN1 = 23, LR_IN2 = 22;   // "Hinten Links"
const int RF_IN1 = 24, RF_IN2 = 25;   // "Hinten Rechts"
const int RR_IN1 = 6,  RR_IN2 = 5;    // "Vorne Rechts"
const int PWM_A = 12;   // ehemals PWM_FRONT
const int PWM_B = 13;   // ehemals PWM_REAR

void stopAll() {
    for (int p : {LF_IN1, LF_IN2, LR_IN1, LR_IN2,
                  RF_IN1, RF_IN2, RR_IN1, RR_IN2})
        gpioWrite(p, 0);
    gpioPWM(PWM_A, 0);
    gpioPWM(PWM_B, 0);
}

void waitEnter(const std::string& msg) {
    std::cout << msg << "  → ENTER drücken\n";
    std::cin.ignore();
}

int main() {
    if (gpioInitialise() < 0) {
        std::cerr << "pigpio init fehlgeschlagen – sudo vergessen?\n";
        return 1;
    }

    for (int p : {LF_IN1, LF_IN2, LR_IN1, LR_IN2,
                  RF_IN1, RF_IN2, RR_IN1, RR_IN2, PWM_A, PWM_B})
        gpioSetMode(p, PI_OUTPUT);

    stopAll();

    std::cout << "=== MOTOR DIAGNOSE ===\n";
    std::cout << "Notiere dir bei jedem Test WELCHER Motor dreht und in welche Richtung.\n\n";

    // ── Test 1: LF (GPIO17/27) mit PWM_A ──
    waitEnter("TEST 1: LF_IN1=17 + LF_IN2=27 vorwärts, PWM_A=12");
    gpioWrite(LF_IN1, 1); gpioWrite(LF_IN2, 0);
    gpioPWM(PWM_A, 180);
    waitEnter("  Welcher Motor dreht? Vorwärts oder rückwärts?");
    stopAll(); gpioDelay(500000);

    // ── Test 2: LF mit PWM_B ──
    waitEnter("TEST 2: LF_IN1=17 + LF_IN2=27 vorwärts, PWM_B=13");
    gpioWrite(LF_IN1, 1); gpioWrite(LF_IN2, 0);
    gpioPWM(PWM_B, 180);
    waitEnter("  Welcher Motor dreht? Vorwärts oder rückwärts?");
    stopAll(); gpioDelay(500000);

    // ── Test 3: LR (GPIO23/22) mit PWM_A ──
    waitEnter("TEST 3: LR_IN1=23 + LR_IN2=22 vorwärts, PWM_A=12");
    gpioWrite(LR_IN1, 1); gpioWrite(LR_IN2, 0);
    gpioPWM(PWM_A, 180);
    waitEnter("  Welcher Motor dreht?");
    stopAll(); gpioDelay(500000);

    // ── Test 4: LR mit PWM_B ──
    waitEnter("TEST 4: LR_IN1=23 + LR_IN2=22 vorwärts, PWM_B=13");
    gpioWrite(LR_IN1, 1); gpioWrite(LR_IN2, 0);
    gpioPWM(PWM_B, 180);
    waitEnter("  Welcher Motor dreht?");
    stopAll(); gpioDelay(500000);

    // ── Test 5: RF (GPIO24/25) mit PWM_A ──
    waitEnter("TEST 5: RF_IN1=24 + RF_IN2=25 vorwärts, PWM_A=12");
    gpioWrite(RF_IN1, 1); gpioWrite(RF_IN2, 0);
    gpioPWM(PWM_A, 180);
    waitEnter("  Welcher Motor dreht?");
    stopAll(); gpioDelay(500000);

    // ── Test 6: RF mit PWM_B ──
    waitEnter("TEST 6: RF_IN1=24 + RF_IN2=25 vorwärts, PWM_B=13");
    gpioWrite(RF_IN1, 1); gpioWrite(RF_IN2, 0);
    gpioPWM(PWM_B, 180);
    waitEnter("  Welcher Motor dreht?");
    stopAll(); gpioDelay(500000);

    // ── Test 7: RR (GPIO6/5) mit PWM_A ──
    waitEnter("TEST 7: RR_IN1=6 + RR_IN2=5 vorwärts, PWM_A=12");
    gpioWrite(RR_IN1, 1); gpioWrite(RR_IN2, 0);
    gpioPWM(PWM_A, 180);
    waitEnter("  Welcher Motor dreht?");
    stopAll(); gpioDelay(500000);

    // ── Test 8: RR mit PWM_B ──
    waitEnter("TEST 8: RR_IN1=6 + RR_IN2=5 vorwärts, PWM_B=13");
    gpioWrite(RR_IN1, 1); gpioWrite(RR_IN2, 0);
    gpioPWM(PWM_B, 180);
    waitEnter("  Welcher Motor dreht?");
    stopAll();

    std::cout << "\n=== DIAGNOSE ABGESCHLOSSEN ===\n";
    std::cout << "Schick mir die Ergebnisse und ich fixe den finalen Code!\n";

    gpioTerminate();
    return 0;
}