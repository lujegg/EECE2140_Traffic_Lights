// Team 9 Hayden Trent, Liang Wenxuan, Jack Lu
// EECE2140

#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

using namespace std;

// Part 1: Defining Traffic Light States (Enumeration Types)
// This section defines the fixed states that each signal can display.
// Principle: enum class binds a readable state name to a single valid value,
// so the program can use names like CarSignal::Red instead of unsafe integers.
enum class CarSignal {
    Red,
    Yellow,
    Green,
    RedArrow,
    YellowArrow,
    GreenArrow,
    FlashingRed,
    FlashingRedArrow
};

// Pedestrian traffic light states.
enum class PedSignal {
    DontWalk,
    Walk,
    FlashingDontWalk,
    Flashing
};

// Part 2: Defining Traffic Light Phases
// Each phase represents one safe configuration of the whole intersection.
// The controller moves through these phases in order based on elapsed time.
enum class Phase {
    NS_Left_Green,
    NS_Left_Yellow,
    NS_Left_AllRed,
    NS_Through_Green,
    NS_Through_Yellow,
    AllRed_To_EW,
    EW_Left_Green,
    EW_Left_Yellow,
    EW_Left_AllRed,
    EW_Through_Green,
    EW_Through_Yellow,
    AllRed_To_NS
};

// Display mode chosen by the user at startup.
enum class DisplayMode {
    Text,
    Symbol,
    Map
};

// Part 3: A structure storing the current state of all lights
// This packages the full intersection output into one object so it can be
// returned and printed all at once.
struct IntersectionState {
    CarSignal nsThrough;
    CarSignal nsLeft;
    CarSignal ewThrough;
    CarSignal ewLeft;
    PedSignal nsPed;
    PedSignal ewPed;
};

// InputHelper keeps validation logic separate from controller logic.
class InputHelper {
public:
    static int getValidPositiveInt(const string& prompt) {
        int value;

        while (true) {
            cout << prompt;
            cin >> value;

            if (cin.fail() || value <= 0) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a positive integer.\n";
            } else {
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                return value;
            }
        }
    }

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

// Part 4: Traffic Light Controller Class
// This class contains the finite state machine for the traffic signal:
// 1. It remembers the current phase.
// 2. It tracks elapsed time inside that phase.
// 3. It switches phases when each timer expires.
// 4. It handles special operating modes like emergency and power loss.
class IntersectionController {
private:
    Phase currentPhase = Phase::NS_Left_Green;
    double timeInPhase = 0.0;

    // Default time lengths in seconds. The user can override them at startup.
    double leftGreenTime = 4.0;
    double leftYellowTime = 2.0;
    double throughGreenTime = 8.0;
    double yellowTime = 3.0;
    double allRedTime = 1.0;

    bool pedWaiting = false;
    bool emergencyMode = false;
    bool powerLossMode = false;

    // Move to a new phase and reset the timer for that phase.
    void changePhase(Phase next) {
        currentPhase = next;
        timeInPhase = 0.0;
    }

public:
    void inputTimers();
    void update(double time);
    IntersectionState getState() const;
    Phase getCurrentPhase() const;

    // Pedestrian button request. In this version, it serves as a tracked event
    // and is cleared once the next through-green phase arrives.
    void pressPedestrianButton() {
        if (!pedWaiting) {
            pedWaiting = true;
            cout << "\n[INFO] Pedestrian button pressed. Walk will be served on the next through phase.\n\n";
        }
    }

    void triggerEmergency(bool status) {
        emergencyMode = status;
        if (status) {
            cout << "\n[ALERT] Emergency override active. All signals forced to red.\n\n";
        } else {
            cout << "\n[ALERT] Emergency cleared. Returning to normal cycle.\n\n";
        }
    }

    void triggerPowerLoss(bool status) {
        powerLossMode = status;
        if (status) {
            cout << "\n[SYSTEM ERROR] Power loss detected. Entering flashing red mode.\n\n";
        } else {
            cout << "\n[SYSTEM OK] Power restored. Returning to normal cycle.\n\n";
        }
    }

    bool isPowerLoss() const { return powerLossMode; }
    bool isEmergency() const { return emergencyMode; }
};

void IntersectionController::inputTimers() {
    leftGreenTime = InputHelper::getValidPositiveInt("Enter left green time: ");
    leftYellowTime = InputHelper::getValidPositiveInt("Enter left yellow time: ");
    throughGreenTime = InputHelper::getValidPositiveInt("Enter through green time: ");
    yellowTime = InputHelper::getValidPositiveInt("Enter through yellow time: ");
    allRedTime = InputHelper::getValidPositiveInt("Enter all-red time: ");
}

// Part 5: Translating the current phase into actual light outputs
// This is the "state translator." It binds the phase machine to the visible
// car and pedestrian signals by returning one IntersectionState object.
IntersectionState IntersectionController::getState() const {
    // Highest priority: power loss mode.
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

    // Second highest priority: emergency mode.
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

    // Normal traffic cycle.
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

    // Safety fallback. This should not normally be reached.
    return {
        CarSignal::FlashingRed,
        CarSignal::FlashingRedArrow,
        CarSignal::FlashingRed,
        CarSignal::FlashingRedArrow,
        PedSignal::Flashing,
        PedSignal::Flashing
    };
}

// Part 6: Core update logic
// This is the main switch-based finite state machine. The logic is kept very
// close to the original code so the phase sequence remains easy to follow.
void IntersectionController::update(double time) {
    if (powerLossMode || emergencyMode) {
        return;
    }

    if (currentPhase == Phase::NS_Through_Green || currentPhase == Phase::EW_Through_Green) {
        pedWaiting = false;
    }

    timeInPhase += time;

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

Phase IntersectionController::getCurrentPhase() const {
    return currentPhase;
}

// Part 7: Rendering and display helpers
// These functions were moved into a dedicated renderer class so output logic
// is separated from the traffic controller state machine.
class IntersectionRenderer {
private:

// -----------------------------------------------------------------------
// Text-mode helpers (original)
// -----------------------------------------------------------------------

string toString(CarSignal s) {
    switch (s) {
    case CarSignal::Red:             return "Red";
    case CarSignal::Yellow:          return "Yellow";
    case CarSignal::Green:           return "Green";
    case CarSignal::RedArrow:        return "RedArrow";
    case CarSignal::YellowArrow:     return "YellowArrow";
    case CarSignal::GreenArrow:      return "GreenArrow";
    case CarSignal::FlashingRed:     return "FlashingRed";
    case CarSignal::FlashingRedArrow:return "FlashingRedArrow";
    }
    return "";
}

string toString(PedSignal s) {
    switch (s) {
    case PedSignal::DontWalk:        return "DontWalk";
    case PedSignal::Walk:            return "Walk";
    case PedSignal::FlashingDontWalk:return "FlashingDontWalk";
    case PedSignal::Flashing:        return "Flashing";
    }
    return "";
}

// -----------------------------------------------------------------------
// Symbol-mode helpers (bracket display)
// -----------------------------------------------------------------------

string toSymbol(CarSignal s) {
    switch (s) {
    case CarSignal::Red:             return "[R] ";
    case CarSignal::Yellow:          return "[Y] ";
    case CarSignal::Green:           return "[G] ";
    case CarSignal::RedArrow:        return "[<R]";
    case CarSignal::YellowArrow:     return "[<Y]";
    case CarSignal::GreenArrow:      return "[<G]";
    case CarSignal::FlashingRed:     return "[*R]";
    case CarSignal::FlashingRedArrow:return "[*<]";
    }
    return "[ ?]";
}

string toSymbol(PedSignal s) {
    switch (s) {
    case PedSignal::DontWalk:        return "[X] ";
    case PedSignal::Walk:            return "[W] ";
    case PedSignal::FlashingDontWalk:return "[~W]";
    case PedSignal::Flashing:        return "[~~]";
    }
    return "[? ]";
}

// -----------------------------------------------------------------------
// Map-mode helpers
//
// Symbol scheme (matches image layout):
//
//   /   = grass / impassable corner
//
//   Car signal char (every road cell, 1 char):
//     G = Green  — go
//     W = Yellow — caution (Warning)
//     R = Red    — stop
//     ~ = Flashing / emergency
//
//   Pedestrian signal char (crosswalk cells only, 2nd of 2 chars):
//     G = Go   (walk)
//     S = Stop (don't walk, or flashing don't walk)
//
//   Normal road cell  = 1 char  e.g. "R"
//   Crosswalk cell    = 2 chars: carChar + pedChar  e.g. "RG" or "RS"
//   Grass cell        = 1 char  "/"
// -----------------------------------------------------------------------

// Car signal -> single char for road cells
char carChar(CarSignal s) {
    switch (s) {
    case CarSignal::Green:            return 'G';
    case CarSignal::GreenArrow:       return 'G';
    case CarSignal::Yellow:           return 'W';  // W = Warning/caution
    case CarSignal::YellowArrow:      return 'W';
    case CarSignal::Red:              return 'R';
    case CarSignal::RedArrow:         return 'R';
    case CarSignal::FlashingRed:      return '~';
    case CarSignal::FlashingRedArrow: return '~';
    }
    return '?';
}

// Pedestrian signal -> single char shown inside crosswalk cells
char pedChar(PedSignal s) {
    switch (s) {
    case PedSignal::Walk:             return 'G';  // Go
    case PedSignal::FlashingDontWalk: return 'S';  // Stop (finishing)
    case PedSignal::DontWalk:         return 'S';  // Stop
    case PedSignal::Flashing:         return '~';  // Emergency
    }
    return '?';
}

// Returns the phase name as a short string for the map header.
string phaseName(Phase p) {
    switch (p) {
    case Phase::NS_Left_Green:     return "NS Left Turn  - Green";
    case Phase::NS_Left_Yellow:    return "NS Left Turn  - Yellow";
    case Phase::NS_Left_AllRed:    return "All Red       - (to NS Through)";
    case Phase::NS_Through_Green:  return "NS Through    - Green";
    case Phase::NS_Through_Yellow: return "NS Through    - Yellow";
    case Phase::AllRed_To_EW:      return "All Red       - (to EW Left)";
    case Phase::EW_Left_Green:     return "EW Left Turn  - Green";
    case Phase::EW_Left_Yellow:    return "EW Left Turn  - Yellow";
    case Phase::EW_Left_AllRed:    return "All Red       - (to EW Through)";
    case Phase::EW_Through_Green:  return "EW Through    - Green";
    case Phase::EW_Through_Yellow: return "EW Through    - Yellow";
    case Phase::AllRed_To_NS:      return "All Red       - (to NS Left)";
    }
    return "Unknown";
}

// -----------------------------------------------------------------------
// printMap  –  compact 13x13 ASCII intersection map
//
// 1 char per cell, grid is square.
//
// Symbols:
//   /  = grass corner
//   -  = plain road surface
//   |  = NS road centre divider
//   +  = centre crosshair of intersection box
//   G  = car Green  (go)   — shown on crosswalk stripe cells
//   W  = car caution Warning (yellow)
//   X  = car red / stop
//   ~  = car flashing emergency
//
// Crosswalk stripes show the car signal of that road.
// Pedestrian signal (G=go, S=stop) printed in summary below map.
//
// Grid layout (cols 0-12, rows 0-12):
//   Col 5 = NS left-turn lane
//   Col 6 = NS road centre divider  |
//   Col 7 = NS through lane
//   Col 4 = W crosswalk column  (EW road rows only)
//   Col 8 = E crosswalk column  (EW road rows only)
//   Row 5 = EW left-turn row
//   Row 6 = EW road centre divider  -
//   Row 7 = EW through row
//   Row 4 = N crosswalk row  (NS road cols only)
//   Row 8 = S crosswalk row  (NS road cols only)
// -----------------------------------------------------------------------
void printMap(const IntersectionState& s, Phase phase) {

    // Car signal chars (used on crosswalk stripe cells)
    char nsL = carChar(s.nsLeft);     // NS left-turn lane signal
    char nsT = carChar(s.nsThrough);  // NS through lane signal
    char ewL = carChar(s.ewLeft);     // EW left-turn lane signal
    char ewT = carChar(s.ewThrough);  // EW through lane signal

    // Ped signal chars (printed in summary, not in grid)
    // ewPed -> N/S crosswalks  (peds cross the EW road)
    // nsPed -> W/E crosswalks  (peds cross the NS road)
    char ewP = pedChar(s.ewPed);
    char nsP = pedChar(s.nsPed);

    // Grid: 13 x 13, single char per cell
    const int SZ = 13;
    char g[SZ][SZ];

    // Column indices for NS road
    const int CL = 5;  // NS left-turn lane col
    const int CD = 6;  // NS centre divider col
    const int CT = 7;  // NS through lane col

    // Row indices for EW road
    const int RL = 5;  // EW left-turn row
    const int RD = 6;  // EW centre divider row
    const int RT = 7;  // EW through row

    // Crosswalk positions (single stripe row/col each)
    const int RN = 4;  // N crosswalk row (on NS cols, just above EW band)
    const int RS = 8;  // S crosswalk row (on NS cols, just below EW band)
    const int CW = 4;  // W crosswalk col (on EW rows, just left of NS road)
    const int CE = 8;  // E crosswalk col (on EW rows, just right of NS road)

    // Helper: safe set
    auto set = [&](int r, int c, char ch) {
        if (r >= 0 && r < SZ && c >= 0 && c < SZ) g[r][c] = ch;
    };

    // 1. Fill everything with grass
    for (int r = 0; r < SZ; r++)
        for (int c = 0; c < SZ; c++)
            g[r][c] = '/';

    // 2. NS road strip: cols CL, CD, CT for all rows
    for (int r = 0; r < SZ; r++) {
        set(r, CL, '-');
        set(r, CD, '|');
        set(r, CT, '-');
    }

    // 3. EW road strip: rows RL, RD, RT for all cols
    for (int c = 0; c < SZ; c++) {
        set(RL, c, '-');
        set(RD, c, '-');
        set(RT, c, '-');
    }

    // 4. Intersection box interior: rows RL-RT, cols CL-CT  -> all '-', centre '+'
    for (int r = RL; r <= RT; r++)
        for (int c = CL; c <= CT; c++)
            set(r, c, '-');
    set(RD, CD, '+');

    // 5. N crosswalk (on NS road cols, row RN) — car signal of NS road
    set(RN, CL, nsL);
    set(RN, CD, '|');  // divider stays
    set(RN, CT, nsT);

    // 6. S crosswalk (on NS road cols, row RS) — car signal of NS road
    set(RS, CL, nsL);
    set(RS, CD, '|');
    set(RS, CT, nsT);

    // 7. W crosswalk (on EW road rows, col CW) — car signal of EW road
    set(RL, CW, ewL);
    set(RD, CW, '-');  // divider row stays '-'
    set(RT, CW, ewT);

    // 8. E crosswalk (on EW road rows, col CE) — car signal of EW road
    set(RL, CE, ewL);
    set(RD, CE, '-');
    set(RT, CE, ewT);

    // 9. Cardinal labels on road surface
    set(1,    CD, 'N');
    set(SZ-2, CD, 'S');
    set(RD,   1,  'W');
    set(RD,   SZ-2, 'E');

    // Print
    cout << "\n  " << phaseName(phase) << "\n\n";
    cout << "  Cars: G=go W=caution X=stop ~=flash\n";
    cout << "  Peds: G=go S=stop  (crosswalk signal shown below map)\n\n";

    for (int r = 0; r < SZ; r++) {
        cout << "  ";
        for (int c = 0; c < SZ; c++) cout << g[r][c];
        if (r == RN) cout << "  <- N xwalk (ewPed=" << toString(s.ewPed) << " -> ped:" << ewP << ")";
        if (r == RS) cout << "  <- S xwalk (ewPed=" << toString(s.ewPed) << " -> ped:" << ewP << ")";
        if (r == RD) cout << "  W xwalk(nsPed -> ped:" << nsP << ")  E xwalk(nsPed -> ped:" << nsP << ")";
        cout << "\n";
    }
    cout << "\n" << string(SZ + 4, '-') << "\n";
}



// -----------------------------------------------------------------------
// Legend printouts
// -----------------------------------------------------------------------

static void printSymbolLegend() {
    cout << "\n=== SYMBOL LEGEND ===\n";
    cout << "  Car Signals:\n";
    cout << "    [R]   = Red (stop)\n";
    cout << "    [Y]   = Yellow (caution)\n";
    cout << "    [G]   = Green (go)\n";
    cout << "    [<R]  = Red Arrow    (left-turn blocked)\n";
    cout << "    [<Y]  = Yellow Arrow (left-turn caution)\n";
    cout << "    [<G]  = Green Arrow  (left-turn go)\n";
    cout << "    [*R]  = Flashing Red       (emergency stop)\n";
    cout << "    [*<]  = Flashing Red Arrow (emergency left-turn stop)\n";
    cout << "\n";
    cout << "  Pedestrian Signals:\n";
    cout << "    [X]   = Don't Walk\n";
    cout << "    [W]   = Walk\n";
    cout << "    [~W]  = Flashing Don't Walk (finish crossing)\n";
    cout << "    [~~]  = Flashing (emergency)\n";
    cout << "=====================\n\n";
}

static void printMapLegend() {
    cout << "\n=== MAP LEGEND ===\n";
    cout << "  /  = grass (impassable corner)\n";
    cout << "  -  = road surface\n";
    cout << "  |  = NS road centre divider\n";
    cout << "  +  = intersection centre\n";
    cout << "  Car signal (crosswalk cells): G=go  W=caution  X=stop  ~=emergency\n";
    cout << "  Ped signal (shown beside map): G=go  S=stop\n";
    cout << "  N/S crosswalks cross the EW road (controlled by ewPed).\n";
    cout << "  W/E crosswalks cross the NS road (controlled by nsPed).\n";
    cout << "==================\n\n";
}
public:
// -----------------------------------------------------------------------
// Legend printout handler
// -----------------------------------------------------------------------
static void printLegend(DisplayMode mode) {
    if (mode == DisplayMode::Symbol) {
        printSymbolLegend();
    } else if (mode == DisplayMode::Map) {
        printMapLegend();
    }
}
// -----------------------------------------------------------------------
// printState — dispatches to the chosen display mode
// -----------------------------------------------------------------------

static void printState(const IntersectionState& s, DisplayMode mode, Phase phase) {
    if (mode == DisplayMode::Text) {
        cout << "NS Through: " << toString(s.nsThrough) << "\n";
        cout << "NS Left   : " << toString(s.nsLeft)    << "\n";
        cout << "EW Through: " << toString(s.ewThrough) << "\n";
        cout << "EW Left   : " << toString(s.ewLeft)    << "\n";
        cout << "NS Ped    : " << toString(s.nsPed)     << "\n";
        cout << "EW Ped    : " << toString(s.ewPed)     << "\n";
        cout << "--------------------------\n";

    } else if (mode == DisplayMode::Symbol) {
        cout << "          | NS Through | NS Left | EW Through | EW Left |\n";
        cout << "  Cars    |    "
             << toSymbol(s.nsThrough) << "    |   "
             << toSymbol(s.nsLeft)    << "  |    "
             << toSymbol(s.ewThrough) << "    |   "
             << toSymbol(s.ewLeft)    << "  |\n";
        cout << "          | NS Ped     |         | EW Ped     |         |\n";
        cout << "  Peds    |    "
             << toSymbol(s.nsPed) << "    |         |    "
             << toSymbol(s.ewPed) << "    |         |\n";
        cout << "-------------------------------------------------------------\n";

    } else {
        // Map mode
        printMap(s, phase);
    }
}

};

int main() {
    cout << "--- Traffic Light Controller Initialization ---\n";

    DisplayMode displayMode = InputHelper::chooseDisplayMode();
    IntersectionRenderer::printLegend(displayMode);

    IntersectionController controller;
    controller.inputTimers();

    auto lastTime = chrono::steady_clock::now();

    bool lastEmergencyState = controller.isEmergency();
    bool lastPowerLossState = controller.isPowerLoss();
    Phase lastPrintedPhase = controller.getCurrentPhase();

    IntersectionRenderer::printState(controller.getState(), displayMode, controller.getCurrentPhase());

    int loopCounter = 0;

    while (true) {
        auto currentTime = chrono::steady_clock::now();
        chrono::duration<double> elapsed = currentTime - lastTime;
        lastTime = currentTime;

        // Demo events carried over from the richer controller example.
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

            IntersectionRenderer::printState(
                controller.getState(),
                displayMode,
                controller.getCurrentPhase());
        }

        this_thread::sleep_for(chrono::milliseconds(50));
    }

    return 0;
}

