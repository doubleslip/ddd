/*
 * dddsingleton.hpp
 */
//although singleton related types are declared here, the means to
// use/modify those types are implemented in other files


#define PY_SSIZE_T_CLEAN
#define Py_LIMITED_API 3
#include <Python.h>


#ifndef __DDDSINGLETON_HPP
#define __DDDSINGLETON_HPP

#include <vector>
#include <string>
#include <unordered_set>
#include <set>
#include <unordered_map>
#include <filesystem>
#include <deque>
#include <mutex>
#include <atomic>

#include "card.h" //for card_data
#include "field.h" //for returns


/*
 * structs/classes primarily modified in dddutil
 */
struct DeckCards {
  std::vector<unsigned int> main;
  std::vector<unsigned int> extra;
  std::vector<unsigned int> side;
};

struct card_texts {
  std::string name;
  std::string cardText;
  std::string cardStrs[16];
};

struct ConfigDebug {
  std::set<std::string> printCardAttributes;
  std::set<std::string> printGroupAttributes;
  std::set<std::string> printEffectAttributes;
  bool printGamestateChanges;
};

struct ConfigLog {
  std::set<std::string> highlights;
  std::set<std::string> filters; //ignore
  bool highlightOverridesFilter;

  bool forcePrintf;
  bool logToFile;
  std::filesystem::path logFile;
  bool cacheLogs;
  std::vector<std::string> logCache;
};


/*
 * structs/classes primarily modified in dddapi*
 */
struct DDDReturn {
  union {
    int8 bvalue[SIZE_RETURN_VALUE];
    byte bvaluebytes[SIZE_RETURN_VALUE];
    int32 ivalue;
  } u;
  bool iactive = false;
};

struct DuelStateResponse {
  DDDReturn response;
  bool responseSet = false;
};

struct DuelState {
  intptr_t pDuel = 0;
  unsigned long long shadowDsId = 0;
  DuelStateResponse lastResponse;
  bool active = false;
};

struct ShadowDuelStateResponses {
  std::vector<DuelStateResponse> responses;
  int maxExpectedResponses = 1;
};

struct ShadowDuelState {
  intptr_t pDuel = 0;
  std::unordered_map<unsigned long long, ShadowDuelStateResponses> responsesMap;
};


/*
 * singleton immediate member structs
 */
struct CardDbs {
  std::unordered_map<unsigned int, card_data> cardDb; //card_data from cards.h
  std::unordered_map<unsigned int, card_texts> cardTextsDb; //card_texts is custom struct
};

struct Config {
  bool lastInitSuccess;

  std::filesystem::path confPath;
  std::filesystem::path cardsCdbPath;
  std::filesystem::path player0DeckPath;
  std::filesystem::path player1DeckPath;
  int player0Lp;
  int player1Lp;
  int player0StartDraw;
  int player1StartDraw;
  int player0DrawPerTurn;
  int player1DrawPerTurn;
  std::filesystem::path customLuaScriptsDir;
  std::filesystem::path dcsScriptsDir;

  bool autoStart;
  bool autoProcess;
  bool useFixedSeed;
  unsigned int seed;

  std::deque<std::pair<std::string, int>> dcsCommandsCache;
  bool echoDcsCommand;

  ConfigDebug debug;
  ConfigLog log;
};

struct DDDApi {
  std::unordered_map<unsigned long long, DuelState> duelStates;
  std::unordered_map<unsigned long long, ShadowDuelState> shadowDuelStates;
  std::atomic<unsigned long long> currId;

  std::mutex duelStatesMutex;
  std::mutex shadowDuelStatesMutex;
  std::mutex genMutex; //all-purpose mutex for a given context
};



class DDD_GS { //global singleton
public:
  CardDbs cardDbs;
  Config conf;
  DDDApi dddapi;

  DDD_GS(); //can't be declared in hpp file (here)?

  DDD_GS(const DDD_GS&) = delete;
  DDD_GS& operator= (const DDD_GS&) = delete;

  static DDD_GS& get() { //can't be declared static in cpp file?
    static DDD_GS ddd;
    return ddd;
  }
};

#endif
