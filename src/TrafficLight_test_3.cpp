// EECE2140 - Group Project
// Team 9: Hayden Trent, Wenxuan Liang, Jack Lu
// Description: Intersection traffic light controller simulation

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <limits>

using namespace std;

// Part 1: Defining Traffic Light States (Enumeration Type)
// This part of the code defines the various states that the traffic lights can display, using an enumeration type to bind the state name and its specific meaning together.
// Principle: C++'s enum class makes each state a fixed name, so the program can directly use the name (e.g., CarSignal::Red) to represent the red light, making it less prone to errors.

// Status of vehicle traffic lights
enum class CarSignal {
    Red, Yellow, Green,   // Regular red, yellow, and green
    RedArrow, YellowArrow, GreenArrow, // Arrow light (for left turns only)
    FlashingRed, FlashingRedArrow // Flashing red light (for fault or power failure mode)
};

// Pedestrian traffic light status
enum class PedSignal {
    DontWalk, Walk, FlashingDontWalk, Flashing   // No passage, Pass, Flashing prohibition, Normal flashing
};


// Part Two: Defining the Phases (Phase) of the Traffic Light Cycle
// This part of the code lists all the different phases in the entire traffic light cycle, each phase corresponding to a set of lights.
// Principle: A complete traffic light cycle is broken down into multiple smaller phases, such as "green light for left turns from north to south," "green light for straight traffic from north to south," and "all red," etc.
// The program executes each phase sequentially, with each phase lasting for a certain period, thus achieving automatic traffic light switching.
enum class Phase {
    NS_Left_Green, NS_Left_Yellow, NS_Left_AllRed, // Turn left from north to south: green → yellow → all red
    NS_Through_Green, NS_Through_Yellow, AllRed_To_EW,  // Northbound straight travel: Green → Yellow → All red (Preparing to switch to eastbound/westbound)
    EW_Left_Green, EW_Left_Yellow, EW_Left_AllRed,  // Turn left from east to west: green → yellow → all red
    EW_Through_Green, EW_Through_Yellow, AllRed_To_NS  // East-west straight traffic: green → yellow → all red (preparing to switch back to north-south)
};

// Part 3: A structure storing the current state of all lights
// This structure stores the color that each direction and each type of light should display at a given moment.
// Principle: A single structure packages the states of 8 lights (north-south straight, north-south left turn, east-west straight, east-west left turn, north-south pedestrian, east-west pedestrian) together,
// for easy one-time retrieval and output.

struct IntersectionState {
    CarSignal nsThrough;  // Northbound straight traffic lights
    CarSignal nsLeft;  // Left turn signal for northbound traffic
    CarSignal ewThrough;  // East-west direction straight traffic lights
    CarSignal ewLeft;  // Left turn signal for east-west direction
    PedSignal nsPed;    // Pedestrian lights from north to south
    PedSignal ewPed;    // East-West Pedestrian Lights
};

// Part Four: Traffic Light Controller Class
// This class is the core of the program. It is responsible for:
// 1. Remembering the current stage
// 2. Automatically switching to the next stage based on time
// 3. Handling special situations such as pedestrian buttons, emergency vehicles, and power outages
// Principle: This is a finite state machine. The program has a state variable (currentPhase), and the update() function is called every short interval,
// determining whether to jump to the next state based on the length of time spent in that state. The getState() function returns the specific color of each light based on the current state (and whether it's an emergency/power outage).
class IntersectionController {
private:
    Phase currentPhase = Phase::NS_Left_Green;  // Current phase, starting from the green light for left turns from north to south
    double timeInPhase = 0.0;                    // How many seconds has the current phase been

// Default time length (seconds), which users can modify by input.    double leftGreenTime = 4.0;
    // Duration of left turn green light
     double leftGreenTime = 4.0;   //Left turn green light duration
    double leftYellowTime = 2.0;  // Left turn yellow light duration
    double throughGreenTime = 8.0;  // Green light duration for straight traffic
    double yellowTime = 3.0;    // Yellow light duration for straight traffic
    double allRedTime = 1.0;   //Duration of all red (red lights in all directions)

    bool pedWaiting = false;    //Are there any pedestrians waiting to cross the road?
    bool emergencyMode = false;  // Is it in emergency mode (e.g., all directions are forced to turn red when an ambulance passes)?
    bool powerLossMode = false; //  Is it in power-off mode (all lights flashing red)?

    // Helper to switch phase
    void changePhase(Phase next) {
        currentPhase = next;
        timeInPhase = 0.0;
        // cout << "DEBUG: Phase changed to " << (int)next << endl; 
    }

    // Input validation function: Prevents the program from crashing due to user input of letters
// Principle: Use cin to read integers. If it fails (the user inputs a letter), clear the error flag, discard the erroneous input, and then prompt the user for input again.
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
// Allow users to customize the duration of each stage
public:
    void inputTimers();
// Update the controller state; the time parameter is the number of seconds that have elapsed in the past period.
    void update(double time);
// Get the current display status of all lights
    IntersectionState getState() const;
// Get the current phase (mainly used for debugging and determining whether there has been a change)
    Phase getCurrentPhase() const;
// Pedestrian button: After pressing, you will be allowed to cross the road when the next green light for straight traffic begins.
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
