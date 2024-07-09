/*
 * dddutil.hpp
 */

#ifndef __DDDUTIL_HPP
#define __DDDUTIL_HPP

#ifndef SANDBOX_DIR
#define SANDBOX_DIR "sandbox"
#endif

#define MAX_OCGCORE_BUFFER_SIZE 0x20000
#define MAX_DUEL_BUFFER_SIZE 0x1000
#define MAX_STR_BUFFER_SIZE 0x100

#include <iostream>
#include <string>
#include <random>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <deque>

#ifdef BUILD_WIN
#include "windows.h"
#endif

#include "sqlite3.h"
#include "json.hpp"
#include "lua.hpp"
#include "ocgapi.h"
#include "card.h"
#include "effect.h"
#include "mtrandom.h"


#include "dddsingleton.hpp"
#include "dddapi.hpp"


struct card_texts;
struct CardDbs;
struct Config;
struct ConfigDebug;
class Buffer;


#ifdef CLOG_ENABLE
//forward declaration required for clog which uses this function in
// in dddapi to get its singleton instance
extern "C" DECL_DLLEXPORT DDD_GS& getDDD_GS();
#endif

//needs to be declared here since it's a template function
template<typename... T>
void clog(const std::string& flags, const T&... msgs) {

#ifdef CLOG_ENABLE
  std::stringstream buffer;
  bool checkIsOneArgument = true;

  if (!getDDD_GS().conf.log.forcePrintf) {
#ifndef CLOG_FORCE_PRINTF
    //print message/log with most clog features

#ifdef BUILD_WIN
    //determine appropriate colors to print based on flags
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO info;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    short ogc = info.wAttributes;
    short ogt = ogc & 15;
    short bgc = ogc & 240;
    short msgc = 7;

    bool combineBgc = false;
    //msg should not be highlightable or filtered by default
    bool filterable = (flags.find('f') != std::string::npos);

    if (flags.find('e') != std::string::npos) {
      msgc = 4;
      combineBgc = true;
      filterable = false;
    } else if (flags.find('w') != std::string::npos) {
      msgc = 14;
      combineBgc = true;
      filterable = false;
    } else if (flags.find('l') != std::string::npos) {
      msgc = 1;
      combineBgc = true;
    } else if (flags.find('d') != std::string::npos) {
      msgc = 128;
      combineBgc = false;
      filterable = false;
    } else if (flags.find('i') != std::string::npos) {
      msgc = 3;
      combineBgc = true;
    } else if ((flags.find('o') != std::string::npos) ||
               (flags.find('k') != std::string::npos)) {
      msgc = 2;
      combineBgc = true;
      filterable = false;
    } else {
      msgc = 7;
      combineBgc = true;
    }
#endif //end #ifdef BUILD_WIN
#ifdef BUILD_NIX
    //determine appropriate colors to print based on flags
    //msg should not be highlightable or filtered by default
    bool filterable = (flags.find('f') != std::string::npos);
    std::string asciiPrefix = "\033[";

    if (flags.find('e') != std::string::npos) {
      asciiPrefix += "31m";
      filterable = false;
    } else if (flags.find('w') != std::string::npos) {
      asciiPrefix += "93m";
      filterable = false;
    } else if (flags.find('l') != std::string::npos) {
      asciiPrefix += "34m";
    } else if (flags.find('d') != std::string::npos) {
      asciiPrefix += "30;47m";
      filterable = false;
    } else if (flags.find('i') != std::string::npos) {
      asciiPrefix += "36m";
    } else if ((flags.find('o') != std::string::npos) ||
               (flags.find('k') != std::string::npos)) {
      asciiPrefix += "32m";
      filterable = false;
    } else {
      asciiPrefix += "37m";
    }
#endif //end #ifdef BUILD_NIX

    //concatenate contents of msg to buffer
    ( [&buffer, &checkIsOneArgument](const auto &msg) {
      checkIsOneArgument = false;

      if constexpr((std::is_same_v<T, int8>) || (std::is_same_v<T, uint8>)) {
        buffer << (int) msg;
      } else {
        buffer << msg; // * * * * * NOTE TO SELF: if you're reading this while compiling, you're probably trying to print something that can't be printed; check the line *in the compilation buffer* above this line for the invalid type and also the string "required from here" (likely just above that line) for the location of the problem line in the source code * * * * *
      }
    }(msgs), ...);

    if (checkIsOneArgument) {
      //single msg without flags passed
      // (set some defaults and interpret flags as msg instead)
#ifdef BUILD_WIN
      msgc = 7;
#endif
#ifdef BUILD_NIX
      asciiPrefix += "31m";
#endif
      buffer << flags;
    }

    auto highlightList = getDDD_GS().conf.log.highlights;
    auto filterList = getDDD_GS().conf.log.filters;
    bool shouldFilter = false;

    std::string s = buffer.str();

    //check msg for matches in filter list
    if (filterable)
      for (const auto &f: filterList)
        if (s.find(f) != std::string::npos)
          //matched something in the filter
          if (getDDD_GS().conf.log.highlightOverridesFilter)
            shouldFilter = true; //check if msg has highlights that override this
          else
            return; //immediately exit function and don't print message

    //check msg for matches in highlight list
    std::map<int, int> sections;
    for (const auto &h: highlightList) {
      if (h.empty())
        continue;

      std::string tempstr = s;
      bool noMoreResults = false;
      unsigned int lastResult = 0;

      while (!noMoreResults) {
        auto result = tempstr.find(h);
        if (result != std::string::npos) {
          auto start = result + lastResult;
          auto end = result + h.length() + lastResult;
          sections[start] += 1;
          sections[end] -= 1;
          tempstr = tempstr.substr(result + 1);
          lastResult += result + 1;

        } else {
          noMoreResults = true;
        }
      }
    }

    //msg matches filter and no highlights; do not print and exit
    if (shouldFilter)
      if (sections.size() == 0)
        return;

    std::vector<std::pair<std::string, bool>> highlightSections;
    if ((!filterable) || (sections.size() == 0)) {
      //print message as is (without highlights; also should have bypassed filter)
      highlightSections.push_back(std::make_pair(s, false));

    } else {
      //parse message and apply highlights
      int currDepth = 0;
      int lastSectionIndex = 0;

      for (const auto &p: sections) {
        bool isInSection = (currDepth != 0);
        currDepth += p.second;

        if (isInSection) {
          //searching for section end

          if (currDepth == 0) {
            //found section end
            int substrLength = p.first - lastSectionIndex;
            if (substrLength > 0)
              highlightSections.push_back
                (std::make_pair(s.substr(lastSectionIndex, substrLength), true));
            lastSectionIndex = p.first;
          }
        } else {
          //searching for and found next section
          int substrLength = p.first - lastSectionIndex;
          if (substrLength > 0)
            highlightSections.push_back
              (std::make_pair(s.substr(lastSectionIndex, substrLength), false));
          lastSectionIndex = p.first;
        }
      }
      if (lastSectionIndex < s.length())
        highlightSections.push_back
          (std::make_pair(s.substr(lastSectionIndex), (currDepth != 0)));
    }

#ifdef BUILD_WIN
    //print the outputs with appropriate colors
    SetConsoleTextAttribute(hConsole, msgc);
    for (const auto &hs: highlightSections) {
      if (hs.second) {
        //highlight
        SetConsoleTextAttribute(hConsole, msgc | (7 << 4));
        //SetConsoleTextAttribute(hConsole, 1 | (8 << 4));
        std::cout << hs.first;
      } else {
        //print normally
        SetConsoleTextAttribute(hConsole, msgc | ((combineBgc) ? bgc : 0));
        std::cout << hs.first;
      }
    }
    std::cout << std::endl;
    SetConsoleTextAttribute(hConsole, bgc + ogt);
#endif //end #ifdef BUILD_WIN
#ifdef BUILD_NIX
    //print the outputs with appropriate colors
    for (const auto &hs: highlightSections) {
      if (hs.second) {
        //highlight
        std::cout << "\033[47m" << hs.first << "\033[0m";
      } else {
        //print normally
        std::cout << asciiPrefix << hs.first << "\033[0m";
      }
    }
    std::cout << std::endl;
#endif //end #ifdef BUILD_NIX

#endif //end ifndef CLOG_FORCE_PRINTF

  } else {
    //printf version

    ( [&buffer, &checkIsOneArgument](const auto &msg) {
      checkIsOneArgument = false;
      buffer << msg; // * * * * * NOTE TO SELF: if you're reading this while compiling, you're probably trying to print something that can't be printed; check the line *in the compilation buffer* above this line for the invalid type and also the string "required from here" (likely just above that line) for the location of the problem line in the source code * * * * *
    }(msgs), ...);

    if (checkIsOneArgument)
      buffer << flags;

    printf("%s\n", buffer.str().c_str());
  }

  //also log output to file if set in conf
  if (getDDD_GS().conf.log.logToFile) {
    if (!getDDD_GS().conf.log.logFile.empty()) {
      try {
        std::ofstream logFile;
        logFile.open(getDDD_GS().conf.log.logFile, std::ios_base::app);
        logFile << buffer.str() << std::endl;
        logFile.close();
      } catch (std::exception &e) {
        //...can't really call clog here on threat of recursion
        printf("%s\n", e.what());
        printf("Unable to log output to file.\n");
      }
    }
  }

  //also cache output for later retrieval if set in conf
  if (getDDD_GS().conf.log.cacheLogs) {
    getDDD_GS().conf.log.logCache.push_back(buffer.str());
  }
#endif //end CLOG_ENABLE

}

//helper class for working with/storing byte arrays
class Buffer {
private:
  std::vector<byte> buffer;
  int currIndex;
  int maxSize;

  template<typename T>
  T get(const bool);
  template<typename T>
  bool write(const T, const bool);
  template<typename T>
  int inc(const int);

public:
  Buffer();
  Buffer(const std::vector<byte>&);
  Buffer(const std::vector<byte>&, const int);
  Buffer(byte*, const int);

  int8 g8();
  int8 g8(const bool);
  int8 get8();
  int8 get8(const bool);
  int16 g16();
  int16 g16(const bool);
  int16 get16();
  int16 get16(const bool);
  int32 g32();
  int32 g32(const bool);
  int32 get32();
  int32 get32(const bool);
  int64 g64();
  int64 g64(const bool);
  int64 get64();
  int64 get64(const bool);
  bool w8(const int8);
  bool w8(const int8, const bool);
  bool write8(const int8);
  bool write8(const int8, const bool);
  bool w16(const int16);
  bool w16(const int16, const bool);
  bool write16(const int16);
  bool write16(const int16, const bool);
  bool w32(const int32);
  bool w32(const int32, const bool);
  bool write32(const int32);
  bool write32(const int32, const bool);
  bool w64(const int64);
  bool w64(const int64, const bool);
  bool write64(const int64);
  bool write64(const int64, const bool);
  int inc8();
  int inc8(int);
  int inc16();
  int inc16(int);
  int inc32();
  int inc32(int);
  int inc64();
  int inc64(int);

  bool set(byte*, int);
  void printBuffer();
  void printBuffer(bool);
  void printBuffer(bool, bool);


  int operator++();
  int operator++(int);

  Buffer& getResponseB(intptr_t);
  void setResponseB(intptr_t);

  Buffer& resetItr();
  bool isValid();
  bool isItrAtEnd();
};


std::string decodeCode(const uint32);
std::string decodeCodeFromLuaGlobalString(const std::string&);
std::string decodeDesc(const uint32);
std::string decodeAttribute(const uint32);
std::string decodeRace(const uint32);
std::string decodeType(const uint32);
std::string decodePosition(const uint8);
std::string decodeLocation(const uint8, const bool);
std::string decodeLocation(const uint8);
std::string decodeLocation(const uint32, const bool);
std::string decodeLocation(const uint32);
std::string decodeFieldLocation(const uint8, const uint8);
std::string decodeFieldLocation(const uint32);
std::string decodeTiming(const int32);
std::string decodeReason(const uint32);
std::string decodeStatus(const uint32);
std::string decodePlayer(const uint8);
std::string decodeEffectType(const uint16);
std::string decodeEffectCode(const uint32);
std::string decodeEffectStatus(const uint16);
std::string decodeEffectCountCode(const uint32);
std::string decodeEffectFlag(const uint32);
std::string decodeEffectFlag2(const uint32);
std::string decodeReset(const uint32);
std::string decodeMsgHint(const uint32);
std::string decodeProcessorType(const uint16);


bool confirmYN(const std::string&);
bool confirmYN(const std::string&, const bool);
void* getPtrFromInput();
int32 getRefFromInput();

std::string trimString(const std::string&);
std::vector<std::string> splitString(const std::string&, const std::string&);
std::string joinString(const std::vector<std::string>&);
std::string joinString(const std::vector<std::string>&, const std::string&);
bool searchString(const std::string&, const std::string&);
std::string dequoteString(const std::string&);

nlohmann::json getJson(const std::filesystem::path&);
bool dddGsInit();
bool dddGsInit(const std::filesystem::path&, const bool);
ConfigDebug initDebug(const nlohmann::json&);
ConfigLog initLog(const nlohmann::json&);

byte* dddScriptReader(const char*, int*);
uint32 dddCardReader(unsigned int, card_data*);
uint32 dddMessageHandler(intptr_t, uint32);
bool readCardsDb(const bool);
DeckCards readYdk(const std::filesystem::path&);
intptr_t setUpAndStartDuel(const bool, const bool, const unsigned int);

void executeSimpleLuaScript(lua_State*, std::string);
void loadAndCallFromLuaScript(lua_State*, std::string, const std::string&);

int getLastNumberFromString(const std::string&);
bool parseStringToBool(const std::string&);

void loadDcsCommandsFromScript(std::filesystem::path);
std::pair<std::string, int> getDcsCommand();
std::tuple<std::string, std::unordered_map<std::string, std::string>, std::unordered_set<std::string>> parseDcsCommand(const std::string&);
#endif
