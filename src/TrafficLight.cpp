// Team 9 Hayden Trent, Liang Wenxuan, Jack Lu
// EECE2140

#include <chrono>   // Time measurement utilities for real-time simulation
#include <iostream> // Standard input and output
#include <limits>   // numeric_limits for input cleanup
#include <string>   // std::string
#include <thread>   // std::this_thread::sleep_for

using namespace std;

// Part 1: Defining Traffic Light States (Enumeration Types)
// This section defines the fixed states that each signal can display.
// Principle: enum class binds a readable state name to a single valid value,
// so the program can use names like CarSignal::Red instead of unsafe integers.
// This makes the code easier to read, less error-prone, and compatible with switch statements.

// Vehicle traffic light states
enum class CarSignal {
    Red,              // Regular red light
    Yellow,           // Regular yellow light
    Green,            // Regular green light
    RedArrow,         // Red arrow for protected turn lane
    YellowArrow,      // Yellow arrow for protected turn lane
    GreenArrow,       // Green arrow for protected turn lane
    FlashingRed,      // Flashing red used in fault or power-loss mode
    FlashingRedArrow  // Flashing red arrow used in fault or power-loss mode
};

// Pedestrian traffic light states
enum class PedSignal {
    DontWalk,         // Pedestrians must not cross
    Walk,             // Pedestrians may cross
    FlashingDontWalk, // Pedestrians should finish crossing if already in crosswalk
    Flashing          // Generic flashing fallback for emergency / abnormal display
};

// Part 2: Defining Traffic Light Phases
// This section lists all the stages in the traffic signal cycle.
// Each phase corresponds to one full safe combination of car and pedestrian lights.
// Principle: instead of managing each light independently, the program treats the
// whole intersection as a finite state machine that moves from one safe phase to the next.
enum class Phase {
    NS_Left_Green,     // North-south left-turn movement has protected green arrow
    NS_Left_Yellow,    // North-south left-turn movement is ending
    NS_Left_AllRed,    // All-red clearance before north-south through begins
    NS_Through_Green,  // North-south through movement has green
    NS_Through_Yellow, // North-south through movement is ending
    AllRed_To_EW,      // All-red clearance before east-west movement begins
    EW_Left_Green,     // East-west left-turn movement has protected green arrow
    EW_Left_Yellow,    // East-west left-turn movement is ending
    EW_Left_AllRed,    // All-red clearance before east-west through begins
    EW_Through_Green,  // East-west through movement has green
    EW_Through_Yellow, // East-west through movement is ending
    AllRed_To_NS       // All-red clearance before the cycle returns to north-south
};

// Display mode chosen by the user at startup.
// Principle: the controller logic stays the same, but the renderer can present
// the same intersection state in different output formats.
enum class DisplayMode {
    Text,   // Full word-based output
    Symbol, // Compact bracket/symbol-based output
    Map     // ASCII top-down intersection map
};

// Part 3: Structure storing the current state of all lights
// This struct packages the current output of the whole intersection into a single object.
// Principle: instead of returning each light separately, the controller returns one
// IntersectionState object that contains everything the renderer needs to display.
struct IntersectionState {
    CarSignal nsThrough; // North-south through traffic light
    CarSignal nsLeft;    // North-south left-turn traffic light
    CarSignal ewThrough; // East-west through traffic light
    CarSignal ewLeft;    // East-west left-turn traffic light
    PedSignal nsPed;     // Pedestrian signal associated with north-south crossing
    PedSignal ewPed;     // Pedestrian signal associated with east-west crossing
};

// Part 4: Input Helper Class
// This helper class keeps user-input validation separate from the traffic controller.
// Principle: separating input logic from control logic follows OOP design principles,
// because each class has one main responsibility.
class InputHelper {
public:
    // Input validation function
    // This prevents the program from crashing if the user types letters or invalid values.
    // Principle: cin.fail() detects invalid input, then the input buffer is cleaned
    // and the user is asked again until a valid positive integer is entered.
    static int getValidPositiveInt(const string& prompt) {
        int value;

        while (true) {
            cout << prompt;
            cin >> value;

            if (cin.fail() || value <= 0) {
                cin.clear(); // Clear the fail state
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Remove bad input
                cout << "Invalid input. Please enter a positive integer.\n";
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Remove any extra characters
                return value;
            }
        }
    }

    // Lets the user choose how the intersection should be displayed.
    // Principle: the same controller logic can support multiple user views.
    static DisplayMode chooseDisplayMode() {
        int choice;

        while (true) {
            cout << "\nChoose display mode:\n";
            cout << "  1 - Text   (e.g. \"Green\", \"DontWalk\")\n";
            cout << "  2 - Symbol (e.g. [G], [X])\n";
            cout << "  3 - Map    (ASCII top-down intersection diagram)\n";
            cout << "Enter 1, 2, or 3: ";

            cin >> choice;

            if (cin.fail() || choice < 1 || choice > 3) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter 1, 2, or 3.\n";
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                break;
            }
        }

        if (choice == 1) return DisplayMode::Text;
        if (choice == 2) return DisplayMode::Symbol;
        return DisplayMode::Map;
    }
};

// Part 5: Traffic Light Controller Class
// This class is the core finite state machine for the traffic signal.
// It is responsible for:
// 1. Remembering the current phase
// 2. Tracking elapsed time inside that phase
// 3. Switching phases when timers expire
// 4. Handling pedestrian request, emergency mode, and power-loss mode
// Principle: the controller decides what the intersection is doing,
// while rendering is handled elsewhere.
class IntersectionController {
private:
    Phase currentPhase = Phase::NS_Left_Green; // Starting phase of the cycle
    double timeInPhase = 0.0;                  // Elapsed time in the current phase

    // Default time lengths in seconds.
    // These values can be overridden by the user at startup.
    double leftGreenTime = 4.0;     // Protected left-turn green duration
    double leftYellowTime = 2.0;    // Protected left-turn yellow duration
    double throughGreenTime = 8.0;  // Through movement green duration
    double yellowTime = 3.0;        // Through movement yellow duration
    double allRedTime = 1.0;        // All-red clearance duration

    // Special mode flags
    bool pedWaiting = false;    // True if a pedestrian request has been made
    bool emergencyMode = false; // True if emergency override is active
    bool powerLossMode = false; // True if power-loss flashing mode is active

    // Helper to change phase and reset the timer.
    // Principle: whenever the signal moves to a new phase,
    // the elapsed time inside that phase must start back at zero.
    void changePhase(Phase next) {
        currentPhase = next;
        timeInPhase = 0.0;
    }

public:
    // Allows the user to customize phase durations.
    void inputTimers();

    // Updates the controller by the amount of real time that has passed.
    void update(double time);

    // Returns the full light state for the current moment.
    IntersectionState getState() const;

    // Returns the current phase.
    Phase getCurrentPhase() const;

    // Pedestrian button logic
    // Once pressed, the request is remembered until a through-green phase occurs.
    void pressPedestrianButton() {
        if (!pedWaiting) {
            pedWaiting = true;
            cout << "\n[INFO] Pedestrian button pressed. Walk will be served on the next through phase.\n\n";
        }
    }

    // Emergency mode logic
    // true  = force the intersection into all-red mode
    // false = return to normal cycle
    void triggerEmergency(bool status) {
        emergencyMode = status;
        if (status) {
            cout << "\n[ALERT] Emergency override active. All signals forced to red.\n\n";
        } else {
            cout << "\n[ALERT] Emergency cleared. Returning to normal cycle.\n\n";
        }
    }

    // Power-loss mode logic
    // true  = flashing red mode
    // false = return to normal cycle
    void triggerPowerLoss(bool status) {
        powerLossMode = status;
        if (status) {
            cout << "\n[SYSTEM ERROR] Power loss detected. Entering flashing red mode.\n\n";
        } else {
            cout << "\n[SYSTEM OK] Power restored. Returning to normal cycle.\n\n";
        }
    }

    // Getter functions for special mode checks
    bool isPowerLoss() const { return powerLossMode; }
    bool isEmergency() const { return emergencyMode; }
};

// Part 6: User-inputted phase times
// This function asks the user to enter durations for the main parts of the cycle.
// Principle: valid input is guaranteed through InputHelper::getValidPositiveInt().
void IntersectionController::inputTimers() {
    leftGreenTime = InputHelper::getValidPositiveInt("Enter left green time: ");
    leftYellowTime = InputHelper::getValidPositiveInt("Enter left yellow time: ");
    throughGreenTime = InputHelper::getValidPositiveInt("Enter through green time: ");
    yellowTime = InputHelper::getValidPositiveInt("Enter through yellow time: ");
    allRedTime = InputHelper::getValidPositiveInt("Enter all-red time: ");
}

// Part 7: Translating the current phase into actual light outputs
// This function is the "state translator."
// It converts the controller's abstract phase into the actual visible car and pedestrian signals.
// Principle:
// Highest priority: power-loss mode
// Second priority: emergency mode
// Otherwise: use currentPhase to return the normal signal combination.
IntersectionState IntersectionController::getState() const {
    // Priority 1: power-loss mode
    if (powerLossMode) {
        return {
            CarSignal::FlashingRed,
            CarSignal::FlashingRedArrow,
            CarSignal::FlashingRed,
            CarSignal::FlashingRedArrow,
            PedSignal::FlashingDontWalk,
            PedSignal::FlashingDontWalk
        };
    }

    // Priority 2: emergency mode
    if (emergencyMode) {
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };
    }

    // Normal phase-based operation
    switch (currentPhase) {
    case Phase::NS_Left_Green:
        return {
            CarSignal::Red,
            CarSignal::GreenArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };

    case Phase::NS_Left_Yellow:
        return {
            CarSignal::Red,
            CarSignal::YellowArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };

    case Phase::NS_Left_AllRed:
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };

    case Phase::NS_Through_Green:
        return {
            CarSignal::Green,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::Walk,
            PedSignal::DontWalk
        };

    case Phase::NS_Through_Yellow:
        return {
            CarSignal::Yellow,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::FlashingDontWalk,
            PedSignal::DontWalk
        };

    case Phase::AllRed_To_EW:
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };

    case Phase::EW_Left_Green:
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::GreenArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };

    case Phase::EW_Left_Yellow:
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::YellowArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };

    case Phase::EW_Left_AllRed:
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };

    case Phase::EW_Through_Green:
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Green,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::Walk
        };

    case Phase::EW_Through_Yellow:
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Yellow,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::FlashingDontWalk
        };

    case Phase::AllRed_To_NS:
        return {
            CarSignal::Red,
            CarSignal::RedArrow,
            CarSignal::Red,
            CarSignal::RedArrow,
            PedSignal::DontWalk,
            PedSignal::DontWalk
        };
    }

    // Safety fallback
    // This should never normally be reached, but returning a flashing state
    // makes the failure condition visible instead of undefined.
    return {
        CarSignal::FlashingRed,
        CarSignal::FlashingRedArrow,
        CarSignal::FlashingRed,
        CarSignal::FlashingRedArrow,
        PedSignal::Flashing,
        PedSignal::Flashing
    };
}

// Part 8: Core Update Logic
// This is the main switch-based finite state machine.
// It checks how long the controller has been in the current phase
// and changes to the next phase when that phase's timer expires.
// Principle: the overall logic stays close to the original version,
// so the phase sequence remains easy to follow.
void IntersectionController::update(double time) {
    // In emergency or power-loss mode, the normal cycle is paused
    if (powerLossMode || emergencyMode) {
        return;
    }

    // If a through-green phase is active, any pending pedestrian request has been served
    if (currentPhase == Phase::NS_Through_Green || currentPhase == Phase::EW_Through_Green) {
        pedWaiting = false;
    }

    // Accumulate elapsed time in the current phase
    timeInPhase += time;

    // Check if the current phase has lasted long enough to move to the next phase
    switch (currentPhase) {
    case Phase::NS_Left_Green:
        if (timeInPhase >= leftGreenTime) changePhase(Phase::NS_Left_Yellow);
        break;

    case Phase::NS_Left_Yellow:
        if (timeInPhase >= leftYellowTime) changePhase(Phase::NS_Left_AllRed);
        break;

    case Phase::NS_Left_AllRed:
        if (timeInPhase >= allRedTime) changePhase(Phase::NS_Through_Green);
        break;

    case Phase::NS_Through_Green:
        if (timeInPhase >= throughGreenTime) changePhase(Phase::NS_Through_Yellow);
        break;

    case Phase::NS_Through_Yellow:
        if (timeInPhase >= yellowTime) changePhase(Phase::AllRed_To_EW);
        break;

    case Phase::AllRed_To_EW:
        if (timeInPhase >= allRedTime) changePhase(Phase::EW_Left_Green);
        break;

    case Phase::EW_Left_Green:
        if (timeInPhase >= leftGreenTime) changePhase(Phase::EW_Left_Yellow);
        break;

    case Phase::EW_Left_Yellow:
        if (timeInPhase >= leftYellowTime) changePhase(Phase::EW_Left_AllRed);
        break;

    case Phase::EW_Left_AllRed:
        if (timeInPhase >= allRedTime) changePhase(Phase::EW_Through_Green);
        break;

    case Phase::EW_Through_Green:
        if (timeInPhase >= throughGreenTime) changePhase(Phase::EW_Through_Yellow);
        break;

    case Phase::EW_Through_Yellow:
        if (timeInPhase >= yellowTime) changePhase(Phase::AllRed_To_NS);
        break;

    case Phase::AllRed_To_NS:
        if (timeInPhase >= allRedTime) changePhase(Phase::NS_Left_Green);
        break;
    }
}

// Returns the current phase.
// This is mainly useful for comparing whether the phase changed between loop iterations.
Phase IntersectionController::getCurrentPhase() const {
    return currentPhase;
}

// Part 9: Rendering and Display Helpers
// This class handles all user-visible output.
// Principle: the controller should decide the state,
// while the renderer decides how to display that state.
class IntersectionRenderer {
private:
    // Converts a car signal enum into readable text.
    static string toString(CarSignal s) {
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

    // Converts a pedestrian signal enum into readable text.
    static string toString(PedSignal s) {
        switch (s) {
        case PedSignal::DontWalk: return "DontWalk";
        case PedSignal::Walk: return "Walk";
        case PedSignal::FlashingDontWalk: return "FlashingDontWalk";
        case PedSignal::Flashing: return "Flashing";
        }
        return "";
    }

    // Converts a car signal into a compact bracket symbol for symbol display mode.
    static string toSymbol(CarSignal s) {
        switch (s) {
        case CarSignal::Red: return "[R] ";
        case CarSignal::Yellow: return "[Y] ";
        case CarSignal::Green: return "[G] ";
        case CarSignal::RedArrow: return "[<R]";
        case CarSignal::YellowArrow: return "[<Y]";
        case CarSignal::GreenArrow: return "[<G]";
        case CarSignal::FlashingRed: return "[*R]";
        case CarSignal::FlashingRedArrow: return "[*<]";
        }
        return "[ ?]";
    }

    // Converts a pedestrian signal into a compact bracket symbol for symbol display mode.
    static string toSymbol(PedSignal s) {
        switch (s) {
        case PedSignal::DontWalk: return "[X] ";
        case PedSignal::Walk: return "[W] ";
        case PedSignal::FlashingDontWalk: return "[~W]";
        case PedSignal::Flashing: return "[~~]";
        }
        return "[? ]";
    }

    // Converts a car signal into a single-character map symbol for map mode.
    static char carChar(CarSignal s) {
        switch (s) {
        case CarSignal::Green:
        case CarSignal::GreenArrow:
            return 'G';
        case CarSignal::Yellow:
        case CarSignal::YellowArrow:
            return 'W';
        case CarSignal::Red:
        case CarSignal::RedArrow:
            return 'X';
        case CarSignal::FlashingRed:
        case CarSignal::FlashingRedArrow:
            return '~';
        }
        return '?';
    }

    // Converts a pedestrian signal into a single-character map summary symbol.
    static char pedChar(PedSignal s) {
        switch (s) {
        case PedSignal::Walk:
            return 'G';
        case PedSignal::FlashingDontWalk:
        case PedSignal::DontWalk:
            return 'S';
        case PedSignal::Flashing:
            return '~';
        }
        return '?';
    }

    // Converts a phase into a short readable title for map display.
    static string phaseName(Phase p) {
        switch (p) {
        case Phase::NS_Left_Green: return "NS Left Turn  - Green";
        case Phase::NS_Left_Yellow: return "NS Left Turn  - Yellow";
        case Phase::NS_Left_AllRed: return "All Red       - (to NS Through)";
        case Phase::NS_Through_Green: return "NS Through    - Green";
        case Phase::NS_Through_Yellow: return "NS Through    - Yellow";
        case Phase::AllRed_To_EW: return "All Red       - (to EW Left)";
        case Phase::EW_Left_Green: return "EW Left Turn  - Green";
        case Phase::EW_Left_Yellow: return "EW Left Turn  - Yellow";
        case Phase::EW_Left_AllRed: return "All Red       - (to EW Through)";
        case Phase::EW_Through_Green: return "EW Through    - Green";
        case Phase::EW_Through_Yellow: return "EW Through    - Yellow";
        case Phase::AllRed_To_NS: return "All Red       - (to NS Left)";
        }
        return "Unknown";
    }

    // Text mode output
    // Prints the full state in detailed word-based format.
    static void printTextState(const IntersectionState& s) {
        cout << "NS Through: " << toString(s.nsThrough) << "\n";
        cout << "NS Left   : " << toString(s.nsLeft) << "\n";
        cout << "EW Through: " << toString(s.ewThrough) << "\n";
        cout << "EW Left   : " << toString(s.ewLeft) << "\n";
        cout << "NS Ped    : " << toString(s.nsPed) << "\n";
        cout << "EW Ped    : " << toString(s.ewPed) << "\n";
        cout << "--------------------------\n";
    }

    // Symbol mode output
    // Prints the same state using compact bracket symbols.
    static void printSymbolState(const IntersectionState& s) {
        cout << "          | NS Through | NS Left | EW Through | EW Left |\n";
        cout << "  Cars    |    "
             << toSymbol(s.nsThrough) << "    |   "
             << toSymbol(s.nsLeft) << "  |    "
             << toSymbol(s.ewThrough) << "    |   "
             << toSymbol(s.ewLeft) << "  |\n";
        cout << "          | NS Ped     |         | EW Ped     |         |\n";
        cout << "  Peds    |    "
             << toSymbol(s.nsPed) << "    |         |    "
             << toSymbol(s.ewPed) << "    |         |\n";
        cout << "-------------------------------------------------------------\n";
    }

    // Map mode output
    // Prints a top-down ASCII map of the intersection.
    // Principle: the grid is drawn first, then special cells are overwritten
    // with the current light states.
    static void printMapState(const IntersectionState& s, Phase phase) {
        char nsL = carChar(s.nsLeft);
        char nsT = carChar(s.nsThrough);
        char ewL = carChar(s.ewLeft);
        char ewT = carChar(s.ewThrough);

        char ewP = pedChar(s.ewPed);
        char nsP = pedChar(s.nsPed);

        const int size = 13;
        char grid[size][size];

        const int colLeft = 5;
        const int colDivider = 6;
        const int colThrough = 7;

        const int rowLeft = 5;
        const int rowDivider = 6;
        const int rowThrough = 7;

        const int rowNorthCrosswalk = 4;
        const int rowSouthCrosswalk = 8;
        const int colWestCrosswalk = 4;
        const int colEastCrosswalk = 8;

        // Small helper for safely writing a character into the map grid.
        auto setCell = [&](int row, int col, char ch) {
            if (row >= 0 && row < size && col >= 0 && col < size) {
                grid[row][col] = ch;
            }
        };

        // Fill the whole map with grass first.
        for (int row = 0; row < size; row++) {
            for (int col = 0; col < size; col++) {
                grid[row][col] = '/';
            }
        }

        // Draw the north-south road
        for (int row = 0; row < size; row++) {
            setCell(row, colLeft, '-');
            setCell(row, colDivider, '|');
            setCell(row, colThrough, '-');
        }

        // Draw the east-west road
        for (int col = 0; col < size; col++) {
            setCell(rowLeft, col, '-');
            setCell(rowDivider, col, '-');
            setCell(rowThrough, col, '-');
        }

        // Draw the intersection box
        for (int row = rowLeft; row <= rowThrough; row++) {
            for (int col = colLeft; col <= colThrough; col++) {
                setCell(row, col, '-');
            }
        }
        setCell(rowDivider, colDivider, '+');

        // Draw north crosswalk signals
        setCell(rowNorthCrosswalk, colLeft, nsL);
        setCell(rowNorthCrosswalk, colDivider, '|');
        setCell(rowNorthCrosswalk, colThrough, nsT);

        // Draw south crosswalk signals
        setCell(rowSouthCrosswalk, colLeft, nsL);
        setCell(rowSouthCrosswalk, colDivider, '|');
        setCell(rowSouthCrosswalk, colThrough, nsT);

        // Draw west crosswalk signals
        setCell(rowLeft, colWestCrosswalk, ewL);
        setCell(rowDivider, colWestCrosswalk, '-');
        setCell(rowThrough, colWestCrosswalk, ewT);

        // Draw east crosswalk signals
        setCell(rowLeft, colEastCrosswalk, ewL);
        setCell(rowDivider, colEastCrosswalk, '-');
        setCell(rowThrough, colEastCrosswalk, ewT);

        // Add simple direction labels
        setCell(1, colDivider, 'N');
        setCell(size - 2, colDivider, 'S');
        setCell(rowDivider, 1, 'W');
        setCell(rowDivider, size - 2, 'E');

        cout << "\n  " << phaseName(phase) << "\n\n";
        cout << "  Cars: G=go W=caution X=stop ~=flash\n";
        cout << "  Peds: G=go S=stop  (crosswalk signal shown below map)\n\n";

        for (int row = 0; row < size; row++) {
            cout << "  ";
            for (int col = 0; col < size; col++) {
                cout << grid[row][col];
            }

            if (row == rowNorthCrosswalk) {
                cout << "  <- N xwalk (ewPed=" << toString(s.ewPed) << " -> ped:" << ewP << ")";
            }
            if (row == rowSouthCrosswalk) {
                cout << "  <- S xwalk (ewPed=" << toString(s.ewPed) << " -> ped:" << ewP << ")";
            }
            if (row == rowDivider) {
                cout << "  W xwalk(nsPed -> ped:" << nsP << ")  E xwalk(nsPed -> ped:" << nsP << ")";
            }
            cout << "\n";
        }

        cout << "\n" << string(size + 4, '-') << "\n";
    }

public:
    // Prints a legend for the selected display mode.
    static void printLegend(DisplayMode mode) {
        if (mode == DisplayMode::Symbol) {
            cout << "\n=== SYMBOL LEGEND ===\n";
            cout << "  Car Signals:\n";
            cout << "    [R]   = Red (stop)\n";
            cout << "    [Y]   = Yellow (caution)\n";
            cout << "    [G]   = Green (go)\n";
            cout << "    [<R]  = Red Arrow    (left-turn blocked)\n";
            cout << "    [<Y]  = Yellow Arrow (left-turn caution)\n";
            cout << "    [<G]  = Green Arrow  (left-turn go)\n";
            cout << "    [*R]  = Flashing Red       (emergency stop)\n";
            cout << "    [*<]  = Flashing Red Arrow (emergency left-turn stop)\n\n";
            cout << "  Pedestrian Signals:\n";
            cout << "    [X]   = Don't Walk\n";
            cout << "    [W]   = Walk\n";
            cout << "    [~W]  = Flashing Don't Walk\n";
            cout << "    [~~]  = Flashing (emergency)\n";
            cout << "=====================\n\n";
        } else if (mode == DisplayMode::Map) {
            cout << "\n=== MAP LEGEND ===\n";
            cout << "  /  = grass (impassable corner)\n";
            cout << "  -  = road surface\n";
            cout << "  |  = NS road centre divider\n";
            cout << "  +  = intersection centre\n";
            cout << "  Car signal chars: G=go  W=caution  X=stop  ~=emergency\n";
            cout << "  Ped summary chars: G=go  S=stop\n";
            cout << "  N/S crosswalks cross the EW road (controlled by ewPed).\n";
            cout << "  W/E crosswalks cross the NS road (controlled by nsPed).\n";
            cout << "==================\n\n";
        }
    }

    // Main renderer dispatch
    // Chooses which display format to use for the same intersection state.
    static void printState(const IntersectionState& state, DisplayMode mode, Phase phase) {
        if (mode == DisplayMode::Text) {
            printTextState(state);
        } else if (mode == DisplayMode::Symbol) {
            printSymbolState(state);
        } else {
            printMapState(state, phase);
        }
    }
};

// Part 10: Main Function (Program Entry Point and Main Loop)
// This part of the code is responsible for:
// 1. Printing the initialization banner
// 2. Letting the user choose a display mode
// 3. Creating the controller object
// 4. Allowing the user to input the time for each phase
// 5. Running the real-time simulation loop
// 6. Demonstrating pedestrian request, emergency mode, and power-loss mode
// Principle:
// - chrono measures real elapsed time
// - update(elapsedTime) advances the controller
// - sleep_for keeps the loop from consuming too much CPU
// - the program only reprints when something important changes
int main() {
    cout << "--- Traffic Light Controller Initialization ---\n";

    // Ask the user how they want the state displayed
    DisplayMode displayMode = InputHelper::chooseDisplayMode();

    // Print the legend for the selected display mode if needed
    IntersectionRenderer::printLegend(displayMode);

    // Create the traffic light controller
    IntersectionController controller;

    // Ask the user to enter custom timer values
    controller.inputTimers();

    // Save the current time so future loops can calculate elapsed time
    auto lastTime = chrono::steady_clock::now();

    // Store previous system states so the screen only updates on changes
    bool lastEmergencyState = controller.isEmergency();
    bool lastPowerLossState = controller.isPowerLoss();
    Phase lastPrintedPhase = controller.getCurrentPhase();

    // Print the initial state of the intersection
    IntersectionRenderer::printState(
        controller.getState(),
        displayMode,
        controller.getCurrentPhase());

    // Demo loop counter
    // This is used to trigger sample events in sequence for testing purposes.
    int loopCounter = 0;

    while (true) {
        // Measure real elapsed time since the last loop
        auto currentTime = chrono::steady_clock::now();
        chrono::duration<double> elapsed = currentTime - lastTime;
        lastTime = currentTime;

        // Demo events carried over from the richer controller example
        // These are loop-based triggers for testing special modes.
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

        // Update the controller based on the real elapsed time
        controller.update(elapsed.count());

        // Read current special mode flags
        bool currentEmergencyState = controller.isEmergency();
        bool currentPowerLossState = controller.isPowerLoss();

        // Reprint only if phase or special mode changed
        if (controller.getCurrentPhase() != lastPrintedPhase ||
            currentEmergencyState != lastEmergencyState ||
            currentPowerLossState != lastPowerLossState) {

            lastPrintedPhase = controller.getCurrentPhase();
            lastEmergencyState = currentEmergencyState;
            lastPowerLossState = currentPowerLossState;

            IntersectionRenderer::printState(
                controller.getState(),
                displayMode,
                controller.getCurrentPhase());
        }

        // Pause briefly to simulate real-time operation without wasting CPU
        this_thread::sleep_for(chrono::milliseconds(50));
    }

    return 0;
}

