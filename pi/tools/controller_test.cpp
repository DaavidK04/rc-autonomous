#include <SDL2/SDL.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <string>

// Hilfsfunktion zum "Säubern" des Terminals (Cursor nach oben links)
void resetCursor() {
    std::cout << "\033[H" << std::flush;
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) return 1;

    SDL_GameController* controller = nullptr;
    
    // Arrays zum Speichern der aktuellen Zustände
    int axes[6] = {0};       // LX, LY, RX, RY, L2, R2
    bool buttons[15] = {false}; // Cross, Circle, Square, Triangle, etc.

    // Namen der Buttons für die Tabelle
    std::string btnNames[] = {
        "A (Cross)   ", "B (Circle)  ", "X (Square)  ", "Y (Triangle)", 
        "Back        ", "Guide       ", "Start       ", "L-Stick Push", 
        "R-Stick Push", "L1-Bumper   ", "R1-Bumper   ", "DPAD Up     ", 
        "DPAD Down   ", "DPAD Left   ", "DPAD Right  "
    };

    std::cout << "\033[2J"; // Einmalig den Bildschirm löschen

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;

            if (event.type == SDL_CONTROLLERDEVICEADDED) {
                controller = SDL_GameControllerOpen(event.cdevice.which);
            }

            // Achsen-Werte aktualisieren und skalieren
            if (event.type == SDL_CONTROLLERAXISMOTION) {
                if (event.caxis.axis < 6) {
                    axes[event.caxis.axis] = event.caxis.value / 256;
                }
            }

            // Button-Zustände aktualisieren
            if (event.type == SDL_CONTROLLERBUTTONDOWN) buttons[event.cbutton.button] = true;
            if (event.type == SDL_CONTROLLERBUTTONUP)   buttons[event.cbutton.button] = false;
        }

        // --- DAS DASHBOARD ZEICHNEN ---
        resetCursor();
        std::cout << "========== PS5 CONTROLLER DASHBOARD ==========" << std::endl;
        std::cout << "Status: " << (controller ? "VERBUNDEN" : "SUCHE...") << std::endl;
        std::cout << "----------------------------------------------" << std::endl;

        // Tabelle für Achsen (Sticks)
        std::cout << " ACHSEN (Bereich -128 bis 128)" << std::endl;
        std::cout << " Linker Stick:  X: " << std::setw(4) << axes[0] << "  Y: " << std::setw(4) << axes[1] << std::endl;
        std::cout << " Rechter Stick: X: " << std::setw(4) << axes[2] << "  Y: " << std::setw(4) << axes[3] << std::endl;
        std::cout << " Trigger:       L2: " << std::setw(4) << (axes[4]+128) << " R2: " << std::setw(4) << (axes[5]+128) << std::endl;
        std::cout << "----------------------------------------------" << std::endl;

        // Tabelle für Buttons
        std::cout << " BUTTONS" << std::endl;
        for (int i = 0; i < 15; i++) {
            std::cout << " " << btnNames[i] << ": [" << (buttons[i] ? "X" : " ") << "]  ";
            if ((i + 1) % 3 == 0) std::cout << std::endl; // Alle 3 Buttons eine neue Zeile
        }
        std::cout << "\n----------------------------------------------" << std::endl;
        std::cout << " Beenden mit [STRG + C]" << std::endl;

        SDL_Delay(10); // CPU entlasten (100 FPS reichen völlig)
    }

    if (controller) SDL_GameControllerClose(controller);
    SDL_Quit();
    return 0;
}