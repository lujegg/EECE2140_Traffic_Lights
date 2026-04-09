// EECE2140 - Group Project
// Team 9: Hayden Trent, Wenxuan Liang, Jack Lu
// Description: Intersection traffic light controller simulation

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <limits>

using namespace std;

// Signal states
enum class CarSignal {
    Red, Yellow, Green,
    RedArrow, YellowArrow, GreenArrow,
    FlashingRed, FlashingRedArrow
};

enum class PedSignal {
    DontWalk, Walk, FlashingDontWalk, Flashing
};

// Cycle phases for the state machine
enum class Phase {
    NS_Left_Green, NS_Left_Yellow, NS_Left_AllRed,
    NS_Through_Green, NS_Through_Yellow, AllRed_To_EW,
    EW_Left_Green, EW_Left_Yellow, EW_Left_AllRed,
    EW_Through_Green, EW_Through_Yellow, AllRed_To_NS
};

struct IntersectionState {
    CarSignal nsThrough;
    CarSignal nsLeft;
    CarSignal ewThrough;
    CarSignal ewLeft;
    PedSignal nsPed;
    PedSignal ewPed;
};

class IntersectionController {
private:
    Phase currentPhase = Phase::NS_Left_Green;
    double timeInPhase = 0.0;

    // Default timers
    double leftGreenTime = 4.0;
    double leftYellowTime = 2.0;
    double throughGreenTime = 8.0;
    double yellowTime = 3.0;
    double allRedTime = 1.0;

    bool pedWaiting = false;
    bool emergencyMode = false;
    bool powerLossMode = false; // Added to handle the power loss edge case requirement

    // Helper to switch phase
    void changePhase(Phase next) {
        currentPhase = next;
        timeInPhase = 0.0;
        // cout << "DEBUG: Phase changed to " << (int)next << endl; 
    }

    // Input validation so cin doesn't crash if someone types a letter
    int getValidIntInput(const string& prompt) {
        int value;
        string badInput;
        cout << prompt;
        while (true) {
            cin >> value;
            if (cin.fail() || value <= 0) {
                cin.clear(); // clear the error flag
                cin >> badInput; // read the invalid input as a string to remove it
                cout << "Invalid input. Please enter a positive number: ";
            } else {
                break; // input is good, exit the loop
            }
        }
        return value;
    }

public:
    void inputTimers();
    void update(double time);
    IntersectionState getState() const;
    Phase getCurrentPhase() const;

    void pressPedestrianButton() {
        if (!pedWaiting) {
            pedWaiting = true;
            cout << "\n[INFO] Pedestrian Button Pressed! Will walk on next green.\n\n";
        }
    }

    void triggerEmergency(bool status) {
        emergencyMode = status;
        if (status) {
            cout << "\n[ALERT] EMERGENCY OVERRIDE! All lights RED.\n\n";
        } else {
            cout << "\n[ALERT] Emergency clear. Resuming.\n\n";
        }
    }

    void triggerPowerLoss(bool status) {
        powerLossMode = status;
        if (status) {
            cout << "\n[SYSTEM ERROR] POWER LOSS! Flashing Red mode.\n\n";
        } else {
            cout << "\n[SYSTEM OK] Power restored.\n\n";
        }
    }

    bool isPowerLoss() const { return powerLossMode; }
    bool isEmergency() const { return emergencyMode; }
};

void IntersectionController::inputTimers() {
    leftGreenTime = getValidIntInput("Enter left green time: ");
    leftYellowTime = getValidIntInput("Enter left yellow time: ");
    throughGreenTime = getValidIntInput("Enter through green time: ");
    yellowTime = getValidIntInput("Enter through yellow time: ");
    allRedTime = getValidIntInput("Enter all-red time: ");
}

IntersectionState IntersectionController::getState() const {
    // Priority 1: Power Loss
    if (powerLossMode) {
        return {
            CarSignal::FlashingRed, CarSignal::FlashingRedArrow,
            CarSignal::FlashingRed, CarSignal::FlashingRedArrow,
            PedSignal::FlashingDontWalk, PedSignal::FlashingDontWalk
        };
    }

    // Priority 2: Emergency
    if (emergencyMode) {
        return {
            CarSignal::Red, CarSignal::RedArrow,
            CarSignal::Red, CarSignal::RedArrow,
            PedSignal::DontWalk, PedSignal::DontWalk
        };
    }

    // Normal traffic cycle
    switch (currentPhase) {
    case Phase::NS_Left_Green:
        return { CarSignal::Red, CarSignal::GreenArrow, CarSignal::Red, CarSignal::RedArrow, PedSignal::DontWalk, PedSignal::DontWalk };
    case Phase::NS_Left_Yellow:
        return { CarSignal::Red, CarSignal::YellowArrow, CarSignal::Red, CarSignal::RedArrow, PedSignal::DontWalk, PedSignal::DontWalk };
    case Phase::NS_Left_AllRed:
        return { CarSignal::Red, CarSignal::RedArrow, CarSignal::Red, CarSignal::RedArrow, PedSignal::DontWalk, PedSignal::DontWalk };
    case Phase::NS_Through_Green:
        return { CarSignal::Green, CarSignal::RedArrow, CarSignal::Red, CarSignal::RedArrow, PedSignal::Walk, PedSignal::DontWalk };
    case Phase::NS_Through_Yellow:
        return { CarSignal::Yellow, CarSignal::RedArrow, CarSignal::Red, CarSignal::RedArrow, PedSignal::FlashingDontWalk, PedSignal::DontWalk };
    case Phase::AllRed_To_EW:
        return { CarSignal::Red, CarSignal::RedArrow, CarSignal::Red, CarSignal::RedArrow, PedSignal::DontWalk, PedSignal::DontWalk };
    case Phase::EW_Left_Green:
        return { CarSignal::Red, CarSignal::RedArrow, CarSignal::Red, CarSignal::GreenArrow, PedSignal::DontWalk, PedSignal::DontWalk };
    case Phase::EW_Left_Yellow:
        return { CarSignal::Red, CarSignal::RedArrow, CarSignal::Red, CarSignal::YellowArrow, PedSignal::DontWalk, PedSignal::DontWalk };
    case Phase::EW_Left_AllRed:
        return { CarSignal::Red, CarSignal::RedArrow, CarSignal::Red, CarSignal::RedArrow, PedSignal::DontWalk, PedSignal::DontWalk };
    case Phase::EW_Through_Green:
        return { CarSignal::Red, CarSignal::RedArrow, CarSignal::Green, CarSignal::RedArrow, PedSignal::DontWalk, PedSignal::Walk };
    case Phase::EW_Through_Yellow:
        return { CarSignal::Red, CarSignal::RedArrow, CarSignal::Yellow, CarSignal::RedArrow, PedSignal::DontWalk, PedSignal::FlashingDontWalk };
    case Phase::AllRed_To_NS:
        return { CarSignal::Red, CarSignal::RedArrow, CarSignal::Red, CarSignal::RedArrow, PedSignal::DontWalk, PedSignal::DontWalk };
    }

    return { CarSignal::FlashingRed, CarSignal::FlashingRedArrow, CarSignal::FlashingRed, CarSignal::FlashingRedArrow, PedSignal::Flashing, PedSignal::Flashing };
}

void IntersectionController::update(double time) {
    if (powerLossMode || emergencyMode) return; 

    // Clear ped flag if walk sign is on
    if (currentPhase == Phase::NS_Through_Green || currentPhase == Phase::EW_Through_Green) {
        pedWaiting = false; 
    }

    timeInPhase += time;

    switch (currentPhase) {
    case Phase::NS_Left_Green:      if (timeInPhase >= leftGreenTime) changePhase(Phase::NS_Left_Yellow); break;
    case Phase::NS_Left_Yellow:     if (timeInPhase >= leftYellowTime) changePhase(Phase::NS_Left_AllRed); break;
    case Phase::NS_Left_AllRed:     if (timeInPhase >= allRedTime) changePhase(Phase::NS_Through_Green); break;
    case Phase::NS_Through_Green:   if (timeInPhase >= throughGreenTime) changePhase(Phase::NS_Through_Yellow); break;
    case Phase::NS_Through_Yellow:  if (timeInPhase >= yellowTime) changePhase(Phase::AllRed_To_EW); break;
    case Phase::AllRed_To_EW:       if (timeInPhase >= allRedTime) changePhase(Phase::EW_Left_Green); break;
    case Phase::EW_Left_Green:      if (timeInPhase >= leftGreenTime) changePhase(Phase::EW_Left_Yellow); break;
    case Phase::EW_Left_Yellow:     if (timeInPhase >= leftYellowTime) changePhase(Phase::EW_Left_AllRed); break;
    case Phase::EW_Left_AllRed:     if (timeInPhase >= allRedTime) changePhase(Phase::EW_Through_Green); break;
    case Phase::EW_Through_Green:   if (timeInPhase >= throughGreenTime) changePhase(Phase::EW_Through_Yellow); break;
    case Phase::EW_Through_Yellow:  if (timeInPhase >= yellowTime) changePhase(Phase::AllRed_To_NS); break;
    case Phase::AllRed_To_NS:       if (timeInPhase >= allRedTime) changePhase(Phase::NS_Left_Green); break;
    }
}

Phase IntersectionController::getCurrentPhase() const {
    return currentPhase;
}

string toString(CarSignal s) {
    switch (s) {
    case CarSignal::Red: return "Red";
    case CarSignal::Yellow: return "Yellow";
    case CarSignal::Green: return "Green";
    case CarSignal::RedArrow: return "RedArrow";
    case CarSignal::YellowArrow: return "YellowArrow";
    case CarSignal::GreenArrow: return "GreenArrow";
    case CarSignal::FlashingRed: return "FlashingRed";
    case CarSignal::FlashingRedArrow: return "FlashingRedArrow";
    }
    return "";
}

string toString(PedSignal s) {
    switch (s) {
    case PedSignal::DontWalk: return "DontWalk";
    case PedSignal::Walk: return "Walk";
    case PedSignal::FlashingDontWalk: return "FlashingDontWalk";
    case PedSignal::Flashing: return "Flashing";
    }
    return "";
}

char getSymbol(CarSignal s) {
    if (s == CarSignal::Green || s == CarSignal::GreenArrow) return 'o';
    if (s == CarSignal::Yellow || s == CarSignal::YellowArrow) return '/';
    if (s == CarSignal::FlashingRed || s == CarSignal::FlashingRedArrow) return '*';
    return '-'; 
}

void printState(const IntersectionState& s) {
    cout << "\n======================================\n";
    cout << "NS Traffic : [" << getSymbol(s.nsThrough) << " " << getSymbol(s.nsThrough) << "] Through | [" << getSymbol(s.nsLeft) << "] Left\n";
    cout << "EW Traffic : [" << getSymbol(s.ewThrough) << " " << getSymbol(s.ewThrough) << "] Through | [" << getSymbol(s.ewLeft) << "] Left\n";
    cout << "--------------------------------------\n";
    cout << "NS Through: " << toString(s.nsThrough) << "\n"; 
    cout << "NS Left   : " << toString(s.nsLeft) << "\n";
    cout << "EW Through: " << toString(s.ewThrough) << "\n";
    cout << "EW Left   : " << toString(s.ewLeft) << "\n";
    cout << "NS Ped    : " << toString(s.nsPed) << "\n";
    cout << "EW Ped    : " << toString(s.ewPed) << "\n";
    cout << "======================================\n\n";
}

int main() {
    IntersectionController controller;
    
    cout << "--- Traffic Light Controller Initialization ---\n";
    controller.inputTimers();

    auto programStartTime = chrono::steady_clock::now();
    auto lastTime = programStartTime;
    
    bool lastEmergencyState = false;
    bool lastPowerLossState = false; 
    Phase lastPrintedPhase = controller.getCurrentPhase();

    printState(controller.getState());

    int loopCounter = 0; // Using a counter for demo events is much more reliable

    while (true) {
        auto currentTime = chrono::steady_clock::now();
        chrono::duration<double> elapsed = currentTime - lastTime;
        lastTime = currentTime;
        
        // Hardcoded simulation events based on loops rather than exact float seconds
        // (Assuming ~50ms per loop: 20 loops approx 1 sec)
        if (loopCounter == 200) {
            controller.pressPedestrianButton();
        } else if (loopCounter == 400) {
            controller.triggerEmergency(true);
        } else if (loopCounter == 500) {
            controller.triggerEmergency(false);
        } else if (loopCounter == 600) {
            controller.triggerPowerLoss(true);
        } else if (loopCounter == 700) {
            controller.triggerPowerLoss(false);
        }
        loopCounter++;

        controller.update(elapsed.count());

        bool currentEmergencyState = controller.isEmergency();
        bool currentPowerLossState = controller.isPowerLoss();

        if (controller.getCurrentPhase() != lastPrintedPhase || 
            currentEmergencyState != lastEmergencyState ||
            currentPowerLossState != lastPowerLossState) {
            
            lastPrintedPhase = controller.getCurrentPhase();
            lastEmergencyState = currentEmergencyState;
            lastPowerLossState = currentPowerLossState;
            
            printState(controller.getState());
        }

        this_thread::sleep_for(chrono::milliseconds(50));
    }

    return 0;
}
