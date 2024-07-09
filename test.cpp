/*
 * test.cpp
 */
//the eternal test... (hopefully not eternal)

//test program to interact with the dddapi library and also
// quickly experiment with anything from exploring internal workings
// to implementing possible new features
// (generally anything defined here is not meant to be used externally)

//contains a basic command handling system and ways to configure
// the program and automate commands



#include <thread>
#include <execution>
#include <condition_variable>

#include "getopt.h"
#include "omp.h"

#include "dddutil.hpp"
#include "dddapi.hpp"
#include "ddddebug.hpp"




#ifdef MONITOR_GAMESTATE
#include "dddstatemonitor.hpp"
#endif

//alias for commonly used tuple
using ChoiceTuple =
  std::tuple<std::vector<DDDReturn>,  //choices
             std::vector<int>,  //extras
             std::vector<std::string>,  //hint strings
             std::vector<std::string>,  //choice strings
             std::vector<int>>;  //choices filter

bool setResponseByInput(const unsigned long long, const ChoiceTuple&, std::string);
bool setResponse(const unsigned long long, const ChoiceTuple&, const int);
bool setResponse(const unsigned long long, const ChoiceTuple&, const std::vector<int>&);
bool setResponse(DuelState&, const ChoiceTuple&);
bool setResponse(DuelState&, const ChoiceTuple&, const std::vector<int>&);

void gdb(intptr_t);

void minDistanceTest();
void printDuelEffects(unsigned long long);
void printDuelGroups(unsigned long long);
void printDuelCoroutines(unsigned long long);
void printLuaStack(lua_State*);
void printLuaStack(lua_State*, const int, intptr_t);
void printDuelLuaStacks(unsigned long long);
void printDuelLuaStacks(unsigned long long, int);
void printLuaGlobals(unsigned long long);
void printLuaRegistryEntries(unsigned long long);
void popEverythingTest(intptr_t);
void printSizes(intptr_t);
void printLuaGlobalsAssociations(unsigned long long);
void printLuaRegistryAssociations(unsigned long long);
void printDuelVarAssociations(unsigned long long);
void printDuelVarScAssociations(unsigned long long);
void printCardCLS(intptr_t);
void searchPtrInDuel(unsigned long long);
void searchRhInDuel(unsigned long long);
void searchDuelVarsByCardName(unsigned long long);
void changeLogOptions();
void callLuaScript(intptr_t);
void indexerTest(intptr_t);
void luaGlobalComparisonTest(intptr_t, intptr_t);
void duelVarComparisonTest(intptr_t, intptr_t);
void printEffectCctvo(intptr_t);
void printDuelProcessorUnits(unsigned long long);

void bruteForceStateChoices(const unsigned long long, const ChoiceTuple&, const int, const unsigned long long, const unsigned long long, std::unordered_map<unsigned long long, ChoiceTuple>&, const std::unordered_set<int>&, const std::string&, const int, const bool, const bool);
std::vector<std::tuple<unsigned long long, std::string, int>> bruteForceStateChoicesAtState(const unsigned long long, std::unordered_map<unsigned long long, ChoiceTuple>&, const std::unordered_set<int>&, const std::unordered_set<int>&, const bool);
void bruteForceStateChoicesBfs(const unsigned long long, const ChoiceTuple&, const int, const unsigned long long, const unsigned long long, std::unordered_map<unsigned long long, ChoiceTuple>&, const std::unordered_set<int>&, const bool, const bool);
void bruteForceStateChoicesDfs(const unsigned long long, const ChoiceTuple&, const int, const unsigned long long, const unsigned long long, std::unordered_map<unsigned long long, ChoiceTuple>&, const std::unordered_set<int>&, const bool, const bool);
void bruteForceStateChoicesDfsMt(const unsigned long long, const ChoiceTuple&, const int, const unsigned long long, const unsigned long long, std::unordered_map<unsigned long long, ChoiceTuple>&, const int, const std::unordered_set<int>&, const bool, const bool);
void bruteForceStateChoicesDfsPar(const unsigned long long, const ChoiceTuple&, const int, const unsigned long long, const unsigned long long, std::unordered_map<unsigned long long, ChoiceTuple>&, const std::unordered_set<int>&, const bool, const bool);
void bruteForceStateChoicesDfsPar2(const unsigned long long, const ChoiceTuple&, const int, const unsigned long long, const unsigned long long, std::unordered_map<unsigned long long, ChoiceTuple>&, const std::unordered_set<int>&, const int, const bool, const bool);
void bruteForceStateChoicesDfsOmp(const unsigned long long, const ChoiceTuple&, const int, const unsigned long long, const unsigned long long, std::unordered_map<unsigned long long, ChoiceTuple>&, const std::unordered_set<int>&, const bool, const bool);

std::vector<std::vector<int>> getPossibleChoiceCombinations(const ChoiceTuple&);
std::tuple<bool, bool, ChoiceTuple> autoProcessLoop(const unsigned long long, const std::unordered_set<int>&, const std::unordered_set<int>&, const bool, const bool);
std::tuple<bool, bool, ChoiceTuple> autoProcessLoop(DuelState&, ShadowDuelStateResponses&, const std::unordered_set<int>&, const std::unordered_set<int>&, const bool, const bool);
std::tuple<bool, bool, ChoiceTuple> autoProcessLoop(DuelState&, ShadowDuelStateResponses&, const std::unordered_set<int>&, const std::unordered_set<int>&, const bool, const bool, const bool);
void gpccInner(const int, const int, const std::vector<int>&, std::vector<int>&, std::vector<std::vector<int>>&);
void clearDuelTest(unsigned long long);



//main function with initialization and main loop
int main(int argc, char* argv[]) {

  try {
    //initialize some vars to defaults
    std::filesystem::path confPath = "./ddd_conf.json";
    unsigned long long dsId = 0;
    unsigned long long dsIdOriginal = dsId;
    bool stayInMainLoop = true;
    bool autoDcsScript = false;
    bool autoStart = false;
    bool autoProcess = false; //perhaps better named shouldAutoProcess
    bool interactCheck = false;
    bool duelInProgress = false;

    std::string cmd = "";
    std::unordered_map<std::string, std::string> cmdArgs;
    std::unordered_set<std::string> cmdFlags;

    //declare choice tuple here and declare some more intuitive
    // variable names for the tuple members (mostly for legacy reasons)
    ChoiceTuple lastValidChoicesTpl;
    auto& lastValidChoices = std::get<0>(lastValidChoicesTpl);
    auto& lastValidExtras = std::get<1>(lastValidChoicesTpl);
    lastValidExtras.push_back(0); //empty msg
    auto& lastValidHintStrings = std::get<2>(lastValidChoicesTpl);
    auto& lastValidChoiceLabels = std::get<3>(lastValidChoicesTpl);
    auto& lastValidFilterChoices = std::get<4>(lastValidChoicesTpl);

    //if tuples are prohibitively expensive to store, could probably
    // get away with just having a set of keys and then getting
    // duel state's choices when necessary
    std::unordered_map<unsigned long long, ChoiceTuple> msgChoicesMap;

    {
      //parse args passed to main function

      //options and other necessary declarations
      struct option long_options[] = {
        {"dcs", required_argument, 0, 'd'}
        ,{"load-dcs-script", required_argument, 0, 'd'}
        ,{"conf", required_argument, 0, 'c'}
        ,{"conf-file", required_argument, 0, 'c'}
        ,{"auto-start", optional_argument, 0, 'a'}
        //...maybe other stuff like seed or exiting after running dcs
        ,{0, 0, 0, 0} //must be last option
      };
      int option_index;
      bool argStatus;
      int c;

      //handle args (1st pass; only handle conf path and unknown args)
      option_index = 0;
      optind = 1; //extern
      argStatus = true;
      while (true) {
        c = getopt_long(argc, argv, "d", long_options, &option_index);
        if (c == -1)
          break;

        switch (c) {
        case 'c':
          {
            confPath = std::filesystem::path(optarg);
          }
          break;
        case '?':
          {
            //error
            argStatus = false;
          }
          break;
        case 'a':
        case 'd':
          //do nothing until 2nd pass
          // (specified here to avoid default case)
          break;
        default:
          {
            clog("w", "Unable to handle arg '", c, "'.");
            argStatus = false;
          }
        }
      }

      //check all args passed to main function were valid
      if (!argStatus) {
        clog("e", "One or more invalid args specified; exiting.");
        return 1;
      }

      //attempt to initialize with conf file
      clog("i", "Initializing...");
      if ((!dddGsInit(confPath, true)) ||
          (!getDDD_GS().conf.lastInitSuccess)) {
        clog("e", "Initialization failed; exiting.");
        return 1;
      }

      //set other vars based on conf file
      autoStart = getDDD_GS().conf.autoStart;

      //handle args (2nd pass; handle everything else)
      // (vals set here override that of the conf file)
      option_index = 0;
      optind = 1; //extern
      argStatus = true;
      while (true) {
        c = getopt_long(argc, argv, "d", long_options, &option_index);
        if (c == -1)
          break;

        std::string argStr = (optarg == NULL) ? "NULL" : optarg;

        switch (c) {
        case 'd':
          {
            autoDcsScript = true;
            std::string dcsStr = "dcs src='" + argStr + "'";
            getDDD_GS().conf.dcsCommandsCache
              .push_back(std::make_pair(dcsStr, -1));
          }
          break;
        case 'a':
          {
            try {
              autoStart = (optarg == NULL) ? true :
                parseStringToBool(argStr);
            } catch (std::exception& e) {
              clog("e", "Invalid argument '", argStr,
                   "' for --auto-start.");
              argStatus = false;
            }
          }
          break;
        }
      }
      if (!argStatus) {
        clog("e", "One or more invalid args specified; exiting.");
        return 1;
      }
    }

    #ifdef MONITOR_GAMESTATE
    //monitor and print changes to duel if enabled
    // (needs to be compiled with the MONITOR_GAMESTATE macro defined
    //  and conf file needs printGamestateChanges under debug
    //  set to true)
    StateMonitorTest smt;
    #endif

    //main loop
    while (stayInMainLoop) {

      //clear previous iteration of command
      cmd.clear();
      cmdArgs.clear();
      cmdFlags.clear();

      //potentially display hint string or hints for
      // possible user actions
      if (interactCheck) {
        if (lastValidChoices.size() > 0)
          clog("i", "Vals set; process to use set vals and continue.");

      } else {
        if (lastValidHintStrings.size() > 0) {
          //hint strings available to print
          for (auto i = lastValidHintStrings.begin(); i !=
                 lastValidHintStrings.end(); ++i)
            if (i == lastValidHintStrings.begin())
              clog("i", *i);
            else
              clog("l", "  ", *i);

          //let user know choices are available to be selected
          if (lastValidChoices.size() > 0)
            clog("i", "Choices are available to be selected.");

        } else {
          //no hint strings and vals not set; so presumably nothing to
          // interact so set to true (doesn't account for if setting
          // to print hint strings was not enabled)
          interactCheck = true;
        }
      }

      //automatically set command under some circumstances
      if (autoDcsScript) {
        //if enabled (only as arg passed to main function),
        // automatically set first command to load a dcs script
        // at the val that was passed with the arg

        autoDcsScript = false;
        autoStart = false; //implies this?

        auto dcsCmd = getDcsCommand().first;
        auto tpl = parseDcsCommand(dcsCmd);
        cmd = std::get<0>(tpl);
        cmdArgs = std::get<1>(tpl);
        cmdFlags = std::get<2>(tpl);

      } else if (autoStart) {
        //if enabled, automatically set first command when
        // entering the main loop to the 'start' command to
        // automatically create a duel state

        autoStart = false;
        cmd = "start";

      } else if ((lastValidChoices.size() == 0) && (autoProcess)) {
        //if auto processing enabled and no choices are
        // available to be selected (for last valid choices),
        // automatically use the 'process' command (without setting
        // any response)

        cmd = "process";

      }

      //check if command automatically set; if not, either get command
      // from dcs or from user input
      if (cmd.empty()) {
        autoProcess = false;

        //try to get next dcs command
        auto p = getDcsCommand();
        std::string& dcsCmd = p.first;

        //check result of dcs command
        if (dcsCmd.empty()) {
          //no dcs command; get from user instead and then parse it
          std::cout << "Enter a command: ";
          std::getline(std::cin, cmd);

          auto tpl = parseDcsCommand(trimString(cmd));
          cmd = std::get<0>(tpl);
          cmdArgs = std::get<1>(tpl);
          cmdFlags = std::get<2>(tpl);

        } else {
          //got dcs command; parse it
          auto tpl = parseDcsCommand(dcsCmd);
          cmd = std::get<0>(tpl);
          cmdArgs = std::get<1>(tpl);
          cmdFlags = std::get<2>(tpl);

          //print dcs command itself and the line number it was on
          // from dcs script (if enabled in conf file)
          if (getDDD_GS().conf.echoDcsCommand) {
            clog("d", "[executing dcs cmd (line ", p.second, ")]: \t ",
                 p.first);
          }
        }
      }

      if (cmd == "gdb") {
        //bit of a helper command to provide an intuitive and easily
        // setable breakpoint to be set in gdb (via "b gdb" in gdb)
        // (also attempt to pass pointer to duel to function if
        //  duel state is active as it might be useful for debugging)

        clog("d", "gdb() called; (c)ontinue to resume.");
        gdb((getDDD_GS().dddapi.duelStates.find(dsId) !=
            getDDD_GS().dddapi.duelStates.end())
            ? getDDD_GS().dddapi.duelStates.at(dsId).pDuel : 0);
        clog("d", "\nEnd of gdb() call.");
        continue;

      } else if (cmd == "dcs") {
        //perform an action relating to dcs commands
        // (if ran with 0 args, will prompt user to input a path
        //  to load dcs script; if ran with only 1 unspecified arg,
        //  will attempt to use arg as path for loading dcs script)

        bool status = true;
        bool ignoreWarnings = (cmdFlags.find("!") != cmdFlags.end());
        std::string src;
        for (const auto &argp: cmdArgs) {
          try {
            if (argp.first == "src") {
              //specify path/src to load dcs script
              src = dequoteString(argp.second);

            } else if ((argp.first == "clear") ||
                       (argp.first == "stop")) {
              //if true (and confirmed), will clear rest of dcs
              // commands in dcs command cache in singleton
              bool shouldClear = parseStringToBool(argp.second);
              if (!shouldClear)
                continue;
              else if (!ignoreWarnings)
                if (!confirmYN("Clear dcs command cache?", false)) {
                  clog("i", "Will not clear remaining dcs command(s).");
                  continue;
                }

              clear_dcs_commands();

              //print number of commands that were left
              int sz = getDDD_GS().conf.dcsCommandsCache.size();
              clog("w", "Clearing dcs command cache (will discard ",
                   sz, " remaining dcs command(s)).");
              getDDD_GS().conf.dcsCommandsCache.clear();

            } else {
              //if only one arg specified in cmd, treat it
              // as path for dcs script
              if ((cmdArgs.size() == 1) &&
                  (parseStringToBool(argp.second))) {
                src = argp.first;
                break;
              }

              //...otherwise invalid arg
              clog("w", "Unknown arg '", argp.first, "' in command '",
                   cmd, "'.");
              status = false;
            }
          } catch (std::exception& e) {
            clog("e", e.what());
            clog("w", "Invalid val '", argp.second, "' for arg '",
                 argp.first, "'.");
            status = false;
          }
        }
        if (!status)
          continue;

        if (!src.empty())
          //attempt to load dcs script from path/src
          loadDcsCommandsFromScript(dequoteString(src));
        else if (cmdArgs.size() == 0)
          //allow user to specify path/src to dcs script (if no
          // other args provided; handled by function by passing
          // empty string to it)
          loadDcsCommandsFromScript("");

        continue;

      } else if ((cmd == "set") || (cmd == "setval") || (cmd == "sv")) {
        //allow user to set certain variables

        bool ignoreWarnings = (cmdFlags.find("!") != cmdFlags.end());
        for (const auto &argp: cmdArgs) {
          try {
            if ((argp.first == "autoProcess") ||
                (argp.first == "auto_process")) {
              //set auto process variable in singleton (not to be
              // confused with the auto process variable in main
              // loop, which is likely to be false anyway for the
              // user to be able to reach here)
              bool val = parseStringToBool(argp.second);
              getDDD_GS().conf.autoProcess = val;
              clog("ok", "Set val ", argp.first, " to: ", !!val);

            } else if ((argp.first == "echoDcsCommand") ||
                       (argp.first == "echo_dcs_command")) {
              //set echo dcs command variable in singleton
              bool val = parseStringToBool(argp.second);
              getDDD_GS().conf.echoDcsCommand = val;
              clog("ok", "Set val ", argp.first, " to: ", !!val);

            } else if ((argp.first == "dsId") ||
                       (argp.first == "dsid")) {
              //set dsid variable in main loop
              // (unlike when done from the swap command, no checks
              //  are done here and even a shadow duel state can be
              //  switched to...as well as nonexistent ones which
              //  will crash the program)

              unsigned long long val = std::stoll(argp.second);
              if (!ignoreWarnings) {
                std::stringstream buffer;
                clog("w", "Attempting to potentially, unsafely set"
                     " duel state (consider using the 'swap' command"
                     " to safely change duel states instead).");
                buffer << "Really set " << val <<
                  " as current/active duel state (this might"
                  " segfault something...)?";

                if (!confirmYN(buffer.str(), false)) {
                  clog("i", "Will not set dsId to new var.");
                  continue;
                }
              }
              dsId = val;
              clog("ok", "Set val '", argp.first, "' to: ", val);

            } else if ((argp.first == "logToFile") ||
                       (argp.first == "log_to_file")) {
              //set the logToFile variable in singleton
              // (not to be confused with the logFile variable;
              //  this variable specifies whether to log clog output
              //  to file or not)

              bool val = parseStringToBool(argp.second);
              if (val) {
                getDDD_GS().conf.log.logToFile = val;
                clog("ok", "Set val ", argp.first, " to: ", !!val);
              }

            } else if ((argp.first == "logFile") ||
                       (argp.first == "log_file")) {
              //set to logFile variable in singleton
              // (not to be confused with the logToFile variable;
              //  this variable specifies the log file's name)

              getDDD_GS().conf.log.logFile = dequoteString(argp.second);
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else if ((argp.first == "useFixedSeed") ||
                       (argp.first == "use_fixed_seed")) {
              //set the useFixedSeed variable in singleton

              bool val = parseStringToBool(argp.second);
              if (val) {
                getDDD_GS().conf.useFixedSeed = val;
                clog("ok", "Set val ", argp.first, " to: ", !!val);
              }

            } else if ((argp.first == "player0DeckPath") ||
                       (argp.first == "player0_deck_path")) {
              getDDD_GS().conf.player0DeckPath =
                dequoteString(argp.second);
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else if ((argp.first == "player1DeckPath") ||
                       (argp.first == "player1_deck_path")) {
              getDDD_GS().conf.player1DeckPath =
                dequoteString(argp.second);
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else if ((argp.first == "player0Lp") ||
                       (argp.first == "player0_lp")) {
              getDDD_GS().conf.player0Lp =
                std::stoi(dequoteString(argp.second));
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else if ((argp.first == "player1Lp") ||
                       (argp.first == "player1_lp")) {
              getDDD_GS().conf.player1Lp =
                std::stoi(dequoteString(argp.second));
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else if ((argp.first == "player0StartDraw") ||
                       (argp.first == "player0_start_draw")) {
              getDDD_GS().conf.player0StartDraw =
                std::stoi(dequoteString(argp.second));
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else if ((argp.first == "player1StartDraw") ||
                       (argp.first == "player1_start_draw")) {
              getDDD_GS().conf.player1StartDraw =
                std::stoi(dequoteString(argp.second));
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else if ((argp.first == "player0DrawPerTurn") ||
                       (argp.first == "player0_draw_per_turn")) {
              getDDD_GS().conf.player0DrawPerTurn =
                std::stoi(dequoteString(argp.second));
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else if ((argp.first == "player1DrawPerTurn") ||
                       (argp.first == "player1_draw_per_turn")) {
              getDDD_GS().conf.player1DrawPerTurn =
                std::stoi(dequoteString(argp.second));
              clog("ok", "Set val '", argp.first, "' to: ", argp.second);

            } else {
              clog("w", "Unknown arg '", argp.first, "' in command '",
                   cmd, "'.");

            }
          } catch (std::exception& e) {
            clog("e", e.what());
            clog("w", "Invalid val '", argp.second, "' for arg '",
                 argp.first, "'.");
            continue;
          }
        }
        continue;

      } else if (cmd == "echo") {
        //print a simple string with a possible flag to using clog()
        // (nothing sophisticated like variables; mostly used for
        //  dcs scripts that to print a hardcoded string)
        bool status = true;
        std::string flag;
        std::string msg;
        for (const auto &argp: cmdArgs) {
          if (argp.first == "flag") {
            flag = dequoteString(argp.second);
          } else if ((argp.first == "msg") || (argp.first == "message")) {
            msg = dequoteString(argp.second);
          } else {
            clog("w", "Unknown arg '", argp.first, "' in command '", cmd, "'.");
            status = false;
          }
        }
        if (!status)
          continue;

        if (flag.empty())
          flag = "d";

        clog(flag, msg);
        continue;

      } else if ((cmd == "help") || (cmd == "h")) {
        //print list of common commands and some of their short forms

        clog("d", "Commands:");
        clog("d", "  (note: some commands may accept args and some only work when used with args.");
        clog("i", "");
        clog("d", "Common commands:");
        clog("i", "h / help\t Print this help.");
        clog("i", "rs / restart\t Start a new duel.");
        clog("i", "q / quit / exit\t Quit program.");
        clog("i", "gs / gamestate\t Print (simplified) gamestate.");
        clog("i", "i / interact\t Show avilable choices and set vals that affect how the duel is processed.");
        clog("i", "p / process\t Do a game tick based on vals set.");
        clog("i", "c / cancel\t Set vals to attempt to cancel or finish (early) current interaction before processing.");
        clog("i", "");
        clog("d", "Other commands:");
        clog("i", "t / test\t Enter testing/debugging menu.");
        clog("i", "d / duplicate\t Attempt to duplicate current duel state.");
        clog("i", "s / swap\t Switch current duel to another duel state.");
        clog("i", "ra / reactivate / activate\t Activate current duel state.");
        clog("i", "da / deactivate\t Deactivate current duel state.");
        clog("i", "bf / bruteforce\t Brute force current duel state.");
        clog("i", "dcs\t Load a dcs script.");
        clog("i", "sv / setval\t Set certain (non-duel) program values.");
        clog("i", "");
        clog("d", "Program start args:");
        clog("i", "-d / --dcs / --load-dcs-script\t Load and run a specified dcs script.");
        clog("i", "-c / --conf / --conf-file\t Load and use a specified conf file.");
        clog("i", "-a / --auto-start\t Immediately start a duel on program start unless set to false.");
        clog("i");

        continue;

      } else if ((cmd == "start") || (cmd == "st") ||
                 (cmd == "restart") || (cmd == "rs")) {
        //start a duel if not already started or if already started,
        // prompt user to destroy current duel and create a new one

        bool status = true;
        bool ignoreWarnings = (cmdFlags.find("!") != cmdFlags.end());
        bool useSpecifiedSeed = false;
        unsigned int seed = 0;

        for (const auto &argp: cmdArgs) {
          if (argp.first == "seed") {
            //enable and use specify seed
            // (if arg specified, overrides variable in conf file to
            //  use random seed or not; if not specified, will use
            //  conf file to either use random seed or to use seed
            //  specified in conf file)
            try {
              seed = std::stoll(argp.second);
              useSpecifiedSeed = true;
            } catch (std::exception& e) {
              clog("e", "Invalid val '", argp.second,
                   "' for arg 'seed'.");
              status = false;
            }
          } else {
            clog("w", "Unknown arg '", argp.first, "' in command '",
                 cmd, "'.");
            status = false;
          }
        }
        if (!status)
          continue;

        //check if duel state created
        if (dsId != 0) {
          //duel state already created
          bool resp = false;
          if (ignoreWarnings) {
            resp = true;
          } else {
            resp = confirmYN("Destroy current duel and start another"
                             " (all duplicated states will also be"
                             " lost)?");
          }
          if (!resp) {
            clog("i", "Keeping current duel.");
            continue;
          }

          clog("w", "Destroying duel...");

          //destroy all duel states (not just current one)
          // need to make a copy since (duelStates) container is
          // invalidated whenever remove_duel_state() called
          std::unordered_set<unsigned long long> dsSet; //copy
          for (auto &d: getDDD_GS().dddapi.duelStates)
            dsSet.insert(d.first);
          for (auto &d: dsSet)
            remove_duel_state(d);

          //reset variables
          dsId = 0;
          lastValidChoices.clear();
          lastValidExtras = {0};
          lastValidHintStrings.clear();
          lastValidChoiceLabels.clear();
          lastValidFilterChoices.clear();
          msgChoicesMap.clear();
          interactCheck = true;
          duelInProgress = true;

          #ifdef MONITOR_GAMESTATE
          smt.reset();
          #endif

          //create new duel state
          clog("i", "Starting another...");
          dsId = ddd::createDuelState(true, useSpecifiedSeed, seed);
          dsIdOriginal = dsId;

        } else {
          //duel state not created yet

          //create new duel state
          dsId = ddd::createDuelState(true, useSpecifiedSeed, seed);
          dsIdOriginal = dsId;
          lastValidExtras = {0};
        }

        autoProcess = getDDD_GS().conf.autoProcess;
        continue;

      } else if ((cmd == "exit") || (cmd == "quit") || (cmd == "q")) {
        //break out of main loop and exit program

        stayInMainLoop = false;
        break;

      } else if ((cmd == "gamestate") || (cmd == "game state") ||
                 (cmd == "gs") || (cmd == "g")) {
        //print visual representation of game state

        //reactivate...?

        auto fvgTuple = ddd::getFieldVisualGamestate(dsId);

        auto fvg0 = std::get<0>(fvgTuple);
        auto fvg1 = std::get<1>(fvgTuple);
        auto fvg2 = std::get<2>(fvgTuple);
        auto fvg3 = std::get<3>(fvgTuple);
        auto fvg4 = std::get<4>(fvgTuple);
        auto fvg5 = std::get<5>(fvgTuple);

        auto printLines = [](const std::vector<std::string>& lines, bool indent) {
          for (auto i = lines.begin(); i != lines.end(); ++i)
            if (i == lines.begin())
              clog("i", *i);
            else
              if (indent)
                clog("l", "  ", *i);
              else
                clog("l", *i);
        };

        //player 1 info + cards in hand
        printLines(fvg4, false);

        //field info
        for (const auto &line: fvg0)
          clog("i", line);

        //player 0 info + cards in hand
        printLines(fvg3, false);

        //cards on player 0's field
        if (fvg1.size() > 1)
          printLines(fvg1, true);

        //cards on player 1's field
        if (fvg2.size() > 1)
          printLines(fvg2, true);

        //chain info
        printLines(fvg5, true);

        //deactivate...?

        continue;

      } else if ((cmd == "test") || (cmd == "testing") || (cmd == "t")) {
        //menu for some miscellaneous functions; primarily those that
        // can either help print an aspect of the gamestate, change
        // some variables or otherwise demonstrate something

        std::vector<std::string> choicesOuter = {
           "Print / output"
          ,"Search"
          ,"Change variables"
          ,"Demonstrations"
          ,"Old stuff"
        };
        std::vector<std::vector<std::string>> choices = {
          {
            //Print / output
             "Print duel groups"
            ,"Print duel effects"
            ,"Print duel var associations"
            ,"Print duel var (self contained) associations"
            ,"Print duel coroutines"
            ,"Print lua stacks"
            ,"Print lua globals"
            ,"Print lua globals associations"
            ,"Print lua registry entries"
            ,"Print lua registry associations"
            ,"Print processor units"
          },
          {
            //Search
             "Search for ptr"
            ,"Search ref handle"
            ,"Search duel vars by card name"
          },
          {
            //Change variables
             "Change log options"
          },
          {
            //Demonstrations
             "Min distance between 2 cards"
            ,"Print card c-l-s info"
            ,"Indexer test"
            //,"Lua global comparison test"
            //,"Lua registry comparison test"
            ,"Compare effect c-c-t-v-o info"
          },
          {
            //Old stuff
             "Print Sizes"
            ,"Call lua script"
            ,"Clear duel"
          }
        };

        clog("d", "Testing/debugging!");

        std::string choiceStr = "";
        int choice = -1;
        int choiceOuter = -1;
        int choiceInner = -1;

        try {
          //print choices for outer menu
          clog("i", "Test/debug choices available:");
          for (int i = 0; i < choicesOuter.size(); ++i) {
            clog("l", "[Choice ", i, "] ", choicesOuter[i]);
          }

          //get input for outer menu
          std::string dcsCmd = getDcsCommand().first;
          if (dcsCmd.empty()) {
            std::cout << "Select a choice: ";
            std::getline(std::cin, choiceStr);
            choiceStr = trimString(choiceStr);
          } else {
            choiceStr = trimString(dcsCmd);
          }

          if ((choiceStr == "c") ||
              (choiceStr == "C") ||
              (choiceStr == "cancel")) {
            clog("i", "Cancelled by user.");
            continue;
          }

          choiceOuter = std::stoi(choiceStr);

          //print choices for inner menu
          clog("i", "Test/debug choices available:");
          for (int i = 0; i < choices[choiceOuter].size(); ++i) {
            clog("l", "[Choice ", i, "] ", choices[choiceOuter][i]);
          }

          //get input for inner menu
          dcsCmd = getDcsCommand().first;
          if (dcsCmd.empty()) {
            std::cout << "Select a choice: ";
            std::getline(std::cin, choiceStr);
            choiceStr = trimString(choiceStr);
          } else {
            choiceStr = trimString(dcsCmd);
          }

          if ((choiceStr == "c") ||
              (choiceStr == "C") ||
              (choiceStr == "cancel")) {
            clog("i", "Cancelled by user.");
            continue;
          }

          choiceInner = std::stoi(choiceStr);
          choiceStr = choices[choiceOuter][choiceInner];

          //call function based on outer and inner selected choice
          {
            intptr_t pDuel = 0;
            if (getDDD_GS().dddapi.duelStates.find(dsId) ==
                getDDD_GS().dddapi.duelStates.end()) {
              clog("w", "Current duel state inactive or invalid.");
              continue;
            }
            pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
            if (!pDuel) {
              clog("w", "Current duel state inactive or invalid.");
              continue;
            }

            if (choiceStr == "Print duel groups") printDuelGroups(dsId);
            if (choiceStr == "Print duel effects") printDuelEffects(dsId);
            if (choiceStr == "Print duel var associations") printDuelVarAssociations(dsId);
            if (choiceStr == "Print duel var (self contained) associations") printDuelVarScAssociations(dsId);
            if (choiceStr == "Print duel coroutines") printDuelCoroutines(dsId);
            if (choiceStr == "Print lua stacks") printDuelLuaStacks(dsId);
            if (choiceStr == "Print lua globals") printLuaGlobals(dsId);
            if (choiceStr == "Print lua globals associations") printLuaGlobalsAssociations(dsId);
            if (choiceStr == "Print lua registry entries") printLuaRegistryEntries(dsId);
            if (choiceStr == "Print lua registry associations") printLuaRegistryAssociations(dsId);
            if (choiceStr == "Print processor units") printDuelProcessorUnits(dsId);
            if (choiceStr == "Search for ptr") searchPtrInDuel(dsId);
            if (choiceStr == "Search ref handle") searchRhInDuel(dsId);
            if (choiceStr == "Search duel vars by card name") searchDuelVarsByCardName(dsId);
            if (choiceStr == "Change log options") changeLogOptions();
            if (choiceStr == "Min distance between 2 cards") minDistanceTest();
            if (choiceStr == "Print card c-l-s info") printCardCLS(pDuel);
            if (choiceStr == "Indexer test") indexerTest(pDuel);
            if (choiceStr == "Lua global comparison test") clog("d", "Not implemented.");
            if (choiceStr == "Lua registry comparison test") clog("d", "Not implemented.");
            if (choiceStr == "Compare effect c-c-t-v-o info") printEffectCctvo(pDuel);
            if (choiceStr == "Print Sizes") printSizes(pDuel);
            if (choiceStr == "Call lua script") callLuaScript(pDuel);
            if (choiceStr == "Clear duel") clearDuelTest(dsId);
          }

        } catch (std::exception& e) {
          clog("e", e.what());
          clog("w", "  (invalid last choice / selection '", choiceStr,
               "'?)");
          continue;
        }

        continue;

      } else if ((cmd == "clear") || (cmd == "cls")) {
        //attempt to clear terminal screen

        #ifdef BUILD_WIN

        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo (GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        std::cout << std::string(csbi.srWindow.Bottom - csbi.srWindow.Top, '\n')
                  << std::endl;

        #else
        clog("d", "Not implemented.");
        #endif
        continue;

      } else if ((cmd == "swap") || (cmd == "switch") || (cmd == "s")) {
        //allow user to set the current duel state

        //check if any other (non shadow) duel states exist
        if (getDDD_GS().dddapi.duelStates.size() < 2) {
          clog("i", "No other duel states available.");
          continue;
        }

        //generate list of available duel states to switch to
        std::set<unsigned long long> dsSet;
        std::vector<std::string> dsStrV;
        for (const auto &ds: getDDD_GS().dddapi.duelStates) {
          dsSet.insert(ds.first);
        }

        if (dsSet.size() > 200) {
          //only print first and last 100 duel states of set
          auto i = dsSet.begin();
          auto j = dsSet.rbegin();
          for (int k = 0; k < 100; ++k, ++i)
            dsStrV.push_back(std::to_string(*i));
          dsStrV.push_back("...... ");

          std::vector<std::string> dsStrRV;
          for (int k = 0; k < 100; ++k, ++j)
            dsStrRV.push_back(std::to_string(*j));
          std::reverse(dsStrRV.begin(), dsStrRV.end());
          dsStrV.insert(dsStrV.end(), dsStrRV.begin(), dsStrRV.end());

        } else {
          //print entirety of set
          for (const auto &ds: dsSet)
            dsStrV.push_back(std::to_string(ds));
        }

        //print available duel states to switch to (or a partial
        // list if excessive)
        clog("i", "\nAvailable duel states:");
        clog("l", "        ", joinString(dsStrV, ", "));
        clog("l", "    Original duel state: ", dsIdOriginal);
        clog("l", "    Currently selected duel state: ", dsId);

        //get dcs command or otherwise user input
        std::cout << "Enter a duel state: ";
        std::string dsStr = "";
        unsigned long long selectedDs = -1;

        std::string dcsCmd = getDcsCommand().first;
        if (dcsCmd.empty()) {
          std::getline(std::cin, dsStr);
          dsStr = trimString(dsStr);
        } else {
          dsStr = dcsCmd;
        }

        //validate selected duel state

        //check selection valid
        try {
          selectedDs = std::stoi(dsStr);
        } catch (std::exception &e) {
          clog("e", e.what());
          clog("w", "Invalid duel state '", dsStr,
               "'; duel state not swapped.");
          continue;
        }

        //check selection is not already current duel state id
        if (selectedDs == dsId) {
          clog("w", "Already using duel state ", dsId,
               "); duel state not swapped.");
          continue;
        }

        //check selection exists in singleton (duel states map)
        if (getDDD_GS().dddapi.duelStates.find(selectedDs) ==
            getDDD_GS().dddapi.duelStates.end()) {
          if (getDDD_GS().dddapi.shadowDuelStates.find(selectedDs) !=
              getDDD_GS().dddapi.shadowDuelStates.end()) {
            //exists as shadow state, which is an invalid selection
            clog("w", "Invalid duel state ", selectedDs, " selected"
                 " (is a shadow duel state and cannot be selected);"
                 "  duel state not swapped.");
          } else {
            //doesn't exist
            clog("w", "Invalid duel state ", selectedDs, " selected"
                 " (was not found); duel state not swapped.");
          }
          continue;
        }

        //maybe should deactivate dsId before swapping and
        // possibly reactivate selectedDs?

        //save previous last choices tuple and set selected duel state
        msgChoicesMap[dsId] = lastValidChoicesTpl;
        dsId = selectedDs;

        //update current last choices tuple
        const auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
        lastValidChoicesTpl = ddd::getChoicesFromDuelState(ds, true, {});

        autoProcess = false;
        interactCheck = false;

        clog("i", "Swapped to duel state ", dsId, ".");
        continue;
      }


      //check if duel state valid (because commands beyond this
      // point would not be valid in these cases)
      if (dsId == 0) {
        //check if duel state has been created
        clog("w", "Duel not created yet. Start one first.");
        autoProcess = false;
        continue;

      } else if (lastValidExtras[0] == MSG_WIN) {
        //check if one of the players has already won
        clog("l", "The duel is over! Start another!");
        autoProcess = false;
        continue;

      }


      #ifdef MONITOR_GAMESTATE
      //update gamestate monitor variables
      // (requires printGamestateChanges in the conf file under debug
      //  to be set to true)
      // (can be very verbose(!) but output can be filtered)
      // (note that since process() has a loop inside of it, there
      //  are likely changes that will be entirely missed since
      //  check here only happens when process is finished;
      //  e.g. a variable was set, then unset before the process ends)
      if (getDDD_GS().conf.debug.printGamestateChanges) {
        if (getDDD_GS().dddapi.duelStates.find(dsId) !=
            getDDD_GS().dddapi.duelStates.end()) {
          if (is_duel_state_active(dsId)) {
            intptr_t pD = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
            smt.captureGamestate(pD);
          }
        }
      }
      #endif


      if ((cmd == "interact") || (cmd == "i")) {
        //select one of the available choices for current message
        // and set response corresponding with the choice
        // (without processing)

        if (lastValidChoices.size() < 1) {
          clog("w", "No last valid msg (or need) to interact with/set values.");
          continue;
        }

        bool status = true;
        bool ignoreWarnings = (cmdFlags.find("!") != cmdFlags.end());
        std::string choiceStr;

        for (const auto &argp: cmdArgs) {
          if ((argp.first == "choice") || (argp.first == "c")) {
            choiceStr = argp.second;

          } else {

            //if only one arg specified in cmd, can also treat it
            // as val for choice
            if ((cmdArgs.size() == 1) &&
                (parseStringToBool(argp.second))) {
              choiceStr = argp.first;
              break;
            }

            clog("w", "Unknown arg '", argp.first, "' in command '",
                 cmd, "'.");
            status = false;
          }
        }
        if (!status)
          continue;

        //attempt to set response
        interactCheck =
          setResponseByInput(dsId, lastValidChoicesTpl, choiceStr);

        //print hint specifically for MSG_SELECT_UNSELECT_CARD
        if ((lastValidExtras[0] == MSG_SELECT_UNSELECT_CARD) &&
            (interactCheck) && (!ignoreWarnings))
          clog("l", "  (note: process again before selecting the"
               " next card(s))");

      } else if ((cmd == "process") || (cmd == "p")) {
        //attempt to perform a process iteration on duel state and
        // if successful, updates the last valid choice tuple
        // (and if also enabled, will enable auto processing)

        bool status = true;
        bool ignoreWarnings = (cmdFlags.find("!") != cmdFlags.end());
        bool shouldProcess = true;
        std::string choiceStr;
        for (const auto &argp: cmdArgs) {
          if ((argp.first == "choice") || (argp.first == "c")) {
            choiceStr = argp.second;
          } else {

            //if only one arg specified in cmd, can also treat it
            // as val for choice
            if ((cmdArgs.size() == 1) &&
                (parseStringToBool(argp.second))) {
              choiceStr = argp.first;
              break;
            }

            clog("w", "Unknown arg '", argp.first, "' in command '", cmd, "'.");
            status = false;
          }
        }
        if (!status)
          continue;

        //set response here if choice arg passed and valid
        // (instead of needing it through the interact command)
        if (!choiceStr.empty()) {
          interactCheck =
            setResponseByInput(dsId, lastValidChoicesTpl, choiceStr);
          if (!interactCheck)
            continue;
        }

        if (!ignoreWarnings) {
          if ((!autoProcess) && (!interactCheck)) {
            if (lastValidChoices.size() > 0) {
              clog("w", "Attempted to process without interacting where options"
                   " are available.");
              shouldProcess = confirmYN("Really skip interacting and continue"
                                        " with processing anyway?");
            } else {
              clog("w", "Attempted to process without interacting.");
              shouldProcess = confirmYN("Continue with processing?");
            }

            if (shouldProcess) {
              clog("i", "Will continue processing.");
            } else {
              clog("i", "Process command cancelled.");
              continue; //return to start of main loop
            }
          }
        }

        //attempt a process iteration
        process_duel_state(dsId);

        //get choice tuple after processing
        const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
        auto choicesTpl = ddd::getChoicesFromDuelState(ds, true, {});
        const int msg = std::get<1>(choicesTpl)[0];

        //handle based on message
        if (msg == 0) {
          //got an (assumed to be) empty buffer
          clog("d", "Skipping empty buffer.");
          lastValidChoicesTpl = choicesTpl; //maybe move?

          autoProcess = getDDD_GS().conf.autoProcess;
          interactCheck = true; //maybe?

        } else if (msg != 1) {
          //got a nonerror message
          lastValidChoicesTpl = choicesTpl; //maybe move?

          autoProcess = getDDD_GS().conf.autoProcess;
          interactCheck = false;

        } else {
          //got an error (MSG_RETRY)
          autoProcess = false;
          interactCheck = false; //maybe?

          for (auto i = std::get<2>(choicesTpl).begin();
               i != std::get<2>(choicesTpl).end(); ++i) {
            if (i == std::get<2>(choicesTpl).begin())
              clog("w", *i);
            else
              clog("w", "  ", *i);
          }
        }

        //stop auto processing and issuing dcs commands if process did not succeed
        if (dsId) {
          if (msg == 1) {
            if (!getDDD_GS().conf.dcsCommandsCache.empty()) {
              autoProcess = false;
              clog("w", "Prematurely ending dcs script because last"
                   " process did not succeed.");
              clog("w", "  (stopped before executing '",
                   getDDD_GS().conf.dcsCommandsCache.front().first,
                   "' on line ",
                   getDDD_GS().conf.dcsCommandsCache.front().second,
                   " of dcs script; discarding ",
                   getDDD_GS().conf.dcsCommandsCache.size(),
                   " remaining commands)");
              getDDD_GS().conf.dcsCommandsCache.clear();
            }
          }
        }

      } else if ((cmd == "cancel") || (cmd == "c")) {
        //set the response to -1 (without processing)
        // (usually does nothing since most commands cannot be
        //  cancelled once processed but depending on the message,
        //  this command may instead, cancel the command, finish
        //  the command early or be equivalent to selecting
        //  none of the choices)

        set_duel_state_responsei(dsId, -1);
        clog("i", "Val set to attempt to cancel current command");
        clog("l", "  (process to see if action cancellable or interact"
             " again to set new vals.)");
        interactCheck = true;

      } else if ((cmd == "duplicate") || (cmd == "d")) {
        //creates a copy of the duel state such that responses set
        // and process iterations performed on one duel state do not
        // affect the other
        bool resp = false;
        bool ignoreWarnings = (cmdFlags.find("!") != cmdFlags.end());
        for (const auto &argp: cmdArgs) {
          //currently nothing
        }

        if (ignoreWarnings) {
          resp = true;
        } else {
          resp = confirmYN("Duplicate current gamestate?", false);
        }
        if (!resp) {
          clog("i", "Gamestate was not duplicated.");
          continue;
        }

        int sdsmSizeOld = getDDD_GS().dddapi.duelStates.size();
        unsigned long long sourceDsId = dsId;

        //duplicate the duel state
        unsigned long long newDsId = duplicate_duel_state(sourceDsId);

        if (!newDsId) {
          clog("e", "Unable to duplicate duel state ", sourceDsId, ".");
          continue;
        }

        //bit of a check to ensure duel state duplicated properly
        int sdsmSizeNew = getDDD_GS().dddapi.duelStates.size();
        if (sdsmSizeNew != (sdsmSizeOld + 1)) {
          clog("e", "Size of gamestate mappings did not change as expected.");
          clog("l", "(got: ", sdsmSizeNew, "; expected: ",
               (sdsmSizeOld + 1), ")");
          // clog("d", "(old addr: ", sdsmAddrOld, "; new addr: ",
          //      sdsmAddrNew, ")");
          clog("e", "Unable to duplicate duel state ", sourceDsId, ".");
          remove_duel_state(newDsId);
          continue;
        }

        clog("ok", "Successfully duplicated current gamestate!");
        msgChoicesMap[newDsId] = lastValidChoicesTpl;
        msgChoicesMap[sourceDsId] = lastValidChoicesTpl;

        //prompt user to switch to duplicated gamestate or not
        if (confirmYN("Switch to new gamestate?", false)) {
          clog("i", "Current duel state is now ", newDsId,
               " (duplicated from duel state ", sourceDsId, ").");
          dsId = newDsId;

        } else {
          clog("i", "Continuing to use duel state ", sourceDsId,
               " (duplicated duel state: ", newDsId, ").");
          dsId = sourceDsId;

        }

      } else if ((cmd == "bruteforce") || (cmd == "bf")) {
        //attempt to brute force all possible duel states up to a
        // specified depth by generating a duel state for each
        // valid state and saving the state and its choices (if any)
        // by updating the msgChoicesMap map

        bool status = true;
        int maxDepth = 4;
        std::string type = "dfs";
        bool showMessages = false;
        bool ignoreWarnings = (cmdFlags.find("!") != cmdFlags.end());
        std::unordered_set<int> filter;
        int threadCount = std::thread::hardware_concurrency();
        unsigned long long maxDepthResultsLimit = 0;
        unsigned long long showResultsLimit = 0; //0 for no limit

        for (const auto &argp: cmdArgs) {
          try {
            if ((argp.first == "depth") || (argp.first == "maxDepth")) {
              maxDepth = std::stoll(argp.second);

            } else if ((argp.first == "showMsg") ||
                       (argp.first == "show_msg") ||
                       (argp.first == "showMessages") ||
                       (argp.first == "show_messages")) {
              showMessages = parseStringToBool(argp.second);

            } else if (argp.first == "filter") {
              std::string filterStr =
                trimString(dequoteString(argp.second));
              if (filterStr.empty())
                continue; //empty filter

              auto choiceFilterSplit = splitString(filterStr, ",");
              for (const auto &cf: choiceFilterSplit)
                filter.insert(std::stoi(cf));

            } else if (argp.first == "type") {
              type = trimString(argp.second);

            } else if ((argp.first == "threads") ||
                       (argp.first == "maxThreads") ||
                       (argp.first == "max_threads") ||
                       (argp.first == "threadCount") ||
                       (argp.first == "thread_count")) {
              threadCount = std::stoi(dequoteString(argp.second));

            } else if ((argp.first == "resultsLimit") ||
                       (argp.first == "results_limit") ||
                       (argp.first == "maxDepthResultsLimit") ||
                       (argp.first == "max_depth_results_limit")) {
              maxDepthResultsLimit = std::stoll(argp.second);

            } else if ((argp.first == "showResultsLimit") ||
                       (argp.first == "show_results_limit") ||
                       (argp.first == "showLimit") ||
                       (argp.first == "show_limit")) {
              showResultsLimit = std::stoll(argp.second);

            } else {
              clog("w", "Unknown arg '", argp.first, "' in command '",
                   cmd, "'.");
              status = false;
            }
          } catch (std::exception& e) {
            clog("e", e.what());
            clog("w", "Invalid val '", argp.second, "' for arg '",
                 argp.first, "'.");
            status = false;
          }
        }
        if (!status)
          continue;

        bruteForceStateChoices(dsId, lastValidChoicesTpl, maxDepth,
                               maxDepthResultsLimit, showResultsLimit,
                               msgChoicesMap, filter, type,
                               threadCount, showMessages,
                               ignoreWarnings);

      } else if ((cmd == "activate") || (cmd == "reactivate") ||
                 (cmd == "ra")) {
        //attempts to activate a duel state and returns whether
        // it succeeded or not
        if (reactivate_duel_state(dsId)) {
          clog("ok", "Duel state ", dsId, " active.");
          intptr_t pDuel
            = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
          unsigned long long sdsId
            = getDDD_GS().dddapi.duelStates.at(dsId).shadowDsId;
          intptr_t pDuelShadow
            = getDDD_GS().dddapi.shadowDuelStates.at(sdsId).pDuel;
          clog("l", "  (pDuel: ", pDuel, " (", (duel*) pDuel,
               "); shadow pDuel: ", pDuelShadow, " (",
               (duel*) pDuelShadow, "))");
        }

      } else if ((cmd == "deactivate") || (cmd == "da")) {
        //attempts to deactivate a duel state and returns whether
        // it succeeded or not
        if (deactivate_duel_state(dsId)) {
          clog("ok", "Duel state ", dsId, " not active.");
        }

      } else {
        clog("w", "Not sure what '", cmd,
             "' is; use the 'help' command for a list of commands.");
      }
    }

    //destroy all duel states (not just current one)
    // need to make a copy since (duelStates) container is
    // invalidated whenever remove_duel_state() called
    std::unordered_set<unsigned long long> dsSet; //copy
    unsigned int activeCount = 0;
    unsigned int unactiveCount = 0;
    for (auto &d: getDDD_GS().dddapi.duelStates)
      dsSet.insert(d.first);

    if (dsSet.size() > 0)
      clog("i", "Destroying ", dsSet.size(), " duels...");

    for (auto &d: dsSet) {
      if (is_duel_state_active(d))
        ++activeCount;
      else
        ++unactiveCount;

      remove_duel_state(d);
    }

    clog("i", "Destroyed ", dsSet.size(), " duels (active: ",
         activeCount, ";  unactive: ", unactiveCount, ") ...");

    clog("ok", "Exiting...\n");
    return 0;

  } catch (std::exception &e) {
    clog("e", e.what());
    clog("e", "Uncaught exception. Exiting...");
    return 1;
  }
}



bool setResponseByInput(unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, std::string choiceStr) {
  bool status = false;

  const auto& choices = std::get<0>(lastValidChoicesTpl);
  const auto& extras = std::get<1>(lastValidChoicesTpl);
  const auto& hintStrings = std::get<2>(lastValidChoicesTpl);
  const auto& choiceLabels = std::get<3>(lastValidChoicesTpl);
  const int msg = extras[0];

  int minChoices = 1;
  int maxChoices = 1;

  if (msg == 15) { //MSG_SELECT_CARD
    if (extras.size() < 4) {
      clog("e", "Malformed extras received for msg 15: ", extras[0]);
      clog("d", extras.size());
      return false;
    }
    minChoices = extras[1];
    maxChoices = extras[2];

  } else if (msg == 23) { //MSG_SELECT_SUM
    if (extras.size() < 6) {
      clog("e", "Malformed extras received for msg 23: ", extras[0]);
      return false;
    }
    minChoices = extras[1];
    maxChoices = extras[2];
  }

  if (choiceStr.empty()) {
    std::string inputString = "";

    if ((minChoices > 1) || (minChoices != maxChoices)) {
      inputString += "Select ";
      if (maxChoices == 99) {
        inputString += "(" + std::to_string(minChoices) + "+)";
      } else if (minChoices != maxChoices) {
        inputString += "(" + std::to_string(minChoices) + " - " +
          std::to_string(maxChoices) + ")";
      } else {
        inputString += "(" + std::to_string(minChoices) + ")";
      }
      inputString += " choices (if selecting multiple choices,"
        " separate by commas).";
    } else {
      inputString += "Select a choice.";
    }

    for (auto i = hintStrings.begin(); i != hintStrings.end(); ++i)
      if (i == hintStrings.begin())
        clog("i", *i);
      else
        clog("l", "  ", *i);

    clog("i", "Available choices:");

    for (auto i = 0; i < choiceLabels.size(); ++i)
      clog("l", "  [Choice ", i, "] ", choiceLabels[i]);

    clog("i", inputString);

    std::string dcsCmd = getDcsCommand().first;
    if (dcsCmd.empty()) {
      std::cout << "Choice(s): ";
      std::getline(std::cin, choiceStr);
    } else {
      choiceStr = dcsCmd;
    }
  }

  if ((trimString(choiceStr) == "c") ||
      (trimString(choiceStr) == "C") ||
      (trimString(choiceStr) == "cancel")) {
    clog("w", "Cancelled by user; vals not set.");
    return false;
  }

  std::set<int> selectionsSet;

  try {
    std::vector<std::string> selectionsStr = splitString(choiceStr, ",");
    for (const auto &s: selectionsStr) {
      selectionsSet.insert(std::stoi(trimString(s)));
    }

  } catch (std::exception& e) {
    clog("e", e.what());
    clog("w", "Invalid input '", choiceStr, "'; vals not set.");
    return false;
  }


  if (selectionsSet.size() < minChoices) {
    clog("w", "Not enough choices selected (", selectionsSet.size(),
         " given; need at least ", minChoices, ").");
    clog("w", "Vals not set.");
    return false;
  }
  if (selectionsSet.size() > maxChoices) {
    clog("w", "Too many choices selected (", selectionsSet.size(),
         " given; cannot select more than ", maxChoices, ").");
    clog("w", "Vals not set.");
    return false;
  }
  for (const auto &s: selectionsSet) {
    if ((s < 0) || (s >= choices.size())) {
      clog("w", "Invalid choice '", s, "' selected.");
      clog("w", "Vals not set.");
      return false;
    }
  }

  if (setResponse(dsId, lastValidChoicesTpl, std::vector<int>
                  (selectionsSet.begin(), selectionsSet.end()))) {
    if (selectionsSet.size() == 0) {
      clog("i", "No selections were made (but response was set).");
    } else if (selectionsSet.size() == 1) {
      clog("i", "Selected: ", choiceLabels[*(selectionsSet.begin())]);
    } else {
      clog("i", "Selected:");
      for (const auto &s: selectionsSet)
        if (s < choiceLabels.size())
          clog("l", "  ", choiceLabels[s]);
        else
          clog("w", "  ", "Unknown choice '", s, "'");
    }
    return true;
  } else {
    clog("e", "Vals were not set.");
  }

  return status;
}

bool setResponse(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const int selection) {
  return setResponse(dsId, lastValidChoicesTpl, {});
}
bool setResponse(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const std::vector<int>& selections) {

  auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
  return setResponse(ds, lastValidChoicesTpl, selections);
}
bool setResponse(DuelState& ds, const ChoiceTuple& lastValidChoicesTpl) {
  return setResponse(ds, lastValidChoicesTpl, {});
}
bool setResponse(DuelState& ds, const ChoiceTuple& lastValidChoicesTpl, const std::vector<int>& selections) {
  bool status = true;

  const auto& choices = std::get<0>(lastValidChoicesTpl);
  const auto& extras = std::get<1>(lastValidChoicesTpl);
  const int msg = extras[0];

  try {
    if (msg == 15) {
      byte ba[64];
      ba[0] = selections.size();
      for (int i = 1; i < 64; ++i)
        if ((i - 1) < selections.size())
          ba[i] = selections[i - 1];
        else
          ba[i] = 0;

      ddd::setDuelStateResponseB(ds, ba);

    } else if (msg == 23) {
      byte ba[64];
      int8 mustSelectSize = extras[5];
      ba[0] = selections.size() + mustSelectSize;

      for (int i = 1; i < 64; ++i)
        if ((i - 1) < mustSelectSize)
          ba[i] = (i - 1);
        else if ((i - mustSelectSize - 1) < selections.size())
          ba[i] = selections[i - mustSelectSize - 1];
        else
          ba[i] = 0;

      ddd::setDuelStateResponseB(ds, ba);
    } else {
      if (choices[0].iactive) {
        ddd::setDuelStateResponseI(ds, choices[selections[0]].u.ivalue);
      } else {
        ddd::setDuelStateResponseB(ds, choices[selections[0]].u.bvaluebytes);
      }
    }
  } catch (std::exception &e) {
    clog("e", e.what());
    return false;
  }
  return status;
}

//convenience function to allow for "b gdb" to set a breakpoint here
// in gdb and entered when the "gdb" command (in this program) is used
void gdb(intptr_t pD) { int dddgdb = 0xDDD6DB; } //sup

void minDistanceTest() {
  if (!confirmYN("Brute force min distance between card codes?", false)) {
    clog("l", "Cancelled brute force.");
    return;
  }
  clog("i", "Brute forcing min distance between card codes!"
       " This might take a while...");
  int minDiff = 99999999;
  std::vector<std::pair<int32,int32>> v;

  int currItr = 0;
  int totalItrs = 0;
  for (int i = 0; i < getDDD_GS().cardDbs.cardDb.size(); ++i) {
    totalItrs += i;
  }

  for (auto itr1 = getDDD_GS().cardDbs.cardDb.begin();
       itr1 != getDDD_GS().cardDbs.cardDb.end(); ++itr1) {
    for (auto itr2 = itr1; itr2 != getDDD_GS().cardDbs.cardDb.end(); ++itr2) {
      if (((currItr % 5000000) == 0) || ((currItr + 0) == totalItrs)) {
        clog("l", "itr = ", currItr, " / ", totalItrs, " (",
             (currItr/(totalItrs/100)), "%)");
      }
      ++currItr;

      if (itr1 != itr2) {
        std::string s1 = (decodeCode((*itr1).second.code));
        if (s1.length() > 8)
          if (s1.substr(s1.length() - 7).find(" Token") != std::string::npos)
            continue;
        std::string s2 = (decodeCode((*itr2).second.code));
        if (s2.length() > 8)
          if (s2.substr(s2.length() - 7).find(" Token") != std::string::npos)
            continue;
        if (s1 == s2) continue;
        int diff = (*itr1).second.code - (*itr2).second.code;
        if (diff == 0) continue;
        if (diff < 0) diff *= -1;
        int min = 5;
        if (diff < minDiff) {
          if (minDiff > min) {
            minDiff = diff;
            v.clear();
          }
          v.push_back(std::make_pair((*itr1).second.code, (*itr2).second.code));
        } else if ((diff == minDiff) || (diff <= min))  {
          v.push_back(std::make_pair((*itr1).second.code, (*itr2).second.code));
        }
      }
    }
  }
  clog("d", "Closest distance between codes = ", minDiff);
  for (auto vi: v) {
    int diff = vi.first - vi.second;
    if (diff < 0) diff *= -1;
    clog("l", vi.first, " and ", vi.second, " (diff=", diff, ")  [",
         decodeCode(vi.first), "] and [", decodeCode(vi.second), "]");
  }
}


void printDuelEffects(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print effects; current duel state id (",
         dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print effects; current duel state id (",
         dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;

  clog("d", "Printing select details of effect set in duel ptr.");
  int i = 0;
  std::vector<std::string> refHandleChain; //temp!

  for (auto effect: pD->effects) {
    if (!decodeCode(effect->owner->data.code).empty()) {
      clog("i", i, " [ref_handle=", effect->ref_handle, "] ",
           decodeCode(effect->owner->data.code),
           "  desc=", decodeDesc(effect->description));

    } else {
      clog("w", i, " [ref_handle=", effect->ref_handle, "] code=",
           effect->owner->data.code, "  desc=", effect->description);
      printEffectDebug(effect, 1);

    }

    ++i;
  }

  clog("d", "field aura_effects size = ",
       pD->game_field->effects.aura_effect.size());
  for (auto x: pD->game_field->effects.aura_effect) {
    clog("l", x.first, ": ", x.second->owner->data.code,
         "  desc = ", x.second->description, "  (",
         decodeDesc(x.second->description), ")  flag[2] = [",
         x.second->flag[0], ", ", x.second->flag[1],"]");
  }

  clog("d", "field continuous_effects size = ",
       pD->game_field->effects.continuous_effect.size());
  for (auto x: pD->game_field->effects.continuous_effect) {
    clog("l", x.first, ": ", x.second->owner->data.code,
         "  desc = ", x.second->description, "  (",
         decodeDesc(x.second->description), ")  flag[2] = [",
         x.second->flag[0], ", ", x.second->flag[1],"]");
  }

  clog("d", "effect_count_code size = ",
       pD->game_field->core.effect_count_code.size());
  for (auto x: pD->game_field->core.effect_count_code) {
    uint32 code = x.first;
    uint32 count = x.second;
    int player = x.first & 0x10000000;

    std::string codeDecoded = decodeCode(code);
    if (!codeDecoded.empty())
      codeDecoded = " (" + codeDecoded + ")";

    clog("l", "code: ", code, codeDecoded, "\t  times activated: ", count,
         "  (for player ", player, ")");
  }
  clog("d", "effect_count_code_duel size = ",
       pD->game_field->core.effect_count_code_duel.size());
  for (auto x: pD->game_field->core.effect_count_code_duel) {
    uint32 code = x.first;
    uint32 count = x.second;
    int8 player = x.first & 0x10000000;

    std::string codeDecoded = decodeCode(code);
    if (!codeDecoded.empty())
      codeDecoded = " (" + codeDecoded + ")";

    clog("l", "code: ", code, codeDecoded, "\t  times activated: ", count,
         "  (for player ", player, ")");
  }
  clog("d", "effect_count_code_chain size = ",
       pD->game_field->core.effect_count_code_chain.size());
  for (auto x: pD->game_field->core.effect_count_code_chain) {
    uint32 code = x.first;
    uint32 count = x.second;
    int player = x.first & 0x10000000;

    std::string codeDecoded = decodeCode(code);
    if (!codeDecoded.empty())
      codeDecoded = " (" + codeDecoded + ")";

    clog("l", "code: ", code, codeDecoded, "\t  times activated: ", count,
         "  (for player ", decodePlayer(player), ")");
  }
}

void printDuelGroups(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print groups; current duel state id (",
         dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print groups; current duel state id (",
         dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;

  clog("i", "duel->groups (", pD->groups.size(),
       ((pD->groups.size()) ? "):" : ")"));
  for (auto g: pD->groups) {
    clog("i", "Group g(", g, ")  ref_handle: ", g->ref_handle,
         "  (container size: ", g->container.size(), ")");
    for (auto gc: g->container) {
      clog("l", "\tc(", gc, ") [", gc->data.code, "] ", decodeCode(gc->data.code));
    }
  }
  clog("l", "");
  clog("i", "duel->sgroups (", pD->sgroups.size(),
       ((pD->sgroups.size()) ? "):" : ")"));
  for (auto s: pD->sgroups) {
    clog("i", "(S)Group g(", s, ")  ref_handle: ", s->ref_handle,
         "  (container size: ", s->container.size(), ")");

    if (pD->groups.find(s) == pD->groups.end())
      clog("w", "(this (s)group does not exist in duel.groups)");

    for (auto sc: s->container) {
      clog("l", "\tc(", sc, ") [", sc->data.code, "] ", decodeCode(sc->data.code));
    }
  }
}

void printLuaStack(lua_State *L) {
  printLuaStack(L, 0, 0);
}
void printLuaStack(lua_State *L, const int depth, intptr_t pDuel) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  int top = lua_gettop(L);

  if (top == 0) {
    clog("l", depthIndent, "Stack is empty (size = 0).");
    return;
  }

  for (int i = 1; i <= top; ++i) { //maybe set i to 0 anyway?
    std::string type = lua_typename(L, lua_type(L, i));
    std::string val = luaValueAtIndexToString(L, i);
    const void* ptr = lua_topointer(L, i);

    if (type == "thread") {
      clog("i", depthIndent, "idx: ", i, "  type: ", type, "\tptr: (", ptr,
           ") \tstatus: ", lua_status(lua_tothread(L, i)));
      printLuaStack((lua_State*) ptr, (depth + 1), pDuel);

    // } else if (type == "table") {
    //   clog("i", depthIndent, "idx: ", i, "  type: ", type, "\tptr: (", ptr, ")");
    //   lua_pushvalue(L, i);
    //   printLuaGlobals(L, (depth + 1), false);
    //   lua_pop(L, 1);

    } else {
      clog("i", depthIndent, "idx: ", i, "  type: ", type, "\tval: ", val);

      if ((pDuel != 0) && ((type == "userdata") || (type == "function"))) {
        duel* pD = (duel*) pDuel;

        bool foundInDuelVars = printDuelVarsByPtr(pDuel, ptr, (depth + 1), false);
        bool foundInLuaGlobals = printLuaGlobalsByPtr(pD->lua->lua_state, ptr,
                                                      (depth + 1), false);
        bool foundInLuaRegistry = printLuaRegistryEntryByPtr(pD->lua->lua_state,
                                                             ptr, (depth + 1),
                                                             false);
        if ((!foundInDuelVars) && (!foundInLuaGlobals) && (!foundInLuaRegistry)) {
          clog("w", depthIndent, indent, "Ptr (", ptr,
               ") does not seem to be associated with any known"
               " duel vars or lua globals.");
        }
      }
    }
  }
}


void printDuelLuaStacks(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print duel lua stacks; current duel state id (",
         dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print duel lua stacks; current duel state id (",
         dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;

  clog("i", "Printing stack for lua_state (", pD->lua->lua_state, "):");
  printLuaStack(pD->lua->lua_state, 0, pDuel);
  if (pD->lua->lua_state == pD->lua->current_state) {
    clog("i", "Note: current_state is currently equal to lua_state.");
  } else if (pD->lua->current_state == lua_topointer(pD->lua->lua_state, -1)) {
    clog("i", "Note: current_state is currently equal to the element at"
         " top of lua_state stack.");
  } else {

    int foundIndex = -1;
    int lsTop = lua_gettop(pD->lua->lua_state);
    for (int i = 1; i <= lsTop; ++i) {
      const void* ptr = lua_topointer(pD->lua->lua_state, i);

      if (pD->lua->current_state == ptr) {
        foundIndex = i;
        break;
      }
    }

    if (foundIndex != -1) {
      std::string type = lua_typename(pD->lua->lua_state,
                                      lua_type(pD->lua->lua_state, foundIndex));
      clog("i", "Note: current_state is currently equal to ", type, " at index ",
           foundIndex, " of lua_state.");
    } else {
      clog("w", "Note: current_state is neither equal to lua_state or any"
           " element in lua_state stack.");
      clog("i", "Printing stack for current_state (", pD->lua->current_state,"):");
      printLuaStack(pD->lua->current_state, 0, pDuel);
    }
  }
}


void printDuelCoroutines(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print lua coroutines; current duel state id (",
         dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print lua coroutines; current duel state id (",
         dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;

  clog("i", "coroutine size in interpreter: ", pD->lua->coroutines.size());
  auto lgv = luaGlobalsToMap(pD->lua->lua_state);
  auto lre = luaRegistryToMap(pD->lua->lua_state);
  for (auto cr: pD->lua->coroutines) {
    std::stringstream buffer;
    buffer << " ptr: (" << cr.second.first << ")";

    if (lre.find(cr.first) != lre.end()) {
      auto re = lre.at(cr.first);
      if (lgv.find(re.ptr) != lgv.end()) {
        auto er = lgv.equal_range(re.ptr);
        std::vector<std::string> v;
        for (auto i = er.first; i != er.second; ++i)
          v.push_back(i->second.name);
        buffer << " (coroutine func: " << joinString(v, ", ") << ")";
      }
    }
    clog("i", "   {func ref_handle: ", cr.first, ", ", buffer.str(), "}");
    printLuaStack(cr.second.first, 2, pDuel);
  }
}


void printLuaGlobals(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print lua globals; current duel state id (",
         dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print lua globals; current duel state id (",
         dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;

  lua_State* L = pD->lua->lua_state;

  if (!confirmYN("Print via c api method?", true)) {
    executeSimpleLuaScript(L, "print_globals.lua");
  } else {
    int maxDepth = 3;
    auto lgm = luaGlobalsToMap(pD->lua->lua_state, maxDepth, 0);
    std::map<std::string, LuaGlobalEntry> lgsm;
    for (const auto &lg: lgm) {
      lgsm[lg.second.name] = lg.second;
    }

    int entryNum = 0;
    for (const auto &lg: lgsm) {
      int depth = splitString(lg.second.name, ".").size() - 0 /*1*/;

      clog("l", "[", entryNum++, "]", std::string((depth * 4), ' '),
           lg.second.name, " \ttype: ", lg.second.type, " \t",
           lg.second.value);
    }
  }
}


void printLuaRegistryEntries(unsigned long long dsId) {
  // mini spoilers:
  // index[0] => number (0; possibly an invalid index)
  // index[1] => thread (ptr to pduel->lua->lua_state)
  // index[2] => table (_G global table)

  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print lua registry; current duel state id (",
         dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print lua registry; current duel state id (",
         dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;
  auto lrm = luaRegistryToMap(pD->lua->lua_state);
  std::map<int32, LuaRegistryEntry> lrsm =
    std::map<int32, LuaRegistryEntry>(lrm.begin(), lrm.end());

  for (const auto &lre: lrsm) {
    std::string strFlag = (lre.second.type == "nil") ? "w" : "l";

    clog(strFlag, "ref_handle: ", lre.first, "  type: ",
         lre.second.type, "  value: ", lre.second.value);
  }
}


void popEverythingTest(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;

  if (confirmYN("Really pop everything off lua stack??")) {
    lua_pop(pD->lua->lua_state, lua_gettop(pD->lua->lua_state));
    lua_pop(pD->lua->current_state, lua_gettop(pD->lua->current_state));
  }
  clog("l", "Popped everything!");
}

void printSizes(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;

  clog("i", "Number of cards: ", pD->cards.size());
  clog("i", "Number of assumes: ", pD->assumes.size());
  clog("i", "Number of groups: ", pD->groups.size());
  clog("i", "Number of sgroups: ", pD->sgroups.size());
  clog("i", "Number of effects: ", pD->effects.size());
  clog("i", "Number of uncopy: ", pD->uncopy.size());

  clog("i", "Number of processor summon_counter: ",
       pD->game_field->core.summon_counter.size());
  clog("i", "Number of processor normalsummon_counter: ",
       pD->game_field->core.normalsummon_counter.size());
  clog("i", "Number of processor spsummon_counter: ",
       pD->game_field->core.spsummon_counter.size());
  clog("i", "Number of processor flipsummon_counter: ",
       pD->game_field->core.flipsummon_counter.size());
  clog("i", "Number of processor attack_counter: ",
       pD->game_field->core.attack_counter.size());
  clog("i", "Number of processor chain_counter: ",
       pD->game_field->core.chain_counter.size());

  clog("i", "sizeof(card): ", sizeof(card), "  sizeof(card*): ", sizeof(card*));
  clog("i", "sizeof(effect): ", sizeof(effect), "  sizeof(effect*): ", sizeof(effect*));
  clog("i", "sizeof(group): ", sizeof(group), "  sizeof(group*): ", sizeof(group*));
}


void printLuaGlobalsAssociations(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print lua global associations; current duel"
         " state id (", dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print lua global associations; current duel"
         " state id (", dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;

  auto dvm = duelVarsToMap(pDuel);
  auto lgm = luaGlobalsToMap(pD->lua->lua_state);
  auto lgsm = std::multimap<std::string, LuaGlobalEntry>();
  for (const auto &lg: lgm) {
    lgsm.insert(std::make_pair(lg.second.name, lg.second));
  }
  auto lrm = luaRegistryToMap(pD->lua->lua_state);

  std::multimap<std::string, LuaRegistryEntry> foundReResults;
  std::multimap<std::string, std::pair<DuelVarEntry, std::string>> foundDvResults;

  //let's see if there are any lua global (functions) that cannot be attributed
  // to any duel vars or lua registry entries
  for (const auto &lg: lgsm) {
    //ignore these constants
    if (lg.second.type == "number") continue;
    if (lg.second.type == "boolean") continue;
    if (lg.second.type == "string") continue;

    if (lg.second.ptr == nullptr) {
      clog("w", "!! ", lg.second.ptr, " !! * * *");
      //break;
    }

    int depth = splitString(lg.second.name, ".").size() - 1;
    bool found = false; //found an association to either a duel var or lua registry entry

    //check lua registry entries
    for (const auto &lre: lrm) {
      if (lre.second.ptr == lg.second.ptr) {
          found = true;
          foundReResults.insert(std::make_pair(lg.second.name, lre.second));
      }
    }

    //check duel vars
    //iterate through map because want to do fancier check
    //  than just dvm.equal_range(lg.second.ptr)
    for (const auto &dv: dvm) {

      //equivalent to dvm.equal_range(lg.second.ptr) but should never be true anyway
      if (lg.second.ptr == dv.second.ptr) {
        found = true;
        std::stringstream buffer;
        buffer << "duel var ptr (" << dv.second.ptr << ")";
        foundDvResults.insert
          (std::make_pair(lg.second.name,
                          std::make_pair(dv.second, buffer.str())));
      }

      if (dv.second.type == "card") {
        card* c = (card*) dv.second.ptr;
        std::stringstream cBuffer;
        cBuffer << c;
        std::string cStr = cBuffer.str();

        std::vector<std::pair<card::effect_container*, std::string>>
          effectContainerBindings = {
          std::make_pair(&(c->single_effect), "single_effect"),
          std::make_pair(&(c->field_effect), "field_effect"),
          std::make_pair(&(c->equip_effect), "equip_effect"),
          std::make_pair(&(c->target_effect), "target_effect"),
          std::make_pair(&(c->xmaterial_effect), "xmaterial_effect")
        };

        for (const auto &ecb: effectContainerBindings) {
          for (const auto &ec: *(ecb.first)) {
            std::vector<std::pair<uint32, std::string>> effectBindings = {
              std::make_pair(ec.second->condition, "condition"),
              std::make_pair(ec.second->cost, "cost"),
              std::make_pair(ec.second->target, "target"),
              std::make_pair(ec.second->value, "value"),
              std::make_pair(ec.second->operation, "operation")
            };
            std::stringstream eBuffer;
            eBuffer << ec.second;
            std::string eStr = eBuffer.str();

            for (const auto &eb: effectBindings) {
              if (eb.first == 0)
                continue;

              if (eb.second == "value")
                if (!(ec.second->flag[0] & EFFECT_FLAG_FUNC_VALUE))
                  continue;

              auto elre = getRegistryEntryByRef(pD->lua->lua_state,
                                                eb.first, true);
              if ((elre.type != "nil") && (elre.ptr != nullptr)) {
                if (lg.second.ptr == elre.ptr) {
                  found = true;
                  foundDvResults
                    .insert
                    (std::make_pair(lg.second.name, std::make_pair
                                    (dv.second, "c(" + cStr + ")->"
                                     + ecb.second + " => e(" + eStr
                                     + ")->" + eb.second + "  ("
                                     + std::to_string(elre.ref_handle)
                                     + ")")));
                }
              }
            }
          }
        }
      } else if (dv.second.type == "group") {
      } else if (dv.second.type == "effect") {
        effect* e = (effect*) dv.second.ptr;

        std::vector<std::pair<uint32, std::string>> effectBindings = {
          std::make_pair(e->condition, "condition"),
          std::make_pair(e->cost, "cost"),
          std::make_pair(e->target, "target"),
          std::make_pair(e->value, "value"),
          std::make_pair(e->operation, "operation")
        };
        std::stringstream eBuffer;
        eBuffer << e;
        std::string eStr = eBuffer.str();

        for (const auto &eb: effectBindings) {
          if (eb.first == 0)
            continue;

          if (eb.second == "value")
            if (!(e->flag[0] & EFFECT_FLAG_FUNC_VALUE))
              continue;

          auto elre = getRegistryEntryByRef(pD->lua->lua_state,
                                            eb.first, true);
          if ((elre.type != "nil") && (elre.ptr != nullptr)) {
            if (lg.second.ptr == elre.ptr) {
              found = true;
              foundDvResults
                .insert
                (std::make_pair(lg.second.name, std::make_pair
                                (dv.second, "e(" + eStr + ")->"
                                 + eb.second + "  ("
                                 + std::to_string(elre.ref_handle)
                                 + ")")));
            }
          }
        }
      } else {
        clog("w", "Not sure what to do with duel var with type '",
             dv.second.type, "'; ignoring.");
      }
    }
  }

  std::set<std::string> keySet;
  for (const auto &lg: lgm) {
    keySet.insert(lg.second.name);
  }
  for (const auto &lg: keySet) {
    auto lrer = foundReResults.equal_range(lg);
    auto dver = foundDvResults.equal_range(lg);

    std::vector<std::string> bothResults;
    for (auto i = lrer.first; i != lrer.second; ++i) {
      std::stringstream buffer;
      buffer << i->second.ref_handle;
      bothResults.push_back(buffer.str());
    }
    for (auto i = dver.first; i != dver.second; ++i) {
      std::stringstream buffer;
      buffer << i->second.second;
      bothResults.push_back(buffer.str());
    }

    auto custIndentDepth
      = std::string((splitString(lg, ".").size() - 1) * 4, ' ');

    if (bothResults.size() > 0) {
      clog("l", custIndentDepth, lg);
      for (const auto &br: bothResults) {
        clog("l", custIndentDepth, "    [", br, "]");
      }
    } else {
      clog("w", custIndentDepth, lg);
    }
  }
}
void printLuaRegistryAssociations(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print lua registry associations; current"
         " duel state id (", dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print lua registry associations; current"
         " duel state id (", dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;

  auto dvm = duelVarsToMap(pDuel);
  //auto lgm = luaGlobalsToMap(pD->lua->lua_state);
  auto lrm = luaRegistryToMap(pD->lua->lua_state);
  auto lrsm = std::map<uint32, LuaRegistryEntry>(lrm.begin(), lrm.end());

  //let's see if there are any lua registry entries that cannot be
  // attributed to a lua global or duel var
  std::multimap<uint32, std::pair<DuelVarEntry, std::string>> foundResults;
  std::vector<LuaRegistryEntry> notFoundResults;

  for (const auto &lre: lrsm) {
    bool found = false;

    //shouldn't matter but just in case
    if (lre.second.ref_handle == 0)
      continue; //ignore

    //check lua global via ptr
    // actually not necessary as proven when commented out
    // suggests only duel vars are associated with lua registry vars
    //  (though some lua globals (particularly functions) can be
    //   associated with some lua registry vars)
    if (lre.second.ptr != nullptr) {
      //if (lgm.find(lre.second.ptr) != lgm.end())
      //  found = true;
    }

    //check duel var via ref handle
    for (const auto &dv: dvm) {
      if (dv.second.type == "card") {
        card* c = ((card*) dv.second.ptr);
        if ((lre.second.ref_handle == c->ref_handle) &&
            (c->ref_handle != 0)) {
          found = true;
          foundResults.insert
            (std::make_pair(lre.second.ref_handle,
                            std::make_pair(dv.second, "(card*)")));
        }
      } else if (dv.second.type == "group") {
        group* g = ((group*) dv.second.ptr);
        if ((lre.second.ref_handle == g->ref_handle) &&
            (g->ref_handle != 0)) {
          found = true;
          foundResults.insert
            (std::make_pair(lre.second.ref_handle,
                            std::make_pair(dv.second, "(group*)")));
        }
      } else if (dv.second.type == "effect") {
        effect* e = ((effect*) dv.second.ptr);
        if ((lre.second.ref_handle == (e->ref_handle)) &&
            (e->ref_handle != 0)) {
          found = true;
          foundResults.insert
            (std::make_pair(lre.second.ref_handle,
                            std::make_pair(dv.second, "(effect*)")));
        }
        if ((lre.second.ref_handle == (e->condition)) &&
            (e->condition != 0)) {
          found = true;
          foundResults.insert
            (std::make_pair(lre.second.ref_handle,
                            std::make_pair(dv.second, "e->condition")));
        }
        if ((lre.second.ref_handle == (e->cost)) &&
            (e->cost != 0)) {
          found = true;
          foundResults.insert
            (std::make_pair(lre.second.ref_handle,
                            std::make_pair(dv.second, "e->cost")));
        }
        if ((lre.second.ref_handle == (e->target)) &&
            (e->target != 0)) {
          found = true;
          foundResults.insert
            (std::make_pair(lre.second.ref_handle,
                            std::make_pair(dv.second, "e->target")));
        }
        if ((lre.second.ref_handle == (e->value)) &&
            (e->value != 0) && (e->flag[0] & EFFECT_FLAG_FUNC_VALUE)) {
          found = true;
          foundResults.insert
            (std::make_pair(lre.second.ref_handle,
                            std::make_pair(dv.second, "e->value")));
        }
        if ((lre.second.ref_handle == (e->operation)) &&
            (e->operation != 0)) {
          found = true;
          foundResults.insert
            (std::make_pair(lre.second.ref_handle,
                            std::make_pair(dv.second, "e->operation")));
        }
      } else if (dv.second.type == "coroutine") {

        for (const auto &cr: pD->lua->coroutines) {
          if (dv.second.ptr == cr.second.first) {
            if (lre.second.ref_handle == cr.first) {
              found = true;
              foundResults.insert
                (std::make_pair(lre.second.ref_handle,
                                std::make_pair(dv.second, "coroutines")));
            }
          }
        }
      } else {
        clog("w", "Found a duel var with type '", dv.second.type,
             "' but not sure how to handle; ignoring.");
      }
    }
    if (!found) {
      notFoundResults.push_back(lre.second);
    }
  }

  for (const auto &lre: lrsm) {
    auto er = foundResults.equal_range(lre.second.ref_handle);
    if (er.first != er.second) {
      std::vector<std::string> resultsStr;
      for (auto i = er.first; i != er.second; ++i) {
        std::stringstream buffer;
        buffer << i->second.first.ptr << " " << i->second.second;
        resultsStr.push_back(buffer.str());
      }
      clog("l", lre.second.ref_handle, "  type: ", lre.second.type,
           "  [", joinString(resultsStr, "], ["), "]");
    } else {
      clog("w", lre.second.ref_handle, "  type: ", lre.second.type,
           "  does not seem to be associated with any duel vars"
           " (or lua globals).");
      continue;
    }
  }

  std::vector<std::string> notFoundRefHandles;
  for (const auto &lre: notFoundResults) {
    notFoundRefHandles.push_back(std::to_string(lre.ref_handle));
  }
  clog("d", "Found associations for ", foundResults.size(), " / ",
       lrsm.size(), " lua registry entries.");
  clog("d", "  (could not find associations for (",
       notFoundResults.size(), ") ref handles: [",
       joinString(notFoundRefHandles, ", "), "])");
}
void printDuelVarAssociations(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print duel var associations; current duel"
         " state id (", dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print duel var associations; current duel"
         " state id (", dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;
  const std::string indent = std::string(4, ' ');

  auto dvm = duelVarsToMap(pDuel);
  auto lgm = luaGlobalsToMap(pD->lua->lua_state);
  auto lrm = luaRegistryToMap(pD->lua->lua_state);

  //let's see if there are any duel vars that cannot be attributed to any
  // lua globals or lua registry entries
  std::set<const void*> foundPtrs;
  std::multimap<const void*,
                std::pair<LuaRegistryEntry, std::string>> foundReResults;
  std::multimap<const void*,
                std::pair<LuaGlobalEntry, std::string>> foundLgResults;
  std::vector<DuelVarEntry> notFoundResults;

  for (const auto &dv: dvm) {

    bool found = false;

    if (dv.second.type == "card") {

      card* c = (card*) dv.second.ptr;
      auto lre =
        getRegistryEntryByRef(pD->lua->lua_state, c->ref_handle, true);

      std::stringstream cBuffer;
      cBuffer << c;
      std::string cStr = cBuffer.str();

      if ((c->ref_handle != 0) && (lre.type != "nil")) {
        //check lua registry entries
        card* lrec = *(card**) lre.ptr;
        if ((dv.second.ptr == lrec) || (dv.second.ptr == lre.ptr)) {
          foundPtrs.insert(c);
          foundReResults.insert
            (std::make_pair(c, std::make_pair(lre, "c(" + cStr
                                              + ")->ref_handle ("
                                              + std::to_string
                                              (c->ref_handle) + ")")));
          found = true;
        }

        //check lua globals
        std::vector<std::pair<card::effect_container*, std::string>>
          effectContainerBindings = {
          std::make_pair(&(c->single_effect), "single_effect"),
          std::make_pair(&(c->field_effect), "field_effect"),
          std::make_pair(&(c->equip_effect), "equip_effect"),
          std::make_pair(&(c->target_effect), "target_effect"),
          std::make_pair(&(c->xmaterial_effect), "xmaterial_effect")
        };

        for (const auto &ecb: effectContainerBindings) {
          for (const auto &ec: *(ecb.first)) {
            std::vector<std::pair<uint32, std::string>> effectBindings = {
              std::make_pair(ec.second->condition, "condition"),
              std::make_pair(ec.second->cost, "cost"),
              std::make_pair(ec.second->target, "target"),
              std::make_pair(ec.second->value, "value"),
              std::make_pair(ec.second->operation, "operation")
            };
            std::stringstream eBuffer;
            eBuffer << ec.second;
            std::string eStr = eBuffer.str();

            for (const auto &eb: effectBindings) {
              if (eb.first == 0)
                continue;

              if (eb.second == "value")
                if (!(ec.second->flag[0] & EFFECT_FLAG_FUNC_VALUE))
                  continue;

              auto elre = getRegistryEntryByRef(pD->lua->lua_state,
                                                eb.first, true);
              if (elre.ptr != nullptr) {
                auto lger = lgm.equal_range(elre.ptr);
                for (auto i = lger.first; i != lger.second; ++i) {

                  foundPtrs.insert(c);
                  foundPtrs.insert(ec.second);
                  foundLgResults
                    .insert
                    (std::make_pair(c, std::make_pair
                                    (i->second, "c(" + cStr + ")->"
                                     + ecb.second + " => e(" + eStr
                                     + ")->" + eb.second
                                     + "  [" + i->second.name + "]")));
                  found = true;
                }
              }
            }
          }
        }
      }

    } else if (dv.second.type == "group") {
      group* g = (group*) dv.second.ptr;
      auto lre = getRegistryEntryByRef(pD->lua->lua_state,
                                       g->ref_handle, true);

      std::stringstream gBuffer;
      gBuffer << g;
      std::string gStr = gBuffer.str();

      if ((g->ref_handle != 0) && (lre.type != "nil")) {

        //check lua registry entries
        group* lreg = *(group**) lre.ptr;
        if ((dv.second.ptr == lreg) || (dv.second.ptr == lre.ptr)) {
          foundPtrs.insert(g);
          foundReResults
            .insert
            (std::make_pair(g, std::make_pair(lre, "g(" + gStr
                                              + ")->ref_handle ("
                                              + std::to_string
                                              (g->ref_handle) + ")")));
          found = true;
        }
      }

    } else if (dv.second.type == "effect") {
      effect* e = (effect*) dv.second.ptr;
      auto lre = getRegistryEntryByRef(pD->lua->lua_state,
                                       e->ref_handle, true);

      std::stringstream eBuffer;
      eBuffer << e;
      std::string eStr = eBuffer.str();

      if ((e->ref_handle != 0) && (lre.type != "nil")) {
        //check lua registry entries
        effect* lree = *(effect**) lre.ptr;
        if ((dv.second.ptr == lree) || (dv.second.ptr == lre.ptr)) {
          foundPtrs.insert(e);
          foundReResults
            .insert
            (std::make_pair(e, std::make_pair(lre, "e(" + eStr
                                              + ")->ref_handle ("
                                              + std::to_string
                                              (e->ref_handle) + ")")));
          found = true;
        }
      }
    }

    if (!found) {
      if (foundPtrs.find(dv.second.ptr) == foundPtrs.end())
        notFoundResults.push_back(dv.second);
    }
  }


  for (const auto &p: foundPtrs) {
    clog("l", p);
    auto lrer = foundReResults.equal_range(p);
    for (auto i = lrer.first; i != lrer.second; ++i)
      clog("l", indent, i->second.second);

    auto lger = foundLgResults.equal_range(p);
    for (auto i = lger.first; i != lger.second; ++i)
      clog("l", indent, i->second.second);
  }

  if (notFoundResults.size() > 0) {
    clog("w", "Could not find results for the follow (",
         notFoundResults.size(), ") duel vars:");
    for (const auto &dv: notFoundResults) {
      clog("l", indent, "(", dv.ptr, ")  type: ", dv.type,
           "  from: ", dv.origin);
    }
  }


  std::unordered_set<const void*> testSet;
  for (const auto &p: dvm)
    testSet.insert(p.first);

  clog("d", "unique ptrs in duel map: ", testSet.size());
  clog("d", "duel vars found in either lua registry or lua global: ",
       foundPtrs.size());

}

void printDuelVarScAssociations(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print (self contained) duel var"
         " associations; current duel state id (", dsId,
         ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print (self contained) duel var"
         " associations; current duel state id (", dsId,
         ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  duel* pD = (duel*) pDuel;
  const std::string indent = std::string(4, ' ');

  bool onlyShowIsolatedDuelVars =
    confirmYN("Omit duel vars with at least 1 reference and reference"
              " at least 1 other duel var?", true);

  auto dvm = duelVarsToMap(pDuel);
  auto lrm = luaRegistryToMap(pD->lua->lua_state);

  //let's see which duel vars are associated with other duel vars (as direct members)
  std::multimap<const void*,
                std::pair<DuelVarEntry, std::string>> referredToResults;
  std::multimap<const void*,
                std::pair<DuelVarEntry, std::string>> referredByResults;

  for (const auto &dv: dvm) {

    if (dv.second.type == "card") {
      card* c = (card*) dv.second.ptr;

      std::vector<std::pair<card::effect_container*,
                            std::string>> effectContainerBindings = {
        std::make_pair(&(c->single_effect), "single_effect"),
        std::make_pair(&(c->field_effect), "field_effect"),
        std::make_pair(&(c->equip_effect), "equip_effect"),
        std::make_pair(&(c->target_effect), "target_effect"),
        std::make_pair(&(c->xmaterial_effect), "xmaterial_effect")
      };

      for (const auto &ecb: effectContainerBindings) {
        for (const auto &ec: *(ecb.first)) {
          if (dvm.find(ec.second) != dvm.end()) {
            std::stringstream buffer;
            buffer << "." << ecb.second << " => effect* (" << ec.second << ")";
            referredToResults
              .insert
              (std::make_pair(dv.second.ptr,
                              std::make_pair(dv.second, buffer.str())));
            std::stringstream buffer2;
            buffer2 << "card* (" << dv.second.ptr << ")." << ecb.second;
            referredByResults
              .insert
              (std::make_pair(ec.second, std::make_pair(dv.second,
                                                        buffer2.str())));
          }
        }
      }
      //also exists .indexer, .relate_effect and .immune effect if you want to check more

      if (dvm.find(c->unique_effect) != dvm.end()) {
        std::stringstream buffer;
        buffer << ".unique_effect => effect* (" << c->unique_effect << ")";
        referredToResults
          .insert
          (std::make_pair(dv.second.ptr,
                          std::make_pair(dv.second, buffer.str())));
        std::stringstream buffer2;
        buffer2 << "card* (" << dv.second.ptr << ").unique_effect";
        referredByResults
          .insert
          (std::make_pair(c->unique_effect,
                          std::make_pair(dv.second, buffer2.str())));
      }

      if (dvm.find(c->current.reason_effect) != dvm.end()) {
        //in addiiton to current,
      }
    } else if (dv.second.type == "group") {
      group* g = (group*) dv.second.ptr;
      for (const auto &c: g->container) {
        if (dvm.find(c) != dvm.end()) {
          std::stringstream buffer;
          buffer << ".container => card* (" << c << ")";
          referredToResults
            .insert
            (std::make_pair(dv.second.ptr,
                            std::make_pair(dv.second, buffer.str())));
          std::stringstream buffer2;
          buffer2 << "group* (" << dv.second.ptr << ").container";
          referredByResults
            .insert
            (std::make_pair(c,
                            std::make_pair(dv.second, buffer2.str())));
        }
      }
    } else if (dv.second.type == "effect") {
      effect* e = (effect*) dv.second.ptr;

      std::vector<std::pair<card*, std::string>> cardBindings = {
        std::make_pair(e->owner, "owner"),
        std::make_pair(e->handler, "handler"),
        std::make_pair(e->active_handler, "active_handler")
      };

      for (const auto &cb: cardBindings) {
        if ((cb.first == 0) || (cb.first == nullptr))
          continue;

        if (dvm.find(cb.first) != dvm.end()) {
          std::stringstream buffer;
          buffer << "." << cb.second << " => card* (" << cb.first << ")";
          referredToResults
            .insert
            (std::make_pair(dv.second.ptr,
                            std::make_pair(dv.second, buffer.str())));
          std::stringstream buffer2;
          buffer2 << "effect* (" << dv.second.ptr << ")." << cb.second;
          referredByResults
            .insert
            (std::make_pair(cb.first,
                            std::make_pair(dv.second, buffer2.str())));
        }
      }

    } else {
      clog("w", "Not sure what to do with type '", dv.second.type,
           "'; ignoring.");
    }
  }

  std::set<const void*> foundResults;
  std::set<const void*> notFoundResults;
  std::set<const void*> noRefResults;
  std::set<const void*> refOnlyResults;

  for (const auto &rtrk: referredToResults)
    foundResults.insert(rtrk.first);

  for (const auto &rbrk: referredByResults)
    foundResults.insert(rbrk.first);

  for (const auto &dv: dvm)
    if (foundResults.find(dv.second.ptr) == foundResults.end())
      notFoundResults.insert(dv.second.ptr);

  if (foundResults.size() > 0) {
    clog("d", "Found (", foundResults.size(),
         ") duel vars with at least 1 assocation to other duel vars:");

    if (onlyShowIsolatedDuelVars)
      clog("l", indent, "[ omitted ]");

    for (const auto &frk: foundResults) {
      auto rtrer = referredToResults.equal_range(frk);
      auto rbrer = referredByResults.equal_range(frk);

      auto er = dvm.equal_range(frk);
      bool isGroup = false;
      std::vector<std::string> typeStr;
      for (auto i = er.first; i != er.second; ++i) {
        typeStr.push_back(i->second.type);

        if ((i->second.type).find("group") != std::string::npos)
          isGroup = true;
      }

      bool isNoRef = (rtrer.first == rtrer.second);
      bool isRefOnly = (rbrer.first == rbrer.second);

      if (!onlyShowIsolatedDuelVars)
        if (((!isNoRef) && (!isRefOnly)) || (isGroup))
          clog("l", joinString(typeStr, "*/"), "* (", frk, ")");

      for (auto fr = rtrer.first; fr != rtrer.second; ++fr) {
        if ((isRefOnly) && (!isGroup)) {
          noRefResults.insert(frk);
        } else if (!onlyShowIsolatedDuelVars) {
          clog("l", indent, fr->second.second);
        }
      }

      for (auto fr = rbrer.first; fr != rbrer.second; ++fr) {
        if (isNoRef) {
          refOnlyResults.insert(frk);
        } else if (!onlyShowIsolatedDuelVars) {
          clog("l", indent, "ref by:  ", fr->second.second);
        }
      }
    }
  }
  if (noRefResults.size() > 0) {
    clog("w", "Found (", noRefResults.size(),
         ") results that are not referenced by, but do reference by"
         " other duel vars:");

    for (const auto &nrr: noRefResults) {
      auto er = dvm.equal_range(nrr);
      for (auto i = er.first; i != er.second; ++i) {
        clog("l", i->second.type, "*  (", i->second.ptr, ")");

        auto rtr = referredToResults.equal_range(i->second.ptr);
        for (auto j = rtr.first; j != rtr.second; ++j)
          clog("l", indent, j->second.second);
      }
    }
  }
  if (refOnlyResults.size() > 0) {
    clog("w", "Found (", refOnlyResults.size(),
         ") results that are referenced by, but do not reference"
         " other duel vars:");
    for (const auto &ror: refOnlyResults) {
      auto er = dvm.equal_range(ror);
      for (auto i = er.first; i != er.second; ++i) {
        clog("l", i->second.type, "*  (", i->second.ptr, ")");

        auto rbr = referredByResults.equal_range(i->second.ptr);
        for (auto j = rbr.first; j != rbr.second; ++j)
          clog("l", indent, "ref by:  ", j->second.second);
      }
    }
  }

  if (notFoundResults.size() > 0) {
    clog("w", "Found (", notFoundResults.size(),
         ") results that neither reference or are referenced by"
         " other duel vars:");

    for (const auto &nfr: notFoundResults) {
      auto nfer = dvm.equal_range(nfr);

      for (auto nf = nfer.first; nf != nfer.second; ++nf)
        clog("l", "(", nfr, ")  type: ", nf->second.type);
    }
  }

  clog("d", "Note: there are ", dvm.size(), " duel vars, ",
       (foundResults.size() + notFoundResults.size()),
       " of which are unique.");
}


void searchPtrInDuel(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to search for ptrs in duel; current duel state"
         " id (", dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to search for ptrs in duel; current duel state"
         " id (", dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  ptrSearch(pDuel);
}
void searchRhInDuel(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to search for ref handles in duel; current duel"
         " state id (", dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to search for ref handles in duel; current duel"
         " state id (", dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  refSearch(pDuel);
}
void searchDuelVarsByCardName(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to search X; current duel state id (",
         dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to search X; current duel state id (",
         dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  cardNameSearch(pDuel, true);
}

void printCardCLS(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;

  //demonstrate that all card* in a duel are unique not just by pointer
  // but also a combination of controller+location+sequence (duplicates
  //  if any should be highlighted)

  std::map<uint8, std::map<uint8, std::map<uint8, card*>>> clsMap;
  for (const auto &c: pD->cards) {
    auto cVal = clsMap[c->current.controler];
    auto lVal = cVal[c->current.location];
    lVal[c->current.sequence] = c;
    cVal[c->current.location] = lVal;
    clsMap[c->current.controler] = cVal;
  }

  std::set<std::tuple<uint8, uint8, uint8>> clsTupleSet;
  //for (const auto &c: cardSet) {
  for (const auto &cMap: clsMap) {
    for (const auto &lMap: cMap.second) {
      for (const auto &sMap: lMap.second) {
        card* c = sMap.second;
        std::string logFlag = "l";

        auto clsTuple = std::make_tuple(c->current.controler,
                                        c->current.location,
                                        c->current.sequence);

        if (clsTupleSet.find(clsTuple) != clsTupleSet.end())
          logFlag = "w";

        clog(logFlag, "code: ", c->data.code, " \tc: ",
             (int) c->current.controler, "   l: ",
             (int) c->current.location, "   s: ",
             (int) c->current.sequence, "\t[",
             decodeCode(c->data.code), "]");

        clsTupleSet.insert(clsTuple);
      }
    }
  }
}


void changeLogOptions() {
  clog("d", "Current log options:");
  clog("i", "Strings to be filtered (ignored) (",
       getDDD_GS().conf.log.filters.size(), "):");
  for (const auto &f: getDDD_GS().conf.log.filters)
    clog("l", "    ", f);
  clog("i", "Strings to be highlighted (",
       getDDD_GS().conf.log.highlights.size(), "):");
  for (const auto &f: getDDD_GS().conf.log.highlights)
    clog("l", "    ", f);
  clog("i", "Show messages with highlights, even if filtered: ",
       ((getDDD_GS().conf.log.highlightOverridesFilter) ? "yes" : "no"));


  clog("i", "\nAvailable log option choices:");
  clog("l", "[Choice 0]: reload log options from conf file");
  clog("l", "[Choice 1]: add string to filters");
  clog("l", "[Choice 2]: remove filter");
  clog("l", "[Choice 3]: add string to highlights");
  clog("l", "[Choice 4]: remove highlight");
  clog("l", "[Choice 5]: toggle filter-highlight priority");

  std::string choiceStr = "";
  int choice = -1;

  std::cout << "Select a choice: ";
  std::getline(std::cin, choiceStr);
  choiceStr = trimString(choiceStr);

  try {
    choice = std::stoi(choiceStr);
  } catch (std::exception &e) {
    clog("e", e.what());
    clog("w", "Invalid choice '", choiceStr, "'.");
    return;
  }

  if (choice == 0) {
    if (confirmYN("Really reload log options from conf file"
                  " (filters and highlights added will be lost)?")) {
      auto confJson = getJson(getDDD_GS().conf.confPath);
      if (confJson.contains("log")) {
        getDDD_GS().conf.log = initLog(confJson["log"]);
        clog("ok", "Log options successfully reloaded.");
      } else {
        clog("w", "Conf file (", getDDD_GS().conf.confPath,
             ") contains no log section or is invalid.");
      }
    } else {
      clog("i", "Log options were not reloaded from conf file or changed.");
    }
  } else if (choice == 1) {
    clog("i", "Enter a new message filter (or leave blank to cancel)");
    std::string filterStr = "";
    std::cout << "Filter: ";
    std::getline(std::cin, filterStr);

    if (filterStr.size() > 0) {
      getDDD_GS().conf.log.filters.insert(filterStr);
      clog("ok", "Added '", filterStr, "' to filters.");
    } else {
      clog("i", "Cancelled.");
      return;
    }

  } else if (choice == 2) {
    if (getDDD_GS().conf.log.filters.size() > 0) {
      std::vector<std::string> choices;
      clog("i", "Available filters to remove (",
           getDDD_GS().conf.log.filters.size(), "):");

      int i = 0; //can't init two different types in for loop
      for (auto itr = getDDD_GS().conf.log.filters.begin();
           i < getDDD_GS().conf.log.filters.size(); ++i, ++itr) {
        choices.push_back(*itr);
        clog("l", "[Choice ", i, "] Remove '", *itr, "'");
      }

      std::string removeChoiceStr = "";
      int removeChoice = -1;

      std::cout << "Select a choice: ";
      std::getline(std::cin, removeChoiceStr);
      removeChoiceStr = trimString(removeChoiceStr);

      try {
        removeChoice = std::stoi(removeChoiceStr);
      } catch (std::exception &e) {
        clog("e", e.what());
        clog("w", "Invalid choice '", removeChoiceStr, "'.");
        return;
      }

      if ((removeChoice < 0) && (removeChoice >= choices.size())) {
        clog("w", "Invalid choice '", removeChoiceStr, "'.");
        return;
      } else {
        getDDD_GS().conf.log.filters.erase(choices[removeChoice]);
        clog("o", "Removed '", choices[removeChoice], "' from filters.");
      }

    } else {
      clog("i", "Currently not filtering any log messages"
           " (no filters available to remove).");
    }
  } else if (choice == 3) {
    clog("i", "Enter a new highlight for messages"
         " (or leave blank to cancel)");
    std::string highlightStr = "";
    std::cout << "Highlight: ";
    std::getline(std::cin, highlightStr);

    if (highlightStr.size() > 0) {
      getDDD_GS().conf.log.highlights.insert(highlightStr);
      clog("ok", "Added '", highlightStr, "' to highlights.");
    } else {
      clog("i", "Cancelled.");
      return;
    }

  } else if (choice == 4) {
    if (getDDD_GS().conf.log.highlights.size() > 0) {
      std::vector<std::string> choices;
      clog("i", "Available highlights to remove (",
           getDDD_GS().conf.log.highlights.size(), "):");

      int i = 0;
      for (auto itr = getDDD_GS().conf.log.highlights.begin();
           i < getDDD_GS().conf.log.highlights.size(); ++i, ++itr) {
        choices.push_back(*itr);
        clog("l", "[Choice ", i, "] Remove '", *itr, "'");
      }

      std::string removeChoiceStr = "";
      int removeChoice = -1;

      std::cout << "Select a choice: ";
      std::getline(std::cin, removeChoiceStr);
      removeChoiceStr = trimString(removeChoiceStr);

      try {
        removeChoice = std::stoi(removeChoiceStr);
      } catch (std::exception &e) {
        clog("e", e.what());
        clog("w", "Invalid choice '", removeChoiceStr, "'.");
        return;
      }

      if ((removeChoice < 0) && (removeChoice >= choices.size())) {
        clog("w", "Invalid choice '", removeChoiceStr, "'.");
        return;
      } else {
        getDDD_GS().conf.log.highlights.erase(choices[removeChoice]);
        clog("o", "Removed '", choices[removeChoice],
             "' from highlights.");
      }

    } else {
      clog("i", "Currently not highlighting any log message"
           " s( no highlights available to remove).");
    }
  } else if (choice == 5) {
    if (confirmYN("Toggle filter-highlight priority?")) {
      getDDD_GS().conf.log.highlightOverridesFilter =
        !getDDD_GS().conf.log.highlightOverridesFilter;
      clog("ok", "Successfully toggled (now: ",
           ((getDDD_GS().conf.log.highlightOverridesFilter)
            ? "true" : "false"), ")");
    } else {
      clog("i", "Log options not changed.");
    }
  } else {
    clog("w", "Invalid choice '", choiceStr, "'.");
  }
}

void callLuaScript(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;

  std::string choiceStr = "";
  lua_State* L = nullptr;
  int choice = -1;

  clog("d", "lua_state = ", pD->lua->lua_state, "  current_state = ",
       pD->lua->current_state, "  (lua_state == current_state) = ",
       (pD->lua->lua_state == pD->lua->current_state));
  if (pD->lua->lua_state == pD->lua->current_state) {
    clog("l", "Note: current_state is currently equal to lua_state.");
    choice = 0; //states are same so no need to ask
  } else {
    clog("i", "Load lua script into lua_state or current_state?");
    clog("l", "[Choice 0]: lua_state");
    clog("l", "[Choice 1]: current_state");

    std::cout << "Select a choice: ";
    std::getline(std::cin, choiceStr);
    choiceStr = trimString(choiceStr);

    try {
      choice = std::stoi(choiceStr);
    } catch (std::exception &e) {
      clog("e", e.what());
      clog("w", "Invalid choice '", choiceStr, "'.");
      return;
    }
  }

  if (choice == 0) {
    clog("ok", "Selected lua_state.");
    L = pD->lua->lua_state;
  } else if (choice == 1) {
    clog("ok", "Selected current_state.");
    L = pD->lua->current_state;
  } else {
    clog("w", "Invalid choice '", choiceStr, "'.");
    return;
  }

  std::string scriptNameStr = "";
  clog("i", "Enter the name of a lua script to call"
       " (or leave blank for default script):");
  std::cout << "Script name: ";
  std::getline(std::cin, scriptNameStr);

  if (trimString(scriptNameStr).empty())
    scriptNameStr = "default.lua";

  executeSimpleLuaScript(L, scriptNameStr);
}


void indexerTest(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;
  clog("d", "Various card indexer tests/demonstrations...");

  {
    int total = 0;
    int nonEmptyEc = 0;
    int nonEmptyIdxr = 0;

    for (const auto &c: pD->cards) {
      ++total;
      if ((c->single_effect.size() > 0) ||
          (c->field_effect.size() > 0) ||
          (c->equip_effect.size() > 0) ||
          (c->target_effect.size() > 0) ||
          (c->xmaterial_effect.size() > 0))
        ++nonEmptyEc;

      if (c->indexer.size() > 0)
        ++nonEmptyIdxr;
    }

    clog("i", nonEmptyEc, " / ", total,
         " cards have at least 1 nonempty effect container.");
    clog("i", nonEmptyIdxr, " / ", total,
         " cards have at least 1 nonempty indexer.");
  }

  {
    const std::string tablehf =
      "[code]\t\t  [s|f|e|t|x]_effect sz\t  indexer sz";
    clog("i", tablehf);
    for (const auto &c: pD->cards) {
      clog("l", "c(", c, ")\t  [", c->single_effect.size(), " ",
           c->field_effect.size(), " ", c->equip_effect.size(), " ",
           c->target_effect.size(), " ", c->xmaterial_effect.size(),
           "]\t\t  ", c->indexer.size(),
           "\t  [", decodeCode(c->data.code), "]");
    }
    clog("i", tablehf);
    clog("");
  }

  {
    for (const auto &c: pD->cards) {
      clog("i", "c(", c, ") [", decodeCode(c->data.code), "]:");
      if (c->single_effect.size() > 0) {
        for (const auto &mme: c->single_effect)
          clog("l", "\t", "s[", mme.first, "] => e(", mme.second, ")");
      }
      if (c->field_effect.size() > 0) {
        for (const auto &mme: c->field_effect)
          clog("l", "\t", "f[", mme.first, "] => e(", mme.second, ")");
      }
      if (c->equip_effect.size() > 0) {
        for (const auto &mme: c->equip_effect)
          clog("l", "\t", "e[", mme.first, "] => e(", mme.second, ")");
      }
      if (c->target_effect.size() > 0) {
        for (const auto &mme: c->target_effect)
          clog("l", "\t", "t[", mme.first, "] => e(", mme.second, ")");
      }
      if (c->xmaterial_effect.size() > 0) {
        for (const auto &mme: c->xmaterial_effect)
          clog("l", "\t", "x[", mme.first, "] => e(", mme.second, ")");
      }

      if (c->indexer.size() > 0) {
        for (const auto &idxr: c->indexer)
          clog("l", "\t", "indexer[e(", idxr.first, ")] => p{ ",
               idxr.second->first, ", e(", idxr.second->second, ") }");
      }


      for (const auto &idxr: c->indexer) {
        int found = 0;
        for (auto i = c->single_effect.begin();
             i != c->single_effect.end(); ++i) {
          if (std::addressof(*(idxr.second)) == std::addressof(*i)) {
            clog("l", "\t", "indexer[e(", idxr.first, ")] itr => s[",
                 i->first, "]");
            ++found;
          }
        }
        for (auto i = c->field_effect.begin();
             i != c->field_effect.end(); ++i) {
          if (std::addressof(*(idxr.second)) == std::addressof(*i)) {
            clog("l", "\t", "indexer[e(", idxr.first, ")] itr => f[",
                 i->first, "]");
            ++found;
          }
        }
        for (auto i = c->equip_effect.begin();
             i != c->equip_effect.end(); ++i) {
          if (std::addressof(*(idxr.second)) == std::addressof(*i)) {
            clog("l", "\t", "indexer[e(", idxr.first, ")] itr => e[",
                 i->first, "]");
            ++found;
          }
        }
        for (auto i = c->target_effect.begin();
             i != c->target_effect.end(); ++i) {
          if (std::addressof(*(idxr.second)) == std::addressof(*i)) {
            clog("l", "\t", "indexer[e(", idxr.first, ")] itr => t[",
                 i->first, "]");
            ++found;
          }
        }
        for (auto i = c->xmaterial_effect.begin();
             i != c->xmaterial_effect.end(); ++i) {
          if (std::addressof(*(idxr.second)) == std::addressof(*i)) {
            clog("l", "\t", "indexer[e(", idxr.first, ")] itr => x[",
                 i->first, "]");
            ++found;
          }
        }
        if (found == 0)
          clog("w", "\t", "indexer[e(", idxr.first,
               ")] itr => <not found>");
        else if (found > 1)
          clog("w", "\t", "indexer[e(", idxr.first,
               ")] itr => <multiple (", found, ")>");
      }
    }
  }
}


void luaGlobalComparisonTest(intptr_t pDuelOriginal, intptr_t pDuel) {
  if (pDuelOriginal == pDuel) {
    clog("d", "Current duel is same; swap to different one to compare.");
    return;
  }

  duel* pDOriginal = (duel*) pDuelOriginal;
  duel* pDNew = (duel*) pDuel;

  clog("i", "Comparing lua globals for duel(", pDOriginal,
       ") and duel(", pDNew, "):");

  int depth = 3;
  auto lgOriginal = luaGlobalsToMap(pDOriginal->lua->lua_state, depth, 0);
  auto lgNew = luaGlobalsToMap(pDNew->lua->lua_state, depth, 0);

  std::vector<LuaGlobalEntry> missingInOld;
  std::vector<LuaGlobalEntry> missingInNew;

  std::map<std::string, LuaGlobalEntry> lgOriginalSMap;
  std::map<std::string, LuaGlobalEntry> lgNewSMap;

  for (const auto &lg: lgOriginal)
    if (lgOriginalSMap.find(lg.second.name) == lgOriginalSMap.end())
      lgOriginalSMap[lg.second.name] = lg.second;
    else
      clog("e", "Duplicate key name [", lg.second.name, "]");

  for (const auto &lg: lgNew)
    if (lgNewSMap.find(lg.second.name) == lgNewSMap.end())
      lgNewSMap[lg.second.name] = lg.second;
    else
      clog("e", "Duplicate key name [", lg.second.name, "]");

  for (const auto &lg: lgOriginalSMap) {
    if (lgNewSMap.find(lg.first) == lgNewSMap.end()) {
      missingInNew.push_back(lg.second);
    }
  }
  for (const auto &lg: lgNewSMap) {
    if (lgOriginalSMap.find(lg.first) == lgOriginalSMap.end()) {
      missingInOld.push_back(lg.second);
    }
  }

  if (missingInOld.size() > 0) {
    clog("d", "Lua globals present in new duel but not old duel (",
         missingInOld.size(), "):");
    for (const auto &mio: missingInOld)
      clog("l", "\t", mio.name, "  \ttype: ", mio.type);
  }
  if (missingInNew.size() > 0) {
    clog("d", "Lua globals present in old duel but not new duel (",
         missingInNew.size(), "):");
    for (const auto &min: missingInNew)
      clog("l", "\t", min.name, "  \ttype: ", min.type);
  }
  if ((missingInOld.size() == 0) && (missingInNew.size() == 0)) {
    clog("o", "All (", lgOriginalSMap.size(),
         ") globals accounted for (at a depth of ", depth, ")!");
  }
}


void duelVarComparisonTest(intptr_t pDuelOriginal, intptr_t pDuel) {

  if (pDuelOriginal == pDuel) {
    clog("d", "Current duel is original; swap to different one to compare.");
    return;
  }

  duel* pDOriginal = (duel*) pDuelOriginal;
  duel* pDNew = (duel*) pDuel;
  bool status = true;

  clog("i", "Comparing duel var count for original duel(", pDOriginal,
       ") and current duel(", pDNew, "):");


  int ocSize = pDOriginal->cards.size();
  int ncSize = pDNew->cards.size();
  std::string cardsFlag = (ocSize != ncSize) ? "w" : "l";
  clog(cardsFlag, "pDuel->cards: (", ocSize, " vs ", ncSize, ")");

  int oeSize = pDOriginal->effects.size();
  int neSize = pDNew->effects.size();

  std::string effectsFlag = (oeSize != neSize) ? "w" : "l";
  clog(effectsFlag, "pDuel->effects: (", oeSize, " vs ", neSize, ")");

}


void printEffectCctvo(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;

  std::map<
    std::tuple<int32,int32,int32,int32,int32,card*>, effect*> cctvoMap;

  for (const auto &e: pD->effects) {
    auto tuple = std::make_tuple(e->condition, e->cost, e->target,
                                 e->value, e->operation, e->owner);
    if (cctvoMap.find(tuple) == cctvoMap.end()) {
      cctvoMap.insert(std::make_pair(tuple, e));
    } else {
      clog("w", "Found duplicate c-c-t-v-o value for e(",
           cctvoMap.at(tuple), ") and e(", e, ")");
      clog("l", "\te->{condition, cost, target, value, operation,"
           " owner}: ", e->condition, ", ", e->cost, ", ", e->target,
           ", ", e->value, ", ", e->operation, ", c(", e->owner, ")");
    }
  }
  clog("i", cctvoMap.size(), "/", pD->effects.size(),
       " of all effects have unique c-c-t-v-o values.");
}



void printDuelProcessorUnits(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("w", "Unable to print duel processor units; current duel"
         " state id (", dsId, ") not valid or does not exist.");
    return;
  }
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    clog("w", "Unable to print duel processor units; current duel"
         " state id (", dsId, ") currently not activated.");
    return;
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;

  auto printDuelProcessorUnitsInner = [](intptr_t pDuel) {
    duel* pD = (duel*) pDuel;
    int i = 0;

    auto printPu = [](const processor_unit& pu, int i) {
      if (i >= 0)
        clog("i", "    ", "processor_unit at (", i, "):");

      std::string typeDecoded = decodeProcessorType(pu.type);
      if (!typeDecoded.empty())
        typeDecoded = " (" + typeDecoded + ") ";

      clog("l", "      ", "type: ", pu.type, typeDecoded, "  step: ",
           pu.step);
      clog("l", "      ", "peffect: (", pu.peffect, ")  ptarget: (",
           pu.ptarget, ")");
      clog("l", "      ", "arg1: ", pu.arg1, "  arg2: ", pu.arg2,
           "  arg3: ", pu.arg3, "  arg4: ", pu.arg4);
      clog("l", "      ", "ptr1: (", pu.ptr1, ")  ptr2: (", pu.ptr2, ")");
    };

    clog("i", "  pD->game_field->core.units (",
         pD->game_field->core.units.size(), "):");
    i = 0;
    if (pD->game_field->core.units.size() > 0)
      for (const auto &pu: pD->game_field->core.units)
        printPu(pu, i++);
    else
      clog("l", "    (empty)");

    clog("i", "  pD->game_field->core.subunits (",
         pD->game_field->core.subunits.size(), "):");
    i = 0;
    if (pD->game_field->core.subunits.size() > 0)
      for (const auto &pu: pD->game_field->core.subunits)
        printPu(pu, i++);
    else
      clog("l", "    (empty)");

    clog("i", "  pD->game_field->core.damage_step_reserved:");
    if (pD->game_field->core.damage_step_reserved.type != 0)
      printPu(pD->game_field->core.damage_step_reserved, -1);
    else
      clog("l", "    (empty)");

    clog("i", "  pD->game_field->core.summon_reserved:");
    if (pD->game_field->core.summon_reserved.type != 0)
      printPu(pD->game_field->core.summon_reserved, -1);
    else
      clog("l", "    (empty)");

    clog("i", "  pD->game_field->core.recover_damage_reserve (",
         pD->game_field->core.recover_damage_reserve.size(), "):");
    i = 0;
    if (pD->game_field->core.recover_damage_reserve.size() > 0)
      for (const auto &pu: pD->game_field->core.recover_damage_reserve)
        printPu(pu, i++);
    else
      clog("l", "    (empty)");
  };

  clog("i", "Processor units for ", pDuel, " (", (duel*) pDuel, "):");
  printDuelProcessorUnitsInner(pDuel);

  if (!ddd::isDuelDuplicatable(pDuel))
    clog("d", "  (note: current duel state not duplicatable;"
         " shadow state processor_units (if any) may not be updated)");
}


//determine which brute forcing method to use based on
//  the 'type' argument passed
void bruteForceStateChoices(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const int depthLimit, const unsigned long long maxDepthResultsLimit, const unsigned long long showResultsLimit, std::unordered_map<unsigned long long, ChoiceTuple>& msgChoicesMap, const std::unordered_set<int>& inFilter, const std::string& type, const int threadCount, const bool showMessages, const bool ignoreWarnings) {

  if (type == "dfs")
    bruteForceStateChoicesDfs(dsId, lastValidChoicesTpl, depthLimit,
                              maxDepthResultsLimit, showResultsLimit,
                              msgChoicesMap, inFilter,
                              showMessages, ignoreWarnings);
  /*
  else if (type == "dfs-mt")
    bruteForceStateChoicesDfsMt(dsId, lastValidChoicesTpl, depthLimit,
                                maxDepthResultsLimit, showResultsLimit,
                                msgChoicesMap, threadCount, inFilter,
                                showMessages, ignoreWarnings);
  else if (type == "dfs-par")
    bruteForceStateChoicesDfsPar(dsId, lastValidChoicesTpl, depthLimit,
                                 msgChoicesMap, inFilter,
                                 showMessages, ignoreWarnings);

  else if (type == "dfs-par2")
    bruteForceStateChoicesDfsPar2(dsId, lastValidChoicesTpl, depthLimit,
                                  maxDepthResultsLimit, showResultsLimit,
                                  msgChoicesMap, inFilter, threadCount,
                                  showMessages, ignoreWarnings);

  else if (type == "dfs-omp")
    bruteForceStateChoicesDfsOmp(dsId, lastValidChoicesTpl, depthLimit,
                                 maxDepthResultsLimit, showResultsLimit,
                                 msgChoicesMap, inFilter,
                                 showMessages, ignoreWarnings);
  */
  else if (type == "bfs")
    bruteForceStateChoicesBfs(dsId, lastValidChoicesTpl, depthLimit,
                              maxDepthResultsLimit, showResultsLimit,
                              msgChoicesMap, inFilter,
                              showMessages, ignoreWarnings);
  else
    clog("w", "Unknown type '", type, "' passed for brute"
         " forcing function.");
}


//helper function for bruteForceStateChoices() functions
//creates a duplicate of the duel state for every possible combination
// of choices (usually just one choice each) and for each duplicated
// duel state, attempts to process until duel state until new choices
// are then available or a recognized end message is reached
//returns a vector of tuples containing the valid duel states that
// were successfully brute forced as well as the choice used to
// enter that duel state as well as a value to suggest whether that
// state should be further brute forced or not
std::vector<std::tuple<unsigned long long, std::string, int>> bruteForceStateChoicesAtState(const unsigned long long dsId, std::unordered_map<unsigned long long, ChoiceTuple>& foundChoicesMap, const std::unordered_set<int>& choiceFilter, const std::unordered_set<int>& endMsgs, const bool showMessages) {

  //results to return
  std::vector<std::tuple<unsigned long long, std::string, int>> stateResults;

  //get choices tuple and make copy of certain tuple members
  // (which might be modified later)
  const auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
  auto choices = ddd::getChoicesFromDuelState(ds, true, choiceFilter);
  const auto& lastValidMsg = std::get<1>(choices)[0];
  //int totalChoices = std::get<0>(choices).size(); //new copy
  //std::vector<int> fil = std::get<4>(choices); //new copy

  //shouldn't reach in here but extra check just in case
  if (endMsgs.find(lastValidMsg) != endMsgs.end())
    return stateResults;

  //start of brute forcing for each choice (combination)
  //if enabled, print message letting user know
  if (showMessages)
    clog("i", "Brute forcing pDuel ", dsId, ".");

  //get all possible combinations of choices
  auto possibleChoices = getPossibleChoiceCombinations(choices);

  //iterate all possible combinations of choices
  for (const auto &c: possibleChoices) {

    //attempt to duplicate state
    unsigned long long newDsId = duplicate_duel_state(dsId);

    //exit and return results found so far if unable to duplicate state
    if (!newDsId) {
      clog("e", "Error duplicating duel state ", dsId,
           " to brute force.");
      return stateResults;
    }

    //determine if combination of choices can be expected to fail
    // (if so, checkFirstProcess will be set to true and if the
    //  1st process iteration performed on duel state (in the
    //  autoProcessLoop) did not succeed, will not consider that
    //  to be an error but will instead silently remove it)
    bool checkFirstProcess = false;
    if (c.size() > 0)
      //combination of at least 1 choice
      // (set response to duplicated duel state)
      setResponse(newDsId, choices, c);
    else
      //combination of 0 choices
      // (do not set response and set checkFirstProcess to true)
      checkFirstProcess = true;

    //high probability of msg 23 creating invalid combination
    // of choices
    // (so set checkFirstProcess to true)
    if (lastValidMsg == 23)
      checkFirstProcess = true;

    //process duplicated duel state and get result
    auto aplTuple = autoProcessLoop(newDsId, endMsgs, choiceFilter,
                                    showMessages, checkFirstProcess);

    //handle based on result from auto process loop
    if ((!std::get<0>(aplTuple)) && (std::get<1>(aplTuple))) {
      //1st process iteration on duplicate duel state did not
      // succeed but not considered to be an error
      // (as it was expected it might not succeed)
      remove_duel_state(newDsId);

    } else if ((!std::get<0>(aplTuple))) {
      //encountered a process iteration that did not succeed
      // (that should have succeeded)
      const auto& aplTupleMsg = std::get<1>(std::get<2>(aplTuple))[0];
      clog("e", "Advancing duel state ", newDsId, " resulted in an"
           " unexpected invalid msg (", aplTupleMsg, ").");
      remove_duel_state(newDsId);

    } else {
      //duplicated state reached valid state after being processed
      // with choice; handle based on number of choices in combination
      // of choices

      //add choices to choice map
      foundChoicesMap[newDsId] = std::get<2>(aplTuple);

      //get choice string(s)
      if (c.size() == 0) {
        //successfully reached duel state after processing without
        // setting any choices

        if (showMessages)
          clog("d", "  Duplicated to ", newDsId, " without using"
               " choices '(continue processing)'.");

        //build tuple for duel state and push to result
        std::tuple<unsigned long long, std::string, int> tpl;
        std::get<0>(tpl) = newDsId;
        std::get<1>(tpl) = "(continue processing)";
        std::get<2>(tpl) = std::get<0>(aplTuple);
        stateResults.push_back(tpl);

      } else if (c.size() == 1) {
        //successfully reached duel state after processing
        // with 1 choice set

        if (showMessages)
          clog("d", "  Duplicated to ", newDsId, " using choice (",
               c[0], ") '", std::get<3>(choices)[c[0]], "'.");

        //build tuple for duel state and push to result
        std::tuple<unsigned long long, std::string, int> tpl;
        std::get<0>(tpl) = newDsId;
        std::get<1>(tpl) = std::get<3>(choices)[c[0]];
        std::get<2>(tpl) = std::get<0>(aplTuple);
        stateResults.push_back(tpl);

      } else {
        //successfully reached duel state after processing
        // with multiple choices set

        //multiple choices to string vector to be joined later
        std::vector<std::string> v;
        for (const auto &i: c)
          v.push_back(std::to_string(i));

        if (showMessages) {
          clog("d", "  Duplicated to ", newDsId, " using choices (",
               joinString(v, ", "), "):");
          for (const auto &i: c)
            clog("d", "     ", std::get<3>(choices)[i]);
        }

        //build tuple for duel state and push to result
        std::tuple<unsigned long long, std::string, int> tpl;
        std::get<0>(tpl) = newDsId;
        std::get<1>(tpl) = joinString(v, ", ");
        std::get<2>(tpl) = std::get<0>(aplTuple);
        stateResults.push_back(tpl);

      }

      deactivate_duel_state(newDsId);
    }
  }

  //if enabled, print the number of duel states that were
  // successfully duplicated versus the possible number of
  // combinations of choices
  if (showMessages)
    clog("d", "\nDuplicated to ", stateResults.size(), " / ",
         possibleChoices.size(), " choices.");

  return stateResults;
}


//given a duel state id, brute forces all possible choices given the
// current state, up to a specified depth and then saves the resulting
// states to map passed to function (by reference)
// (brute forcing style is breadth first where all choices in a given
//  node are explored to only the next depth before moving to the
//  next node)
void bruteForceStateChoicesBfs(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const int depthLimit, const unsigned long long maxDepthResultsLimit, const unsigned long long showResultsLimit, std::unordered_map<unsigned long long, ChoiceTuple>& msgChoicesMap, const std::unordered_set<int>& inFilter, const bool showMessages, const bool ignoreWarnings) {
  if (!ignoreWarnings) {
    if (!confirmYN("Really brute force all states (to depth " +
                   std::to_string(depthLimit) +
                   ") with current choices?", false)) {
      clog("i", "Not doing it.");
      return;
    }
  }
  bool shouldDeactivate = !is_duel_state_active(dsId);
  reactivate_duel_state(dsId);

  //Node struct defintion and some other var declarations needed
  // to either brute force or otherwise keep track of results
  struct Node {
    unsigned long long parent;
    std::vector<unsigned long long> children;
    int depth;
    std::string choice; //from parent to reach this node
  };
  std::unordered_map<unsigned long long, ChoiceTuple> foundChoicesMap;
  std::unordered_map<unsigned long long, Node> choicesTree;
  std::multimap<int, unsigned long long> depthMap;
  std::vector<unsigned long long> parentsToBf;
  const std::unordered_set<int> endMsgs = {
      5  //MSG_WIN
    ,40  //MSG_NEW_PHASE
    ,41  //MSG_NEW_TURN
  };
  int currDepth;
  int maxDepthReached; //not just currDepth - 1
  bool endSearchEarly = false;

  //get actual choice filter based on filter passed from function
  const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
  const auto choiceFilter = ddd::getChoiceFilter(ds, inFilter);

  //some initial set up for vars for first brute force iteration
  parentsToBf.push_back(dsId);
  foundChoicesMap.insert(std::make_pair(dsId, lastValidChoicesTpl));
  Node dsIdNode;
  dsIdNode.parent = 0;
  dsIdNode.depth = 0;
  dsIdNode.choice = "(start)";
  choicesTree[dsId] = dsIdNode;
  maxDepthReached = 0;

  //start of brute forcing functionality
  auto startTime = std::chrono::system_clock::now();

  //iterate until depth limit reached
  for (currDepth = 1; currDepth <= depthLimit; ++currDepth) {
    clog("d", "Current depth: ", currDepth,
         " \tDuel states found so far: ", foundChoicesMap.size() - 1);

    //check if results limit reached
    if (maxDepthResultsLimit) {
      if (maxDepthReached == depthLimit) {
        auto er = depthMap.equal_range(maxDepthReached);
        if (std::distance(er.first, er.second) >= maxDepthResultsLimit) {
          //results limit reached

          endSearchEarly = true;
          break; //exit from loop
        }
      }
    }

    std::vector<unsigned long long> childrenToBf;

    //iterate through all parent duel state ids to brute force
    // (since this a breadth first algorithm, these are essentially
    //  all the duel states at the current deepest depth)
    for (const auto &p: parentsToBf) {

      //activate (parent) duel state, attempt to brute force using all
      // its current choices to a variable containing all resulting
      // children duel states that were valid
      reactivate_duel_state(p);
      auto bfStates
        = bruteForceStateChoicesAtState(p, foundChoicesMap,
                                        choiceFilter, endMsgs,
                                        showMessages);
      deactivate_duel_state(p);

      //iterate through all valid (children) duel states of current
      // parent duel state
      for (const auto &bfs: bfStates) {
        //create node for child duel state and insert it to tree
        Node n;
        n.parent = p;
        n.depth = currDepth;
        n.choice = std::get<1>(bfs);
        choicesTree[std::get<0>(bfs)] = n;

        //add child duel state id to parent's children list
        choicesTree[p].children.push_back(std::get<0>(bfs));
        //add child duel state id to current depth of depth map
        depthMap.insert(std::make_pair(currDepth, std::get<0>(bfs)));

        //check if child duel state is candidate to be further
        // brute forced and if so, add it to list
        //  (may not be a candidate for reasons such as reaching
        //   an end message signifying not to continue once reached)
        if (endMsgs.find(std::get<2>(bfs)) == endMsgs.end())
          childrenToBf.push_back(std::get<0>(bfs));

        //print current chain of choices used to reach this this
        // node (if enabled)
        if (showMessages) {
          std::vector<std::string> v;
          v.push_back(std::get<1>(bfs));
          for (unsigned long long curr = p;
               curr != 0; curr = choicesTree[curr].parent) {
            v.push_back(choicesTree[curr].choice);
          }
          std::reverse(v.begin(), v.end());
          clog("d", std::get<0>(bfs), " ", joinString(v, " => "));
        }
      }

      //print number of duel states found once in a while
      // (roughly after every 100 found)
      if (((foundChoicesMap.size() - 1) / 100) >
          ((foundChoicesMap.size() - bfStates.size() - 1) / 100)) {
        clog("d", "\t\tDuel States found so far: ",
             (foundChoicesMap.size() - 1));
      }
    }

    //check number of children nodes to brute force
    if (!childrenToBf.size()) {
      //no more children nodes to brute force; exit loop early
      break;
    } else {
      //found more children nodes to brute force
      //assign current children to brute force as the next iteration
      // of parents to brute force
      //also update max depth reached
      parentsToBf = childrenToBf;
      maxDepthReached = currDepth;
    }
    //if max depth reached, exit regardless
  }

  auto endTime = std::chrono::system_clock::now();
  auto takenTime = std::chrono::duration_cast
    <std::chrono::seconds>(endTime - startTime);

  //lambda declaration to print choices to reach duel state
  // from original duel state
  auto getChoiceChain = [&](const unsigned long long inDsId) {
    std::vector<std::string> v;
    for (unsigned long long cDsId = inDsId;
         cDsId != 0; cDsId = choicesTree.at(cDsId).parent) {
      v.push_back(choicesTree.at(cDsId).choice);
    }
    std::reverse(v.begin(), v.end());
    return "[dsid " + std::to_string(inDsId) + "]:  \t" +
            joinString(v, " => ");
  };

  //erase only the original duel state id (to adjust tallys)
  foundChoicesMap.erase(dsId);

  //get duel states at deepest depth reached
  auto dmer = depthMap.equal_range(maxDepthReached);
  std::vector<std::string> deepestDuelStatesVec;
  std::vector<std::string> deepestDuelStatesChoicesVec;
  unsigned long deepestDuelStatesCount = 0;

  for (auto i = dmer.first; i != dmer.second; ++i) {
    ++deepestDuelStatesCount;

    //store results if under limit (or no limit)
    if ((!showResultsLimit) || //no limit
        (deepestDuelStatesCount <= showResultsLimit))
      deepestDuelStatesChoicesVec.push_back(getChoiceChain(i->second));
  }

  //print results
  for (const auto& res: deepestDuelStatesChoicesVec)
    clog("d", "    ", res);

  if ((showResultsLimit) &&
      (showResultsLimit < deepestDuelStatesCount)) {
    clog("d", "    \t ......");
    clog("d", "    \t(omitted  ",
         deepestDuelStatesCount - showResultsLimit, " more results ("
         "limited to showing ", showResultsLimit, " results))");
  }

  //print deepest depth reached for user and the duel states
  // that reached that depth
  clog("d", "Found total of ", foundChoicesMap.size(),
       " duel states with ", deepestDuelStatesCount,
       " duel states at deepest depth of ", maxDepthReached, ".");
  clog("d", "    \t(", (foundChoicesMap.size() /
                        ((takenTime.count()) ? takenTime.count() : 1)),
       " duel states per second)");

  if (endSearchEarly) {
    clog("d", "    \t(search ended early because result limit of ",
         maxDepthResultsLimit, " for the deepest depth was reached)");

    //deactivate any possibly active duel states
    for (const auto &dmp: depthMap)
      if (dmp.second != dsId)
        deactivate_duel_state(dmp.second);
  }
  clog("d", "");

  //update contents of choices map with choices found from
  // brute forcing
  for (const auto &fc: foundChoicesMap)
    msgChoicesMap.insert(std::move(fc));

  if (shouldDeactivate)
    deactivate_duel_state(dsId);
}


void bruteForceStateChoicesDfs(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const int depthLimit, const unsigned long long maxDepthResultsLimit, const unsigned long long showResultsLimit, std::unordered_map<unsigned long long, ChoiceTuple>& msgChoicesMap, const std::unordered_set<int>& inFilter, const bool showMessages, const bool ignoreWarnings) {
  if (!ignoreWarnings) {
    if (!confirmYN("Really brute force all states (to depth " +
                   std::to_string(depthLimit) +
                   ") with current choices?", false)) {
      clog("i", "Not doing it.");
      return;
    }
  }
  bool shouldDeactivate = !is_duel_state_active(dsId);
  reactivate_duel_state(dsId);

  //Node struct defintion and some other var declarations needed
  // to either brute force or otherwise keep track of results
  struct Node {
    unsigned long long parent;
    std::vector<unsigned long long> children;
    int depth;
    std::string choice; //from parent to reach this node
  };
  std::unordered_map<unsigned long long, ChoiceTuple> foundChoicesMap;
  std::unordered_map<unsigned long long, Node> choicesTree;
  std::multimap<int, unsigned long long> depthMap;
  std::vector<
    std::tuple<unsigned long long, std::vector<int>, bool>> dfsStack;
  std::unordered_multimap<
    unsigned long long, unsigned long long> dsIdReserve;
  std::list<unsigned long long> dsIdReusable;
  const std::unordered_set<int> endMsgs = {
      5  //MSG_WIN
    ,40  //MSG_NEW_PHASE
    ,41  //MSG_NEW_TURN
  };
  int maxDepthReached;
  bool endSearchEarly = false;

  //get actual choice filter based on filter passed from function
  const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
  const auto choiceFilter = ddd::getChoiceFilter(ds, inFilter);

  //populate stack with initial values
  foundChoicesMap[dsId] =
    ddd::getChoicesFromDuelState(ds, true, choiceFilter);
  const auto possibleChoices =
    getPossibleChoiceCombinations(foundChoicesMap.at(dsId));
  for (auto i = possibleChoices.begin(); i != possibleChoices.end(); ++i)
    dfsStack.push_back
      (std::tuple<unsigned long long, std::vector<int>, bool>
       (dsId, std::move(*i), false));

  //populate initial node (with dsId) and other values
  Node dsIdNode;
  dsIdNode.parent = 0;
  dsIdNode.depth = 0;
  dsIdNode.choice = "(start)";
  choicesTree[dsId] = std::move(dsIdNode);
  maxDepthReached = 0;

  //start of brute forcing functionality
  auto startTime = std::chrono::system_clock::now();

  //iterate until stack cleared
  while (!dfsStack.empty()) {

    //check if results limit reached
    if (maxDepthResultsLimit) {
      if (maxDepthReached == depthLimit) {
        auto er = depthMap.equal_range(maxDepthReached);
        if (std::distance(er.first, er.second) >= maxDepthResultsLimit) {
          //results limit reached

          endSearchEarly = true;
          break; //exit from loop
        }
      }
    }

    //get parent duel state id and its choice combination
    // and then pop stack
    const unsigned long long parentDsId =
      std::move(std::get<0>(dfsStack.back()));
    const auto parentCc = std::move(std::get<1>(dfsStack.back()));
    const bool isLast = std::move(std::get<2>(dfsStack.back()));
    dfsStack.pop_back();

    //check max depth not exceeded
    int currDepth = choicesTree.at(parentDsId).depth + 1;
    if (currDepth > depthLimit) {
      if (isLast) {
        if (ddd::isDuelDuplicatable(getDDD_GS().dddapi
                                    .duelStates.at(parentDsId).pDuel))
          dsIdReusable.push_back(parentDsId);
        else
          deactivate_duel_state(parentDsId);
      }

      continue;
    }

    //get new duel state to duplicate
    unsigned long long newDsId;
    if (isLast) {
      //assume parent duel state that is no longer needed
      newDsId = assume_duel_state(parentDsId);

      //check dsIdReserve if any entries
      if (dsIdReserve.find(parentDsId) != dsIdReserve.end()) {
        //iterate all entries and remove them and then remove key

        //iterate all entries
        const auto er = dsIdReserve.equal_range(parentDsId);
        for (auto i = er.first; i != er.second; ++i) {
          //remove (destroy) duel states
          remove_duel_state(i->second);
        }

        //remove all entries from map using the same key
        dsIdReserve.erase(parentDsId);
      }

    } else {

      //check if duel state available in reserve
      const auto p = dsIdReserve.find(parentDsId);
      if (p == dsIdReserve.end()) {
        //no available duel states in reserve; create new duplicate
        //newDsId = duplicate_duel_state(parentDsId);

        if (!dsIdReusable.empty()) {
          newDsId = duplicate_duel_state_reusing(parentDsId,
                                                 dsIdReusable.back());
          dsIdReusable.pop_back();

        } else {
          newDsId = duplicate_duel_state(parentDsId);
        }

      } else {
        //available duel state in reserve; use that duel state and
        // also remove it from reserve
        newDsId = p->second;
        dsIdReserve.erase(p);
      }
    }

    if (newDsId == 0) {
      clog("e", "Error duplicating duel state ", parentDsId,
           " to brute force (dfs).");
      return;
    }

    //determine if combination of choices can be expected to fail
    // (if so, checkFirstProcess will be set to true and if the
    //  1st process iteration performed on duel state (in the
    //  autoProcessLoop) did not succeed, will not consider that
    //  to be an error but will instead silently remove it)
    bool checkFirstProcess = false;
    if (parentCc.size() > 0)
      //combination of at least 1 choice
      // (set response to duplicated duel state)
      setResponse(newDsId, foundChoicesMap.at(parentDsId), parentCc);
    else
      //combination of 0 choices
      // (do not set response and set checkFirstProcess to true)
      checkFirstProcess = true;

    //check parent message because high probability of message 23
    // creating invalid combination of choices
    // (so set checkFirstProcess to true)
    if (std::get<1>(foundChoicesMap.at(parentDsId))[0] == 23)
      checkFirstProcess = true;

    //process duplicated duel state and get result
    const auto aplTuple = autoProcessLoop(newDsId, endMsgs,
                                          choiceFilter, showMessages,
                                          checkFirstProcess);

    //handle based on result from auto process loop
    if ((!std::get<0>(aplTuple)) && (std::get<1>(aplTuple))) {
      //1st process iteration on duplicate duel state did not
      // succeed but not considered to be an error
      // (as it was expected it might not succeed)

      //save duel state where it may be reused in place of
      // duplicating the same parent (instead of instantly removing)
      dsIdReserve.emplace(parentDsId, newDsId);

      continue;

    } else if ((!std::get<0>(aplTuple))) {
      //encountered a process iteration that did not succeed
      // (that should have succeeded)
      const auto& aplTupleMsg = std::get<1>(std::get<2>(aplTuple))[0];
      clog("e", "Advancing duel state ", newDsId, " resulted in an"
           " unexpected invalid msg (", aplTupleMsg, ").");
      remove_duel_state(newDsId);
      return; //exit prematurely

    }

    //add choices to choice map
    foundChoicesMap[newDsId] = std::get<2>(aplTuple);

    //duplicated state reached valid state after being processed
    // with choice
    Node newDsIdNode;
    newDsIdNode.parent = parentDsId;
    newDsIdNode.depth = currDepth;

    //get choice string for node
    if (parentCc.size() == 0) {
      //successfully reached duel state after processing without
      // setting any choices

      if (showMessages)
        clog("d", "  Duplicated to ", newDsId, " without using"
             " choices '(continue processing)'.");

      //build tuple for duel state and push to result
      newDsIdNode.choice = "(continue processing)";

    } else if (parentCc.size() == 1) {
      //successfully reached duel state after processing
      // with 1 choice set

      const auto& choiceLabels
        = std::get<3>(foundChoicesMap.at(parentDsId));

      if (showMessages)
        clog("d", "  Duplicated to ", newDsId, " using choice (",
             parentCc[0], ") '", choiceLabels[parentCc[0]], "'.");

      newDsIdNode.choice = choiceLabels[parentCc[0]];

    } else {
      //successfully reached duel state after processing
      // with multiple choices set

      const auto& choiceLabels
        = std::get<3>(foundChoicesMap.at(parentDsId));

      //multiple choices to string vector to be joined later
      std::vector<std::string> v;
      for (const auto &i: parentCc)
        v.push_back(std::to_string(i));

      if (showMessages) {
        clog("d", "  Duplicated to ", newDsId, " using choices (",
             joinString(v, ", "), "):");
        for (const auto &i: parentCc)
          clog("d", "     ", choiceLabels[i]);
      }

      newDsIdNode.choice = joinString(v, ", ");
    }

    //update results
    choicesTree.emplace(newDsId, std::move(newDsIdNode));
    choicesTree.at(parentDsId).children.push_back(newDsId);
    depthMap.emplace(currDepth, newDsId);
    maxDepthReached = currDepth;

    //print # of duel states found so far for user
    if ((foundChoicesMap.size() > 1) &&
        (((foundChoicesMap.size() - 1) % 100) == 0)) {
      clog("d", "\tDuel states found so far: ",
           (foundChoicesMap.size() - 1));
    }

    //find children if depth limit not yet reached
    if (currDepth >= depthLimit) {
      if (ddd::isDuelDuplicatable(getDDD_GS().dddapi.
                                  duelStates.at(newDsId).pDuel))
        dsIdReusable.push_back(newDsId);
        //dsIdReserve.insert(std::make_pair(0, newDsId));
      else
        deactivate_duel_state(newDsId);

      continue;
    }

    //get potential children
    const auto choiceCombinations =
      getPossibleChoiceCombinations(std::move(std::get<2>(aplTuple)));
    dfsStack.reserve(dfsStack.size() + choiceCombinations.size());

    //check if any children found
    if (choiceCombinations.size() == 0) {
      if (ddd::isDuelDuplicatable(getDDD_GS().dddapi.duelStates.
                                  at(newDsId).pDuel))
        dsIdReusable.push_back(newDsId);
        //dsIdReserve.insert(std::make_pair(0, newDsId));
      else
        deactivate_duel_state(newDsId);

      continue;
    }

    //push children to stack
    for (auto i = choiceCombinations.begin();
         i != choiceCombinations.end(); ++i)
      dfsStack.push_back
        (std::tuple<unsigned long long, std::vector<int>, bool>
         (newDsId, std::move(*i), (i == choiceCombinations.begin())));
  }

  auto endTime = std::chrono::system_clock::now();
  auto takenTime = std::chrono::duration_cast
    <std::chrono::seconds>(endTime - startTime);

  for (const auto &dsId: dsIdReusable)
    deactivate_duel_state(dsId);
  dsIdReusable.clear();


  //lambda declaration to print choices to reach duel state
  // from original duel state
  auto getChoiceChain = [&](const unsigned long long inDsId) {
    std::vector<std::string> v;
    for (unsigned long long cDsId = inDsId;
         cDsId != 0; cDsId = choicesTree.at(cDsId).parent) {
      v.push_back(choicesTree.at(cDsId).choice);
    }
    std::reverse(v.begin(), v.end());
    return "[dsid " + std::to_string(inDsId) + "]:  \t" +
            joinString(v, " => ");
  };

  //erase only the original duel state id (to adjust tallys)
  foundChoicesMap.erase(dsId);

  //get duel states at deepest depth reached
  auto dmer = depthMap.equal_range(maxDepthReached);
  std::vector<std::string> deepestDuelStatesVec;
  std::vector<std::string> deepestDuelStatesChoicesVec;
  unsigned long deepestDuelStatesCount = 0;

  for (auto i = dmer.first; i != dmer.second; ++i) {
    ++deepestDuelStatesCount;

    //store results if under limit (or no limit)
    if ((!showResultsLimit) || //no limit
        (deepestDuelStatesCount <= showResultsLimit))
      deepestDuelStatesChoicesVec.push_back(getChoiceChain(i->second));
  }

  //print results
  for (const auto& res: deepestDuelStatesChoicesVec)
    clog("d", "    ", res);

  if ((showResultsLimit) &&
      (showResultsLimit < deepestDuelStatesCount)) {
    clog("d", "    \t ......");
    clog("d", "    \t(omitted  ",
         deepestDuelStatesCount - showResultsLimit, " more results ("
         "limited to showing ", showResultsLimit, " results))");
  }

  //print deepest depth reached for user and the duel states
  // that reached that depth
  clog("d", "Found total of ", foundChoicesMap.size(),
       " duel states with ", deepestDuelStatesCount,
       " duel states at deepest depth of ", maxDepthReached, ".");
  clog("d", "    \t(", (foundChoicesMap.size() /
                        ((takenTime.count()) ? takenTime.count() : 1)),
       " duel states per second)");

  if (endSearchEarly) {
    clog("d", "    \t(search ended early because result limit of ",
         maxDepthResultsLimit, " for the deepest depth was reached)");

    //deactivate any possibly active duel states
    for (const auto &dmp: depthMap)
      if (dmp.second != dsId)
        deactivate_duel_state(dmp.second);
  }
  clog("d", "");

  //update contents of choices map with choices found from
  // brute forcing
  for (const auto &fc: foundChoicesMap)
    msgChoicesMap.insert(std::move(fc));

  if (shouldDeactivate)
    deactivate_duel_state(dsId);
}


void bruteForceStateChoicesDfsMt(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const int depthLimit, const unsigned long long maxDepthResultsLimit, const unsigned long long showResultsLimit, std::unordered_map<unsigned long long, ChoiceTuple>& msgChoicesMap, const int threadCount, const std::unordered_set<int>& inFilter, const bool showMessages, const bool ignoreWarnings) {
  if (!ignoreWarnings) {
    if (!confirmYN("Really brute force all states (to depth " +
                   std::to_string(depthLimit) +
                   ") with current choices?", false)) {
      clog("i", "Not doing it.");
      return;
    }

    if (threadCount > std::thread::hardware_concurrency()) {
      clog("w", "Specified threads to use (", threadCount,
           ") is more than the recommended amount for system (",
           std::thread::hardware_concurrency(), ").");
      if (!confirmYN("Really brute force using " +
                     std::to_string(threadCount) + " threads?",
                     false)) {
        clog("i", "Not doing it.");
        return;
      }
    }
  }
  bool shouldDeactivate = !is_duel_state_active(dsId);
  reactivate_duel_state(dsId);

  //Node struct defintion and some other var declarations needed
  // to either brute force or otherwise keep track of results
  struct Node {
    unsigned long long parent;
    std::vector<unsigned long long> children;
    int depth;
    std::string choice; //from parent to reach this node
  };
  struct TaskStruct {
    const unsigned long long parentDsId;
    const std::vector<int> parentCc; //doesn't like reference...?
    const DuelState parentDs;
    const ChoiceTuple parentChoices; //doesn't like reference...?
    const int currDepth;
    const intptr_t pDuelShadow;
    ShadowDuelStateResponses sdsr;

    TaskStruct(const unsigned long long inParentDsId,
                  const std::vector<int>& inParentCc,
                  const DuelState& inParentDs,
                  const ChoiceTuple& inParentChoices,
                  const int inCurrDepth,
                  const intptr_t inPDuelShadow,
                  ShadowDuelStateResponses inSdsr) : //const?
      parentDsId(inParentDsId), parentCc(inParentCc),
      parentDs(inParentDs), parentChoices(inParentChoices),
      currDepth(inCurrDepth), pDuelShadow(inPDuelShadow),
      sdsr(inSdsr) {
    }
    // void operator()(DfsParallelizer* pDfsp) {
    //   pDfsp->threadTask(parentDsId, parentCc, parentDs,
    //                  parentChoices, currDepth, false,
    //                  pDuelShadow, sdsr);
    // }
  };
  //Class to organize variables and functions as well as limit scopes
  class DfsParallelizer {
    std::unordered_map<unsigned long long, ChoiceTuple> foundChoicesMap;
    std::unordered_map<unsigned long long, Node> choicesTree;
    std::multimap<int, unsigned long long> depthMap;
    std::vector<
      std::tuple<unsigned long long, std::vector<int>, int>> dfsStack;
    std::vector<
      std::tuple<unsigned long long, std::vector<int>, int>> pushCache;
    std::unordered_multimap<
      unsigned long long,
      std::tuple<unsigned long long, DuelState>> dsIdReserve;
    std::vector<unsigned long long> deactivateCache;

    std::unordered_map<int, bool> threadStatus; //if true, thread waiting

    int maxDepthReached;
    bool endSearchEarly;
    unsigned long maxDepthResultsLimit;
    const int depthLimit;
    const std::unordered_set<int> endMsgs;
    const std::unordered_set<int> choiceFilter;
    const bool showMessages;

    //std::mutex foundChoicesMapMutex;
    //std::mutex choicesTreeMutex;
    //std::mutex depthMapMutex;
    //std::mutex dfsStackMutex;
    std::mutex pushCacheMutex;
    std::mutex dsIdReserveMutex;
    std::mutex everythingMutex;

    const int maxThreads;
    std::atomic<int> threadsBusy;
    bool shouldTerminate;
    std::condition_variable nextActionAvailableCv;
    std::condition_variable threadsFinishedIterationCv;

    std::vector<std::thread> threadPool;
    std::deque<TaskStruct>taskQueue;

    //std::mutex cvMutex;
    std::mutex nextActionAvailableMutex;
    std::mutex taskQueueMutex;
    std::mutex threadsBusyMutex;
    std::mutex tfiMutex;

  public:
    DfsParallelizer(const unsigned long long dsId, const int inDepthLimit, const unsigned long inMaxDepthResultsLimit, const std::unordered_set<int>& inEndMsgs, const std::unordered_set<int>& inChoiceFilter, const int inMaxThreads, const bool inShowMessages)
      : depthLimit(inDepthLimit), endMsgs(inEndMsgs),
        choiceFilter(inChoiceFilter), maxThreads(inMaxThreads),
        showMessages(inShowMessages)
    {
      //initialize
      maxDepthReached = 0;
      endSearchEarly = false;
      maxDepthResultsLimit = inMaxDepthResultsLimit;

      const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
      foundChoicesMap[dsId] =
        ddd::getChoicesFromDuelState(ds, true, choiceFilter);
      const auto possibleChoices =
        getPossibleChoiceCombinations(foundChoicesMap.at(dsId));
      //deactivateCache.push_back(dsId);
      for (int i = 0; i < possibleChoices.size(); ++i)
        dfsStack.push_back
          (std::tuple<unsigned long long, std::vector<int>, int>
           (dsId, std::move(possibleChoices.at(i)), i + 1));

      Node dsIdNode;
      dsIdNode.parent = 0;
      dsIdNode.depth = 0;
      dsIdNode.choice = "(start)";
      choicesTree[dsId] = std::move(dsIdNode);
    }
    int getMaxDepthReached() {
      return maxDepthReached;
    }
    std::unordered_map<unsigned long long, ChoiceTuple>& getFoundChoicesMap() {
      return foundChoicesMap;
    }
    const std::unordered_map<unsigned long long, Node>& getChoicesTree() {
      return choicesTree;
    }
    const std::multimap<int, unsigned long long>& getDepthMap() {
      return depthMap;
    }
    bool wasSearchEndedEarly() {
      return endSearchEarly;
    }
    void iterate() {
      //the theoretically inefficient version where a thread is
      // created and destroyed each time the threadTask function
      // is called

      //tbb::task_group tg;

      while ((!dfsStack.empty()) || (!pushCache.empty())) {

        //check if results limit reached
        if (maxDepthResultsLimit) {
          if (maxDepthReached == depthLimit) {
            auto er = depthMap.equal_range(maxDepthReached);
            if (std::distance(er.first, er.second) >= maxDepthResultsLimit) {
              //results limit reached

              endSearchEarly = true;
              break; //exit from loop
            }
          }
        }

        //add from push cache to dfsStack
        if (!pushCache.empty()) {
          dfsStack.reserve(dfsStack.size() + pushCache.size());
          std::move(std::begin(pushCache), std::end(pushCache),
                    std::back_inserter(dfsStack));
          pushCache.clear();
        }

        //track size for later
        const auto prevChoicesMapSize = foundChoicesMap.size() - 1;

        //create local variables that can be copied/passed by
        // reference to loop body
        const int forSize = std::get<2>(dfsStack.back());
        const unsigned long long currParent = std::get<0>(dfsStack.back());
        const int currDepth = choicesTree.at(currParent).depth + 1;
        const auto parentChoiceTpl = foundChoicesMap.at(currParent);

        //single serial iteration for element at very back of the stack
        {
          const DuelState& parentDs = getDDD_GS()
            .dddapi.duelStates.at(currParent);
          const auto& sds = getDDD_GS().dddapi
            .shadowDuelStates.at(parentDs.shadowDsId);
          const intptr_t pShadowDuel = sds.pDuel;
          const ShadowDuelStateResponses& sdsr =
            sds.responsesMap.at(currParent);

          threadTask(currParent, std::get<1>(dfsStack.back()),
                     parentDs, parentChoiceTpl, currDepth,
                     true, pShadowDuel, sdsr);
        }

        //prepare threads to execute in parallel
        //  (limited to choices of common parent at back of stack)
        if (forSize > 1) {
          const DuelState parentDs = getDDD_GS()
            .dddapi.duelStates.at(currParent);
          const auto& sds = getDDD_GS().dddapi  //not passed so not copied
            .shadowDuelStates.at(parentDs.shadowDsId);
          const intptr_t pShadowDuel = sds.pDuel;
          const ShadowDuelStateResponses sdsr =
            sds.responsesMap.at(currParent);

          std::vector<std::thread> threads;

          //create and execute threads in parallel
          // (should limit to std::thread::hardware_concurrency() ?)
          for (auto i = dfsStack.rbegin() + 1;
               i != dfsStack.rbegin() + forSize; ++i) {
            const auto& parentCc = std::get<1>(*i);
            threads.emplace_back([&, sdsr, parentCc]() {
              threadTask(currParent, parentCc,
                         parentDs, parentChoiceTpl,
                         currDepth, false, pShadowDuel, sdsr);
            });
          }

          for (auto& t: threads)
            t.join();

          /*
          for (auto i = dfsStack.rbegin() + 1;
               i != dfsStack.rbegin() + forSize; ++i) {
            const auto& parentCc = std::get<1>(*i);
            tg.run([&, sdsr, parentCc]() {
              threadTask(currParent, parentCc,
                         parentDs, parentChoiceTpl,
                         currDepth, false, pShadowDuel, sdsr);
            });
          }
          tg.wait();
          */
        }

        const auto currChoicesMapSize = foundChoicesMap.size() - 1;
        if ((currChoicesMapSize > 1) &&
            (((currChoicesMapSize / 100) >
              (prevChoicesMapSize / 100))))
          clog("d", "\tDuel states found so far: ",
               (foundChoicesMap.size() - 1));

        //deactivate any states that are no longer needed
        deactivateCache.push_back(currParent);
        for (const auto &dsId: deactivateCache) {
          deactivate_duel_state(dsId);
        }
        deactivateCache.clear();

        //remove any reserve states for current parent
        // (should be all of them)
        if (dsIdReserve.find(currParent) != dsIdReserve.end()) {
          auto ar = dsIdReserve.equal_range(currParent);
          for (auto i = ar.first; i != ar.second; ++i) {
            end_duel(std::get<1>(i->second).pDuel);
          }
          dsIdReserve.erase(currParent);
        }

        //remove last elements of common parent at back of stack,
        // including the last parent
        //dfsStack.erase(dfsStack.rbegin(), dfsStack.rbegin() + forSize);
        if ((dfsStack.size() - forSize) < 1)
          dfsStack.clear();
        else
          dfsStack.resize(dfsStack.size() - forSize);

      }
    }
    void iterateThreadPool() {
      //initiate thread pool

      shouldTerminate = false;
      for (int i = 0; i < maxThreads; ++i) {
        threadPool.emplace_back
          (std::thread(&DfsParallelizer::threadBody, this, (i + 1)));
        threadPool.back().detach();
      }
      threadsBusy = 0;

      //main loop
      while (true) {
        //add from push cache to dfsStack
        if (!pushCache.empty()) {
          dfsStack.reserve(dfsStack.size() + pushCache.size());
          std::move(std::begin(pushCache), std::end(pushCache),
                    std::back_inserter(dfsStack));
          pushCache.clear();
        }

        if (dfsStack.empty() && pushCache.empty() && !taskQueue.empty()) {
          shouldTerminate = true;
          clog("d", "Main thread set shouldTerminate to true.");
          nextActionAvailableCv.notify_all();

          // for (auto& t: threadPool)
          //   t.join();

          break;
        }

        //track size for later
        const auto prevChoicesMapSize = foundChoicesMap.size() - 1;

        //create local variables that can be copied/passed by
        // reference to loop body
        const int forSize = std::get<2>(dfsStack.back());
        const unsigned long long currParent = std::get<0>(dfsStack.back());
        const int currDepth = choicesTree.at(currParent).depth + 1;
        const auto parentChoiceTpl = foundChoicesMap.at(currParent);

        //single serial iteration for element at very back of the stack
        {
          const DuelState& parentDs = getDDD_GS()
            .dddapi.duelStates.at(currParent);
          const auto& sds = getDDD_GS().dddapi
            .shadowDuelStates.at(parentDs.shadowDsId);
          const intptr_t pShadowDuel = sds.pDuel;
          const ShadowDuelStateResponses& sdsr =
            sds.responsesMap.at(currParent);

          threadTask(currParent, std::get<1>(dfsStack.back()),
                     parentDs, parentChoiceTpl, currDepth,
                     true, pShadowDuel, sdsr);
        }

        //prepare threads to execute in parallel
        //  (limited to choices of common parent at back of stack)
        if (forSize > 1) {
          const DuelState parentDs = getDDD_GS()
            .dddapi.duelStates.at(currParent);
          const auto& sds = getDDD_GS().dddapi  //not passed so not copied
            .shadowDuelStates.at(parentDs.shadowDsId);
          const intptr_t pShadowDuel = sds.pDuel;
          const ShadowDuelStateResponses sdsr =
            sds.responsesMap.at(currParent);

          {
            std::scoped_lock<std::mutex> tqLock(taskQueueMutex);
            //taskQueue.reserve(taskQueue.size() + forSize - 1);
            for (auto i = dfsStack.rbegin() + 1;
                 i != dfsStack.rbegin() + forSize; ++i) {
              TaskStruct ts(currParent, std::get<1>(*i),
                            parentDs, parentChoiceTpl,
                            currDepth, pShadowDuel, sdsr);
              taskQueue.emplace_front(std::move(ts));
            }
            nextActionAvailableCv.notify_all();
          }
          {
            std::unique_lock<std::mutex> tfiLock(tfiMutex);
            /*
            clog("d", "Main thread waiting for worker threads to",
                 " complete task queue.");
            */
            threadsFinishedIterationCv.wait(tfiLock, [this]() {
              std::scoped_lock<std::mutex, std::mutex> lock
                (taskQueueMutex, threadsBusyMutex);
              return taskQueue.empty() && (threadsBusy == 0);
            });
          }
        }

        const auto currChoicesMapSize = foundChoicesMap.size() - 1;
        /*
        clog("d", "  (from main thread) Duel states found so far: ",
             (foundChoicesMap.size() - 1));
        */

        if ((currChoicesMapSize > 1) &&
            (((currChoicesMapSize / 100) >
              (prevChoicesMapSize / 100))))
          clog("d", "\tDuel states found so far: ",
               (foundChoicesMap.size() - 1));

        //deactivate any states that are no longer needed
        deactivateCache.push_back(currParent);
        for (const auto &dsId: deactivateCache) {
          deactivate_duel_state(dsId);
        }
        deactivateCache.clear();

        //remove any reserve states for current parent
        // (should be all of them)
        if (dsIdReserve.find(currParent) != dsIdReserve.end()) {
          auto ar = dsIdReserve.equal_range(currParent);
          for (auto i = ar.first; i != ar.second; ++i) {
            end_duel(std::get<1>(i->second).pDuel);
          }
          dsIdReserve.erase(currParent);
        }

        //remove last elements of common parent at back of stack,
        // including the last parent
        //dfsStack.erase(dfsStack.rbegin(), dfsStack.rbegin() + forSize);
        if ((dfsStack.size() - forSize) < 1)
          dfsStack.clear();
        else
          dfsStack.resize(dfsStack.size() - forSize);

      }

      threadPool.clear();
    }
    void threadBody(const int threadId) {

      std::optional<TaskStruct> oTis;
      try {
      while (true) {
        {
          std::unique_lock<std::mutex> naaLock(nextActionAvailableMutex);
          /*
          clog("d", "Thread ", threadId, " waiting for next",
               " action available.");
          */

          nextActionAvailableCv.wait(naaLock, [this]() {
            return shouldTerminate || !taskQueue.empty();
          });
          /*
          clog("d", "Thread ", threadId, " resumed.");
          */

          if (shouldTerminate) {
            /*
            clog("d", "Thread ", threadId, " found shouldTerminate.");
            */
            break;
          }

          if (!taskQueue.empty()) {
            {
              std::scoped_lock<std::mutex> tbLock(threadsBusyMutex);
              ++threadsBusy;
            }

            std::scoped_lock<std::mutex> tqLock(taskQueueMutex);
            //oTis.emplace(std::move(taskQueue.back()));
            oTis.emplace(taskQueue.back());
            taskQueue.pop_back();
          }
          nextActionAvailableCv.notify_one();
        }

        if (oTis.has_value()) {
          /*
          clog("d", "Thread ", threadId,
               " performing a task for ds id ", oTis->parentDsId);
          */
          threadTask(oTis->parentDsId, oTis->parentCc, oTis->parentDs,
                     oTis->parentChoices, oTis->currDepth, false,
                     oTis->pDuelShadow, oTis->sdsr);
          //(*oTis)(this);
          oTis.reset();
          {
            std::scoped_lock<std::mutex> tbLock(threadsBusyMutex);
            --threadsBusy;
          }

          /*
          clog("ok", "Thread ", threadId, " finished performing",
               " task for ds id ", oTis->parentDsId);
          */
        }
        threadsFinishedIterationCv.notify_all();
      }

      /*
      clog("d", "Thread ", threadId, " reached end of thread body.");
      */

      } catch (std::exception& e) {
        clog("e", e.what());
        clog("e", "Thread ", threadId, " encountered exception!");
      }
    }
    void threadTask(const unsigned long long parentDsId, const std::vector<int>& parentCc, const DuelState& parentDs, const ChoiceTuple& parentChoices, const int currDepth, const bool canAssume, const intptr_t pShadowDuel, ShadowDuelStateResponses sdsr) {

      if (currDepth > depthLimit) {
        //already at depth limit; should never reach here...?
        clog("w", "Reached here! ", parentDsId);
        return;
      }

      //get new duel state to duplicate
      std::optional<
        std::tuple<unsigned long long, DuelState>> oNewDsIdTpl;
      std::optional<DuelState> oUpdatedParentDs;

      bool shouldDuplicateNew = false;
      if (canAssume) {
        oUpdatedParentDs = parentDs; //copy
        oNewDsIdTpl = ddd::assumeDuelState(*oUpdatedParentDs);

      } else {
        std::scoped_lock<std::mutex> dsIdRLock(dsIdReserveMutex);
        auto dsIdReserveItr = dsIdReserve.find(parentDsId);
        if (dsIdReserveItr != dsIdReserve.end()) {
          oNewDsIdTpl = std::move(dsIdReserveItr->second);
          dsIdReserve.erase(dsIdReserveItr);

        } else {
          shouldDuplicateNew = true;
        }
      }

      if (shouldDuplicateNew) {
        oNewDsIdTpl =
          ddd::duplicateDuelStateFromShadow(pShadowDuel,
                                            parentDs, sdsr);
      }

      const unsigned long long newDsId = std::get<0>(*oNewDsIdTpl);
      auto& newDs = std::get<1>(*oNewDsIdTpl);

      if (newDsId == 0) {
        clog("e", "Error duplicating duel state ", parentDsId,
             " to brute force (dfs-par; in parallelForBody).");
        return;
      }

      //reactivate duel state if necessary
      if (!ddd::isDuelStateActive(newDs)) {
        if (!ddd::reactivateDuelState(newDsId, newDs,
                                      pShadowDuel, sdsr)) {
          clog("e", "Error reactivating deactivated duel state ",
               parentDsId, " to brute force (dfs-par; in"
               " parallelForBody).");
          return;
        }
      }

      //determine if combination of choices can be expected to fail
      // (if so, checkFirstProcess will be set to true and if the
      //  1st process iteration performed on duel state (in the
      //  autoProcessLoop) did not succeed, will not consider that
      //  to be an error but will instead silently remove it)
      bool checkFirstProcess = false;
      if (parentCc.size() > 0) {
        //combination of at least 1 choice
        // (set response to duplicated duel state)

        setResponse(newDs, parentChoices, parentCc);
      } else {
        //combination of 0 choices
        // (do not set response and set checkFirstProcess to true)
        checkFirstProcess = true;
      }

      //check parent message because high probability of message 23
      // creating invalid combination of choices
      // (so set checkFirstProcess to true)
      if (std::get<1>(parentChoices)[0] == 23)
        checkFirstProcess = true;

      //process duplicated duel state and get result
      const auto aplTuple = autoProcessLoop(newDs, sdsr, endMsgs,
                                            choiceFilter, showMessages,
                                            checkFirstProcess);

      //handle based on result from auto process loop
      // (unlike single threaded implementation, returning early here
      //  is not considered an error and does not stop other
      //  threads that may be running)
      if ((!std::get<0>(aplTuple)) && (std::get<1>(aplTuple))) {
        //1st process iteration on duplicate duel state did not
        // succeed but not considered to be an error
        // (as it was expected it might not succeed)

        //update parent duel state if duel state was assumed
        // from parent
        if (oUpdatedParentDs.has_value()) {
          std::scoped_lock<std::mutex> dddgs
            (getDDD_GS().dddapi.duelStatesMutex);
          getDDD_GS().dddapi.duelStates.at(parentDsId) =
            std::move(*oUpdatedParentDs);
        }

        //save duel state where it may be reused in place of
        // duplicating the same parent
        std::scoped_lock<std::mutex> dsIdRLock(dsIdReserveMutex);
        dsIdReserve.emplace(parentDsId, std::move(*oNewDsIdTpl));
        return;

      } else if ((!std::get<0>(aplTuple))) {
        //encountered a process iteration that did not succeed
        // (that should have succeeded)
        const auto& aplTupleMsg = std::get<1>(std::get<2>(aplTuple))[0];
        clog("e", "Advancing duel state ", newDsId, " resulted in an"
             " unexpected invalid msg (", aplTupleMsg, ").");

        if (oUpdatedParentDs.has_value()) {
          std::scoped_lock<std::mutex> dddgs
            (getDDD_GS().dddapi.duelStatesMutex);
          getDDD_GS().dddapi.duelStates.at(parentDsId) =
            std::move(*oUpdatedParentDs);
        }

        return; //exit prematurely
      }

      //duplicated state reached valid state after being processed
      // with choice
      Node newDsIdNode;
      newDsIdNode.parent = parentDsId;
      newDsIdNode.depth = currDepth;

      //get choice string for node
      if (parentCc.size() == 0) {
        //successfully reached duel state after processing without
        // setting any choices

        if (showMessages)
          clog("d", "  Duplicated to ", newDsId, " without using"
               " choices '(continue processing)'.");

        //build tuple for duel state and push to result
        newDsIdNode.choice = "(continue processing)";

      } else if (parentCc.size() == 1) {
        //successfully reached duel state after processing
        // with 1 choice set

        const auto& choiceLabels = std::get<3>(parentChoices);

        if (showMessages)
          clog("d", "  Duplicated to ", newDsId, " using choice (",
               parentCc[0], ") '", choiceLabels[parentCc[0]], "'.");

        newDsIdNode.choice = choiceLabels[parentCc[0]];

      } else {
        //successfully reached duel state after processing
        // with multiple choices set

        const auto& choiceLabels = std::get<3>(parentChoices);

        //multiple choices to string vector to be joined later
        std::vector<std::string> v;
        for (const auto &i: parentCc)
          v.push_back(std::to_string(i));

        if (showMessages) {
          clog("d", "  Duplicated to ", newDsId, " using choices (",
               joinString(v, ", "), "):");
          for (const auto &i: parentCc)
            clog("d", "     ", choiceLabels[i]);
        }

        newDsIdNode.choice = joinString(v, ", ");
      }

      //merge everything together (for singleton variables only)
      {
        std::scoped_lock<std::mutex> dddgs
          (getDDD_GS().dddapi.duelStatesMutex);

        getDDD_GS().dddapi.duelStates.emplace(newDsId, std::move(newDs));
        getDDD_GS().dddapi.shadowDuelStates.at(parentDs.shadowDsId)
          .responsesMap.emplace(newDsId, std::move(sdsr));
        if (oUpdatedParentDs.has_value())
          getDDD_GS().dddapi.duelStates.at(parentDsId) =
            std::move(*oUpdatedParentDs);
      }

      //merge everything else together
      std::scoped_lock<std::mutex> everythingLock(everythingMutex);

      if (maxDepthReached < currDepth)
        maxDepthReached = currDepth;

      foundChoicesMap.emplace(newDsId, std::get<2>(aplTuple));
      choicesTree.emplace(newDsId, std::move(newDsIdNode));
      choicesTree.at(parentDsId).children.push_back(newDsId);
      depthMap.emplace(currDepth, newDsId);

      //find children if depth limit not yet reached
      if (currDepth >= depthLimit) {
        //deactivate_duel_state(newDsId);
        deactivateCache.push_back(newDsId);
        return;
      }

      //get potential children
      const auto choiceCombinations =
        getPossibleChoiceCombinations(std::move(std::get<2>(aplTuple)));

      //check if any children found
      if (choiceCombinations.size() == 0) {
        deactivateCache.push_back(newDsId);
        return;
      }

      //push children to cache to be added stack later
      std::vector< //temp vector to be moved...
        std::tuple<unsigned long long, std::vector<int>, int>> ccv;
      ccv.reserve(choiceCombinations.size());

      //build vector to be inserted into cache
      for (int i = 0; i < choiceCombinations.size(); ++i)
        ccv.push_back
          (std::tuple<unsigned long long, std::vector<int>, int>
           (newDsId, std::move(choiceCombinations.at(i)), i + 1));

      pushCache.reserve(pushCache.size() + ccv.size());
      std::move(std::begin(ccv), std::end(ccv),
                std::back_inserter(pushCache));
    }
  };

  const std::unordered_set<int> endMsgs = {
      5  //MSG_WIN
    ,40  //MSG_NEW_PHASE
    ,41  //MSG_NEW_TURN
  };

  //get actual choice filter based on filter passed from function
  const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
  const auto choiceFilter = ddd::getChoiceFilter(ds, inFilter);

  //populate stack with initial values
  DfsParallelizer dfsp = DfsParallelizer(dsId, depthLimit,
                                         maxDepthResultsLimit, endMsgs,
                                         choiceFilter, threadCount,
                                         showMessages);

  //start of brute forcing functionality
  clog("d", "Using max ", threadCount,
       (threadCount == 1) ? " thread." : " threads.");
  auto startTime = std::chrono::system_clock::now();

  //dfsp.iterate();
  dfsp.iterateThreadPool();

  auto endTime = std::chrono::system_clock::now();
  auto takenTime = std::chrono::duration_cast
    <std::chrono::seconds>(endTime - startTime);


  auto& foundChoicesMap = dfsp.getFoundChoicesMap();
  const auto& choicesTree = dfsp.getChoicesTree();
  const auto& depthMap = dfsp.getDepthMap();
  const auto maxDepthReached = dfsp.getMaxDepthReached();
  bool endSearchEarly = dfsp.wasSearchEndedEarly();

  //lambda declaration to print choices to reach duel state
  // from original duel state
  auto getChoiceChain = [&](const unsigned long long inDsId) {
    std::vector<std::string> v;
    for (unsigned long long cDsId = inDsId;
         cDsId != 0; cDsId = choicesTree.at(cDsId).parent) {
      v.push_back(choicesTree.at(cDsId).choice);
    }
    std::reverse(v.begin(), v.end());
    return "[dsid " + std::to_string(inDsId) + "]:  \t" +
      joinString(v, " => ");
  };

  //erase only the original duel state id (to adjust tallys)
  foundChoicesMap.erase(dsId);

  //get duel states at deepest depth reached
  auto dmer = depthMap.equal_range(maxDepthReached);
  std::vector<std::string> deepestDuelStatesVec;
  std::vector<std::string> deepestDuelStatesChoicesVec;
  unsigned long deepestDuelStatesCount = 0;

  for (auto i = dmer.first; i != dmer.second; ++i) {
    ++deepestDuelStatesCount;

    //store results if under limit (or no limit)
    if ((!showResultsLimit) || //no limit
        (deepestDuelStatesCount <= showResultsLimit))
      deepestDuelStatesChoicesVec.push_back(getChoiceChain(i->second));
  }

  //print results
  for (const auto& res: deepestDuelStatesChoicesVec)
    clog("d", "    ", res);

  if ((showResultsLimit) &&
      (showResultsLimit < deepestDuelStatesCount)) {
    clog("d", "    \t ......");
    clog("d", "    \t(omitted  ",
         deepestDuelStatesCount - showResultsLimit, " more results ("
         "limited to showing ", showResultsLimit, " results))");
  }

  //print deepest depth reached for user and the duel states
  // that reached that depth
  clog("d", "Found total of ", foundChoicesMap.size(),
       " duel states with ", deepestDuelStatesCount,
       " duel states at deepest depth of ", maxDepthReached, ".");
  clog("d", "    \t(", (foundChoicesMap.size() /
                        ((takenTime.count()) ? takenTime.count() : 1)),
       " duel states per second)");

  if (endSearchEarly) {
    clog("d", "    \t(search ended early because result limit of ",
         maxDepthResultsLimit, " for the deepest depth was reached)");

    //deactivate any possibly active duel states
    for (const auto &dmp: depthMap)
      if (dmp.second != dsId)
        deactivate_duel_state(dmp.second);
  }
  clog("d", "");

  //update contents of choices map with choices found from
  // brute forcing
  for (const auto &fc: foundChoicesMap)
    msgChoicesMap.insert(std::move(fc));

  if (shouldDeactivate)
    deactivate_duel_state(dsId);
}


void bruteForceStateChoicesDfsPar(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const int depthLimit, const unsigned long long maxDepthResultsLimit, const unsigned long long showResultsLimit, std::unordered_map<unsigned long long, ChoiceTuple>& msgChoicesMap, const std::unordered_set<int>& inFilter, const bool showMessages, const bool ignoreWarnings) {
  if (!ignoreWarnings) {
    if (!confirmYN("Really brute force all states (to depth " +
                   std::to_string(depthLimit) +
                   ") with current choices?", false)) {
      clog("i", "Not doing it.");
      return;
    }
  }
  bool shouldDeactivate = !is_duel_state_active(dsId);
  reactivate_duel_state(dsId);

  //Node struct defintion and some other var declarations needed
  // to either brute force or otherwise keep track of results
  struct Node {
    unsigned long long parent;
    std::vector<unsigned long long> children;
    int depth;
    std::string choice; //from parent to reach this node
  };
  //Class to organize variables and functions as well as limit scopes
  class DfsParallelizer {
    std::unordered_map<unsigned long long, ChoiceTuple> foundChoicesMap;
    std::unordered_map<unsigned long long, Node> choicesTree;
    std::multimap<int, unsigned long long> depthMap;
    std::vector<
      std::tuple<unsigned long long, std::vector<int>, int>> dfsStack;
    std::vector<
      std::tuple<unsigned long long, std::vector<int>, int>> pushCache;
    std::unordered_multimap<
      unsigned long long,
      std::tuple<unsigned long long, DuelState>> dsIdReserve;
    std::vector<intptr_t> pDuelReusableVec;
    std::vector<intptr_t> pDuelReusableVecCache;
    std::vector<unsigned long long> deactivateCache;

    std::vector<
      std::pair<unsigned long long, DuelState>> dddgsDuelStatesCache;
    std::vector<
      std::tuple<unsigned long long,
                 unsigned long long,
                 ShadowDuelStateResponses>> dddgsShadowDuelStatesCache;


    int maxDepthReached;
    bool endSearchEarly;
    unsigned long maxDepthResultsLimit;
    const int depthLimit;
    const std::unordered_set<int> endMsgs;
    const std::unordered_set<int> choiceFilter;
    const bool showMessages;

    //std::mutex foundChoicesMapMutex;
    //std::mutex choicesTreeMutex;
    //std::mutex depthMapMutex;
    //std::mutex dfsStackMutex;
    std::mutex pushCacheMutex;
    std::mutex dsIdReserveMutex;
    std::mutex everythingMutex;
    //std::mutex dsIdReusableMutex;
    std::mutex dsIdReusableCacheMutex;

    std::mutex genMutex;

  public:
    DfsParallelizer(const unsigned long long dsId, const int inDepthLimit, const unsigned long inMaxDepthResultsLimit, const std::unordered_set<int>& inEndMsgs, const std::unordered_set<int>& inChoiceFilter, const bool inShowMessages)
      : depthLimit(inDepthLimit), endMsgs(inEndMsgs),
        choiceFilter(inChoiceFilter), showMessages(inShowMessages)
    {
      //initialize
      maxDepthReached = 0;
      maxDepthResultsLimit = inMaxDepthResultsLimit;

      const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
      foundChoicesMap[dsId] =
        ddd::getChoicesFromDuelState(ds, true, choiceFilter);
      const auto possibleChoices =
        getPossibleChoiceCombinations(foundChoicesMap.at(dsId));
      //deactivateCache.push_back(dsId);
      for (int i = 0; i < possibleChoices.size(); ++i)
        dfsStack.push_back
          (std::tuple<unsigned long long, std::vector<int>, int>
           (dsId, std::move(possibleChoices.at(i)), i + 1));

      Node dsIdNode;
      dsIdNode.parent = 0;
      dsIdNode.depth = 0;
      dsIdNode.choice = "(start)";
      choicesTree[dsId] = std::move(dsIdNode);
    }
    int getMaxDepthReached() {
      return maxDepthReached;
    }
    std::unordered_map<unsigned long long, ChoiceTuple>& getFoundChoicesMap() {
      return foundChoicesMap;
    }
    const std::unordered_map<unsigned long long, Node>& getChoicesTree() {
      return choicesTree;
    }
    const std::multimap<int, unsigned long long>& getDepthMap() {
      return depthMap;
    }
    bool wasSearchEndedEarly() {
      return endSearchEarly;
    }
    void iterate() {

      while ((!dfsStack.empty()) || (!pushCache.empty())) {

        //check if results limit reached
        if (maxDepthResultsLimit) {
          if (maxDepthReached == depthLimit) {
            auto er = depthMap.equal_range(maxDepthReached);
            if (std::distance(er.first, er.second) >= maxDepthResultsLimit) {
              //results limit reached

              endSearchEarly = true;
              break; //exit from loop
            }
          }
        }

        //add from push cache to dfsStack
        if (!pushCache.empty()) {
          dfsStack.reserve(dfsStack.size() + pushCache.size());
          std::move(std::begin(pushCache), std::end(pushCache),
                    std::back_inserter(dfsStack));
          pushCache.clear();
        }

        if (!pDuelReusableVecCache.empty()) {
          pDuelReusableVec.reserve(pDuelReusableVec.size() +
                                   pDuelReusableVecCache.size());
          std::move(std::begin(pDuelReusableVecCache),
                    std::end(pDuelReusableVecCache),
                    std::back_inserter(pDuelReusableVec));
          pDuelReusableVecCache.clear();
        }

        //track size for later
        const auto prevChoicesMapSize = foundChoicesMap.size() - 1;

        //create local variables that can be copied/passed by
        // reference to loop body
        const int forSize = std::get<2>(dfsStack.back());
        const unsigned long long currParent = std::get<0>(dfsStack.back());
        const int currDepth = choicesTree.at(currParent).depth + 1;
        const auto parentChoiceTpl = foundChoicesMap.at(currParent);

        //single serial iteration for element at very back of the stack
        {
          const DuelState& parentDs = getDDD_GS()
            .dddapi.duelStates.at(currParent);
          const auto& sds = getDDD_GS().dddapi
            .shadowDuelStates.at(parentDs.shadowDsId);
          const intptr_t pShadowDuel = sds.pDuel;
          const ShadowDuelStateResponses& sdsr =
            sds.responsesMap.at(currParent);

          parallelForBody(currParent, std::get<1>(dfsStack.back()),
                          parentDs, parentChoiceTpl, currDepth,
                          true, pShadowDuel, sdsr, 0);
        }

        //parallel for
        //  (limited to choices of common parent at back of stack)
        if (forSize > 1) {
          const DuelState parentDs = getDDD_GS()
            .dddapi.duelStates.at(currParent);
          const auto& sds = getDDD_GS().dddapi  //not passed so not copied
            .shadowDuelStates.at(parentDs.shadowDsId);
          const intptr_t pShadowDuel = sds.pDuel;
          const ShadowDuelStateResponses sdsr =
            sds.responsesMap.at(currParent);

          //for std::execution::, can use seq,  par, par_unseq
          //const auto ep = std::execution::par;
          const auto ep = std::execution::seq;


          if (pDuelReusableVec.size() < (forSize - 1))
            for (int i = pDuelReusableVec.size(); i < (forSize - 1); ++i)
              pDuelReusableVec.push_back(0);
          std::atomic<int> ai = pDuelReusableVec.size();
          //clog("d", "forSize: ", forSize, "   ai: ", ai,
          //     "   pDuelReusableVec.size(): ", pDuelReusableVec.size());

          std::for_each(ep, dfsStack.rbegin() + 1,
                        dfsStack.rbegin() + forSize,
                        [&](const auto& tpl) {
                          parallelForBody(currParent, std::get<1>(tpl),
                                          parentDs, parentChoiceTpl,
                                          currDepth, false,
                                          pShadowDuel, sdsr,
                                          pDuelReusableVec.at(--ai));
                        });
          /*
          clog("d", "pDuelReusableVec.size(): ", pDuelReusableVec.size(),
               "    forSize: ", forSize,
               "    asdfxyz: ", (pDuelReusableVec.size() - (forSize - 1)));
          */

          if ((pDuelReusableVec.size() - (forSize - 1)) < 1)
            pDuelReusableVec.clear();
          else
            pDuelReusableVec.resize(pDuelReusableVec.size() - (forSize - 1));

          //clog("d", "pDuelReusableVec.size() (after): ", pDuelReusableVec.size());

          /*
          for (const auto &pDuel: pDuelReusableVec) //TEMP
            end_duel(pDuel); //TEMP
          pDuelReusableVec.clear();
          */

          /*
          tbb::parallel_for(
                            tbb::blocked_range<std::vector<std::tuple<unsigned long long, std::vector<int>, int>>::reverse_iterator>(dfsStack.rbegin() + 1, dfsStack.rbegin() + forSize),
                            [&, sdsr](const tbb::blocked_range<std::vector<std::tuple<unsigned long long, std::vector<int>, int>>::reverse_iterator>& br) {
            for (auto i = br.begin(); i != br.end(); ++i) {
              const auto& parentCc = std::get<1>(*i);
              parallelForBody(currParent, parentCc,
                              parentDs, parentChoiceTpl,
                              currDepth, false,
                              pShadowDuel, sdsr);
            }
          });
          */
        }

        /*
        std::for_each(std::execution::par, dfsStack.rbegin(),
                      dfsStack.rbegin() + forSize, [&](const auto& tpl) {
                        parallelForBody(tpl);
                      });
        */
        /*
        std::for_each_n(std::execution::par, dfsStack.rbegin()
                        forSize, [&](const auto& tpl) {
                          parallelForBody(tpl);
                        });
        */
        /*
        for (auto i = dfsStack.rbegin()
               ; i != dfsStack.rbegin() + forSize; ++i)
          parallelForBody(*i);
        */
        /*
        auto i = dfsStack.rbegin();

        //parallelForBody(*i); //serial
        parallelForBody(currParent, std::get<1>(*i),
                        parentDs, parentChoiceTpl, currDepth,
                        true, pShadowDuel, sdsr, everythingMutex);

        if (forSize > 1) {

          #pragma omp parallel for
          for (i = dfsStack.rbegin() + 1;
               i != dfsStack.rbegin() + forSize; ++i)
            //parallelForBody(*i); //parallel
            parallelForBody(currParent, std::get<1>(*i),
                            parentDs, parentChoiceTpl, currDepth,
                            false, pShadowDuel, sdsr);
        }
        */
        /*
        std::vector<std::thread> threads;
        for (auto i = dfsStack.rbegin() + 1;
             i != dfsStack.rbegin() + forSize; ++i) {
          const auto& parentCc = std::get<1>(*i);
          threads.emplace_back([&, sdsr, parentCc]() {
            parallelForBody(currParent, parentCc,
                            parentDs, parentChoiceTpl,
                            currDepth, false, pShadowDuel, sdsr);
          });
        }

        for (auto& t: threads)
          t.join();
        */

        const auto currChoicesMapSize = foundChoicesMap.size() - 1;
        if ((currChoicesMapSize > 1) &&
            (((currChoicesMapSize / 100) >
              (prevChoicesMapSize / 100))))
          clog("d", "\tDuel states found so far: ",
               (foundChoicesMap.size() - 1));

        //update singleton variables
        {
          auto& duelStates = getDDD_GS().dddapi.duelStates;
          duelStates.reserve(duelStates.size() +
                             dddgsDuelStatesCache.size() + 1);
          for (const auto &dsP: dddgsDuelStatesCache) {
            duelStates.emplace(dsP.first, std::move(dsP.second));
          }
          dddgsDuelStatesCache.clear();

          for (const auto& sdsTpl: dddgsShadowDuelStatesCache) {
            auto &sds = getDDD_GS().dddapi.shadowDuelStates
              .at(std::get<0>(sdsTpl));
            sds.responsesMap.emplace(std::get<1>(sdsTpl),
                                     std::get<2>(std::move(sdsTpl)));
          }
          dddgsShadowDuelStatesCache.clear();
        }

        auto& parentDs = getDDD_GS().dddapi.duelStates.at(currParent);
        if ((parentDs.pDuel) && ddd::isDuelDuplicatable(parentDs.pDuel)) {
          pDuelReusableVecCache.push_back(parentDs.pDuel);
          parentDs.pDuel = 0;
          parentDs.active = false;

        } else {
          deactivateCache.push_back(currParent);
        }

        //deactivate any states that are no longer needed
        for (const auto &dsId: deactivateCache) {
          deactivate_duel_state(dsId);
        }
        deactivateCache.clear();

        //remove any reserve states for current parent
        // (should be all of them)
        if (dsIdReserve.find(currParent) != dsIdReserve.end()) {
          auto ar = dsIdReserve.equal_range(currParent);
          for (auto i = ar.first; i != ar.second; ++i) {
            end_duel(std::get<1>(i->second).pDuel);
          }
          dsIdReserve.erase(currParent);
        }

        //remove last elements of common parent at back of stack,
        // including the last parent
        //dfsStack.erase(dfsStack.rbegin(), dfsStack.rbegin() + forSize);
        if ((dfsStack.size() - forSize) < 1)
          dfsStack.clear();
        else
          dfsStack.resize(dfsStack.size() - forSize);

      }
    }
    void parallelForBody(const unsigned long long parentDsId, const std::vector<int>& parentCc, const DuelState& parentDs, const ChoiceTuple& parentChoices, const int currDepth, const bool canAssume, const intptr_t pShadowDuel, ShadowDuelStateResponses sdsr, const intptr_t pDuelReuse) {

      if (currDepth > depthLimit) {
        //already at depth limit; should never reach here...?
        clog("w", "Reached here! ", parentDsId);
        return;
      }

      //get new duel state to duplicate
      std::optional<std::tuple<unsigned long long, DuelState>> oNewDsIdTpl;
      std::optional<DuelState> oUpdatedParentDs;

      if (canAssume) {
        oUpdatedParentDs = parentDs; //copy
        oNewDsIdTpl = ddd::assumeDuelState(*oUpdatedParentDs);

      } else {
        bool shouldDuplicate = false;
        {
          std::scoped_lock<std::mutex> dsIdRLock(dsIdReserveMutex);
          auto dsIdReserveItr = dsIdReserve.find(parentDsId);
          if (dsIdReserveItr != dsIdReserve.end()) {
            oNewDsIdTpl = std::move(dsIdReserveItr->second);
            dsIdReserve.erase(dsIdReserveItr);

          } else {
            shouldDuplicate = true;
          }
        }
        if (shouldDuplicate) {
          //std::scoped_lock<std::mutex> lock(genMutex); //TEMP
            oNewDsIdTpl =
              ddd::duplicateDuelStateFromShadow(pShadowDuel,
                                                parentDs, sdsr,
                                                //0);
                                                pDuelReuse);
        }
      }


      const unsigned long long newDsId = std::get<0>(*oNewDsIdTpl);
      auto& newDs = std::get<1>(*oNewDsIdTpl);

      if (newDsId == 0) {
        clog("e", "Error duplicating duel state ", parentDsId,
             " to brute force (dfs-par; in parallelForBody).");
        return;
      }

      //reactivate duel state if necessary
      if (!ddd::isDuelStateActive(newDs)) {
        if (!ddd::reactivateDuelState(newDsId, newDs,
                                      pShadowDuel, sdsr)) {
          clog("e", "Error reactivating deactivated duel state ",
               parentDsId, " to brute force (dfs-par; in"
               " parallelForBody).");
          return;
        }
      }

      //determine if combination of choices can be expected to fail
      // (if so, checkFirstProcess will be set to true and if the
      //  1st process iteration performed on duel state (in the
      //  autoProcessLoop) did not succeed, will not consider that
      //  to be an error but will instead silently remove it)
      bool checkFirstProcess = false;
      if (parentCc.size() > 0) {
        //combination of at least 1 choice
        // (set response to duplicated duel state)

        setResponse(newDs, parentChoices, parentCc);
      } else {
        //combination of 0 choices
        // (do not set response and set checkFirstProcess to true)
        checkFirstProcess = true;
      }

      //check parent message because high probability of message 23
      // creating invalid combination of choices
      // (so set checkFirstProcess to true)
      if (std::get<1>(parentChoices)[0] == 23)
        checkFirstProcess = true;

      //process duplicated duel state and get result
      const auto aplTuple = autoProcessLoop(newDs, sdsr, endMsgs,
                                            choiceFilter, showMessages,
                                            checkFirstProcess);

      //handle based on result from auto process loop
      // (unlike single threaded implementation, returning early here
      //  is not considered an error and does not stop other
      //  threads that may be running)
      if ((!std::get<0>(aplTuple)) && (std::get<1>(aplTuple))) {
        //1st process iteration on duplicate duel state did not
        // succeed but not considered to be an error
        // (as it was expected it might not succeed)

        //update parent duel state if duel state was assumed
        // from parent
        if (oUpdatedParentDs.has_value()) {
          std::scoped_lock<std::mutex> dddgs
            (getDDD_GS().dddapi.duelStatesMutex);
          getDDD_GS().dddapi.duelStates.at(parentDsId) =
            std::move(*oUpdatedParentDs);
        }

        //save duel state where it may be reused in place of
        // duplicating the same parent
        std::scoped_lock<std::mutex> dsIdRLock(dsIdReserveMutex);
        dsIdReserve.emplace(parentDsId, std::move(*oNewDsIdTpl));
        return;

      } else if ((!std::get<0>(aplTuple))) {
        //encountered a process iteration that did not succeed
        // (that should have succeeded)
        const auto& aplTupleMsg = std::get<1>(std::get<2>(aplTuple))[0];
        clog("e", "Advancing duel state ", newDsId, " resulted in an"
             " unexpected invalid msg (", aplTupleMsg, ").");

        //immediately update singleton
        if (oUpdatedParentDs.has_value()) {
          std::scoped_lock<std::mutex> dddgs
            (getDDD_GS().dddapi.duelStatesMutex);
          getDDD_GS().dddapi.duelStates.at(parentDsId) =
            std::move(*oUpdatedParentDs);
        }

        return; //exit prematurely
      }

      //duplicated state reached valid state after being processed
      // with choice
      Node newDsIdNode;
      newDsIdNode.parent = parentDsId;
      newDsIdNode.depth = currDepth;

      //get choice string for node
      if (parentCc.size() == 0) {
        //successfully reached duel state after processing without
        // setting any choices

        if (showMessages)
          clog("d", "  Duplicated to ", newDsId, " without using"
               " choices '(continue processing)'.");

        //build tuple for duel state and push to result
        newDsIdNode.choice = "(continue processing)";

      } else if (parentCc.size() == 1) {
        //successfully reached duel state after processing
        // with 1 choice set

        const auto& choiceLabels = std::get<3>(parentChoices);

        if (showMessages)
          clog("d", "  Duplicated to ", newDsId, " using choice (",
               parentCc[0], ") '", choiceLabels[parentCc[0]], "'.");

        newDsIdNode.choice = choiceLabels[parentCc[0]];

      } else {
        //successfully reached duel state after processing
        // with multiple choices set

        const auto& choiceLabels = std::get<3>(parentChoices);

        //multiple choices to string vector to be joined later
        std::vector<std::string> v;
        for (const auto &i: parentCc)
          v.push_back(std::to_string(i));

        if (showMessages) {
          clog("d", "  Duplicated to ", newDsId, " using choices (",
               joinString(v, ", "), "):");
          for (const auto &i: parentCc)
            clog("d", "     ", choiceLabels[i]);
        }

        newDsIdNode.choice = joinString(v, ", ");
      }

      const bool isNewDsDuplicatable =
        ddd::isDuelDuplicatable(newDs.pDuel);

      //merge everything together (for singleton variables only)
      const auto updateDDD_GS = [&]() {
        if (oUpdatedParentDs.has_value()) {
          //update singleton right away
          std::scoped_lock<std::mutex> dddgs
            (getDDD_GS().dddapi.duelStatesMutex);

          getDDD_GS().dddapi.duelStates.emplace(newDsId, std::move(newDs));
          getDDD_GS().dddapi.shadowDuelStates.at(parentDs.shadowDsId)
            .responsesMap.emplace(newDsId, std::move(sdsr));
          if (oUpdatedParentDs.has_value())
            getDDD_GS().dddapi.duelStates.at(parentDsId) =
              std::move(*oUpdatedParentDs);

        } else {
          //update singleton later (along with rest of iterations)

          //(everything)lock should already be acquired
          dddgsDuelStatesCache
            .push_back(std::make_pair(newDsId, std::move(newDs)));
          dddgsShadowDuelStatesCache
            .push_back(std::make_tuple(parentDs.shadowDsId,
                                       newDsId, std::move(sdsr)));
          if (oUpdatedParentDs.has_value()) {
            clog("w", "Should not end up here...?");
            dddgsDuelStatesCache
              .push_back(std::make_pair(parentDsId,
                                        std::move(*oUpdatedParentDs)));
          }
        }
      };

      //merge everything else together
      std::scoped_lock<std::mutex> everythingLock(everythingMutex);

      if (maxDepthReached < currDepth)
        maxDepthReached = currDepth;

      foundChoicesMap.emplace(newDsId, std::get<2>(aplTuple));
      choicesTree.emplace(newDsId, std::move(newDsIdNode));
      choicesTree.at(parentDsId).children.push_back(newDsId);
      depthMap.emplace(currDepth, newDsId);

      //find children if depth limit not yet reached
      if (currDepth >= depthLimit) {
        if (ddd::isDuelDuplicatable(newDs.pDuel)) {
          pDuelReusableVecCache.push_back(newDs.pDuel);
          newDs.pDuel = 0;
          newDs.active = false;

        } else {
          deactivateCache.push_back(newDsId);
        }
        updateDDD_GS();
        return;
      }

      //get potential children
      const auto choiceCombinations =
        getPossibleChoiceCombinations(std::move(std::get<2>(aplTuple)));

      //check if any children found
      if (choiceCombinations.size() == 0) {
        if (ddd::isDuelDuplicatable(newDs.pDuel)) {
          pDuelReusableVecCache.push_back(newDs.pDuel);
          newDs.pDuel = 0;
          newDs.active = false;

        } else {
          deactivateCache.push_back(newDsId);
        }
        updateDDD_GS();
        return;
      }

      //push children to cache to be added stack later
      std::vector< //temp vector to be moved...
        std::tuple<unsigned long long, std::vector<int>, int>> ccv;
      ccv.reserve(choiceCombinations.size());

      //build vector to be inserted into cache
      for (int i = 0; i < choiceCombinations.size(); ++i)
        ccv.push_back
          (std::tuple<unsigned long long, std::vector<int>, int>
           (newDsId, std::move(choiceCombinations.at(i)), i + 1));

      pushCache.reserve(pushCache.size() + ccv.size());
      std::move(std::begin(ccv), std::end(ccv),
                std::back_inserter(pushCache));

      if (ddd::isDuelDuplicatable(newDs.pDuel)) {
        pDuelReusableVecCache.push_back(newDs.pDuel);
        newDs.pDuel = 0;
        newDs.active = false;

      } else {
        deactivateCache.push_back(newDsId);
      }
      updateDDD_GS();
    }
  };

  const std::unordered_set<int> endMsgs = {
      5  //MSG_WIN
    ,40  //MSG_NEW_PHASE
    ,41  //MSG_NEW_TURN
  };

  //get actual choice filter based on filter passed from function
  const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
  const auto choiceFilter = ddd::getChoiceFilter(ds, inFilter);

  //populate stack with initial values
  DfsParallelizer dfsp = DfsParallelizer(dsId, depthLimit,
                                         maxDepthResultsLimit, endMsgs,
                                         choiceFilter, showMessages);
  //start of brute forcing functionality
  auto startTime = std::chrono::system_clock::now();

  dfsp.iterate();
  //dfsp.iterateAllTest();

  auto endTime = std::chrono::system_clock::now();
  auto takenTime = std::chrono::duration_cast
    <std::chrono::seconds>(endTime - startTime);


  auto& foundChoicesMap = dfsp.getFoundChoicesMap();
  const auto& choicesTree = dfsp.getChoicesTree();
  const auto& depthMap = dfsp.getDepthMap();
  const auto maxDepthReached = dfsp.getMaxDepthReached();
  bool endSearchEarly = dfsp.wasSearchEndedEarly();

  //lambda declaration to print choices to reach duel state
  // from original duel state
  auto getChoiceChain = [&](const unsigned long long inDsId) {
    std::vector<std::string> v;
    for (unsigned long long cDsId = inDsId;
         cDsId != 0; cDsId = choicesTree.at(cDsId).parent) {
      v.push_back(choicesTree.at(cDsId).choice);
    }
    std::reverse(v.begin(), v.end());
    return "[dsid " + std::to_string(inDsId) + "]:  \t" +
      joinString(v, " => ");
  };

  //erase only the original duel state id (to adjust tallys)
  foundChoicesMap.erase(dsId);

  //get duel states at deepest depth reached
  auto dmer = depthMap.equal_range(maxDepthReached);
  std::vector<std::string> deepestDuelStatesVec;
  std::vector<std::string> deepestDuelStatesChoicesVec;
  unsigned long deepestDuelStatesCount = 0;

  for (auto i = dmer.first; i != dmer.second; ++i) {
    ++deepestDuelStatesCount;

    //store results if under limit (or no limit)
    if ((!showResultsLimit) || //no limit
        (deepestDuelStatesCount <= showResultsLimit))
      deepestDuelStatesChoicesVec.push_back(getChoiceChain(i->second));
  }

  //print results
  for (const auto& res: deepestDuelStatesChoicesVec)
    clog("d", "    ", res);

  if ((showResultsLimit) &&
      (showResultsLimit < deepestDuelStatesCount)) {
    clog("d", "    \t ......");
    clog("d", "    \t(omitted  ",
         deepestDuelStatesCount - showResultsLimit, " more results ("
         "limited to showing ", showResultsLimit, " results))");
  }

  //print deepest depth reached for user and the duel states
  // that reached that depth
  clog("d", "Found total of ", foundChoicesMap.size(),
       " duel states with ", deepestDuelStatesCount,
       " duel states at deepest depth of ", maxDepthReached, ".");
  clog("d", "    \t(", (foundChoicesMap.size() /
                        ((takenTime.count()) ? takenTime.count() : 1)),
       " duel states per second)");

  if (endSearchEarly) {
    clog("d", "    \t(search ended early because result limit of ",
         maxDepthResultsLimit, " for the deepest depth was reached)");

    //deactivate any possibly active duel states
    for (const auto &dmp: depthMap)
      if (dmp.second != dsId)
        deactivate_duel_state(dmp.second);
  }
  clog("d", "");

  //update contents of choices map with choices found from
  // brute forcing
  for (const auto &fc: foundChoicesMap)
    msgChoicesMap.insert(std::move(fc));

  if (shouldDeactivate)
    deactivate_duel_state(dsId);
}



void bruteForceStateChoicesDfsPar2(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const int depthLimit, const unsigned long long maxDepthResultsLimit, const unsigned long long showResultsLimit, std::unordered_map<unsigned long long, ChoiceTuple>& msgChoicesMap, const std::unordered_set<int>& inFilter, const int threadCount, const bool showMessages, const bool ignoreWarnings) {
  if (!ignoreWarnings) {
    if (!confirmYN("Really brute force all states (to depth " +
                   std::to_string(depthLimit) +
                   ") with current choices?", false)) {
      clog("i", "Not doing it.");
      return;
    }
  }
  bool shouldDeactivate = !is_duel_state_active(dsId);
  reactivate_duel_state(dsId);

  //Class to organize variables and functions as well as limit scopes
  class alignas(64) DfsParallelizer {
    struct StateNode {
      unsigned long long dsId; //necessary?
      unsigned long long parent; //(of this node)
      std::unordered_set<unsigned long long> children; //(of this node)
      StateNode(unsigned long long inDsId, unsigned long long inParent)
        : dsId(inDsId), parent(inParent) {
      }
    };
    struct alignas(256) IterationGroup {

      unsigned long long parent;
      std::vector<std::vector<int>> choices;
      int depth;
      ChoiceTuple ct;

      int currChoiceIndex = 0;
    };
    struct alignas(1024) ParallelForData {

      int depth;
      unsigned long long dsId;
      DuelState duelState;
      ChoiceTuple choices;
      std::vector<int> cc;
      intptr_t pShadowDuel;
      ShadowDuelStateResponses sdsr;
      intptr_t pDuelReuse;

      struct alignas(256) ParallelResult {
        std::vector<
          std::pair<unsigned long long, StateNode>> stateNodeResults;
        std::vector<std::pair<int, unsigned long long>> depthMapResults;
        std::vector<
          std::pair<unsigned long long, DuelState>> duelStateResults;
        std::vector<std::tuple<
                    unsigned long long, //shadow duel state id
                    unsigned long long, //responses map duel state id
                    ShadowDuelStateResponses> //responses map value
                  > responsesMapResults;
        std::vector<IterationGroup> igResults;
        std::vector<intptr_t> endDuelResults;
        std::vector<intptr_t> reusableDuels;

        int maxDepthReached = 0;
      } pr;
    };

    std::vector<IterationGroup> currIterationGroups;
    std::vector<ParallelForData> currParallelIterations;
    std::vector<intptr_t> reusableDuelsAccumulator;

    const int depthLimit;
    const int threadCount;
    const std::unordered_set<int> endMsgs;
    const std::unordered_set<int> choiceFilter;
    const bool showMessages;

    void updateParentsToPr
    (
     ParallelForData& pfd,
     const unsigned long long parentDsId,
     const DuelState& parentDs,
     const unsigned long long parentShadowDsId,
     const ShadowDuelStateResponses& parentSdsr
     ) {
      pfd.pr.duelStateResults.emplace_back(parentDsId,
                                           std::move(parentDs));
      pfd.pr.responsesMapResults.emplace_back(parentShadowDsId,
                                              parentDsId, parentSdsr);
    };
    void updateIterationParentVars
    (unsigned long long& parentDsId, const unsigned long long newDsId,
     DuelState& parentDs, const DuelState& newDs,
     ShadowDuelStateResponses& parentSdsr,
     const ShadowDuelStateResponses& newSdsr
     ) {
      parentDsId = newDsId;
      parentDs = std::move(newDs);
      parentSdsr = std::move(newSdsr);
    }
    void updateIterationParentVars
    (unsigned long long& parentDsId, const unsigned long long newDsId,
     DuelState& parentDs, const DuelState& newDs,
     ShadowDuelStateResponses& parentSdsr,
     const ShadowDuelStateResponses& newSdsr,
     std::vector<int>& parentCc, const std::vector<int>& newCc,
     ChoiceTuple& parentChoices, const ChoiceTuple& newChoices
     ) {
      updateIterationParentVars(parentDsId, newDsId,
                                parentDs, std::move(newDs),
                                parentSdsr, std::move(newSdsr));
      parentCc = std::move(newCc);
      parentChoices = std::move(newChoices);
    }

  public:
    std::unordered_map<unsigned long long, StateNode> resultsMap;
    std::unordered_multimap<int, unsigned long long> depthMap;
    int maxDepthReached;
    bool endSearchEarly;
    unsigned long maxDepthResultsLimit;

    DfsParallelizer(const unsigned long long dsId, const int inDepthLimit, const unsigned long inMaxDepthResultsLimit, const std::unordered_set<int>& inEndMsgs, const std::unordered_set<int>& inChoiceFilter, const int inThreadCount, const bool inShowMessages)
      : depthLimit(inDepthLimit), endMsgs(inEndMsgs),
        choiceFilter(inChoiceFilter), threadCount(inThreadCount),
        showMessages(inShowMessages)
    {
      maxDepthReached = 0;
      endSearchEarly = false;
      maxDepthResultsLimit = inMaxDepthResultsLimit;
      //currIterationGroups.reserve(threadCount);

      //maybe duplicate dsId?

      //populate for 1st iteration
      const auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
      const auto ct = ddd::getChoicesFromDuelState(ds, false,
                                                   choiceFilter);
      const auto choiceCombinations =
        getPossibleChoiceCombinations(ct);
      currIterationGroups.emplace_back(IterationGroup {
          dsId, std::move(choiceCombinations), 0, std::move(ct)
        });

      resultsMap.emplace(dsId, StateNode(0, dsId));
      depthMap.emplace(0, dsId);

    }
    void initializeForParallelFor() {

      //const auto ep = std::execution::par;
      const auto ep = std::execution::seq;
      currIterationGroups
        .erase(std::remove_if(ep,
                              currIterationGroups.begin(),
                              currIterationGroups.end(),
                              [](const IterationGroup& ig) {
                                return ((ig.currChoiceIndex >=
                                         ig.choices.size()) ||
                                        (ig.choices.size() == 0));
                              }),
               currIterationGroups.end());

      //build current iteration
      for (auto& ig: currIterationGroups) {
        auto& ds = getDDD_GS().dddapi.duelStates.at(ig.parent);
        auto& sds = getDDD_GS().dddapi.shadowDuelStates.at(ds.shadowDsId);

        intptr_t pDuelReuse = 0;
        if (!reusableDuelsAccumulator.empty()) {
          pDuelReuse = reusableDuelsAccumulator.back();
          reusableDuelsAccumulator.pop_back();
        }

        currParallelIterations
          .emplace_back(ParallelForData {
              ig.depth,
              ig.parent,
              DuelState(ds),
              ig.ct,
              std::move(ig.choices.at(ig.currChoiceIndex)),
              sds.pDuel,
              sds.responsesMap.at(ig.parent),
              pDuelReuse
            });
        ++(ig.currChoiceIndex);
      }
    }
    void callParallelFor() {

      std::for_each(std::execution::seq,
      //std::for_each(std::execution::par,
      //std::for_each(std::execution::par_unseq,
                    currParallelIterations.begin(),
                    currParallelIterations.end(),
                    [&](ParallelForData& pfd) {
                      parallelForBody(pfd);
      });
      /*
      std::vector<std::thread> threads;
      for (auto& pfd: currParallelIterations) {
        threads.emplace_back(std::thread([&]() {parallelForBody(pfd);}));
      }

      for (auto& t: threads)
        t.join();
      */
      /*
      //#pragma omp parallel for
      for (auto i = currParallelIterations.begin();
           i != currParallelIterations.end(); ++i)
        parallelForBody(*i);
      */
    }
    void processParallelForResults() {
      //std::list<intptr_t> endDuelAccumulator;
      std::vector<intptr_t> endDuelAccumulator;

      const auto prevRmSize = resultsMap.size() - 1;
      std::mutex rmMutex;
      std::mutex dmMutex;
      std::mutex dddgsDsMutex;
      std::mutex dddgsSdsMutex;
      std::mutex cigMutex;
      std::mutex edrMutex;
      std::mutex ruedMutex;

      //for (auto& pfd: currParallelIterations) {
      std::for_each(std::execution::seq,
      //std::for_each(std::execution::par,
      //std::for_each(std::execution::par_unseq,
                    currParallelIterations.begin(),
                    currParallelIterations.end(),
                    [&](ParallelForData& pfd) {

        for (const auto &snp: pfd.pr.stateNodeResults)
          {
          std::scoped_lock<std::mutex> rmLock(rmMutex);
          resultsMap.insert(std::move(snp));
          }

        for (const auto &dp: pfd.pr.depthMapResults)
          {
          std::scoped_lock<std::mutex> dmLock(dmMutex);
          depthMap.insert(std::move(dp));
          }

        //pretty much everything emplaced here is an empty duel state
        auto& duelStates = getDDD_GS().dddapi.duelStates;
        for (const auto &dsp: pfd.pr.duelStateResults) {
          std::scoped_lock<std::mutex> dddgsDsLock(dddgsDsMutex);
          if (duelStates.find(dsp.first) == duelStates.end())
            duelStates.emplace(std::move(dsp));
          else
            duelStates.at(dsp.first) = std::move(dsp.second);
        }

        auto& shadowDuelStates = getDDD_GS().dddapi.shadowDuelStates;
        for (const auto &rmp: pfd.pr.responsesMapResults) {
          std::scoped_lock<std::mutex> dddgsSdsLock(dddgsSdsMutex);
          if (shadowDuelStates.find(std::get<0>(rmp)) ==
              shadowDuelStates.end()) {
            clog("e", "Attempted to update nonexistent shadow duel"
                 " state id ", std::get<0>(rmp), ".");
            break;
          }
          auto& rm = shadowDuelStates.at(std::get<0>(rmp)).responsesMap;
          if (rm.find(std::get<1>(rmp)) == rm.end())
            rm.emplace(std::get<1>(rmp), std::move(std::get<2>(rmp)));
          else
            rm.at(std::get<1>(rmp)) = std::move(std::get<2>(rmp));
        }

        {
        std::scoped_lock<std::mutex> cigLock(cigMutex);
        // currIterationGroups.splice(currIterationGroups.end(),
        //                         std::move(pfd.pr.igResults));

        std::move(std::begin(pfd.pr.igResults),
                  std::end(pfd.pr.igResults),
                  std::back_inserter(currIterationGroups));
        }
        /*
        currIterationGroups.reserve(currIterationGroups.size() +
                                    pfd.pr.igResults.size());

        std::move(std::begin(pfd.pr.igResults),
                  std::end(pfd.pr.igResults),
                  std::back_inserter(currIterationGroups));
        */
        /*
        endDuelAccumulator.splice(endDuelAccumulator.end(),
                                  std::move(pfd.pr.endDuelResults));
        */

        {
        std::scoped_lock<std::mutex> edrLock(edrMutex);
        std::move(std::begin(pfd.pr.endDuelResults),
                  std::end(pfd.pr.endDuelResults),
                  std::back_inserter(endDuelAccumulator));
        }


        for (const auto& pDuel: pfd.pr.reusableDuels) {
          std::scoped_lock<std::mutex> ruedLock(ruedMutex);
          if ((pDuel) && (ddd::isDuelDuplicatable(pDuel)))
            reusableDuelsAccumulator.emplace_back(pDuel);
          else
            endDuelAccumulator.emplace_back(pDuel);
        }

        if (this->maxDepthReached < pfd.pr.maxDepthReached)
          this->maxDepthReached = pfd.pr.maxDepthReached;
      });

      currParallelIterations.clear();

      for (const auto& pDuel: endDuelAccumulator)
        end_duel(pDuel);

      const auto currRmSize = resultsMap.size() - 1;
      if ((currRmSize > 1) &&
          (((currRmSize / 100) >
            (prevRmSize / 100))))
        clog("d", "\tDuel states found so far: ",
             (resultsMap.size() - 1));
    }
    void iterate() {

      while (!currIterationGroups.empty()) {

        //check if results limit reached
        if (maxDepthResultsLimit) {
          if (maxDepthReached == depthLimit) {
            auto er = depthMap.equal_range(maxDepthReached);
            if (std::distance(er.first, er.second) >= maxDepthResultsLimit) {
              //results limit reached

              endSearchEarly = true;
              break; //exit from loop
            }
          }
        }

        initializeForParallelFor();

        if (currParallelIterations.empty())
          continue;

        callParallelFor();
        processParallelForResults();
      }

      for (const intptr_t pDuel: reusableDuelsAccumulator)
        end_duel(pDuel);
    }
    void parallelForBody(ParallelForData& pfd) {

      //parent variables of current iteration
      // (updated after every iteration, including when
      //  breaking from the function)
      unsigned long long parentDsId = pfd.dsId;
      DuelState parentDs = std::move(pfd.duelState);
      std::vector<int> parentCc = std::move(pfd.cc);
      ChoiceTuple parentChoices = std::move(pfd.choices);
      ShadowDuelStateResponses parentSdsr = std::move(pfd.sdsr);
      const unsigned long long parentShadowDsId = parentDs.shadowDsId;
      const intptr_t parentPDuelShadow = pfd.pShadowDuel;
      const intptr_t parentPDuelReuse = pfd.pDuelReuse;

      bool invalidDuelStateProcessResult = false;

      //iterate until max depth reached
      for (int currDepth = pfd.depth + 1;
           currDepth <= this->depthLimit; ++currDepth) {

        //get new duel state to duplicate
        auto newDsIdTpl = (ddd::isDuelStateActive(parentDs))
          ? ddd::assumeDuelState(parentDs)
          : ddd::duplicateDuelStateFromShadow(parentPDuelShadow,
                                              parentDs, parentSdsr,
                                              parentPDuelReuse);

        const unsigned long long newDsId = std::get<0>(newDsIdTpl);
        auto& newDs = std::get<1>(newDsIdTpl);

        if (newDsId == 0) {
          clog("e", "Error assuming duel state ", parentDsId,
             " to brute force (dfs-par2; in parallelForBody).");
          break;
        }

        if (!ddd::isDuelStateActive(newDs)) {
          if (!ddd::reactivateDuelState(newDsId, newDs,
                                        parentPDuelShadow, parentSdsr)) {
            clog("e", "Duel state ", newDsId, " was not activated"
                 " after assumed (from duel state ", parentDsId, ").");
          }
          break;
        }

        //determine if combination of choices can be expected to fail
        // (if so, checkFirstProcess will be set to true and if the
        //  1st process iteration performed on duel state (in the
        //  autoProcessLoop) did not succeed, will not consider that
        //  to be an error but will instead silently remove it)
        bool checkFirstProcess = false;
        if (parentCc.size() > 0) {
          //combination of at least 1 choice
          // (set response to duplicated duel state)

          setResponse(newDs, parentChoices, parentCc);
        } else {
          //combination of 0 choices
          // (do not set response and set checkFirstProcess to true)
          checkFirstProcess = true;
        }

        //check parent message because high probability of message 23
        // creating invalid combination of choices
        // (so set checkFirstProcess to true)
        if (std::get<1>(parentChoices)[0] == 23)
          checkFirstProcess = true;

        //process duplicated duel state and get result
        ShadowDuelStateResponses newSdsr = parentSdsr; //maybe?
        const auto aplTuple =
          autoProcessLoop(newDs, newSdsr, endMsgs, choiceFilter,
                          showMessages, checkFirstProcess,
                          false);

        //handle based on result from auto process loop
        // (unlike single threaded implementation, returning early here
        //  is not considered an error and does not stop other
        //  threads that may be running)
        if ((!std::get<0>(aplTuple)) && (std::get<1>(aplTuple))) {
          //1st process iteration on duplicate duel state did not
          // succeed but not considered to be an error
          // (as it was expected it might not succeed)

          //clog("w", "   hmm... (", newDsId, ")");

          updateParentsToPr(pfd, parentDsId, parentDs,
                            parentShadowDsId, parentSdsr);
          updateIterationParentVars(parentDsId, newDsId,
                                    parentDs, newDs,
                                    parentSdsr, newSdsr);
          invalidDuelStateProcessResult = true;
          break;

        } else if ((!std::get<0>(aplTuple))) {
          //encountered a process iteration that did not succeed
          // (that should have succeeded)
          const auto& aplTupleMsg = std::get<1>(std::get<2>(aplTuple))[0];
          clog("e", "Advancing duel state ", newDsId, " resulted in an"
               " unexpected invalid msg (", aplTupleMsg, ").");
          invalidDuelStateProcessResult = true;
          break;
        }

        //duplicated state reached valid state after being processed
        // with choice
        StateNode newDsIdNode(newDsId, parentDsId);

        //unlike other implementations, no getting choice string
        // for node here

        pfd.pr.stateNodeResults
          .emplace_back(newDsId, std::move(newDsIdNode));
        pfd.pr.depthMapResults
          .emplace_back(currDepth, newDsId);
        pfd.pr.maxDepthReached = currDepth;

        //get children only if depth limit not yet reached
        if (currDepth >= depthLimit) {
          updateParentsToPr(pfd, parentDsId, parentDs,
                            parentShadowDsId, parentSdsr);
          updateIterationParentVars(parentDsId, newDsId,
                                    parentDs, newDs,
                                    parentSdsr, newSdsr);
          break;
        }

        //get potential children
        auto choiceCombinations =
          getPossibleChoiceCombinations(std::get<2>(aplTuple));

        //check if any children found
        if (choiceCombinations.size() == 0) {
          updateParentsToPr(pfd, parentDsId, parentDs,
                            parentShadowDsId, parentSdsr);
          updateIterationParentVars(parentDsId, newDsId,
                                    parentDs, newDs,
                                    parentSdsr, newSdsr);
          break;
        }

        //get future iterations using children, except for last
        // choice combination which will be immediately used for next
        // iteration of loop
        auto lastChoiceCombination = std::move(choiceCombinations.back());
        choiceCombinations.pop_back();

        // IterationGroup ig(newDsId, std::get<2>(aplTuple),
        //                std::move(choiceCombinations), currDepth);
        // pfd.pr.igResults.emplace_back(newDsId,
        //                            std::get<2>(aplTuple),
        //                            std::move(choiceCombinations),
        //                            currDepth
        //                            );
        pfd.pr.igResults.emplace_back(IterationGroup {
            newDsId, std::move(choiceCombinations),
            currDepth, std::get<2>(aplTuple)
          });

        updateParentsToPr(pfd, parentDsId, parentDs,
                          parentShadowDsId, parentSdsr);
        updateIterationParentVars(parentDsId, newDsId,
                                  parentDs, newDs,
                                  parentSdsr, newSdsr,
                                  parentCc, lastChoiceCombination,
                                  parentChoices, std::get<2>(aplTuple));
      }

      if (ddd::isDuelStateActive(parentDs)) {

        if (parentDs.pDuel != parentPDuelReuse) {
          pfd.pr.reusableDuels.emplace_back(parentPDuelReuse);
        }

        if ((parentDs.pDuel) &&
            (ddd::isDuelDuplicatable(parentDs.pDuel)))
          pfd.pr.reusableDuels.emplace_back(parentDs.pDuel);
        else
          pfd.pr.endDuelResults.emplace_back(parentDs.pDuel);

        //manually deactivate
        parentDs.pDuel = 0;
        parentDs.active = false;

      } else {
        clog("w", "Ended up somewhere where we shouldn't...?");
        //if not active, wtf happened to the pDuel?
        //...memory leak...?
      }

      //once more to account for last iteration of loop
      // (unless did not result in a valid state)
      if (!invalidDuelStateProcessResult) {
        updateParentsToPr(pfd, parentDsId, parentDs,
                          parentShadowDsId, parentSdsr);
      }
    }
  };

  const std::unordered_set<int> endMsgs = {
      5  //MSG_WIN
    ,40  //MSG_NEW_PHASE
    ,41  //MSG_NEW_TURN
  };

  //get actual choice filter based on filter passed from function
  const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
  const auto choiceFilter = ddd::getChoiceFilter(ds, inFilter);

  //populate stack with initial values
  DfsParallelizer dfsp = DfsParallelizer(dsId, depthLimit,
                                         maxDepthResultsLimit,
                                         endMsgs, choiceFilter,
                                         threadCount, showMessages);
  //start of brute forcing functionality
  auto startTime = std::chrono::system_clock::now();

  dfsp.iterate();

  auto endTime = std::chrono::system_clock::now();
  auto takenTime = std::chrono::duration_cast
    <std::chrono::seconds>(endTime - startTime);


  //lambda declaration to print choices to reach duel state
  // from original duel state
  auto getChoiceChain = [&](const unsigned long long inDsId) {
    std::vector<std::string> v;
    //need to if not storing the choice string, store the choice
    // to get to child from parent somewhere...
    /*
    for (unsigned long long cDsId = inDsId;
         cDsId != 0; cDsId = dfsp.resultsMap.at(cDsId).parent) {
      v.push_back(dfsp.resultsMap.at(cDsId).choice);
    }
    std::reverse(v.begin(), v.end());
    */
    return "[dsid " + std::to_string(inDsId) + "]:  \t" +
      joinString(v, " => ");
  };

  //erase only the original duel state id (to adjust tallys)
  dfsp.resultsMap.erase(dsId);

  //get duel states at deepest depth reached
  auto dmer = dfsp.depthMap.equal_range(dfsp.maxDepthReached);
  std::vector<std::string> deepestDuelStatesVec;
  std::vector<std::string> deepestDuelStatesChoicesVec;
  unsigned long deepestDuelStatesCount = 0;

  for (auto i = dmer.first; i != dmer.second; ++i) {
    ++deepestDuelStatesCount;

    //store results if under limit (or no limit)
    if ((!showResultsLimit) || //no limit
        (deepestDuelStatesCount <= showResultsLimit))
      deepestDuelStatesChoicesVec.push_back(getChoiceChain(i->second));
  }

  //print results
  for (const auto& res: deepestDuelStatesChoicesVec)
    clog("d", "    ", res);
  /*
  if ((showResultsLimit) &&
      (showResultsLimit < deepestDuelStatesCount)) {
    clog("d", "    \t ......");
    clog("d", "    \t(omitted  ",
         deepestDuelStatesCount - showResultsLimit, " more results ("
         "limited to showing ", showResultsLimit, " results))");
  }
  */

  //print deepest depth reached for user and the duel states
  // that reached that depth
  clog("d", "Found total of ", dfsp.resultsMap.size(),
       " duel states with ", deepestDuelStatesCount,
       " duel states at deepest depth of ", dfsp.maxDepthReached, ".");
  clog("d", "    \t(", (dfsp.resultsMap.size() /
                        ((takenTime.count()) ? takenTime.count() : 1)),
       " duel states per second)");

  if (dfsp.endSearchEarly) {
    clog("d", "    \t(search ended early because result limit of ",
         maxDepthResultsLimit, " for the deepest depth was reached)");

    //deactivate any possibly active duel states
    for (const auto &dmp: dfsp.depthMap)
      if (dmp.second != dsId)
        deactivate_duel_state(dmp.second);
  }
  clog("d", "");

  //update contents of choices map with choices found from
  // brute forcing
  //need to if not storing the choice string, store the choice
  // to get to child from parent somewhere...
  /*
  for (const auto &fc: dfsp.resultsMap)
    msgChoicesMap.insert(std::move(fc));
  */

  if (shouldDeactivate)
    deactivate_duel_state(dsId);
}


void bruteForceStateChoicesDfsOmp(const unsigned long long dsId, const ChoiceTuple& lastValidChoicesTpl, const int depthLimit, const unsigned long long maxDepthResultsLimit, const unsigned long long showResultsLimit, std::unordered_map<unsigned long long, ChoiceTuple>& msgChoicesMap, const std::unordered_set<int>& inFilter, const bool showMessages, const bool ignoreWarnings) {
  if (!ignoreWarnings) {
    if (!confirmYN("Really brute force all states (to depth " +
                   std::to_string(depthLimit) +
                   ") with current choices?", false)) {
      clog("i", "Not doing it.");
      return;
    }
  }
  bool shouldDeactivate = !is_duel_state_active(dsId);
  reactivate_duel_state(dsId);

  //Node struct defintion and some other var declarations needed
  // to either brute force or otherwise keep track of results
  struct Node {
    unsigned long long parent;
    std::vector<unsigned long long> children;
    int depth;
    std::string choice; //from parent to reach this node
  };
  //Class to organize variables and functions as well as limit scopes
  class DfsParallelizer {
    std::unordered_map<unsigned long long, ChoiceTuple> foundChoicesMap;
    std::unordered_map<unsigned long long, Node> choicesTree;
    std::multimap<int, unsigned long long> depthMap;
    std::vector<
      std::tuple<unsigned long long, std::vector<int>, int>> dfsStack;
    std::vector<
      std::tuple<unsigned long long, std::vector<int>, int>> pushCache;
    std::unordered_multimap<
      unsigned long long,
      std::tuple<unsigned long long, DuelState>> dsIdReserve;
    //std::unordered_map<unsigned long long, int> dsIdRefCounts;
    std::vector<unsigned long long> deactivateCache;

    int maxDepthReached;
    bool endSearchEarly;
    unsigned long maxDepthResultsLimit;
    const int depthLimit;
    const std::unordered_set<int> endMsgs;
    const std::unordered_set<int> choiceFilter;
    const bool showMessages;

    //omp_lock_t foundChoicesMapLock;
    //omp_lock_t choicesTreeLock;
    //omp_lock_t depthMapLock;
    //omp_lock_t dfsStackLock;
    omp_lock_t pushCacheLock;
    omp_lock_t dsIdReserveLock;
    //omp_lock_t dsIdRefCountsLock;
    omp_lock_t deactivateCacheLock;
    omp_lock_t dsLock;
    //omp_lock_t sdsLock;
    omp_lock_t everythingLock;

    /*
    std::mutex foundChoicesMapMutex;
    std::mutex choicesTreeMutex;
    std::mutex depthMapMutex;
    std::mutex dfsStackMutex;
    std::mutex pushCacheMutex;
    std::mutex dsIdReserveMutex;
    std::mutex dsIdRefCountsMutex;
    */
    //std::mutex everythingMutex;

  public:
    DfsParallelizer(const unsigned long long dsId, const int inDepthLimit, const unsigned long inMaxDepthResultsLimit, const std::unordered_set<int>& inEndMsgs, const std::unordered_set<int>& inChoiceFilter, const bool inShowMessages)
      : depthLimit(inDepthLimit), endMsgs(inEndMsgs),
        choiceFilter(inChoiceFilter), showMessages(inShowMessages)
    {
      //initialize
      maxDepthReached = 0;
      endSearchEarly = false;
      maxDepthResultsLimit = inMaxDepthResultsLimit;

      const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
      foundChoicesMap[dsId] =
        ddd::getChoicesFromDuelState(ds, true, choiceFilter);
      const auto possibleChoices =
        getPossibleChoiceCombinations(foundChoicesMap.at(dsId));
      //deactivateCache.push_back(dsId);
      for (int i = 0; i < possibleChoices.size(); ++i)
        dfsStack.push_back
          (std::tuple<unsigned long long, std::vector<int>, int>
           (dsId, std::move(possibleChoices.at(i)), i + 1));

      Node dsIdNode;
      dsIdNode.parent = 0;
      dsIdNode.depth = 0;
      dsIdNode.choice = "(start)";
      choicesTree[dsId] = std::move(dsIdNode);

      //omp_init_lock(&foundChoicesMapLock);
      //omp_init_lock(&choicesTreeLock);
      //omp_init_lock(&depthMapLock);
      //omp_init_lock(&dfsStackLock);
      omp_init_lock(&pushCacheLock);
      omp_init_lock(&dsIdReserveLock);
      //omp_init_lock(&dsIdRefCountsLock);
      omp_init_lock(&deactivateCacheLock);
      omp_init_lock(&dsLock);
      //omp_init_lock(&sdsLock);
      omp_init_lock(&everythingLock);

    }
    ~DfsParallelizer() {
      //omp_destroy_lock(&foundChoicesMapLock);
      //omp_destroy_lock(&choicesTreeLock);
      //omp_destroy_lock(&depthMapLock);
      //omp_destroy_lock(&dfsStackLock);
      omp_destroy_lock(&pushCacheLock);
      omp_destroy_lock(&dsIdReserveLock);
      //omp_destroy_lock(&dsIdRefCountsLock);
      omp_destroy_lock(&deactivateCacheLock);
      omp_destroy_lock(&dsLock);
      //omp_destroy_lock(&sdsLock);
      omp_destroy_lock(&everythingLock);

    }
    int getMaxDepthReached() {
      return maxDepthReached;
    }
    /*
    std::mutex& getDfssMutex() {
      return dfsStackMutex;
    }
    */
    std::unordered_map<unsigned long long, ChoiceTuple>& getFoundChoicesMap() {
      return foundChoicesMap;
    }
    const std::unordered_map<unsigned long long, Node>& getChoicesTree() {
      return choicesTree;
    }
    const std::multimap<int, unsigned long long>& getDepthMap() {
      return depthMap;
    }
    bool wasSearchEndedEarly() {
      return endSearchEarly;
    }
    void iterate() {

      while ((!dfsStack.empty()) || (!pushCache.empty())) {

        //check if results limit reached
        if (maxDepthResultsLimit) {
          if (maxDepthReached == depthLimit) {
            auto er = depthMap.equal_range(maxDepthReached);
            if (std::distance(er.first, er.second) >= maxDepthResultsLimit) {
              //results limit reached

              endSearchEarly = true;
              break; //exit from loop
            }
          }
        }

        //add from push cache to dfsStack
        if (!pushCache.empty()) {
          dfsStack.reserve(dfsStack.size() + pushCache.size());
          std::move(std::begin(pushCache), std::end(pushCache),
                    std::back_inserter(dfsStack));
          pushCache.clear();
        }

        //track size for later
        const auto prevChoicesMapSize = foundChoicesMap.size() - 1;

        //create local variables that can be copied/passed by
        // reference to loop body
        const int forSize = std::get<2>(dfsStack.back());
        const unsigned long long currParent = std::get<0>(dfsStack.back());
        const int currDepth = choicesTree.at(currParent).depth + 1;
        const auto parentChoiceTpl = foundChoicesMap.at(currParent);

        auto i = dfsStack.rbegin();

        //single serial iteration for element at very back of the stack
        {
          const DuelState& parentDs = getDDD_GS()
            .dddapi.duelStates.at(currParent);
          const auto& sds = getDDD_GS().dddapi
            .shadowDuelStates.at(parentDs.shadowDsId);
          const intptr_t pShadowDuel = sds.pDuel;
          const ShadowDuelStateResponses& sdsr =
            sds.responsesMap.at(currParent);

          parallelForBody(currParent, std::get<1>(*i),
                          parentDs, parentChoiceTpl, currDepth,
                          true, pShadowDuel, sdsr);
        }

        //parallel for
        //  (limited to choices of common parent at back of stack)
        if (forSize > 1) {
          const DuelState parentDs = getDDD_GS()
            .dddapi.duelStates.at(currParent);
          const auto& sds = getDDD_GS().dddapi  //not passed so not copied
            .shadowDuelStates.at(parentDs.shadowDsId);
          const intptr_t pShadowDuel = sds.pDuel;
          const ShadowDuelStateResponses sdsr =
            sds.responsesMap.at(currParent);

          //#pragma omp parallel for
          #pragma omp parallel for default(none) firstprivate(forSize, currParent, currDepth, parentChoiceTpl, parentDs, pShadowDuel, sdsr)
          for (i = dfsStack.rbegin() + 1;
               i != dfsStack.rbegin() + forSize; ++i)
            parallelForBody(currParent, std::get<1>(*i),
                            parentDs, parentChoiceTpl, currDepth,
                            false, pShadowDuel, sdsr);
        }

        const auto currChoicesMapSize = foundChoicesMap.size() - 1;
        if ((currChoicesMapSize > 1) &&
            (((currChoicesMapSize / 100) >
              (prevChoicesMapSize / 100))))
          clog("d", "\tDuel states found so far: ",
               (foundChoicesMap.size() - 1));

        //deactivate any states that are no longer needed
        for (const auto &dsId: deactivateCache) {
          deactivate_duel_state(dsId);
        }
        deactivateCache.clear();

        //remove any reserve states for current parent
        // (should be all of them)
        if (dsIdReserve.find(currParent) != dsIdReserve.end()) {
          auto ar = dsIdReserve.equal_range(currParent);
          for (auto i = ar.first; i != ar.second; ++i) {
            end_duel(std::get<1>(i->second).pDuel);
          }
          dsIdReserve.erase(currParent);
        }

        //remove last elements of common parent at back of stack,
        // including the last parent
        //dfsStack.erase(dfsStack.rbegin(), dfsStack.rbegin() + forSize);
        if ((dfsStack.size() - forSize) < 1)
          dfsStack.clear();
        else
          dfsStack.resize(dfsStack.size() - forSize);

      }
    }
    void parallelForBody(const unsigned long long parentDsId, const std::vector<int> parentCc, const DuelState& parentDs, const ChoiceTuple& parentChoices, const int currDepth, const bool canAssume, const intptr_t pShadowDuel, ShadowDuelStateResponses sdsr) {

      if (currDepth > depthLimit) {
        //already at depth limit; should never reach here...?
        return;
      }

      //get new duel state to duplicate
      std::optional<std::tuple<unsigned long long, DuelState>> oNewDsIdTpl;
      std::optional<DuelState> oUpdatedParentDs;

      bool shouldDuplicateNew = false;
      if (canAssume) {
        oUpdatedParentDs = parentDs; //copy
        oNewDsIdTpl = ddd::assumeDuelState(*oUpdatedParentDs);

      } else {
        omp_set_lock(&dsIdReserveLock);
        auto dsIdReserveItr = dsIdReserve.find(parentDsId);
        if (dsIdReserveItr != dsIdReserve.end()) {
          oNewDsIdTpl = std::move(dsIdReserveItr->second);
          dsIdReserve.erase(dsIdReserveItr);

        } else {
          shouldDuplicateNew = true;
        }
        omp_unset_lock(&dsIdReserveLock);
      }
      if (shouldDuplicateNew) {
        oNewDsIdTpl =
          ddd::duplicateDuelStateFromShadow(pShadowDuel,
                                            parentDs, sdsr);
      }

      const unsigned long long newDsId = std::get<0>(*oNewDsIdTpl);
      auto& newDs = std::get<1>(*oNewDsIdTpl);

      if (newDsId == 0) {
        clog("e", "Error duplicating duel state ", parentDsId,
             " to brute force (dfs-omp; in parallelForBody).");
        return;
      }

      //reactivate duel state (was likely not activated if not assumed)
      if (!ddd::isDuelStateActive(newDs)) {
        if (!ddd::reactivateDuelState(newDsId, newDs,
                                      pShadowDuel, sdsr)) {
          clog("e", "Error reactivating deactivated duel state ",
               parentDsId, " to brute force (dfs-omp; in"
               " parallelForBody).");
          return;
        }
      }

      //determine if combination of choices can be expected to fail
      // (if so, checkFirstProcess will be set to true and if the
      //  1st process iteration performed on duel state (in the
      //  autoProcessLoop) did not succeed, will not consider that
      //  to be an error but will instead silently remove it)
      bool checkFirstProcess = false;
      if (parentCc.size() > 0) {
        //combination of at least 1 choice
        // (set response to duplicated duel state)

        setResponse(newDs, parentChoices, parentCc);
      } else {
        //combination of 0 choices
        // (do not set response and set checkFirstProcess to true)
        checkFirstProcess = true;
      }

      //check parent message because high probability of message 23
      // creating invalid combination of choices
      // (so set checkFirstProcess to true)
      if (std::get<1>(parentChoices)[0] == 23)
        checkFirstProcess = true;

      //process duplicated duel state and get result
      const auto aplTuple = autoProcessLoop(newDs, sdsr, endMsgs,
                                            choiceFilter, showMessages,
                                            checkFirstProcess);

      //handle based on result from auto process loop
      // (unlike single threaded implementation, returning early here
      //  is not considered an error and does not stop other
      //  threads that may be running)
      if ((!std::get<0>(aplTuple)) && (std::get<1>(aplTuple))) {
        //1st process iteration on duplicate duel state did not
        // succeed but not considered to be an error
        // (as it was expected it might not succeed)

        //update parent duel state if duel state was assumed
        // from parent
        omp_set_lock(&dsLock);
        getDDD_GS().dddapi.duelStates.at(parentDsId) =
            std::move(*oUpdatedParentDs);
        omp_unset_lock(&dsLock);

        //save duel state where it may be reused in place of
        // duplicating the same parent
        omp_set_lock(&dsIdReserveLock);
        dsIdReserve.emplace(parentDsId, std::move(*oNewDsIdTpl));
        omp_unset_lock(&dsIdReserveLock);
        return;

      } else if ((!std::get<0>(aplTuple))) {
        //encountered a process iteration that did not succeed
        // (that should have succeeded)
        const auto& aplTupleMsg = std::get<1>(std::get<2>(aplTuple))[0];
        clog("e", "Advancing duel state ", newDsId, " resulted in an"
             " unexpected invalid msg (", aplTupleMsg, ").");

        omp_set_lock(&dsLock);
        //should use singleton mutex instead...?
        getDDD_GS().dddapi.duelStates.at(parentDsId) =
            std::move(*oUpdatedParentDs);
        omp_unset_lock(&dsLock);

        return; //exit prematurely
      }

      //duplicated state reached valid state after being processed
      // with choice
      Node newDsIdNode;
      newDsIdNode.parent = parentDsId;
      newDsIdNode.depth = currDepth;

      //get choice string for node
      if (parentCc.size() == 0) {
        //successfully reached duel state after processing without
        // setting any choices

        if (showMessages)
          clog("d", "  Duplicated to ", newDsId, " without using"
               " choices '(continue processing)'.");

        //build tuple for duel state and push to result
        newDsIdNode.choice = "(continue processing)";

      } else if (parentCc.size() == 1) {
        //successfully reached duel state after processing
        // with 1 choice set

        const auto& choiceLabels = std::get<3>(parentChoices);

        if (showMessages)
          clog("d", "  Duplicated to ", newDsId, " using choice (",
               parentCc[0], ") '", choiceLabels[parentCc[0]], "'.");

        newDsIdNode.choice = choiceLabels[parentCc[0]];

      } else {
        //successfully reached duel state after processing
        // with multiple choices set

        const auto& choiceLabels = std::get<3>(parentChoices);

        //multiple choices to string vector to be joined later
        std::vector<std::string> v;
        for (const auto &i: parentCc)
          v.push_back(std::to_string(i));

        if (showMessages) {
          clog("d", "  Duplicated to ", newDsId, " using choices (",
               joinString(v, ", "), "):");
          for (const auto &i: parentCc)
            clog("d", "     ", choiceLabels[i]);
        }

        newDsIdNode.choice = joinString(v, ", ");
      }

      //merge everything together (for singleton variables only)
      //#pramga omp critical
      {
        omp_set_lock(&dsLock);

        getDDD_GS().dddapi.duelStates.emplace(newDsId, std::move(newDs));
        getDDD_GS().dddapi.shadowDuelStates.at(parentDs.shadowDsId)
          .responsesMap.emplace(newDsId, std::move(sdsr));
        if (canAssume)
          getDDD_GS().dddapi.duelStates.at(parentDsId) =
            std::move(*oUpdatedParentDs);

        omp_unset_lock(&dsLock);
      }
      //merge everything else together
      omp_set_lock(&everythingLock);

      if (maxDepthReached < currDepth)
        maxDepthReached = currDepth;

      foundChoicesMap.emplace(newDsId, std::get<2>(aplTuple));
      choicesTree.emplace(newDsId, std::move(newDsIdNode));
      choicesTree.at(parentDsId).children.push_back(newDsId);
      depthMap.emplace(currDepth, newDsId);

      //find children if depth limit not yet reached
      if (currDepth >= depthLimit) {
        deactivateCache.push_back(newDsId);
        omp_unset_lock(&everythingLock);
        return;
      }

      //get potential children
      const auto choiceCombinations =
        getPossibleChoiceCombinations(std::move(std::get<2>(aplTuple)));

      //check if any children found
      if (choiceCombinations.size() == 0) {
        //deactivate_duel_state(newDsId);
        deactivateCache.push_back(newDsId);
        omp_unset_lock(&everythingLock);
        return;
      }

      //push children to cache to be added stack later
      std::vector< //temp vector to be moved...
        std::tuple<unsigned long long, std::vector<int>, int>> ccv;
      ccv.reserve(choiceCombinations.size());

      //build vector to be inserted into cache
      for (int i = 0; i < choiceCombinations.size(); ++i)
        ccv.push_back
          (std::tuple<unsigned long long, std::vector<int>, int>
           (newDsId, std::move(choiceCombinations.at(i)), i + 1));

      pushCache.reserve(pushCache.size() + ccv.size());
      std::move(std::begin(ccv), std::end(ccv),
                std::back_inserter(pushCache));
      omp_unset_lock(&everythingLock);
    }
  };

  const std::unordered_set<int> endMsgs = {
      5  //MSG_WIN
    ,40  //MSG_NEW_PHASE
    ,41  //MSG_NEW_TURN
  };

  //get actual choice filter based on filter passed from function
  const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
  const auto choiceFilter = ddd::getChoiceFilter(ds, inFilter);

  //populate stack with initial values
  DfsParallelizer dfsp = DfsParallelizer(dsId, depthLimit,
                                         maxDepthResultsLimit, endMsgs,
                                         choiceFilter, showMessages);

  clog("d", "Brute forcing to depth of ", depthLimit,
       " (omp_get_max_threads: ", omp_get_max_threads(), ")");

  //start of brute forcing functionality
  auto startTime = std::chrono::system_clock::now();

  dfsp.iterate();

  auto endTime = std::chrono::system_clock::now();
  auto takenTime = std::chrono::duration_cast
    <std::chrono::seconds>(endTime - startTime);


  auto& foundChoicesMap = dfsp.getFoundChoicesMap();
  const auto& choicesTree = dfsp.getChoicesTree();
  const auto& depthMap = dfsp.getDepthMap();
  const auto maxDepthReached = dfsp.getMaxDepthReached();
  bool endSearchEarly = dfsp.wasSearchEndedEarly();

  //lambda declaration to print choices to reach duel state
  // from original duel state
  auto getChoiceChain = [&](const unsigned long long inDsId) {
    std::vector<std::string> v;
    for (unsigned long long cDsId = inDsId;
         cDsId != 0; cDsId = choicesTree.at(cDsId).parent) {
      v.push_back(choicesTree.at(cDsId).choice);
    }
    std::reverse(v.begin(), v.end());
    return "[dsid " + std::to_string(inDsId) + "]:  \t" +
      joinString(v, " => ");
  };

  //erase only the original duel state id (to adjust tallys)
  foundChoicesMap.erase(dsId);

  //get duel states at deepest depth reached
  auto dmer = depthMap.equal_range(maxDepthReached);
  std::vector<std::string> deepestDuelStatesVec;
  std::vector<std::string> deepestDuelStatesChoicesVec;
  unsigned long deepestDuelStatesCount = 0;

  for (auto i = dmer.first; i != dmer.second; ++i) {
    ++deepestDuelStatesCount;

    //store results if under limit (or no limit)
    if ((!showResultsLimit) || //no limit
        (deepestDuelStatesCount <= showResultsLimit))
      deepestDuelStatesChoicesVec.push_back(getChoiceChain(i->second));
  }

  //print results
  for (const auto& res: deepestDuelStatesChoicesVec)
    clog("d", "    ", res);

  if ((showResultsLimit) &&
      (showResultsLimit < deepestDuelStatesCount)) {
    clog("d", "    \t ......");
    clog("d", "    \t(omitted  ",
         deepestDuelStatesCount - showResultsLimit, " more results ("
         "limited to showing ", showResultsLimit, " results))");
  }

  //print deepest depth reached for user and the duel states
  // that reached that depth
  clog("d", "Found total of ", foundChoicesMap.size(),
       " duel states with ", deepestDuelStatesCount,
       " duel states at deepest depth of ", maxDepthReached, ".");
  clog("d", "    \t(", (foundChoicesMap.size() /
                        ((takenTime.count()) ? takenTime.count() : 1)),
       " duel states per second)");

  if (endSearchEarly) {
    clog("d", "    \t(search ended early because result limit of ",
         maxDepthResultsLimit, " for the deepest depth was reached)");

    //deactivate any possibly active duel states
    for (const auto &dmp: depthMap)
      if (dmp.second != dsId)
        deactivate_duel_state(dmp.second);
  }
  clog("d", "");

  //update contents of choices map with choices found from
  // brute forcing
  for (const auto &fc: foundChoicesMap)
    msgChoicesMap.insert(std::move(fc));

  if (shouldDeactivate)
    deactivate_duel_state(dsId);
}


//compute all possible combinations of choices that can be
// selected based on the message, choices tuple and filter
// (with minor modifications for this function's specific use case)
std::vector<std::vector<int>> getPossibleChoiceCombinations(const ChoiceTuple& choices) {
  std::vector<std::vector<int>> possibleChoices;
  int minChoices = 1;
  int maxChoices = 1;
  const auto& lastValidMsg = std::get<1>(choices)[0];
  int totalChoices = std::get<0>(choices).size(); //new copy
  std::vector<int> fil = std::get<4>(choices); //new copy

  //specific handling based on certain messages
  if (lastValidMsg == 15) { //MSG_SELECT_CARD
    if (std::get<1>(choices).size() < 4) {
      clog("e", "Malformed extras received for msg 15: ",
           std::get<1>(choices)[0]);
      clog("d", std::get<1>(choices).size());
      return possibleChoices;
    }
    minChoices = std::get<1>(choices)[1];
    maxChoices = std::get<1>(choices)[2];

  } else if (lastValidMsg == 23) { //MSG_SELECT_SUM
    if (std::get<1>(choices).size() < 6) {
      clog("e", "Malformed extras received for msg 23: ",
           std::get<1>(choices)[0]);
      return possibleChoices;
    }
    minChoices = std::get<1>(choices)[1];
    maxChoices = std::get<1>(choices)[2];
    const int mustSelectSize = std::get<1>(choices)[5];

    //adjust number of total choices and filter by offsetting
    // by val of mustSelectSize
    totalChoices -= mustSelectSize;
    fil = std::vector<int>(fil.begin() + mustSelectSize, fil.end());

    //check if no limit (specified in extras) and if not,
    // cap at number of total choices
    if (std::get<1>(choices)[3] == 0)
      maxChoices = totalChoices;
  }

  //iterate based on possible number of choices selectable
  // (will typically only be 1)
  for (int currNumChoices = minChoices;
       currNumChoices <= maxChoices; currNumChoices++) {
    if (currNumChoices > totalChoices)
      break;

    std::vector<int> bufV;

    //actually call the lambda (for currNumChoices)
    gpccInner(currNumChoices, totalChoices, fil, bufV, possibleChoices);
  }

  //push back 0 choices for specific messages
  if ((lastValidMsg == 15) ||  //MSG_SELECT_CARD
      (lastValidMsg == 26) ||  //MSG_SELECT_UNSELECT_CARD
      (lastValidMsg == 16) ||  //MSG_SELECT_CHAIN
      (lastValidMsg == 20))    //MSG_SELECT_TRIBUTE
    possibleChoices.push_back(std::vector<int>());

  return possibleChoices;
}

//recurrsive helper function for getPossibleChoiceCombinations to
// push next valid number to buffer and then recurrsively call itself
void gpccInner(const int currNumChoices, const int totalChoices, const std::vector<int>& fil, std::vector<int>& bufV, std::vector<std::vector<int>>& possibleChoices) {

  const int depth = bufV.size(); //current depth of buffer

  //if max buffer length reached, push result to
  // possible choices and then return
  if (depth == currNumChoices) {
    if (depth > 0) //ignore combinations with 0 choices
      possibleChoices.push_back(bufV);
    return;
  }

  //get starting value for current depth
  // (equal to the current depth at depth 0 (always 0?);
  //  for other depths, equal to the value in the
  //  previous depth + 1 or the depth, whichever is more)
  const int startRange = (depth > 0)
    ? std::max(depth, bufV[depth - 1] + 1)
    : depth;
  //get ending value for current depth
  // (equal to the total number of choices, negatively offset
  //  by the number of choices for combination, positively
  //  offset by the current depth)
  const int endRange = totalChoices - currNumChoices + depth;

  //iterate between range of starting and end values
  for (int i = startRange; i <= endRange; ++i) {
    //ignore value if value at index in filter is equal to 0
    if (fil[i] == 0)
      continue;

    //push current value, recurrsively call lambda, then pop
    bufV.push_back(i);
    gpccInner(currNumChoices, totalChoices, fil,
              bufV, possibleChoices);
    bufV.pop_back();
  }
}

void clearDuelTest(unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("e", "Unable to find duel state ", dsId, " in duel states.");
    return;
  }

  if (!confirmYN("Really clear this duel?")) {
    return;
  }

  if (!ddd::isDuelDuplicatable(getDDD_GS().dddapi.duelStates.at(dsId).pDuel)) {
    clog("w", "Duel may not be safe to clear (potentially stray"
         " variables in stack or in the middle of coroutine)");
    clog("w", "  (clearing may lead to memory leaks...?)");

    if (!confirmYN("Really clear this duel anyway?")) {
      return;
    }
  }
  duel* pD = (duel*) getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  pD->clear_buffer();
  auto stackSize = lua_gettop(pD->lua->lua_state);
  lua_pop(pD->lua->lua_state, stackSize);
  pD->clear();
}

//helper function for bfs and dfs to continuously process a
// duel state until one of possible conditions reached
std::tuple<bool, bool, ChoiceTuple> autoProcessLoop(const unsigned long long apDsId, const std::unordered_set<int>& endMsgs, const std::unordered_set<int>& choiceFilter, const bool printHintStrings, const bool testFirstProcess) {
  auto& ds = getDDD_GS().dddapi.duelStates.at(apDsId);
  auto& sds = getDDD_GS().dddapi.shadowDuelStates.at(ds.shadowDsId);
  auto& sdsr = sds.responsesMap.at(apDsId);

  return autoProcessLoop(ds, sdsr, endMsgs, choiceFilter,
                         printHintStrings, testFirstProcess);
}
std::tuple<bool, bool, ChoiceTuple> autoProcessLoop(DuelState& ds, ShadowDuelStateResponses& sdsr, const std::unordered_set<int>& endMsgs, const std::unordered_set<int>& choiceFilter, const bool printHintStrings, const bool testFirstProcess) {
  return autoProcessLoop(ds, sdsr, endMsgs, choiceFilter,
                         printHintStrings, testFirstProcess, true);
}
std::tuple<bool, bool, ChoiceTuple> autoProcessLoop(DuelState& ds, ShadowDuelStateResponses& sdsr, const std::unordered_set<int>& endMsgs, const std::unordered_set<int>& choiceFilter, const bool printHintStrings, const bool testFirstProcess, const bool getChoiceStrings) {

  bool apStatus = true; //validity; if true, should add pair to map
  bool firstProcessCheck = testFirstProcess;

  std::optional<ChoiceTuple> oc;

  //continuously process duel state
  while (true) {
    //process duel state
    {
      ddd::processDuelState(ds, sdsr);
    }

    //get choices and message
    //const auto &ds = getDDD_GS().dddapi.duelStates.at(apDsId);
    oc = ddd::getChoicesFromDuelState(ds, getChoiceStrings, choiceFilter);
    int m = std::get<1>(*oc)[0];

    //print hintStrings if enabled
    if ((printHintStrings) && (std::get<2>(*oc).size() > 0))
      for (auto i = std::get<2>(*oc).begin();
           i != std::get<2>(*oc).end(); ++i)
        if (m == 1)
          clog("w", "    ", *i);
        else
          if (i == std::get<2>(*oc).begin())
            clog("i", "    ", *i);
          else
            clog("l", "      ", *i);

    //no longer continue to process if one of the following
    // conditions have been met
    if (m == 1) {
      //encountered an error
      apStatus = false;

      // if (!firstProcessCheck)
      //        //process failed outside of 1st process iteration or
      //        // failed during 1st iteration when doing so was not
      //        // potentially expected
      //        vStatus = false;
      break;

    } else if (std::get<0>(*oc).size() > 0) {
      //encountered new choices to select
      break;

    } else if (endMsgs.find(m) != endMsgs.end()) {
      //encountered a msg to stop at
      break;

    }

    //if checking for 1st process iteration, signify current
    // iteration as no longer the 1st iteration
    if (firstProcessCheck)
      firstProcessCheck = false;
  }

  //only add choice to choices map if duel state is considered valid
  //if (apStatus)
  //  foundChoicesMap[apDsId] = c;

  if (!oc.has_value())
    return std::make_tuple(apStatus, firstProcessCheck, ChoiceTuple());

  return std::make_tuple(apStatus, firstProcessCheck, std::move(*oc));
}
