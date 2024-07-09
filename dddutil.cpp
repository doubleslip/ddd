/*
 * dddutil.cpp
 */

#include "dddutil.hpp"


//get value at currIndex
template <typename T>
T Buffer::get(const bool shouldIncrement) {
  if ((currIndex + sizeof(T)) > maxSize) {
    clog("w", "Attempted to get value beyond buffer max size.");
    return -1;
  }

  if ((currIndex + sizeof(T)) > buffer.size()) {
    clog("w", "Attempted to get value beyond size of buffer.");
    return -1;
  }

  T* pT = reinterpret_cast<T*>(&(buffer[currIndex]));
  T t = *pT;

  if (shouldIncrement) {
    currIndex += sizeof(T);
  }

  return t;
}

//write at currIndex and then increment by the amount written
template <typename T>
bool Buffer::write(const T inVal, const bool shouldIncrement) {

  if ((currIndex + sizeof(T)) > maxSize) {
    clog("w", "Attempted to write value beyond buffer max size.");
    return false;
  }

  if ((currIndex + sizeof(T)) >= buffer.size()) {
    buffer.resize(currIndex + sizeof(T));
  }

  T* pT = reinterpret_cast<T*>(&(buffer[currIndex]));
  *pT = inVal;

  if (shouldIncrement) {
    currIndex += sizeof(T);
  }

  return true;
}


template <typename T>
int Buffer::inc(const int inAmount) {
  if ((currIndex + sizeof(T)) > maxSize) {
    clog("w", "Attempted to increment value beyond buffer max size.");
    return currIndex;
  }

  currIndex += (sizeof(T) * inAmount);

  return currIndex;
}


Buffer::Buffer() {
  currIndex = 0;
  maxSize = MAX_OCGCORE_BUFFER_SIZE;
  buffer.reserve(maxSize);
}
Buffer::Buffer(const std::vector<byte>& inBuffer) : Buffer::Buffer(inBuffer, MAX_OCGCORE_BUFFER_SIZE) {
}
Buffer::Buffer(const std::vector<byte>& inBuffer, const int inMaxSize) {
  currIndex = 0;
  maxSize = inMaxSize;
  buffer.reserve(maxSize);
  buffer = std::vector<byte>(inBuffer);
}
Buffer::Buffer(byte* inB, const int inLen) {
  currIndex = 0;
  maxSize = MAX_OCGCORE_BUFFER_SIZE;
  buffer.reserve(maxSize);
  buffer.resize(inLen);
  std::memcpy(&buffer[0], inB, inLen);
}

int8 Buffer::g8() {
  return this->get<int8>(true);
}
int8 Buffer::g8(const bool shouldIncrement) {
  return this->get<int8>(shouldIncrement);
}
int16 Buffer::g16() {
  return this->get<int16>(true);
}
int16 Buffer::g16(const bool shouldIncrement) {
  return this->get<int16>(shouldIncrement);
}
int32 Buffer::g32() {
  return this->get<int32>(true);
}
int32 Buffer::g32(const bool shouldIncrement) {
  return this->get<int32>(shouldIncrement);
}
int64 Buffer::g64() {
  return this->get<int64>(true);
}
int64 Buffer::g64(const bool shouldIncrement) {
  return this->get<int64>(shouldIncrement);
}
int8 Buffer::get8() {
  return this->get<int8>(true);
}
int8 Buffer::get8(const bool shouldIncrement) {
  return this->get<int8>(shouldIncrement);
}
int16 Buffer::get16() {
  return this->get<int16>(true);
}
int16 Buffer::get16(const bool shouldIncrement) {
  return this->get<int16>(shouldIncrement);
}
int32 Buffer::get32() {
  return this->get<int32>(true);
}
int32 Buffer::get32(const bool shouldIncrement) {
  return this->get<int32>(shouldIncrement);
}
int64 Buffer::get64() {
  return this->get<int64>(true);
}
int64 Buffer::get64(const bool shouldIncrement) {
  return this->get<int64>(shouldIncrement);
}
bool Buffer::w8(const int8 inVal) {
  return this->write<int8>(inVal, true);
}
bool Buffer::w8(const int8 inVal, const bool shouldIncrement) {
  return this->write<int8>(inVal, shouldIncrement);
}
bool Buffer::w16(const int16 inVal) {
  return this->write<int16>(inVal, true);
}
bool Buffer::w16(const int16 inVal, const bool shouldIncrement) {
  return this->write<int16>(inVal, shouldIncrement);
}
bool Buffer::w32(const int32 inVal) {
  return this->write<int32>(inVal, true);
}
bool Buffer::w32(const int32 inVal, const bool shouldIncrement) {
  return this->write<int32>(inVal, shouldIncrement);
}
bool Buffer::w64(const int64 inVal) {
  return this->write<int64>(inVal, true);
}
bool Buffer::w64(const int64 inVal, const bool shouldIncrement) {
  return this->write<int64>(inVal, shouldIncrement);
}
bool Buffer::write8(const int8 inVal) {
  return this->write<int8>(inVal, true);
}
bool Buffer::write8(const int8 inVal, const bool shouldIncrement) {
  return this->write<int8>(inVal, shouldIncrement);
}
bool Buffer::write16(const int16 inVal) {
  return this->write<int16>(inVal, true);
}
bool Buffer::write16(const int16 inVal, const bool shouldIncrement) {
  return this->write<int16>(inVal, shouldIncrement);
}
bool Buffer::write32(const int32 inVal) {
  return this->write<int32>(inVal, true);
}
bool Buffer::write32(const int32 inVal, const bool shouldIncrement) {
  return this->write<int32>(inVal, shouldIncrement);
}
bool Buffer::write64(const int64 inVal) {
  return this->write<int64>(inVal, true);
}
bool Buffer::write64(const int64 inVal, const bool shouldIncrement) {
  return this->write<int64>(inVal, shouldIncrement);
}

int Buffer::inc8() {
  return this->inc<int8>(1);
}
int Buffer::inc8(const int inAmount) {
  return this->inc<int8>(inAmount);
}
int Buffer::inc16() {
  return this->inc<int16>(1);
}
int Buffer::inc16(const int inAmount) {
  return this->inc<int16>(inAmount);
}
int Buffer::inc32() {
  return this->inc<int32>(1);
}
int Buffer::inc32(const int inAmount) {
  return this->inc<int32>(inAmount);
}
int Buffer::inc64() {
  return this->inc<int64>(1);
}
int Buffer::inc64(const int inAmount) {
  return this->inc<int64>(inAmount);
}

//increment operators as alias to increment by 1
int Buffer::operator++() {
  return this->inc<int8>(1);
}
int Buffer::operator++(int) {
  int prevIndex = currIndex;
  this->inc<int8>(1);
  return prevIndex;
}

Buffer& Buffer::resetItr() {
  currIndex = 0;
  return *this;
}
bool Buffer::isValid() {
  return (buffer.size() > 0);
}
bool Buffer::isItrAtEnd() {
  return (currIndex >= buffer.size());
}



//Various decode functions used to convert values from a pDuel
// into more user-friendly strings depending on the context
std::string decodeCode(const uint32 i) {
  if (getDDD_GS().cardDbs.cardTextsDb.find(i & 0x0FFFFFFF) ==
      getDDD_GS().cardDbs.cardTextsDb.end())
    return "";

  return getDDD_GS().cardDbs.cardTextsDb[(i & 0x0FFFFFFF)].name;
}

std::string decodeCodeFromLuaGlobalString(const std::string& lgs) {
  std::string decodedCode = "";

  if ((lgs.length() > 1) && (lgs.substr(0, 1) == "c")) {
    unsigned int dc = lgs.find('.');

    if (dc != std::string::npos) {
      std::string ss = lgs.substr(1, (dc - 1));
      bool foundNan = false;

      for (int i = 0; i < ss.length(); ++i) {
        if (!std::isdigit(ss[i])) {
          foundNan = true;
          break;
        }
      }

      if (!foundNan) {
        try {
          uint32 testCode = std::stoi(ss);
          decodedCode = decodeCode(testCode);
        } catch (std::exception &e) {
          clog("e", e.what(), "  (ss = '", ss, "')");
        }
      }
    }
  }
  return decodedCode;
}

std::string decodeDesc(const uint32 i) {

  if (i == 1104) return "[Auxiliary|aux].SpiritReturnReg";
  if (i == 1193) return "[Auxiliary|aux].EnableNeosReturn";
  if (i == 1164) return "[Auxiliary|aux].AddSynchro[Mix]Procedure";
  if (i == 1165) return "[Auxiliary|aux].AddXyzProcedure[LevelFree]";
  if (i == 1163) return "[Auxiliary|aux].EnablePendulumAttribute";
  if (i == 1160) return "[Auxiliary|aux].EnablePendulumAttribute(reg=false)";
  if (i == 1166) return "[Auxiliary|aux].AddLinkProcedure";

  int eff = i & 15;
  int code = i >> 4;

  std::string s = "";

  if ((getDDD_GS().cardDbs.cardDb.find(code) ==
       getDDD_GS().cardDbs.cardDb.end()) ||
      (getDDD_GS().cardDbs.cardTextsDb.find(code) ==
       getDDD_GS().cardDbs.cardTextsDb.end()))
    return s;

  std::string cardName = decodeCode(code);
  std::string cardEffStr = getDDD_GS().cardDbs.cardTextsDb.at(code).cardStrs[eff];

  if (cardName != "") {

    if (cardEffStr != "") {
      s = "Effect of " + cardName + " (" + cardEffStr + ")";
    } else {
      s = cardName + " (effect " + std::to_string(eff) + ")";
    }
  }
  return s;
}


std::string decodeAttribute(const uint32 i) {
  std::vector<std::string> v;
  if (i & ATTRIBUTE_EARTH) v.push_back("EARTH");
  if (i & ATTRIBUTE_WATER) v.push_back("WATER");
  if (i & ATTRIBUTE_FIRE) v.push_back("FIRE");
  if (i & ATTRIBUTE_WIND) v.push_back("WIND");
  if (i & ATTRIBUTE_LIGHT) v.push_back("LIGHT");
  if (i & ATTRIBUTE_DARK) v.push_back("DARK");
  if (i & ATTRIBUTE_DEVINE) v.push_back("DIVINE");

  return joinString(v, "+");
}

std::string decodeRace(const uint32 i) {
  std::vector<std::string> v;
  if (i & RACE_WARRIOR) v.push_back("warrior");
  if (i & RACE_SPELLCASTER) v.push_back("spellcaster");
  if (i & RACE_FAIRY) v.push_back("fairy");
  if (i & RACE_FIEND) v.push_back("fiend");
  if (i & RACE_ZOMBIE) v.push_back("zombie");
  if (i & RACE_MACHINE) v.push_back("machine");
  if (i & RACE_AQUA) v.push_back("aqua");
  if (i & RACE_PYRO) v.push_back("pyro");
  if (i & RACE_ROCK) v.push_back("rock");
  if (i & RACE_WINDBEAST) v.push_back("winged-beast");
  if (i & RACE_PLANT) v.push_back("plant");
  if (i & RACE_INSECT) v.push_back("insect");
  if (i & RACE_THUNDER) v.push_back("thunder");
  if (i & RACE_DRAGON) v.push_back("dragon");
  if (i & RACE_BEAST) v.push_back("beast");
  if (i & RACE_BEASTWARRIOR) v.push_back("beast-warrior");
  if (i & RACE_DINOSAUR) v.push_back("dinosaur");
  if (i & RACE_FISH) v.push_back("fish");
  if (i & RACE_SEASERPENT) v.push_back("sea-serpent");
  if (i & RACE_REPTILE) v.push_back("reptile");
  if (i & RACE_PSYCHO) v.push_back("psychic");
  if (i & RACE_DEVINE) v.push_back("divine?");
  if (i & RACE_CREATORGOD) v.push_back("creator god?");
  if (i & RACE_WYRM) v.push_back("wyrm");
  if (i & RACE_CYBERSE) v.push_back("cyberse");

  return joinString(v, "+");
}

std::string decodeType(const uint32 i) {
  std::vector<std::string> v;
  if (i & TYPE_MONSTER) v.push_back("Monster");
  if (i & TYPE_SPELL) v.push_back("Spell");
  if (i & TYPE_TRAP) v.push_back("Trap");
  if (i & TYPE_NORMAL) v.push_back("Normal");
  if (i & TYPE_EFFECT) v.push_back("Effect");
  if (i & TYPE_FUSION) v.push_back("Fusion");
  if (i & TYPE_RITUAL) v.push_back("Ritual");
  //if (i & TYPE_TRAPMONSTER) v.push_back("Trap monster");
  if (i & TYPE_SPIRIT) v.push_back("Spirit");
  if (i & TYPE_UNION) v.push_back("Union");
  //if (i & TYPE_DUAL) v.push_back("Dual"); //dual attribute?
  if (i & TYPE_TUNER) v.push_back("Tuner");
  if (i & TYPE_SYNCHRO) v.push_back("Synchro");
  if (i & TYPE_TOKEN) v.push_back("Token");
  if (i & TYPE_QUICKPLAY) v.push_back("Quickplay");
  if (i & TYPE_CONTINUOUS) v.push_back("Continuous");
  if (i & TYPE_EQUIP) v.push_back("Equip");
  if (i & TYPE_FIELD) v.push_back("Field");
  if (i & TYPE_COUNTER) v.push_back("Counter");
  if (i & TYPE_FLIP) v.push_back("Flip");
  if (i & TYPE_TOON) v.push_back("Toon");
  if (i & TYPE_XYZ) v.push_back("Xyz");
  if (i & TYPE_PENDULUM) v.push_back("Pendulum");
  //if (i & TYPE_SPSUMMON) v.push_back(""); //nomi?
  if (i & TYPE_LINK) v.push_back("Link");

  return joinString(v, "/");
}

std::string decodePosition(const uint8 i) {
  std::vector<std::string> v;

  if (i & POS_FACEUP)
    v.push_back("face up");
  else if (i & POS_FACEDOWN)
    v.push_back("face down");
  else
    v.push_back("unknown");

  if (i & POS_ATTACK)
    v.push_back("atk");
  else if (i & POS_DEFENSE)
    v.push_back("def");

  v.push_back("position");

  return joinString(v, " ");
}


std::string decodeLocation(const uint8 i, const bool b) {
  std::vector<std::string> v;
  if (i & LOCATION_DECK) v.push_back(((b) ? "in main deck" : "main deck"));
  if (i & LOCATION_HAND) v.push_back(((b) ? "in hand" : "hand"));
  if (i & LOCATION_MZONE) v.push_back(((b) ? "in monster zone" : "monster zone"));
  if (i & LOCATION_SZONE) v.push_back(((b) ? "in s/t zone" : "s/t zone"));
  if (i & LOCATION_GRAVE) v.push_back(((b) ? "in graveyard" : "graveyard"));
  if (i & LOCATION_REMOVED) v.push_back(((b) ? "is banished" : "banished"));
  if (i & LOCATION_EXTRA) v.push_back(((b) ? "in extra deck" : "extra deck"));
  if (i & LOCATION_OVERLAY) {
    v.push_back("(overlayed)");
  } else {
    //if (i & LOCATION_ONFIELD) v.push_back(((b) ? "on field" : "field"));
    if (i & LOCATION_FZONE) v.push_back("(field zone)");
    if (i & LOCATION_PZONE) v.push_back("(pendulum zone)");
  }

  return joinString(v, " ");
}
std::string decodeLocation(const uint8 i) {
  return decodeLocation(i, true);
}
std::string decodeLocation(const uint32 i, const bool b) {
  uint8* p;
  p = ((uint8*) &i) + 1;
  return decodeLocation(*p, b);
}
std::string decodeLocation(const uint32 i) {
  return decodeLocation(i, true);
}

std::string decodeFieldLocation(const uint8 l, const uint8 s) {
  if (!(l & LOCATION_ONFIELD)) return "?";

  std::string zone = "";

  if (l & LOCATION_MZONE) {
    if ((s == 5) || (s == 6)) {
      zone = "(Extra) Monster Zone";
    } else {
      zone = "Monster Zone";
    }
  } else {
    if ((s == 0) || (s == 6)) {
      zone = "S/T (Left Pendulum) Zone";
    } else if ((s == 4) || (s == 7)) {
      zone = "S/T (Right Pendulum) Zone";
    } else if (s == 5) {
      zone = "S/T (Field) Zone";
    } else {
      zone = "S/T Zone";
    }
  }
  return zone + " " + std::to_string(s);
}

std::string decodeFieldLocation(const uint32 i) {
  return decodeFieldLocation(*(((int8*) &i) + 1), *(((int8*) &i) + 2));
}

std::string decodeTiming(const int32 i) {
  std::vector<std::string> v;
  if (i & 0x1) v.push_back("TIMING_DRAW_PHASE");
  if (i & 0x2) v.push_back("TIMING_STANDBY_PHASE");
  if (i & 0x4) v.push_back("TIMING_MAIN_END");
  if (i & 0x8) v.push_back("TIMING_BATTLE_START");
  if (i & 0x10) v.push_back("TIMING_BATTLE_END");
  if (i & 0x20) v.push_back("TIMING_END_PHASE");
  if (i & 0x40) v.push_back("TIMING_SUMMON");
  if (i & 0x80) v.push_back("TIMING_SPSUMMON");
  if (i & 0x100) v.push_back("TIMING_FLIPSUMMON");
  if (i & 0x200) v.push_back("TIMING_MSET");
  if (i & 0x400) v.push_back("TIMING_SSET");
  if (i & 0x800) v.push_back("TIMING_POS_CHANGE");
  if (i & 0x1000) v.push_back("TIMING_ATTACK");
  if (i & 0x2000) v.push_back("TIMING_DAMAGE_STEP");
  if (i & 0x4000) v.push_back("TIMING_DAMAGE_CAL");
  if (i & 0x8000) v.push_back("TIMING_CHAIN_END");
  if (i & 0x10000) v.push_back("TIMING_DRAW");
  if (i & 0x20000) v.push_back("TIMING_DAMAGE");
  if (i & 0x40000) v.push_back("TIMING_RECOVER");
  if (i & 0x80000) v.push_back("TIMING_DESTROY");
  if (i & 0x100000) v.push_back("TIMING_REMOVE");
  if (i & 0x200000) v.push_back("TIMING_TOHAND");
  if (i & 0x400000) v.push_back("TIMING_TODECK");
  if (i & 0x800000) v.push_back("TIMING_TOGRAVE");
  if (i & 0x1000000) v.push_back("TIMING_BATTLE_PHASE");
  if (i & 0x2000000) v.push_back("TIMING_EQUIP");
  if (i & 0x4000000) v.push_back("TIMING_BATTLE_STEP_END");
  if (i & 0x8000000) v.push_back("TIMING_BATTLED");

  return joinString(v, ",");
}

std::string decodeReason(const uint32 i) {
  std::vector<std::string> v;
  if (i & REASON_DESTROY) v.push_back("REASON_DESTROY");
  if (i & REASON_RELEASE) v.push_back("REASON_RELEASE");
  if (i & REASON_TEMPORARY) v.push_back("REASON_TEMPORARY");
  if (i & REASON_MATERIAL) v.push_back("REASON_MATERIAL");
  if (i & REASON_SUMMON) v.push_back("REASON_SUMMON");
  if (i & REASON_BATTLE) v.push_back("REASON_BATTLE");
  if (i & REASON_EFFECT) v.push_back("REASON_EFFECT");
  if (i & REASON_COST) v.push_back("REASON_COST");
  if (i & REASON_ADJUST) v.push_back("REASON_ADJUST");
  if (i & REASON_LOST_TARGET) v.push_back("REASON_LOST_TARGET");
  if (i & REASON_RULE) v.push_back("REASON_RULE");
  if (i & REASON_SPSUMMON) v.push_back("REASON_SPSUMMON");
  if (i & REASON_DISSUMMON) v.push_back("REASON_DISSUMMON");
  if (i & REASON_FLIP) v.push_back("REASON_FLIP");
  if (i & REASON_DISCARD) v.push_back("REASON_DISCARD");
  if (i & REASON_RDAMAGE) v.push_back("REASON_RDAMAGE");
  if (i & REASON_RRECOVER) v.push_back("REASON_RRECOVER");
  if (i & REASON_RETURN) v.push_back("REASON_RETURN");
  if (i & REASON_FUSION) v.push_back("REASON_FUSION");
  if (i & REASON_SYNCHRO) v.push_back("REASON_SYNCHRO");
  if (i & REASON_RITUAL) v.push_back("REASON_RITUAL");
  if (i & REASON_XYZ) v.push_back("REASON_XYZ");
  if (i & REASON_REPLACE) v.push_back("REASON_REPLACE");
  if (i & REASON_DRAW) v.push_back("REASON_DRAW");
  if (i & REASON_REDIRECT) v.push_back("REASON_REDIRECT");
  if (i & REASON_REVEAL) v.push_back("REASON_REVEAL");
  if (i & REASON_LINK) v.push_back("REASON_LINK");
  if (i & REASON_LOST_OVERLAY) v.push_back("REASON_LOST_OVERLAY");
  if (i & REASON_MAINTENANCE) v.push_back("REASON_MAINTENANCE");
  if (i & REASON_ACTION) v.push_back("REASON_ACTION");
  if (i & REASONS_PROCEDURE) v.push_back("REASONs_PROCEDURE");

  return joinString(v, " + ");
}

std::string decodeStatus(const uint32 i) {
  std::vector<std::string> v;
  if (i & STATUS_DISABLED) v.push_back("STATUS_DISABLED");
  if (i & STATUS_TO_ENABLE) v.push_back("STATUS_TO_ENABLE");
  if (i & STATUS_TO_DISABLE) v.push_back("STATUS_TO_DISABLE");
  if (i & STATUS_PROC_COMPLETE) v.push_back("STATUS_PROC_COMPLETE");
  if (i & STATUS_SET_TURN) v.push_back("STATUS_SET_TURN");
  if (i & STATUS_NO_LEVEL) v.push_back("STATUS_NO_LEVEL");
  if (i & STATUS_BATTLE_RESULT) v.push_back("STATUS_BATTLE_RESULT");
  if (i & STATUS_SPSUMMON_STEP) v.push_back("STATUS_SPSUMMON_STEP");
  if (i & STATUS_FORM_CHANGED) v.push_back("STATUS_FORM_CHANGED");
  if (i & STATUS_SUMMONING) v.push_back("STATUS_SUMMONING");
  if (i & STATUS_EFFECT_ENABLED) v.push_back("STATUS_EFFECT_ENABLED");
  if (i & STATUS_SUMMON_TURN) v.push_back("STATUS_SUMMON_TURN");
  if (i & STATUS_DESTROY_CONFIRMED) v.push_back("STATUS_DESTROY_CONFIRMED");
  if (i & STATUS_LEAVE_CONFIRMED) v.push_back("STATUS_LEAVE_CONFIRMED");
  if (i & STATUS_BATTLE_DESTROYED) v.push_back("STATUS_BATTLE_DESTROYED");
  if (i & STATUS_COPYING_EFFECT) v.push_back("STATUS_COPYING_EFFECT");
  if (i & STATUS_CHAINING) v.push_back("STATUS_CHAINING");
  if (i & STATUS_SUMMON_DISABLED) v.push_back("STATUS_SUMMON_DISABLED");
  if (i & STATUS_ACTIVATE_DISABLED) v.push_back("STATUS_ACTIVATE_DISABLED");
  if (i & STATUS_EFFECT_REPLACED) v.push_back("STATUS_EFFECT_REPLACED");
  if (i & STATUS_FLIP_SUMMONING) v.push_back("STATUS_FLIP_SUMMONING");
  if (i & STATUS_ATTACK_CANCELED) v.push_back("STATUS_ATTACK_CANCELED");
  if (i & STATUS_INITIALIZING) v.push_back("STATUS_INITIALIZING");
  if (i & STATUS_TO_HAND_WITHOUT_CONFIRM) v.push_back("STATUS_TO_HAND_WITHOUT_CONFIRM");
  if (i & STATUS_JUST_POS) v.push_back("STATUS_JUST_POS");
  if (i & STATUS_CONTINUOUS_POS) v.push_back("STATUS_CONTINUOUS_POS");
  if (i & STATUS_FORBIDDEN) v.push_back("STATUS_FORBIDDEN");
  if (i & STATUS_ACT_FROM_HAND) v.push_back("STATUS_ACT_FROM_HAND");
  if (i & STATUS_OPPO_BATTLE) v.push_back("STATUS_OPPO_BATTLE");
  if (i & STATUS_FLIP_SUMMON_TURN) v.push_back("STATUS_FLIP_SUMMON_TURN");
  if (i & STATUS_SPSUMMON_TURN) v.push_back("STATUS_SPSUMMON_TURN");
  if (i & STATUS_FLIP_SUMMON_DISABLED) v.push_back("STATUS_FLIP_SUMMON_DISABLED");

  return joinString(v, " + ");
}

std::string decodePlayer(const uint8 i) {
  if (i == PLAYER_SELFDES) return "player self destroy?";
  if (i == PLAYER_ALL) return "both players";
  if (i == PLAYER_NONE) return "no / neither player";
  if (i == 0) return "player 0";
  if (i == 1) return "player 1";

  return "player " + std::to_string((int) i);
}

std::string decodeEffectType(const uint16 i) {
  std::vector<std::string> v;
  if (i & EFFECT_TYPE_SINGLE) v.push_back("EFFECT_TYPE_SINGLE");
  if (i & EFFECT_TYPE_FIELD) v.push_back("EFFECT_TYPE_FIELD");
  if (i & EFFECT_TYPE_EQUIP) v.push_back("EFFECT_TYPE_EQUIP");
  if (i & EFFECT_TYPE_ACTIONS) v.push_back("EFFECT_TYPE_ACTIONS");
  if (i & EFFECT_TYPE_ACTIVATE) v.push_back("EFFECT_TYPE_ACTIVATE");
  if (i & EFFECT_TYPE_FLIP) v.push_back("EFFECT_TYPE_FLIP");
  if (i & EFFECT_TYPE_IGNITION) v.push_back("EFFECT_TYPE_IGNITION");
  if (i & EFFECT_TYPE_TRIGGER_O) v.push_back("EFFECT_TYPE_TRIGGER_O");
  if (i & EFFECT_TYPE_QUICK_O) v.push_back("EFFECT_TYPE_QUICK_O");
  if (i & EFFECT_TYPE_TRIGGER_F) v.push_back("EFFECT_TYPE_TRIGGER_F");
  if (i & EFFECT_TYPE_QUICK_F) v.push_back("EFFECT_TYPE_QUICK_F");
  if (i & EFFECT_TYPE_CONTINUOUS) v.push_back("EFFECT_TYPE_CONTINUOUS");
  if (i & EFFECT_TYPE_XMATERIAL) v.push_back("EFFECT_TYPE_XMATERIAL");
  if (i & EFFECT_TYPE_GRANT) v.push_back("EFFECT_TYPE_GRANT");
  if (i & EFFECT_TYPE_TARGET) v.push_back("EFFECT_TYPE_TARGET");

  return joinString(v, " + ");
}

std::string decodeEffectCode(const uint32 i) {
  if (i == EFFECT_IMMUNE_EFFECT) return "EFFECT_IMMUNE_EFFECT";
  if (i == EFFECT_DISABLE) return "EFFECT_DISABLE";
  if (i == EFFECT_CANNOT_DISABLE) return "EFFECT_CANNOT_DISABLE";
  if (i == EFFECT_SET_CONTROL) return "EFFECT_SET_CONTROL";
  if (i == EFFECT_CANNOT_CHANGE_CONTROL) return "EFFECT_CANNOT_CHANGE_CONTROL";
  if (i == EFFECT_CANNOT_ACTIVATE) return "EFFECT_CANNOT_ACTIVATE";
  if (i == EFFECT_CANNOT_TRIGGER) return "EFFECT_CANNOT_TRIGGER";
  if (i == EFFECT_DISABLE_EFFECT) return "EFFECT_DISABLE_EFFECT";
  if (i == EFFECT_DISABLE_CHAIN) return "EFFECT_DISABLE_CHAIN";
  if (i == EFFECT_DISABLE_TRAPMONSTER) return "EFFECT_DISABLE_TRAPMONSTER";
  if (i == EFFECT_CANNOT_INACTIVATE) return "EFFECT_CANNOT_INACTIVATE";
  if (i == EFFECT_CANNOT_DISEFFECT) return "EFFECT_CANNOT_DISEFFECT";
  if (i == EFFECT_CANNOT_CHANGE_POSITION) return "EFFECT_CANNOT_CHANGE_POSITION";
  if (i == EFFECT_TRAP_ACT_IN_HAND) return "EFFECT_TRAP_ACT_IN_HAND";
  if (i == EFFECT_TRAP_ACT_IN_SET_TURN) return "EFFECT_TRAP_ACT_IN_SET_TURN";
  if (i == EFFECT_REMAIN_FIELD) return "EFFECT_REMAIN_FIELD";
  if (i == EFFECT_MONSTER_SSET) return "EFFECT_MONSTER_SSET";
  if (i == EFFECT_CANNOT_SUMMON) return "EFFECT_CANNOT_SUMMON";
  if (i == EFFECT_CANNOT_FLIP_SUMMON) return "EFFECT_CANNOT_FLIP_SUMMON";
  if (i == EFFECT_CANNOT_SPECIAL_SUMMON) return "EFFECT_CANNOT_SPECIAL_SUMMON";
  if (i == EFFECT_CANNOT_MSET) return "EFFECT_CANNOT_MSET";
  if (i == EFFECT_CANNOT_SSET) return "EFFECT_CANNOT_SSET";
  if (i == EFFECT_CANNOT_DRAW) return "EFFECT_CANNOT_DRAW";
  if (i == EFFECT_CANNOT_DISABLE_SUMMON) return "EFFECT_CANNOT_DISABLE_SUMMON";
  if (i == EFFECT_CANNOT_DISABLE_SPSUMMON) return "EFFECT_CANNOT_DISABLE_SPSUMMON";
  if (i == EFFECT_SET_SUMMON_COUNT_LIMIT) return "EFFECT_SET_SUMMON_COUNT_LIMIT";
  if (i == EFFECT_EXTRA_SUMMON_COUNT) return "EFFECT_EXTRA_SUMMON_COUNT";
  if (i == EFFECT_SPSUMMON_CONDITION) return "EFFECT_SPSUMMON_CONDITION";
  if (i == EFFECT_REVIVE_LIMIT) return "EFFECT_REVIVE_LIMIT";
  if (i == EFFECT_SUMMON_PROC) return "EFFECT_SUMMON_PROC";
  if (i == EFFECT_LIMIT_SUMMON_PROC) return "EFFECT_LIMIT_SUMMON_PROC";
  if (i == EFFECT_SPSUMMON_PROC) return "EFFECT_SPSUMMON_PROC";
  if (i == EFFECT_EXTRA_SET_COUNT) return "EFFECT_EXTRA_SET_COUNT";
  if (i == EFFECT_SET_PROC) return "EFFECT_SET_PROC";
  if (i == EFFECT_LIMIT_SET_PROC) return "EFFECT_LIMIT_SET_PROC";
  if (i == EFFECT_DIVINE_LIGHT) return "EFFECT_DIVINE_LIGHT";
  if (i == EFFECT_CANNOT_DISABLE_FLIP_SUMMON) return "EFFECT_CANNOT_DISABLE_FLIP_SUMMON";
  if (i == EFFECT_INDESTRUCTABLE) return "EFFECT_INDESTRUCTABLE";
  if (i == EFFECT_INDESTRUCTABLE_EFFECT) return "EFFECT_INDESTRUCTABLE_EFFECT";
  if (i == EFFECT_INDESTRUCTABLE_BATTLE) return "EFFECT_INDESTRUCTABLE_BATTLE";
  if (i == EFFECT_UNRELEASABLE_SUM) return "EFFECT_UNRELEASABLE_SUM";
  if (i == EFFECT_UNRELEASABLE_NONSUM) return "EFFECT_UNRELEASABLE_NONSUM";
  if (i == EFFECT_DESTROY_SUBSTITUTE) return "EFFECT_DESTROY_SUBSTITUTE";
  if (i == EFFECT_CANNOT_RELEASE) return "EFFECT_CANNOT_RELEASE";
  if (i == EFFECT_INDESTRUCTABLE_COUNT) return "EFFECT_INDESTRUCTABLE_COUNT";
  if (i == EFFECT_UNRELEASABLE_EFFECT) return "EFFECT_UNRELEASABLE_EFFECT";
  if (i == EFFECT_DESTROY_REPLACE) return "EFFECT_DESTROY_REPLACE";
  if (i == EFFECT_RELEASE_REPLACE) return "EFFECT_RELEASE_REPLACE";
  if (i == EFFECT_SEND_REPLACE) return "EFFECT_SEND_REPLACE";
  if (i == EFFECT_CANNOT_DISCARD_HAND) return "EFFECT_CANNOT_DISCARD_HAND";
  if (i == EFFECT_CANNOT_DISCARD_DECK) return "EFFECT_CANNOT_DISCARD_DECK";
  if (i == EFFECT_CANNOT_USE_AS_COST) return "EFFECT_CANNOT_USE_AS_COST";
  if (i == EFFECT_CANNOT_PLACE_COUNTER) return "EFFECT_CANNOT_PLACE_COUNTER";
  if (i == EFFECT_CANNOT_TO_GRAVE_AS_COST) return "EFFECT_CANNOT_TO_GRAVE_AS_COST";
  if (i == EFFECT_LEAVE_FIELD_REDIRECT) return "EFFECT_LEAVE_FIELD_REDIRECT";
  if (i == EFFECT_TO_HAND_REDIRECT) return "EFFECT_TO_HAND_REDIRECT";
  if (i == EFFECT_TO_DECK_REDIRECT) return "EFFECT_TO_DECK_REDIRECT";
  if (i == EFFECT_TO_GRAVE_REDIRECT) return "EFFECT_TO_GRAVE_REDIRECT";
  if (i == EFFECT_REMOVE_REDIRECT) return "EFFECT_REMOVE_REDIRECT";
  if (i == EFFECT_CANNOT_TO_HAND) return "EFFECT_CANNOT_TO_HAND";
  if (i == EFFECT_CANNOT_TO_DECK) return "EFFECT_CANNOT_TO_DECK";
  if (i == EFFECT_CANNOT_REMOVE) return "EFFECT_CANNOT_REMOVE";
  if (i == EFFECT_CANNOT_TO_GRAVE) return "EFFECT_CANNOT_TO_GRAVE";
  if (i == EFFECT_CANNOT_TURN_SET) return "EFFECT_CANNOT_TURN_SET";
  if (i == EFFECT_CANNOT_BE_BATTLE_TARGET) return "EFFECT_CANNOT_BE_BATTLE_TARGET";
  if (i == EFFECT_CANNOT_BE_EFFECT_TARGET) return "EFFECT_CANNOT_BE_EFFECT_TARGET";
  if (i == EFFECT_IGNORE_BATTLE_TARGET) return "EFFECT_IGNORE_BATTLE_TARGET";
  if (i == EFFECT_CANNOT_DIRECT_ATTACK) return "EFFECT_CANNOT_DIRECT_ATTACK";
  if (i == EFFECT_DIRECT_ATTACK) return "EFFECT_DIRECT_ATTACK";
  if (i == EFFECT_DUAL_STATUS) return "EFFECT_DUAL_STATUS";
  if (i == EFFECT_EQUIP_LIMIT) return "EFFECT_EQUIP_LIMIT";
  if (i == EFFECT_DUAL_SUMMONABLE) return "EFFECT_DUAL_SUMMONABLE";
  if (i == EFFECT_UNION_LIMIT) return "EFFECT_UNION_LIMIT";
  if (i == EFFECT_REVERSE_DAMAGE) return "EFFECT_REVERSE_DAMAGE";
  if (i == EFFECT_REVERSE_RECOVER) return "EFFECT_REVERSE_RECOVER";
  if (i == EFFECT_CHANGE_DAMAGE) return "EFFECT_CHANGE_DAMAGE";
  if (i == EFFECT_REFLECT_DAMAGE) return "EFFECT_REFLECT_DAMAGE";
  if (i == EFFECT_CANNOT_ATTACK) return "EFFECT_CANNOT_ATTACK";
  if (i == EFFECT_CANNOT_ATTACK_ANNOUNCE) return "EFFECT_CANNOT_ATTACK_ANNOUNCE";
  if (i == EFFECT_CANNOT_CHANGE_POS_E) return "EFFECT_CANNOT_CHANGE_POS_E";
  if (i == EFFECT_ACTIVATE_COST) return "EFFECT_ACTIVATE_COST";
  if (i == EFFECT_SUMMON_COST) return "EFFECT_SUMMON_COST";
  if (i == EFFECT_SPSUMMON_COST) return "EFFECT_SPSUMMON_COST";
  if (i == EFFECT_FLIPSUMMON_COST) return "EFFECT_FLIPSUMMON_COST";
  if (i == EFFECT_MSET_COST) return "EFFECT_MSET_COST";
  if (i == EFFECT_SSET_COST) return "EFFECT_SSET_COST";
  if (i == EFFECT_ATTACK_COST) return "EFFECT_ATTACK_COST";

  if (i == EFFECT_UPDATE_ATTACK) return "EFFECT_UPDATE_ATTACK";
  if (i == EFFECT_SET_ATTACK) return "EFFECT_SET_ATTACK";
  if (i == EFFECT_SET_ATTACK_FINAL) return "EFFECT_SET_ATTACK_FINAL";
  if (i == EFFECT_SET_BASE_ATTACK) return "EFFECT_SET_BASE_ATTACK";
  if (i == EFFECT_UPDATE_DEFENSE) return "EFFECT_UPDATE_DEFENSE";
  if (i == EFFECT_SET_DEFENSE) return "EFFECT_SET_DEFENSE";
  if (i == EFFECT_SET_DEFENSE_FINAL) return "EFFECT_SET_DEFENSE_FINAL";
  if (i == EFFECT_SET_BASE_DEFENSE) return "EFFECT_SET_BASE_DEFENSE";
  if (i == EFFECT_REVERSE_UPDATE) return "EFFECT_REVERSE_UPDATE";
  if (i == EFFECT_SWAP_AD) return "EFFECT_SWAP_AD";
  if (i == EFFECT_SWAP_BASE_AD) return "EFFECT_SWAP_BASE_AD";
  if (i == EFFECT_SET_BASE_ATTACK_FINAL) return "EFFECT_SET_BASE_ATTACK_FINAL";
  if (i == EFFECT_SET_BASE_DEFENSE_FINAL) return "EFFECT_SET_BASE_DEFENSE_FINAL";
  if (i == EFFECT_ADD_CODE) return "EFFECT_ADD_CODE";
  if (i == EFFECT_CHANGE_CODE) return "EFFECT_CHANGE_CODE";
  if (i == EFFECT_ADD_TYPE) return "EFFECT_ADD_TYPE";
  if (i == EFFECT_REMOVE_TYPE) return "EFFECT_REMOVE_TYPE";
  if (i == EFFECT_CHANGE_TYPE) return "EFFECT_CHANGE_TYPE";
  if (i == EFFECT_ADD_RACE) return "EFFECT_ADD_RACE";
  if (i == EFFECT_REMOVE_RACE) return "EFFECT_REMOVE_RACE";
  if (i == EFFECT_CHANGE_RACE) return "EFFECT_CHANGE_RACE";
  if (i == EFFECT_ADD_ATTRIBUTE) return "EFFECT_ADD_ATTRIBUTE";
  if (i == EFFECT_REMOVE_ATTRIBUTE) return "EFFECT_REMOVE_ATTRIBUTE";
  if (i == EFFECT_CHANGE_ATTRIBUTE) return "EFFECT_CHANGE_ATTRIBUTE";
  if (i == EFFECT_UPDATE_LEVEL) return "EFFECT_UPDATE_LEVEL";
  if (i == EFFECT_CHANGE_LEVEL) return "EFFECT_CHANGE_LEVEL";
  if (i == EFFECT_UPDATE_RANK) return "EFFECT_UPDATE_RANK";
  if (i == EFFECT_CHANGE_RANK) return "EFFECT_CHANGE_RANK";
  if (i == EFFECT_UPDATE_LSCALE) return "EFFECT_UPDATE_LSCALE";
  if (i == EFFECT_CHANGE_LSCALE) return "EFFECT_CHANGE_LSCALE";
  if (i == EFFECT_UPDATE_RSCALE) return "EFFECT_UPDATE_RSCALE";
  if (i == EFFECT_CHANGE_RSCALE) return "EFFECT_CHANGE_RSCALE";
  if (i == EFFECT_SET_POSITION) return "EFFECT_SET_POSITION";
  if (i == EFFECT_SELF_DESTROY) return "EFFECT_SELF_DESTROY";
  if (i == EFFECT_SELF_TOGRAVE) return "EFFECT_SELF_TOGRAVE";
  if (i == EFFECT_DOUBLE_TRIBUTE) return "EFFECT_DOUBLE_TRIBUTE";
  if (i == EFFECT_DECREASE_TRIBUTE) return "EFFECT_DECREASE_TRIBUTE";
  if (i == EFFECT_DECREASE_TRIBUTE_SET) return "EFFECT_DECREASE_TRIBUTE_SET";
  if (i == EFFECT_EXTRA_RELEASE) return "EFFECT_EXTRA_RELEASE";
  if (i == EFFECT_TRIBUTE_LIMIT) return "EFFECT_TRIBUTE_LIMIT";
  if (i == EFFECT_EXTRA_RELEASE_SUM) return "EFFECT_EXTRA_RELEASE_SUM";
  //if (i == EFFECT_TRIPLE_TRIBUTE) return "EFFECT_TRIPLE_TRIBUTE";
  if (i == EFFECT_ADD_EXTRA_TRIBUTE) return "EFFECT_ADD_EXTRA_TRIBUTE";
  if (i == EFFECT_EXTRA_RELEASE_NONSUM) return "EFFECT_EXTRA_RELEASE_NONSUM";
  if (i == EFFECT_PUBLIC) return "EFFECT_PUBLIC";
  if (i == EFFECT_COUNTER_PERMIT) return "EFFECT_COUNTER_PERMIT";
  if (i == EFFECT_COUNTER_LIMIT) return "EFFECT_COUNTER_LIMIT";
  if (i == EFFECT_RCOUNTER_REPLACE) return "EFFECT_RCOUNTER_REPLACE";
  if (i == EFFECT_LPCOST_CHANGE) return "EFFECT_LPCOST_CHANGE";
  if (i == EFFECT_LPCOST_REPLACE) return "EFFECT_LPCOST_REPLACE";
  if (i == EFFECT_SKIP_DP) return "EFFECT_SKIP_DP";
  if (i == EFFECT_SKIP_SP) return "EFFECT_SKIP_SP";
  if (i == EFFECT_SKIP_M1) return "EFFECT_SKIP_M1";
  if (i == EFFECT_SKIP_BP) return "EFFECT_SKIP_BP";
  if (i == EFFECT_SKIP_M2) return "EFFECT_SKIP_M2";
  if (i == EFFECT_CANNOT_BP) return "EFFECT_CANNOT_BP";
  if (i == EFFECT_CANNOT_M2) return "EFFECT_CANNOT_M2";
  if (i == EFFECT_CANNOT_EP) return "EFFECT_CANNOT_EP";
  if (i == EFFECT_SKIP_TURN) return "EFFECT_SKIP_TURN";
  if (i == EFFECT_SKIP_EP) return "EFFECT_SKIP_EP";
  if (i == EFFECT_DEFENSE_ATTACK) return "EFFECT_DEFENSE_ATTACK";
  if (i == EFFECT_MUST_ATTACK) return "EFFECT_MUST_ATTACK";
  if (i == EFFECT_FIRST_ATTACK) return "EFFECT_FIRST_ATTACK";
  if (i == EFFECT_ATTACK_ALL) return "EFFECT_ATTACK_ALL";
  if (i == EFFECT_EXTRA_ATTACK) return "EFFECT_EXTRA_ATTACK";
  //if (i == EFFECT_MUST_BE_ATTACKED) return "EFFECT_MUST_BE_ATTACKED";
  if (i == EFFECT_ONLY_BE_ATTACKED) return "EFFECT_ONLY_BE_ATTACKED";
  if (i == EFFECT_ATTACK_DISABLED) return "EFFECT_ATTACK_DISABLED";
  if (i == EFFECT_NO_BATTLE_DAMAGE) return "EFFECT_NO_BATTLE_DAMAGE";
  if (i == EFFECT_AVOID_BATTLE_DAMAGE) return "EFFECT_AVOID_BATTLE_DAMAGE";
  if (i == EFFECT_REFLECT_BATTLE_DAMAGE) return "EFFECT_REFLECT_BATTLE_DAMAGE";
  if (i == EFFECT_PIERCE) return "EFFECT_PIERCE";
  if (i == EFFECT_BATTLE_DESTROY_REDIRECT) return "EFFECT_BATTLE_DESTROY_REDIRECT";
  if (i == EFFECT_BATTLE_DAMAGE_TO_EFFECT) return "EFFECT_BATTLE_DAMAGE_TO_EFFECT";
  if (i == EFFECT_BOTH_BATTLE_DAMAGE) return "EFFECT_BOTH_BATTLE_DAMAGE";
  if (i == EFFECT_ALSO_BATTLE_DAMAGE) return "EFFECT_ALSO_BATTLE_DAMAGE";
  if (i == EFFECT_CHANGE_BATTLE_DAMAGE) return "EFFECT_CHANGE_BATTLE_DAMAGE";
  if (i == EFFECT_TOSS_COIN_REPLACE) return "EFFECT_TOSS_COIN_REPLACE";
  if (i == EFFECT_TOSS_DICE_REPLACE) return "EFFECT_TOSS_DICE_REPLACE";
  if (i == EFFECT_FUSION_MATERIAL) return "EFFECT_FUSION_MATERIAL";
  if (i == EFFECT_CHAIN_MATERIAL) return "EFFECT_CHAIN_MATERIAL";
  if (i == EFFECT_EXTRA_SYNCHRO_MATERIAL) return "EFFECT_EXTRA_SYNCHRO_MATERIAL";
  if (i == EFFECT_XYZ_MATERIAL) return "EFFECT_XYZ_MATERIAL";
  if (i == EFFECT_FUSION_SUBSTITUTE) return "EFFECT_FUSION_SUBSTITUTE";
  if (i == EFFECT_CANNOT_BE_FUSION_MATERIAL) return "EFFECT_CANNOT_BE_FUSION_MATERIAL";
  if (i == EFFECT_CANNOT_BE_SYNCHRO_MATERIAL) return "EFFECT_CANNOT_BE_SYNCHRO_MATERIAL";
  if (i == EFFECT_SYNCHRO_MATERIAL_CUSTOM) return "EFFECT_SYNCHRO_MATERIAL_CUSTOM";
  if (i == EFFECT_CANNOT_BE_XYZ_MATERIAL) return "EFFECT_CANNOT_BE_XYZ_MATERIAL";
  if (i == EFFECT_CANNOT_BE_LINK_MATERIAL) return "EFFECT_CANNOT_BE_LINK_MATERIAL";
  if (i == EFFECT_SYNCHRO_LEVEL) return "EFFECT_SYNCHRO_LEVEL";
  if (i == EFFECT_RITUAL_LEVEL) return "EFFECT_RITUAL_LEVEL";
  if (i == EFFECT_XYZ_LEVEL) return "EFFECT_XYZ_LEVEL";
  if (i == EFFECT_EXTRA_RITUAL_MATERIAL) return "EFFECT_EXTRA_RITUAL_MATERIAL";
  if (i == EFFECT_NONTUNER) return "EFFECT_NONTUNER";
  if (i == EFFECT_OVERLAY_REMOVE_REPLACE) return "EFFECT_OVERLAY_REMOVE_REPLACE";
  if (i == EFFECT_SCRAP_CHIMERA) return "EFFECT_SCRAP_CHIMERA";
  if (i == EFFECT_TUNE_MAGICIAN_X) return "EFFECT_TUNE_MAGICIAN_X";
  if (i == EFFECT_PRE_MONSTER) return "EFFECT_PRE_MONSTER";
  if (i == EFFECT_MATERIAL_CHECK) return "EFFECT_MATERIAL_CHECK";
  if (i == EFFECT_DISABLE_FIELD) return "EFFECT_DISABLE_FIELD";
  if (i == EFFECT_USE_EXTRA_MZONE) return "EFFECT_USE_EXTRA_MZONE";
  if (i == EFFECT_USE_EXTRA_SZONE) return "EFFECT_USE_EXTRA_SZONE";
  if (i == EFFECT_MAX_MZONE) return "EFFECT_MAX_MZONE";
  if (i == EFFECT_MAX_SZONE) return "EFFECT_MAX_SZONE";
  if (i == EFFECT_MUST_USE_MZONE) return "EFFECT_MUST_USE_MZONE";
  if (i == EFFECT_HAND_LIMIT) return "EFFECT_HAND_LIMIT";
  if (i == EFFECT_DRAW_COUNT) return "EFFECT_DRAW_COUNT";
  if (i == EFFECT_SPIRIT_DONOT_RETURN) return "EFFECT_SPIRIT_DONOT_RETURN";
  if (i == EFFECT_SPIRIT_MAYNOT_RETURN) return "EFFECT_SPIRIT_MAYNOT_RETURN";
  if (i == EFFECT_CHANGE_ENVIRONMENT) return "EFFECT_CHANGE_ENVIRONMENT";
  if (i == EFFECT_NECRO_VALLEY) return "EFFECT_NECRO_VALLEY";
  if (i == EFFECT_FORBIDDEN) return "EFFECT_FORBIDDEN";
  if (i == EFFECT_NECRO_VALLEY_IM) return "EFFECT_NECRO_VALLEY_IM";
  if (i == EFFECT_REVERSE_DECK) return "EFFECT_REVERSE_DECK";
  if (i == EFFECT_REMOVE_BRAINWASHING) return "EFFECT_REMOVE_BRAINWASHING";
  if (i == EFFECT_BP_TWICE) return "EFFECT_BP_TWICE";
  if (i == EFFECT_UNIQUE_CHECK) return "EFFECT_UNIQUE_CHECK";
  if (i == EFFECT_MATCH_KILL) return "EFFECT_MATCH_KILL";
  if (i == EFFECT_SYNCHRO_CHECK) return "EFFECT_SYNCHRO_CHECK";
  if (i == EFFECT_QP_ACT_IN_NTPHAND) return "EFFECT_QP_ACT_IN_NTPHAND";
  if (i == EFFECT_MUST_BE_SMATERIAL) return "EFFECT_MUST_BE_SMATERIAL";
  if (i == EFFECT_TO_GRAVE_REDIRECT_CB) return "EFFECT_TO_GRAVE_REDIRECT_CB";
  if (i == EFFECT_CHANGE_INVOLVING_BATTLE_DAMAGE) return "EFFECT_CHANGE_INVOLVING_BATTLE_DAMAGE";
  //if (i == EFFECT_CHANGE_RANK_FINAL) return "EFFECT_CHANGE_RANK_FINAL";
  if (i == EFFECT_MUST_BE_FMATERIAL) return "EFFECT_MUST_BE_FMATERIAL";
  if (i == EFFECT_MUST_BE_XMATERIAL) return "EFFECT_MUST_BE_XMATERIAL";
  if (i == EFFECT_MUST_BE_LMATERIAL) return "EFFECT_MUST_BE_LMATERIAL";
  if (i == EFFECT_SPSUMMON_PROC_G) return "EFFECT_SPSUMMON_PROC_G";
  if (i == EFFECT_SPSUMMON_COUNT_LIMIT) return "EFFECT_SPSUMMON_COUNT_LIMIT";
  if (i == EFFECT_LEFT_SPSUMMON_COUNT) return "EFFECT_LEFT_SPSUMMON_COUNT";
  if (i == EFFECT_CANNOT_SELECT_BATTLE_TARGET) return "EFFECT_CANNOT_SELECT_BATTLE_TARGET";
  if (i == EFFECT_CANNOT_SELECT_EFFECT_TARGET) return "EFFECT_CANNOT_SELECT_EFFECT_TARGET";
  if (i == EFFECT_ADD_SETCODE) return "EFFECT_ADD_SETCODE";
  if (i == EFFECT_NO_EFFECT_DAMAGE) return "EFFECT_NO_EFFECT_DAMAGE";
  //if (i == EFFECT_UNSUMMONABLE_CARD) return "EFFECT_UNSUMMONABLE_CARD";
  if (i == EFFECT_DISCARD_COST_CHANGE) return "EFFECT_DISCARD_COST_CHANGE";
  if (i == EFFECT_HAND_SYNCHRO) return "EFFECT_HAND_SYNCHRO";
  if (i == EFFECT_ADD_FUSION_CODE) return "EFFECT_ADD_FUSION_CODE";
  if (i == EFFECT_ADD_FUSION_SETCODE) return "EFFECT_ADD_FUSION_SETCODE";
  if (i == EFFECT_ONLY_ATTACK_MONSTER) return "EFFECT_ONLY_ATTACK_MONSTER";
  if (i == EFFECT_MUST_ATTACK_MONSTER) return "EFFECT_MUST_ATTACK_MONSTER";
  if (i == EFFECT_PATRICIAN_OF_DARKNESS) return "EFFECT_PATRICIAN_OF_DARKNESS";
  if (i == EFFECT_EXTRA_ATTACK_MONSTER) return "EFFECT_EXTRA_ATTACK_MONSTER";
  if (i == EFFECT_UNION_STATUS) return "EFFECT_UNION_STATUS";
  if (i == EFFECT_OLDUNION_STATUS) return "EFFECT_OLDUNION_STATUS";
  //if (i == EFFECT_ADD_FUSION_ATTRIBUTE) return "EFFECT_ADD_FUSION_ATTRIBUTE";
  //if (i == EFFECT_REMOVE_FUSION_ATTRIBUTE) return "EFFECT_REMOVE_FUSION_ATTRIBUTE";
  if (i == EFFECT_CHANGE_FUSION_ATTRIBUTE) return "EFFECT_CHANGE_FUSION_ATTRIBUTE";
  if (i == EFFECT_EXTRA_FUSION_MATERIAL) return "EFFECT_EXTRA_FUSION_MATERIAL";
  if (i == EFFECT_TUNER_MATERIAL_LIMIT) return "EFFECT_TUNER_MATERIAL_LIMIT";
  if (i == EFFECT_ADD_LINK_CODE) return "EFFECT_ADD_LINK_CODE";
  //if (i == EFFECT_ADD_LINK_SETCODE) return "EFFECT_ADD_LINK_SETCODE";
  if (i == EFFECT_ADD_LINK_ATTRIBUTE) return "EFFECT_ADD_LINK_ATTRIBUTE";
  if (i == EFFECT_ADD_LINK_RACE) return "EFFECT_ADD_LINK_RACE";
  if (i == EFFECT_EXTRA_LINK_MATERIAL) return "EFFECT_EXTRA_LINK_MATERIAL";
  if (i == EFFECT_QP_ACT_IN_SET_TURN) return "EFFECT_QP_ACT_IN_SET_TURN";
  if (i == EFFECT_EXTRA_PENDULUM_SUMMON) return "EFFECT_EXTRA_PENDULUM_SUMMON";
  if (i == EFFECT_MATERIAL_LIMIT) return "EFFECT_MATERIAL_LIMIT";
  if (i == EFFECT_SET_BATTLE_ATTACK) return "EFFECT_SET_BATTLE_ATTACK";
  if (i == EFFECT_SET_BATTLE_DEFENSE) return "EFFECT_SET_BATTLE_DEFENSE";
  if (i == EFFECT_OVERLAY_RITUAL_MATERIAL) return "EFFECT_OVERLAY_RITUAL_MATERIAL";
  if (i == EFFECT_CHANGE_GRAVE_ATTRIBUTE) return "EFFECT_CHANGE_GRAVE_ATTRIBUTE";
  if (i == EFFECT_CHANGE_GRAVE_RACE) return "EFFECT_CHANGE_GRAVE_RACE";
  if (i == EFFECT_ACTIVATION_COUNT_LIMIT) return "EFFECT_ACTIVATION_COUNT_LIMIT";
  if (i == EFFECT_LIMIT_SPECIAL_SUMMON_POSITION) return "EFFECT_LIMIT_SPECIAL_SUMMON_POSITION";
  if (i == EFFECT_TUNER) return "EFFECT_TUNER";
  if (i == EFFECT_KAISER_COLOSSEUM) return "EFFECT_KAISER_COLOSSEUM";
  if (i == EFFECT_REPLACE_DAMAGE) return "EFFECT_REPLACE_DAMAGE";

  //if (i == EVENT_STARTUP) return "EVENT_STARTUP";
  if (i == EVENT_FLIP) return "EVENT_FLIP";
  if (i == EVENT_FREE_CHAIN) return "EVENT_FREE_CHAIN";
  if (i == EVENT_DESTROY) return "EVENT_DESTROY";
  if (i == EVENT_REMOVE) return "EVENT_REMOVE";
  if (i == EVENT_TO_HAND) return "EVENT_TO_HAND";
  if (i == EVENT_TO_DECK) return "EVENT_TO_DECK";
  if (i == EVENT_TO_GRAVE) return "EVENT_TO_GRAVE";
  if (i == EVENT_LEAVE_FIELD) return "EVENT_LEAVE_FIELD";
  if (i == EVENT_CHANGE_POS) return "EVENT_CHANGE_POS";
  if (i == EVENT_RELEASE) return "EVENT_RELEASE";
  if (i == EVENT_DISCARD) return "EVENT_DISCARD";
  if (i == EVENT_LEAVE_FIELD_P) return "EVENT_LEAVE_FIELD_P";
  if (i == EVENT_CHAIN_SOLVING) return "EVENT_CHAIN_SOLVING";
  if (i == EVENT_CHAIN_ACTIVATING) return "EVENT_CHAIN_ACTIVATING";
  if (i == EVENT_CHAIN_SOLVED) return "EVENT_CHAIN_SOLVED";
  //if (i == EVENT_CHAIN_ACTIVATED) return "EVENT_CHAIN_ACTIVATED";
  if (i == EVENT_CHAIN_NEGATED) return "EVENT_CHAIN_NEGATED";
  if (i == EVENT_CHAIN_DISABLED) return "EVENT_CHAIN_DISABLED";
  if (i == EVENT_CHAIN_END) return "EVENT_CHAIN_END";
  if (i == EVENT_CHAINING) return "EVENT_CHAINING";
  if (i == EVENT_BECOME_TARGET) return "EVENT_BECOME_TARGET";
  if (i == EVENT_DESTROYED) return "EVENT_DESTROYED";
  if (i == EVENT_MOVE) return "EVENT_MOVE";
  if (i == EVENT_LEAVE_GRAVE) return "EVENT_LEAVE_GRAVE";
  if (i == EVENT_LEAVE_DECK) return "EVENT_LEAVE_DECK";
  if (i == EVENT_ADJUST) return "EVENT_ADJUST";
  if (i == EVENT_BREAK_EFFECT) return "EVENT_BREAK_EFFECT";
  if (i == EVENT_SUMMON_SUCCESS) return "EVENT_SUMMON_SUCCESS";
  if (i == EVENT_FLIP_SUMMON_SUCCESS) return "EVENT_FLIP_SUMMON_SUCCESS";
  if (i == EVENT_SPSUMMON_SUCCESS) return "EVENT_SPSUMMON_SUCCESS";
  if (i == EVENT_SUMMON) return "EVENT_SUMMON";
  if (i == EVENT_FLIP_SUMMON) return "EVENT_FLIP_SUMMON";
  if (i == EVENT_SPSUMMON) return "EVENT_SPSUMMON";
  if (i == EVENT_MSET) return "EVENT_MSET";
  if (i == EVENT_SSET) return "EVENT_SSET";
  if (i == EVENT_BE_MATERIAL) return "EVENT_BE_MATERIAL";
  if (i == EVENT_BE_PRE_MATERIAL) return "EVENT_BE_PRE_MATERIAL";
  if (i == EVENT_DRAW) return "EVENT_DRAW";
  if (i == EVENT_DAMAGE) return "EVENT_DAMAGE";
  if (i == EVENT_RECOVER) return "EVENT_RECOVER";
  if (i == EVENT_PREDRAW) return "EVENT_PREDRAW";
  if (i == EVENT_SUMMON_NEGATED) return "EVENT_SUMMON_NEGATED";
  if (i == EVENT_FLIP_SUMMON_NEGATED) return "EVENT_FLIP_SUMMON_NEGATED";
  if (i == EVENT_SPSUMMON_NEGATED) return "EVENT_SPSUMMON_NEGATED";
  if (i == EVENT_SPSUMMON_SUCCESS_G_P) return "EVENT_SPSUMMON_SUCCESS_G_P";
  if (i == EVENT_CONTROL_CHANGED) return "EVENT_CONTROL_CHANGED";
  if (i == EVENT_EQUIP) return "EVENT_EQUIP";
  if (i == EVENT_ATTACK_ANNOUNCE) return "EVENT_ATTACK_ANNOUNCE";
  if (i == EVENT_BE_BATTLE_TARGET) return "EVENT_BE_BATTLE_TARGET";
  if (i == EVENT_BATTLE_START) return "EVENT_BATTLE_START";
  if (i == EVENT_BATTLE_CONFIRM) return "EVENT_BATTLE_CONFIRM";
  if (i == EVENT_PRE_DAMAGE_CALCULATE) return "EVENT_PRE_DAMAGE_CALCULATE";
  //if (i == EVENT_DAMAGE_CALCULATING) return "EVENT_DAMAGE_CALCULATING";
  if (i == EVENT_PRE_BATTLE_DAMAGE) return "EVENT_PRE_BATTLE_DAMAGE";
  //if (i == EVENT_BATTLE_END) return "EVENT_BATTLE_END";
  if (i == EVENT_BATTLED) return "EVENT_BATTLED";
  if (i == EVENT_BATTLE_DESTROYING) return "EVENT_BATTLE_DESTROYING";
  if (i == EVENT_BATTLE_DESTROYED) return "EVENT_BATTLE_DESTROYED";
  if (i == EVENT_DAMAGE_STEP_END) return "EVENT_DAMAGE_STEP_END";
  if (i == EVENT_ATTACK_DISABLED) return "EVENT_ATTACK_DISABLED";
  if (i == EVENT_BATTLE_DAMAGE) return "EVENT_BATTLE_DAMAGE";
  if (i == EVENT_TOSS_DICE) return "EVENT_TOSS_DICE";
  if (i == EVENT_TOSS_COIN) return "EVENT_TOSS_COIN";
  if (i == EVENT_TOSS_COIN_NEGATE) return "EVENT_TOSS_COIN_NEGATE";
  if (i == EVENT_TOSS_DICE_NEGATE) return "EVENT_TOSS_DICE_NEGATE";
  if (i == EVENT_LEVEL_UP) return "EVENT_LEVEL_UP";
  if (i == EVENT_PAY_LPCOST) return "EVENT_PAY_LPCOST";
  if (i == EVENT_DETACH_MATERIAL) return "EVENT_DETACH_MATERIAL";
  if (i == EVENT_TURN_END) return "EVENT_TURN_END";
  if (i == EVENT_PHASE) return "EVENT_PHASE";
  if (i == EVENT_PHASE_START) return "EVENT_PHASE_START";
  if (i == EVENT_ADD_COUNTER) return "EVENT_ADD_COUNTER";
  if (i == EVENT_REMOVE_COUNTER) return "EVENT_REMOVE_COUNTER";
  if (i == EVENT_CUSTOM) return "EVENT_CUSTOM";

  if (i == DOUBLE_DAMAGE) return "DOUBLE_DAMAGE";
  if (i == HALF_DAMAGE) return "HALF_DAMAGE";

  return ""; //code = 0 or unknown
}

std::string decodeEffectStatus(const uint16 i) {
  std::vector<std::string> v;
  if (i & EFFECT_STATUS_AVAILABLE) v.push_back("EFFECT_STATUS_AVAILABLE");
  //if (i & EFFECT_STATUS_ACTIVATED) v.push_back("EFFECT_STATUS_ACTIVATED");
  if (i & EFFECT_STATUS_SPSELF) v.push_back("EFFECT_STATUS_SPSELF");
  return joinString(v, " + ");
}

std::string decodeEffectCountCode(const uint32 i) {
  std::vector<std::string> v;
  if (i & EFFECT_COUNT_CODE_OATH) v.push_back("EFFECT_COUNT_CODE_OATH");
  if (i & EFFECT_COUNT_CODE_DUEL) v.push_back("EFFECT_COUNT_CODE_DUEL");
  if (i & EFFECT_COUNT_CODE_CHAIN) v.push_back("EFFECT_COUNT_CODE_CHAIN");
  if (i & EFFECT_COUNT_CODE_SINGLE) v.push_back("EFFECT_COUNT_CODE_SINGLE");
  return joinString(v, " + ");
}

std::string decodeEffectFlag(const uint32 i) {
  std::vector<std::string> v;
  if (i & EFFECT_FLAG_INITIAL) v.push_back("EFFECT_FLAG_INITIAL");
  if (i & EFFECT_FLAG_FUNC_VALUE) v.push_back("EFFECT_FLAG_FUNC_VALUE");
  if (i & EFFECT_FLAG_COUNT_LIMIT) v.push_back("EFFECT_FLAG_COUNT_LIMIT");
  if (i & EFFECT_FLAG_FIELD_ONLY) v.push_back("EFFECT_FLAG_FIELD_ONLY");
  if (i & EFFECT_FLAG_CARD_TARGET) v.push_back("EFFECT_FLAG_CARD_TARGET");
  if (i & EFFECT_FLAG_IGNORE_RANGE) v.push_back("EFFECT_FLAG_IGNORE_RANGE");
  if (i & EFFECT_FLAG_ABSOLUTE_TARGET) v.push_back("EFFECT_FLAG_ABSOLUTE_TARGET");
  if (i & EFFECT_FLAG_IGNORE_IMMUNE) v.push_back("EFFECT_FLAG_IGNORE_IMMUNE");
  if (i & EFFECT_FLAG_SET_AVAILABLE) v.push_back("EFFECT_FLAG_SET_AVAILABLE");
  if (i & EFFECT_FLAG_CANNOT_NEGATE) v.push_back("EFFECT_FLAG_CANNOT_NEGATE");
  if (i & EFFECT_FLAG_CANNOT_DISABLE) v.push_back("EFFECT_FLAG_CANNOT_DISABLE");
  if (i & EFFECT_FLAG_PLAYER_TARGET) v.push_back("EFFECT_FLAG_PLAYER_TARGET");
  if (i & EFFECT_FLAG_BOTH_SIDE) v.push_back("EFFECT_FLAG_BOTH_SIDE");
  if (i & EFFECT_FLAG_COPY_INHERIT) v.push_back("EFFECT_FLAG_COPY_INHERIT");
  if (i & EFFECT_FLAG_DAMAGE_STEP) v.push_back("EFFECT_FLAG_DAMAGE_STEP");
  if (i & EFFECT_FLAG_DAMAGE_CAL) v.push_back("EFFECT_FLAG_DAMAGE_CAL");
  if (i & EFFECT_FLAG_DELAY) v.push_back("EFFECT_FLAG_DELAY");
  if (i & EFFECT_FLAG_SINGLE_RANGE) v.push_back("EFFECT_FLAG_SINGLE_RANGE");
  if (i & EFFECT_FLAG_UNCOPYABLE) v.push_back("EFFECT_FLAG_UNCOPYABLE");
  if (i & EFFECT_FLAG_OATH) v.push_back("EFFECT_FLAG_OATH");
  if (i & EFFECT_FLAG_SPSUM_PARAM) v.push_back("EFFECT_FLAG_SPSUM_PARAM");
  //if (i & EFFECT_FLAG_REPEAT) v.push_back("EFFECT_FLAG_REPEAT");
  if (i & EFFECT_FLAG_NO_TURN_RESET) v.push_back("EFFECT_FLAG_NO_TURN_RESET");
  if (i & EFFECT_FLAG_EVENT_PLAYER) v.push_back("EFFECT_FLAG_EVENT_PLAYER");
  if (i & EFFECT_FLAG_OWNER_RELATE) v.push_back("EFFECT_FLAG_OWNER_RELATE");
  if (i & EFFECT_FLAG_CANNOT_INACTIVATE) v.push_back("EFFECT_FLAG_CANNOT_INACTIVATE");
  if (i & EFFECT_FLAG_CLIENT_HINT) v.push_back("EFFECT_FLAG_CLIENT_HINT");
  if (i & EFFECT_FLAG_CONTINUOUS_TARGET) v.push_back("EFFECT_FLAG_CONTINUOUS_TARGET");
  if (i & EFFECT_FLAG_LIMIT_ZONE) v.push_back("EFFECT_FLAG_LIMIT_ZONE");
  if (i & EFFECT_FLAG_ACTIVATE_CONDITION) v.push_back("EFFECT_FLAG_CONDITION");
  //if (i & EFFECT_FLAG_CVAL_CHECK) v.push_back("EFFECT_FLAG_CVAL_CHECK");
  if (i & EFFECT_FLAG_IMMEDIATELY_APPLY) v.push_back("EFFECT_FLAG_IMMEDIATELY_APPLY");
  return joinString(v, " + ");
}

std::string decodeEffectFlag2(const uint32 i) {
  std::vector<std::string> v;
  if (i & EFFECT_FLAG2_REPEAT_UPDATE) v.push_back("EFFECT_FLAG2_REPEAT_UPDATE");
  if (i & EFFECT_FLAG2_COF) v.push_back("EFFECT_FLAG2_COF");
  if (i & EFFECT_FLAG2_WICKED) v.push_back("EFFECT_FLAG2_WICKED");
  if (i & EFFECT_FLAG2_OPTION) v.push_back("EFFECT_FLAG2_OPTION");
  return joinString(v, " + ");
}

std::string decodeReset(const uint32 i) {
  std::vector<std::string> v;
  if (i & RESET_SELF_TURN) v.push_back("RESET_SELF_TURN");
  if (i & RESET_OPPO_TURN) v.push_back("RESET_OPPO_TURN");
  if (i & RESET_PHASE) v.push_back("RESET_PHASE");
  if (i & RESET_CHAIN) v.push_back("RESET_CHAIN");
  if (i & RESET_EVENT) v.push_back("RESET_EVENT");
  if (i & RESET_CARD) v.push_back("RESET_CARD");
  if (i & RESET_CODE) v.push_back("RESET_CODE");
  if (i & RESET_COPY) v.push_back("RESET_COPY");

  if (i & RESET_DISABLE) v.push_back("RESET_DISABLE");
  if (i & RESET_TURN_SET) v.push_back("RESET_TURN_SET");
  if (i & RESET_TOGRAVE) v.push_back("RESET_TOGRAVE");
  if (i & RESET_REMOVE) v.push_back("RESET_REMOVE");
  if (i & RESET_TEMP_REMOVE) v.push_back("RESET_TEMP_REMOVE");
  if (i & RESET_TOHAND) v.push_back("RESET_TOHAND");
  if (i & RESET_TODECK) v.push_back("RESET_TODECK");
  if (i & RESET_LEAVE) v.push_back("RESET_LEAVE");
  if (i & RESET_TOFIELD) v.push_back("RESET_TOFIELD");
  if (i & RESET_CONTROL) v.push_back("RESET_CONTROL");
  if (i & RESET_OVERLAY) v.push_back("RESET_OVERLAY");
  if (i & RESET_MSCHANGE) v.push_back("RESET_MSCHANGE");

  return joinString(v, "+");
}

std::string decodeMsgHint(const uint32 i) {
  if (i == 500) return "HINTMSG_RELEASE";
  if (i == 501) return "HINTMSG_DISCARD";
  if (i == 502) return "HINTMSG_DESTROY";
  if (i == 503) return "HINTMSG_REMOVE";
  if (i == 504) return "HINTMSG_TOGRAVE";
  if (i == 505) return "HINTMSG_RTOHAND";
  if (i == 506) return "HINTMSG_ATOHAND";
  if (i == 507) return "HINTMSG_TODECK";
  if (i == 508) return "HINTMSG_SUMMON";
  if (i == 509) return "HINTMSG_SPSUMMON";
  if (i == 510) return "HINTMSG_SET";
  if (i == 511) return "HINTMSG_FMATERIAL";
  if (i == 512) return "HINTMSG_SMATERIAL";
  if (i == 513) return "HINTMSG_XMATERIAL";
  if (i == 514) return "HINTMSG_FACEUP";
  if (i == 515) return "HINTMSG_FACEDOWN";
  if (i == 516) return "HINTMSG_ATTACK";
  if (i == 517) return "HINTMSG_DEFENSE";
  if (i == 518) return "HINTMSG_EQUIP";
  if (i == 519) return "HINTMSG_REMOVEXYZ";
  if (i == 520) return "HINTMSG_CONTROL";
  if (i == 521) return "HINTMSG_DESREPLACE";
  if (i == 522) return "HINTMSG_FACEUPATTACK";
  if (i == 523) return "HINTMSG_FACEUPDEFENSE";
  if (i == 524) return "HINTMSG_FACEDOWNATTACK";
  if (i == 525) return "HINTMSG_FACEDOWNDEFENSE";
  if (i == 526) return "HINTMSG_CONFIRM";
  if (i == 527) return "HINTMSG_TOFIELD";
  if (i == 528) return "HINTMSG_POSCHANGE";
  if (i == 529) return "HINTMSG_SELF";
  if (i == 530) return "HINTMSG_OPPO";
  if (i == 531) return "HINTMSG_TRIBUTE";
  if (i == 532) return "HINTMSG_DEATTACHFROM";
  if (i == 533) return "HINTMSG_LMATERIAL   ";
  if (i == 549) return "HINTMSG_ATTACKTARGET";
  if (i == 550) return "HINTMSG_EFFECT";
  if (i == 551) return "HINTMSG_TARGET";
  if (i == 552) return "HINTMSG_COIN";
  if (i == 553) return "HINTMSG_DICE";
  if (i == 554) return "HINTMSG_CARDTYPE";
  if (i == 555) return "HINTMSG_OPTION";
  if (i == 556) return "HINTMSG_RESOLVEEFFECT";
  if (i == 560) return "HINTMSG_SELECT";
  if (i == 561) return "HINTMSG_POSITION";
  if (i == 562) return "HINTMSG_ATTRIBUTE";
  if (i == 563) return "HINTMSG_RACE";
  if (i == 564) return "HINTMSG_CODE";
  if (i == 565) return "HINGMSG_NUMBER";
  if (i == 567) return "HINGMSG_LVRANK";
  if (i == 568) return "HINTMSG_RESOLVECARD";
  if (i == 569) return "HINTMSG_ZONE";
  if (i == 570) return "HINTMSG_DISABLEZONE";
  if (i == 571) return "HINTMSG_TOZONE";
  if (i == 572) return "HINTMSG_COUNTER";
  if (i == 573) return "HINTMSG_DISABLE";
  if (i == 574) return "HINTMSG_OPERATECARD";

  return "";
}

std::string decodeProcessorType(const uint16 i) {

  std::vector<std::string> v;

  if (i == PROCESSOR_NONE) v.push_back("PROCESSOR_NONE");
  if (i & PROCESSOR_WAITING) v.push_back("PROCESSOR_WAITING");
  if (i & PROCESSOR_END) v.push_back("PROCESSOR_END");

  if ((i & 0xFFFF) == PROCESSOR_ADJUST) v.push_back("PROCESSOR_ADJUST");
  if ((i & 0xFFFF) == PROCESSOR_HINT) v.push_back("PROCESSOR_HINT");
  if ((i & 0xFFFF) == PROCESSOR_TURN) v.push_back("PROCESSOR_TURN");
  if ((i & 0xFFFF) == PROCESSOR_WAIT) v.push_back("PROCESSOR_WAIT");
  if ((i & 0xFFFF) == PROCESSOR_REFRESH_LOC) v.push_back("PROCESSOR_REFRESH_LOC");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_IDLECMD) v.push_back("PROCESSOR_SELECT_IDLECMD");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_EFFECTYN) v.push_back("PROCESSOR_SELECT_EFFECTYN");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_BATTLECMD) v.push_back("PROCESSOR_SELECT_BATTLECMD");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_YESNO) v.push_back("PROCESSOR_SELECT_YESNO");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_OPTION) v.push_back("PROCESSOR_SELECT_OPTION");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_CARD) v.push_back("PROCESSOR_SELECT_CARD");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_CHAIN) v.push_back("PROCESSOR_SELECT_CHAIN");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_UNSELECT_CARD) v.push_back("PROCESSOR_SELECT_UNSELECT_CARD");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_PLACE) v.push_back("PROCESSOR_SELECT_PLACE");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_POSITION) v.push_back("PROCESSOR_SELECT_POSITION");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_TRIBUTE_P) v.push_back("PROCESSOR_SELECT_TRIBUTE_P");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_COUNTER) v.push_back("PROCESSOR_SELECT_COUNTER");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_SUM) v.push_back("PROCESSOR_SELECT_SUM");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_DISFIELD) v.push_back("PROCESSOR_SELECT_DISFIELD");
  if ((i & 0xFFFF) == PROCESSOR_SORT_CARD) v.push_back("PROCESSOR_SORT_CARD");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_RELEASE) v.push_back("PROCESSOR_SELECT_RELEASE");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_TRIBUTE) v.push_back("PROCESSOR_SELECT_TRIBUTE");
  if ((i & 0xFFFF) == PROCESSOR_POINT_EVENT) v.push_back("PROCESSOR_POINT_EVENT");
  if ((i & 0xFFFF) == PROCESSOR_QUICK_EFFECT) v.push_back("PROCESSOR_QUICK_EFFECT");
  if ((i & 0xFFFF) == PROCESSOR_IDLE_COMMAND) v.push_back("PROCESSOR_IDLE_COMMAND");
  if ((i & 0xFFFF) == PROCESSOR_PHASE_EVENT) v.push_back("PROCESSOR_PHASE_EVENT");
  if ((i & 0xFFFF) == PROCESSOR_BATTLE_COMMAND) v.push_back("PROCESSOR_BATTLE_COMMAND");
  if ((i & 0xFFFF) == PROCESSOR_DAMAGE_STEP) v.push_back("PROCESSOR_DAMAGE_STEP");
  if ((i & 0xFFFF) == PROCESSOR_ADD_CHAIN) v.push_back("PROCESSOR_ADD_CHAIN");
  if ((i & 0xFFFF) == PROCESSOR_SOLVE_CHAIN) v.push_back("PROCESSOR_SOLVE_CHAIN");
  if ((i & 0xFFFF) == PROCESSOR_SOLVE_CONTINUOUS) v.push_back("PROCESSOR_SOLVE_CONTINUOUS");
  if ((i & 0xFFFF) == PROCESSOR_EXECUTE_COST) v.push_back("PROCESSOR_EXECUTE_COST");
  if ((i & 0xFFFF) == PROCESSOR_EXECUTE_OPERATION) v.push_back("PROCESSOR_EXECUTE_OPERATION");
  if ((i & 0xFFFF) == PROCESSOR_EXECUTE_TARGET) v.push_back("PROCESSOR_EXECUTE_TARGET");
  if ((i & 0xFFFF) == PROCESSOR_DESTROY) v.push_back("PROCESSOR_DESTROY");
  if ((i & 0xFFFF) == PROCESSOR_RELEASE) v.push_back("PROCESSOR_RELEASE");
  if ((i & 0xFFFF) == PROCESSOR_SENDTO) v.push_back("PROCESSOR_SENDTO");
  if ((i & 0xFFFF) == PROCESSOR_MOVETOFIELD) v.push_back("PROCESSOR_MOVETOFIELD");
  if ((i & 0xFFFF) == PROCESSOR_CHANGEPOS) v.push_back("PROCESSOR_CHANGEPOS");
  if ((i & 0xFFFF) == PROCESSOR_OPERATION_REPLACE) v.push_back("PROCESSOR_OPERATION_REPLACE");
  if ((i & 0xFFFF) == PROCESSOR_DESTROY_REPLACE) v.push_back("PROCESSOR_DESTROY_REPLACE");
  if ((i & 0xFFFF) == PROCESSOR_RELEASE_REPLACE) v.push_back("PROCESSOR_RELEASE_REPLACE");
  if ((i & 0xFFFF) == PROCESSOR_SENDTO_REPLACE) v.push_back("PROCESSOR_SENDTO_REPLACE");
  if ((i & 0xFFFF) == PROCESSOR_SUMMON_RULE) v.push_back("PROCESSOR_SUMMON_RULE");
  if ((i & 0xFFFF) == PROCESSOR_SPSUMMON_RULE) v.push_back("PROCESSOR_SPSUMMON_RULE");
  if ((i & 0xFFFF) == PROCESSOR_SPSUMMON) v.push_back("PROCESSOR_SPSUMMON");
  if ((i & 0xFFFF) == PROCESSOR_FLIP_SUMMON) v.push_back("PROCESSOR_FLIP_SUMMON");
  if ((i & 0xFFFF) == PROCESSOR_MSET) v.push_back("PROCESSOR_MSET");
  if ((i & 0xFFFF) == PROCESSOR_SSET) v.push_back("PROCESSOR_SSET");
  if ((i & 0xFFFF) == PROCESSOR_SPSUMMON_STEP) v.push_back("PROCESSOR_SPSUMMON_STEP");
  if ((i & 0xFFFF) == PROCESSOR_SSET_G) v.push_back("PROCESSOR_SSET_G");
  if ((i & 0xFFFF) == PROCESSOR_DRAW) v.push_back("PROCESSOR_DRAW");
  if ((i & 0xFFFF) == PROCESSOR_DAMAGE) v.push_back("PROCESSOR_DAMAGE");
  if ((i & 0xFFFF) == PROCESSOR_RECOVER) v.push_back("PROCESSOR_RECOVER");
  if ((i & 0xFFFF) == PROCESSOR_EQUIP) v.push_back("PROCESSOR_EQUIP");
  if ((i & 0xFFFF) == PROCESSOR_GET_CONTROL) v.push_back("PROCESSOR_GET_CONTROL");
  if ((i & 0xFFFF) == PROCESSOR_SWAP_CONTROL) v.push_back("PROCESSOR_SWAP_CONTROL");
  if ((i & 0xFFFF) == PROCESSOR_SELF_DESTROY) v.push_back("PROCESSOR_SELF_DESTROY");
  if ((i & 0xFFFF) == PROCESSOR_TRAP_MONSTER_ADJUST) v.push_back("PROCESSOR_TRAP_MONSTER_ADJUST");
  if ((i & 0xFFFF) == PROCESSOR_PAY_LPCOST) v.push_back("PROCESSOR_PAY_LPCOST");
  if ((i & 0xFFFF) == PROCESSOR_REMOVE_COUNTER) v.push_back("PROCESSOR_REMOVE_COUNTER");
  if ((i & 0xFFFF) == PROCESSOR_ATTACK_DISABLE) v.push_back("PROCESSOR_ATTACK_DISABLE");
  if ((i & 0xFFFF) == PROCESSOR_ACTIVATE_EFFECT) v.push_back("PROCESSOR_ACTIVATE_EFFECT");
  if ((i & 0xFFFF) == PROCESSOR_ANNOUNCE_RACE) v.push_back("PROCESSOR_ANNOUNCE_RACE");
  if ((i & 0xFFFF) == PROCESSOR_ANNOUNCE_ATTRIB) v.push_back("PROCESSOR_ANNOUNCE_ATTRIB");
  if ((i & 0xFFFF) == PROCESSOR_ANNOUNCE_LEVEL) v.push_back("PROCESSOR_ANNOUNCE_LEVEL");
  if ((i & 0xFFFF) == PROCESSOR_ANNOUNCE_CARD) v.push_back("PROCESSOR_ANNOUNCE_CARD");
  if ((i & 0xFFFF) == PROCESSOR_ANNOUNCE_TYPE) v.push_back("PROCESSOR_ANNOUNCE_TYPE");
  if ((i & 0xFFFF) == PROCESSOR_ANNOUNCE_NUMBER) v.push_back("PROCESSOR_ANNOUNCE_NUMBER");
  if ((i & 0xFFFF) == PROCESSOR_ANNOUNCE_COIN) v.push_back("PROCESSOR_ANNOUNCE_COIN");
  if ((i & 0xFFFF) == PROCESSOR_TOSS_DICE) v.push_back("PROCESSOR_TOSS_DICE");
  if ((i & 0xFFFF) == PROCESSOR_TOSS_COIN) v.push_back("PROCESSOR_TOSS_COIN");
  if ((i & 0xFFFF) == PROCESSOR_ROCK_PAPER_SCISSORS) v.push_back("PROCESSOR_ROCK_PAPER_SCISSORS");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_FUSION) v.push_back("PROCESSOR_SELECT_FUSION");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_SYNCHRO) v.push_back("PROCESSOR_SELECT_SYNCHRO");
  if ((i & 0xFFFF) == PROCESSOR_SELECT_XMATERIAL) v.push_back("PROCESSOR_SELECT_XMATERIAL");
  if ((i & 0xFFFF) == PROCESSOR_DISCARD_HAND) v.push_back("PROCESSOR_DISCARD_HAND");
  if ((i & 0xFFFF) == PROCESSOR_DISCARD_DECK) v.push_back("PROCESSOR_DISCARD_DECK");
  if ((i & 0xFFFF) == PROCESSOR_SORT_DECK) v.push_back("PROCESSOR_SORT_DECK");
  if ((i & 0xFFFF) == PROCESSOR_REMOVE_OVERLAY) v.push_back("PROCESSOR_REMOVE_OVERLAY");

  return joinString(v, "+");
}



//helper function to prompt user with a yes/no question, with
// an optional arg to accept a default value, even if the user
// does not enter anything
bool confirmYN(const std::string& q) {
  bool responseValid = false;
  std::string response = "";
  while (!responseValid) {
    if (response.empty()) {
      std::cout << q << " [y/n]: ";
    } else {
      std::cout << "Invalid response. " << q << " [y/n]: ";
    }

    std::string dcsCmd = getDcsCommand().first;
    if (dcsCmd.empty()) {
      std::getline(std::cin, response);
    } else {
      std::cout << std::endl;
      response = dcsCmd;
    }

    responseValid = ((response == "Y") || (response == "y") ||
                     (response == "N") || (response == "n"));
  }

  return ((response == "Y") || (response == "y"));
}
bool confirmYN(const std::string& q, const bool def) {
  bool responseValid = false;
  std::string response = "";
  while (!responseValid) {
    if (response.empty()) {
      std::cout << q << " [y/n] (default: " <<
        ((def) ? "y" : "n") << "): ";
    } else {
      std::cout << "Invalid response. " << q << " [y/n]: ";
    }

    std::string dcsCmd = getDcsCommand().first;
    if (dcsCmd.empty()) {
      std::getline(std::cin, response);
    } else {
      std::cout << std::endl;
      response = dcsCmd;
    }

    if (response.empty())
      response = (def) ? "Y" : "N";

    responseValid = ((response == "Y") || (response == "y") ||
                     (response == "N") || (response == "n"));
  }

  return ((response == "Y") || (response == "y"));
}

//helper function to prompt user to enter a pointer
// (either as base10 or as hex if prefixed with "0x")
void* getPtrFromInput() {
  std::string ptrStr;
  intptr_t searchPtr = 0;

  std::cout << "Enter a ptr: ";
  std::getline(std::cin, ptrStr);

  if ((ptrStr.length() > 2) && (ptrStr.substr(0,2) == "0x")) {
    searchPtr = std::stoll(ptrStr, nullptr, 16);
  } else {
    searchPtr = std::stoll(ptrStr);
  }

  return (void*) searchPtr;
}

//helper function to prompt user for a ref_handle
// (or any int32, really)
int32 getRefFromInput() {
  std::string rhStr;

  std::cout << "Enter a ref handle: ";
  std::getline(std::cin, rhStr);

  return (int32) std::stoi(rhStr);
}

//returns a string with its surrounding whitespace removed
std::string trimString(const std::string& s) {
  const int l = 0;
  const int r = s.length() - 1;
  int i;
  int j;

  for (i = l; i <= r; ++i) {
    if ((s[i] != ' ') && (s[i] != '\t') &&
        (s[i] != '\n') && (s[i] != '\r')) {
      break;
    }
  }
  for (j = r; (j > i); --j) {
    if ((s[j] != ' ') && (s[j] != '\t') &&
        (s[j] != '\n') && (s[j] != '\r')) {
      break;
    }
  }

  return s.substr(i, (j - i + 1));
}

//returns a vector of strings from one string and a delimiter
std::vector<std::string> splitString(const std::string& s, const std::string& d) {
  std::vector<std::string> v;
  std::string tempStr = s;
  bool foundLast = false;

  while (!foundLast) {
    if (tempStr.find(d) != std::string::npos) {
      std::string subString = tempStr.substr(0,tempStr.find(d));
      v.push_back(subString);
      tempStr = tempStr.substr(tempStr.find(d) + d.length());
    } else {
      v.push_back(tempStr);
      foundLast = true;
    }
  }
  return v;
}

//combines a vector of strings to one string with an optional delimiter
std::string joinString(const std::vector<std::string>& v) {
  return joinString(v, ",");
}
std::string joinString(const std::vector<std::string>& v, const std::string& d) {
  std::string s;
  bool b = false;
  for (const auto &i: v) {
    if (b)
      s += d;
    else
      b = true;
    s += i;
  }
  return s;
}

//more specifically, case insensitive search
bool searchString(const std::string& h, const std::string& n) {
  std::string hLower = h;
  std::string nLower = n;

  for (unsigned int i = 0; i < hLower.size(); ++i)
    hLower[i] = std::tolower((unsigned char) hLower[i]);

  for (unsigned int i = 0; i < nLower.size(); ++i)
    nLower[i] = std::tolower((unsigned char) nLower[i]);

  return (hLower.find(nLower) != std::string::npos);
}

//returns a string, stripped of surrounding matching quotes, if any
std::string dequoteString(const std::string& s) {
  if (s.length() > 1) {
    if (s[0] == '\'')
      if (s[s.length() - 1] == '\'')
        return s.substr(1, s.length() - 2);

    if (s[0] == '"')
      if (s[s.length() - 1] == '"')
        return s.substr(1, s.length() - 2);
  }
  return s;
}

//returns a json file from a path (currently only used for conf file)
nlohmann::json getJson(const std::filesystem::path& jsonPath) {

  nlohmann::json empty;

  if (!std::filesystem::exists(jsonPath)) {
    clog("e", "Could not find config file ", jsonPath, ".");
    return empty;
  }

  if (!std::filesystem::is_regular_file(jsonPath)) {
    clog("e", "'", jsonPath, "' does not appear to be a file.");
    return empty;
  }

  std::ifstream ifs(jsonPath);
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return nlohmann::json::parse(buffer.str());;
}

//set up singleton variables from conf file path with option to
// print outputs or not
// (errors will be printed regardless; can be called without args
//  to default to ddd_conf.json in the same directory as conf file)
bool dddGsInit() {
  return dddGsInit("./ddd_conf.json", false);
}
bool dddGsInit(const std::filesystem::path& confPath, const bool showNonErrors) {

  try {
    getDDD_GS().conf.confPath = confPath;
    auto confJson = getJson(getDDD_GS().conf.confPath);

    if (confJson.empty()) {
      clog("e", "Conf file at '", getDDD_GS().conf.confPath,
           "' empty or invalid.");
      getDDD_GS().conf.lastInitSuccess = false;
      return false;
    }

    std::vector<std::filesystem::path> basePathCandidates;
    basePathCandidates.push_back(getDDD_GS().conf.confPath.parent_path());
    basePathCandidates.push_back(std::filesystem::current_path());

    //lambda declaration to check if a path contains a file
    auto getPathForFile = [&basePathCandidates, &confJson] (std::string initialPath) {
      std::filesystem::path initialCandidate =
        std::filesystem::path( std::string(confJson[initialPath]) );

      auto checkPath = [](std::filesystem::path p) {
        return ((std::filesystem::exists(p)) &&
                (std::filesystem::is_regular_file(p)));
      };

      if (checkPath(initialCandidate))
        return initialCandidate;

      if (initialCandidate.is_relative()) {
        for (const auto &bpc: basePathCandidates) {
          auto newCandidatePath = bpc / initialCandidate;
          if (checkPath(newCandidatePath))
            return newCandidatePath;
        }
      }
      //no suitable path found
      return std::filesystem::path(); //return empty path
    };


    if (!confJson.contains("cardsCdbPath")) {
      clog("e", "Field 'cardsCdbPath' not found in conf file; exiting.");
      getDDD_GS().conf.lastInitSuccess = false;
      return false;

    } else {
      auto pathCandidate = getPathForFile("cardsCdbPath");
      if (pathCandidate.empty()) {
        clog("e", "Unable to find or invalid path for 'cardsCdbpath'"
             " in conf file; exiting.");
        getDDD_GS().conf.lastInitSuccess = false;
        return false;

      }
      getDDD_GS().conf.cardsCdbPath = pathCandidate;

      if (showNonErrors)
        clog("d", "Found path for cardsCdbPath: ", pathCandidate);
    }

    if (!confJson.contains("player0DeckPath")) {
      clog("e", "Field 'player0DeckPath' not found in conf file;"
           " exiting.");
      getDDD_GS().conf.lastInitSuccess = false;
      return false;

    } else {
      auto pathCandidate = getPathForFile("player0DeckPath");
      if (pathCandidate.empty()) {
        clog("e", "Unable to find or invalid path for"
             " 'player0DeckPath' in conf file; exiting.");
        getDDD_GS().conf.lastInitSuccess = false;
        return false;

      }
      getDDD_GS().conf.player0DeckPath = pathCandidate;

      if (showNonErrors)
        clog("d", "Found path for player0DeckPath: ", pathCandidate);
    }

    if (!confJson.contains("player1DeckPath")) {
      clog("e", "Field 'player1DeckPath' not found in conf file;"
           " exiting.");
      getDDD_GS().conf.lastInitSuccess = false;
      return false;

    } else {
      auto pathCandidate = getPathForFile("player1DeckPath");
      if (pathCandidate.empty()) {
        clog("e", "Unable to find or invalid path for"
             " 'player1DeckPath' in conf file; exiting.");
        getDDD_GS().conf.lastInitSuccess = false;
        return false;

      }
      getDDD_GS().conf.player1DeckPath = pathCandidate;

      if (showNonErrors)
        clog("d", "Found path for player1DeckPath: ", pathCandidate);
    }

    if (confJson.contains("player0Lp")) {
      getDDD_GS().conf.player0Lp = confJson["player0Lp"];
    } else {
      getDDD_GS().conf.player0Lp = 8000;
    }

    if (confJson.contains("player1Lp")) {
      getDDD_GS().conf.player1Lp = confJson["player1Lp"];
    } else {
      getDDD_GS().conf.player1Lp = 8000;
    }

    if (confJson.contains("player0StartDraw")) {
      getDDD_GS().conf.player0StartDraw = confJson["player0StartDraw"];
    } else {
      getDDD_GS().conf.player0StartDraw = 5;
    }

    if (confJson.contains("player1StartDraw")) {
      getDDD_GS().conf.player1StartDraw = confJson["player1StartDraw"];
    } else {
      getDDD_GS().conf.player1StartDraw = 5;
    }

    if (confJson.contains("player0DrawPerTurn")) {
      getDDD_GS().conf.player0DrawPerTurn = confJson["player0DrawPerTurn"];
    } else {
      getDDD_GS().conf.player0DrawPerTurn = 1;
    }

    if (confJson.contains("player1DrawPerTurn")) {
      getDDD_GS().conf.player1DrawPerTurn = confJson["player1DrawPerTurn"];
    } else {
      getDDD_GS().conf.player1DrawPerTurn = 1;
    }


    if (confJson.contains("autoStart")) {
      getDDD_GS().conf.autoStart = confJson["autoStart"];
    } else {
      getDDD_GS().conf.autoStart = false;
    }

    if (confJson.contains("autoProcess")) {
      getDDD_GS().conf.autoProcess = confJson["autoProcess"];
    } else {
      getDDD_GS().conf.autoProcess = false;
    }

    if (confJson.contains("customLuaScriptsDir")) {
      getDDD_GS().conf.customLuaScriptsDir
        = std::string(confJson["customLuaScriptsDir"]);
    } else {
      getDDD_GS().conf.customLuaScriptsDir = SANDBOX_DIR "/lua_scripts";
    }

    if (confJson.contains("dcsScriptsDir")) {
      getDDD_GS().conf.dcsScriptsDir
        = std::string(confJson["dcsScriptsDir"]);
    } else {
      getDDD_GS().conf.dcsScriptsDir = SANDBOX_DIR "/lua_scripts";
    }

    if (confJson.contains("debug")) {
      getDDD_GS().conf.debug = initDebug(confJson["debug"]);
    }

    if (confJson.contains("useFixedSeed")) {
      getDDD_GS().conf.useFixedSeed = confJson["useFixedSeed"];
      if (confJson.contains("seed")) {
        if (getDDD_GS().conf.useFixedSeed) {
          getDDD_GS().conf.seed = confJson["seed"];
        } else {
          getDDD_GS().conf.seed = 0;
        }
      } else {
        clog("w", "'useFixedseed' found and set to true but no seed"
             " was provided; using seed = 0.");
        getDDD_GS().conf.seed = 0;
      }
    } else {
      getDDD_GS().conf.useFixedSeed = false;
      getDDD_GS().conf.seed = 0;
    }

    if (confJson.contains("echoDcsCommand")) {
      getDDD_GS().conf.echoDcsCommand = confJson["echoDcsCommand"];
    } else {
      getDDD_GS().conf.echoDcsCommand = false;
    }

    if (confJson.contains("log")) {
      getDDD_GS().conf.log = initLog(confJson["log"]);
    }

    if (!readCardsDb(showNonErrors)) { //needed for card_reader
      getDDD_GS().conf.lastInitSuccess = false;
      return false;
    }

    //non conf related functions to be set (for ocgapi)
    set_script_reader((script_reader) dddScriptReader); //read lua scripts
    set_card_reader((card_reader) dddCardReader); //read card info from cards.cdb
    set_message_handler((message_handler) dddMessageHandler); //print messages as soon as they're received

    getDDD_GS().conf.lastInitSuccess = true;
    return true;

  } catch (std::exception &e) {
    clog("e", e.what());
    getDDD_GS().conf.lastInitSuccess = false;
    return false;
  }
}

//set up debug singleton variables from conf file
ConfigDebug initDebug(const nlohmann::json& debugJson) {
  ConfigDebug cd;

  if (!debugJson.is_object()) {
    clog("w", "Field 'debug' in conf file is invalid; ignoring.");
    return cd;
  }

  const std::vector<std::string> debugPrintAttributes = {
    "printCardAttributes",
    "printGroupAttributes",
    "printEffectAttributes"
  };
  for (const auto &dpa: debugPrintAttributes) {
    bool isDpaValid = true;
    std::set<std::string> dpaSet;

    if (debugJson.contains(dpa)) {
      if (!debugJson[dpa].is_array()) {
        clog("w", "Field debug.", dpa,
             " in conf file is invalid; ignoring.");
        isDpaValid = false;
        break;
      }
      for (const auto &a: debugJson[dpa]) {
        if (a.is_string()) {
          std::string s = a; //needed to remove ambiguity
          dpaSet.insert(s);
        } else {
          clog("w", "Field debug.", dpa, " contains an invalid"
               " attribute; ignoring that attribute.");
        }
      }
    }

    if (isDpaValid) {
      if (dpa == "printCardAttributes") {
        cd.printCardAttributes = dpaSet;
      } else if (dpa == "printGroupAttributes") {
        cd.printGroupAttributes = dpaSet;
      } else if (dpa == "printEffectAttributes") {
        cd.printEffectAttributes = dpaSet;
      }
    }
  }
  if (debugJson.contains("printGamestateChanges")) {
    cd.printGamestateChanges = debugJson["printGamestateChanges"];
  } else {
    cd.printGamestateChanges = false;
  }
  return cd;
}

//set up logging singleton variables from conf file
ConfigLog initLog(const nlohmann::json& logJson) {
  ConfigLog cl;

  if (!logJson.is_object()) {
    clog("w", "Field 'log' in conf file is invalid; ignoring.");
    return cl;
  }

  std::vector<std::string> logAttributes = {
    "highlight", "filter"
  };
  for (const auto &la: logAttributes) {
    bool isLaValid = true;
    //std::vector<std::string> laVec;
    std::set<std::string> laSet;

    if (logJson.contains(la)) {
      if (!logJson[la].is_array()) {
        clog("w", "Field debug.", la,
             " in conf file is invalid; ignoring.");
        isLaValid = false;
        break;
      }

      for (const auto &a: logJson[la]) {
        if (a.is_string()) {
          std::string s = a; //needed to remove ambiguity
          laSet.insert(s);
        } else {
          clog("w", "Field log.", la,
               " contains an invalid string; ignoring.");
        }
      }
    }
    if (isLaValid) {
      if (la == "highlight") {
        cl.highlights = laSet;
      } else if (la == "filter") {
        cl.filters = laSet;
      }
    }
  }
#ifdef CLOG_FORCE_PRINTF
  cl.forcePrintf = true;
#else
  if (logJson.contains("forcePrintf")) {
    cl.forcePrintf = logJson["forcePrintf"];
  } else {
    cl.forcePrintf = true;
  }
#endif

  if (logJson.contains("highlightOverridesFilter")) {
    cl.highlightOverridesFilter = logJson["highlightOverridesFilter"];
  } else {
    cl.highlightOverridesFilter = true;
  }
  if (logJson.contains("logToFile")) {
    cl.logToFile = logJson["logToFile"];
  } else {
    cl.logToFile = false;
  }
  if (logJson.contains("logFile")) {
    cl.logFile = std::string(logJson["logFile"]);
  } else {
    cl.logFile = "log.txt";
  }
  if (logJson.contains("cacheLogs")) {
    cl.cacheLogs = logJson["cacheLogs"];
  } else {
    cl.cacheLogs = false;
  }
  return cl;
}

//define script reader function, to be used with
// ocgapi's set_script_reader function
// (called in dddGsInit function;
//  uses and calls ocgapi's default_script_reader function)
byte* dddScriptReader(const char* scriptName, int* len) {

  try {
    std::filesystem::path foundPath;
    std::filesystem::path scriptNamePath = std::filesystem::path(scriptName);

    if (std::filesystem::exists(scriptNamePath)) {
      foundPath = scriptNamePath;
    } else if (scriptNamePath.is_relative()) {
      auto candidatePath1 = std::filesystem::current_path() / scriptNamePath;

      if (std::filesystem::exists(candidatePath1)) {
        foundPath = candidatePath1;
      } else {
        auto candidatePath2 = getDDD_GS().conf.confPath.parent_path()
          / scriptNamePath;
        if (std::filesystem::exists(candidatePath2)) {
          foundPath = candidatePath2;
        }
      }
    }

    if (foundPath.empty()) {

      //ignore temp_card made by duel (why does it even do this?)
      if (std::string(scriptName).find("c0.lua") == std::string::npos) {

        //ignore normal monsters (that are also not pendulums)
        uint32 code = getLastNumberFromString(std::string(scriptName));
        bool isNormalMonster = false;

        if (getDDD_GS().cardDbs.cardDb.find(code) !=
            getDDD_GS().cardDbs.cardDb.end()) {
          uint32 type = getDDD_GS().cardDbs.cardDb.find(code)->second.type;
          if ((type & TYPE_NORMAL) && !(type & TYPE_PENDULUM))
            isNormalMonster = true;
        }

        if (!isNormalMonster) { //check
          clog("e", "Unable to find script with filename '", scriptName, "'.");
          clog("d", "Card db size: ", getDDD_GS().cardDbs.cardDb.size());
        }
      }
      return nullptr;
    }

    auto dsr = default_script_reader(foundPath.string().c_str(), len);
    if (dsr == nullptr) {
      clog("e", "Unable to read script ", foundPath.string().c_str(), ".");
    }
    return dsr;

  } catch (std::exception &e) {
    clog("e", e.what());
    return nullptr;
  }
}

//define card reader function, to be used with
// ocgapi's set_card_reader function
// (read card loaded into card map in singleton)
uint32 dddCardReader(unsigned int code, card_data* data) {
  if (getDDD_GS().cardDbs.cardDb.find(code) !=
      getDDD_GS().cardDbs.cardDb.end()) {
    //set the value at address pointed to by "data"
    *data = getDDD_GS().cardDbs.cardDb.at(code);
  }
  return 0;
}

//define message handler function, to be used with
// ocgapi's set_message_handler function
uint32 dddMessageHandler(intptr_t pDuel, uint32 msg) {
  //not sure if this deals with duel messages or log messages but
  // seems to only be called by ocgcore to report certain errors
  // (msg always 1 except 1 case involving libdebug) so can just
  // log these here actually deal with the duel and log messages
  // in their own specialized and custom functions instead of here

  char buf[MAX_OCGCORE_BUFFER_SIZE];
  get_log_message(pDuel, buf);
  buf[MAX_OCGCORE_BUFFER_SIZE - 1] = '\0';

  std::string s = std::string(buf);
  clog("e", "[message handler for ", pDuel, " (duel*(", pDuel,
       "))]: '", s, "'");

  return s.size();
}

//populate card maps in singleton, querying from the 2 sqlite tables
// in cards.db
bool readCardsDb(const bool showNonErrors) {
  std::filesystem::path cdbPath = getDDD_GS().conf.cardsCdbPath;

  if (!std::filesystem::exists(cdbPath)) {
    clog("e", "Could not find card db at '", cdbPath, "'.");
    return false;
  }

  sqlite3* db;
  int rc;
  sqlite3_stmt* stmt;

  //open db
  rc = sqlite3_open_v2(cdbPath.string().c_str(),
                       &db, SQLITE_OPEN_READONLY, NULL);
  if (rc) {
    clog("e",": Unable to open cards.db '", cdbPath, "'.");
    return false;
  }

  if (showNonErrors)
    clog("ok","Cards db ", cdbPath, " opened; reading db...");

  std::string datasSql = "SELECT id, ot, alias, setcode, type, atk,"
    " def, level, race, attribute, category FROM datas;";

  sqlite3_prepare_v2(db, datasSql.c_str(), datasSql.length(), &stmt, NULL);
  rc = sqlite3_step(stmt);

  while (rc == SQLITE_ROW) {
    card_data cd;

    cd.code = sqlite3_column_int(stmt, 0);
    cd.alias = sqlite3_column_int(stmt, 2);
    cd.set_setcode(sqlite3_column_int64(stmt, 3));
    cd.type = sqlite3_column_int(stmt, 4);
    //cd.level = sqlite3_column_int(stmt, 7);
    cd.level = (((unsigned int) sqlite3_column_int(stmt, 7)) & 0xFFFF);
    cd.attribute = sqlite3_column_int(stmt, 9);
    cd.race = sqlite3_column_int(stmt, 8);
    cd.attack = sqlite3_column_int(stmt, 5);
    cd.defense = sqlite3_column_int(stmt, 6);
    //cd.link_marker = (unsigned int) sqlite3_column_int(stmt, 11);

    if (cd.type & TYPE_PENDULUM) {
      cd.lscale = ((((unsigned int) sqlite3_column_int(stmt, 7))
                    & 0xFF0000) >> 16);
      cd.rscale = ((((unsigned int) sqlite3_column_int(stmt, 7))
                    & 0xFF000000) >> 24);
    } else {
      //unless -1 is a thing...?
      cd.lscale = 0;
      cd.rscale = 0;
    }

    if (cd.type & TYPE_LINK) {
      cd.defense = 0;
      cd.link_marker = sqlite3_column_int(stmt, 6);
    } else {
      cd.link_marker = 0;
    }

    std::string textsSql = "SELECT name, desc, str1, str2, str3, str4,"
      "str5, str6, str7, str8, str9, str10, str11, str12, str13, str14,"
      " str15, str16 FROM texts WHERE id = "
      + std::to_string(cd.code) + ";";
    sqlite3_stmt* stmt2;
    std::string cardName = "?";
    card_texts ct;
    sqlite3_prepare_v2(db, textsSql.c_str(), textsSql.length(), &stmt2, NULL);
    int rc2 = sqlite3_step(stmt2);
    if (rc2 == SQLITE_ROW) {
      cardName = (char*) sqlite3_column_text(stmt2, 0);
      ct.name = cardName;
      ct.cardText = (char*) sqlite3_column_text(stmt2, 1);

      for (int i = 0; i < 14; ++i) {
        ct.cardStrs[i] = (char*) sqlite3_column_text(stmt2, (i + 2));
      }
    }

    getDDD_GS().cardDbs.cardDb.insert(std::make_pair(cd.code, cd));
    getDDD_GS().cardDbs.cardTextsDb.insert(std::make_pair(cd.code, ct));

    rc = sqlite3_step(stmt);
  }

  if (showNonErrors)
    clog("ok","Added '", getDDD_GS().cardDbs.cardDb.size(), "' cards to dbs.");

  return true;
}


//read and parse a ydk file into a struct containing 3 vectors
DeckCards readYdk(const std::filesystem::path& ydkFilename) {

  DeckCards dt;

  if (!std::filesystem::exists(ydkFilename)) {
    clog("e", "Could not find deck ydk at '", ydkFilename, "'.");
    return dt;
  }

  try {
    std::ifstream ifs(ydkFilename);
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    std::string ydkStr = buffer.str();

    bool noMoreNewlines = false;
    bool addingToMain = false;
    bool addingToExtra = false;
    bool addingToSide = false;

    while (!noMoreNewlines) {
      std::string subString;
      auto nl = ydkStr.find("\n");
      if (nl != std::string::npos) {
        subString = ydkStr.substr(0, nl);
        if ((subString.size() > 0) && (subString.back() == '\r'))
          subString.pop_back();

      } else {
        subString = ydkStr;
        noMoreNewlines = true;
      }

      ydkStr = ydkStr.substr(nl + 1);

      //toggle flags when appropriate
      if (subString.empty()) {
        continue;
      } else if (subString == "#main") {
        addingToMain = true; addingToExtra = false; addingToSide = false;
        continue;
      } else if (subString == "#extra") {
        addingToMain = false; addingToExtra = true; addingToSide = false;
        continue;
      } else if (subString == "!side") {
        addingToMain = false; addingToExtra = false; addingToSide = true;
        continue;
      } else if (subString[0] == '#') {
        continue;
      }

      unsigned int ui = std::stoi(subString);

      //add code to deck based on currently active flag
      if (addingToMain) {
        dt.main.push_back(ui);
      } else if (addingToExtra) {
        dt.extra.push_back(ui);
      } else if (addingToSide) {
        dt.side.push_back(ui);
      } else {
        clog("w", "Unable to handle input '", subString, "'.");
        return DeckCards();
      }
    }

    return dt;

  } catch (std::exception &e) {
    clog("e", e.what());
    return DeckCards();
  }
}

//set up duel based on conf and cards loaded and return the pDuel
// to the duel
intptr_t setUpAndStartDuel(const bool showNonErrors, const bool useSpecifiedSeed, const unsigned int inSeed) {

  //create the duel
  if (showNonErrors)
    clog("i","Creating duel...");

  unsigned int seed;
  //set up a random seed for duel if not using fixed seed
  if ((!useSpecifiedSeed) && (!getDDD_GS().conf.useFixedSeed)) {
    std::random_device rd;
    seed = rd();
  } else if (!useSpecifiedSeed) {
    seed = getDDD_GS().conf.seed;
  } else {
    seed = inSeed;
  }

  intptr_t pDuel = create_duel(seed);
  duel* pD = (duel*) pDuel;

  if (!pDuel) {
    clog("e", "Unable to create duel (create_duel returned 0).");
    return false;
  }

  if (showNonErrors) {
    clog("ok", "Duel created!");
    clog("l", "(ptr to duel = ", pDuel, " (", pD, ");  seed = ",
         seed, ")");
  }

  //set some player info for both players
  set_player_info(pDuel, 0,
                  getDDD_GS().conf.player0Lp,
                  getDDD_GS().conf.player0StartDraw,
                  getDDD_GS().conf.player0DrawPerTurn);
  set_player_info(pDuel, 1,
                  getDDD_GS().conf.player1Lp,
                  getDDD_GS().conf.player1StartDraw,
                  getDDD_GS().conf.player1DrawPerTurn);

  //set options relating to duel
  int options = 0; // bitwise OR any options you want

  //load cards into deck structs
  auto deck0 = readYdk(getDDD_GS().conf.player0DeckPath);
  auto deck1 = readYdk(getDDD_GS().conf.player1DeckPath);

  //shuffle (main) decks before loading them
  pD->random.shuffle_vector(deck0.main);
  pD->random.shuffle_vector(deck1.main);


  //load cards from deck structs into
  for (const auto cardId : deck0.main) {
    new_card(pDuel, cardId, 0, 0,
             LOCATION_DECK, SEQ_DECKTOP, POS_FACEDOWN_DEFENSE);
  }
  for (const auto cardId : deck0.extra) {
    new_card(pDuel, cardId, 0, 0,
             LOCATION_EXTRA, SEQ_DECKTOP, POS_FACEDOWN_DEFENSE);
  }
  for (const auto cardId : deck0.side) {
  }
  for (const auto cardId : deck1.main) {
    new_card(pDuel, cardId, 1, 1,
             LOCATION_DECK, SEQ_DECKTOP, POS_FACEDOWN_DEFENSE);
  }
  for (const auto cardId : deck1.extra) {
    new_card(pDuel, cardId, 1, 1,
             LOCATION_EXTRA, SEQ_DECKTOP, POS_FACEDOWN_DEFENSE);
  }
  for (const auto cardId : deck1.side) {
  }

  //actually start the duel that was created
  start_duel(pDuel, options);

  return pDuel;
}

//execute a lua script into the specfied lua state
void executeSimpleLuaScript(lua_State* L, std::string scriptNameStr) {
  if ((scriptNameStr.length() <= 4) ||
      (scriptNameStr.substr(scriptNameStr.length() - 4) != ".lua"))
    scriptNameStr += ".lua";

  std::filesystem::path scriptDirPref = getDDD_GS().conf.customLuaScriptsDir;
  std::filesystem::path scriptPath = scriptDirPref / scriptNameStr;
  if (!std::filesystem::exists(scriptPath)) {
    clog("w", "Unable to find '", scriptNameStr, "' in ",
         scriptDirPref, " dir; double check the path?");
    return;
  }

  int len = 0;
  const int maxLength = 2048;
  byte* bytes = read_script(scriptPath.string().c_str(), &len);
  char buffer[maxLength + 1];

  if (len > maxLength) {
    len = maxLength;
    clog("w", "Script length for '", scriptNameStr,
         "' exceeds char limit of ", maxLength,
         "; script may be truncated and invalidated.");
  }
  memcpy(buffer, bytes, len);
  buffer[len + 1] = '\0';

  clog("ok", "Selected script '", scriptNameStr, "' (len = ", len, ").");

  //modification of macro luaL_dofile; see notes for it in manual
  int status = (luaL_loadbuffer(L, buffer, len, scriptNameStr.c_str()) ||
                lua_pcall(L, 0, LUA_MULTRET, 0));

  if (status != LUA_OK) {
    clog("e", "Something went wrong...");
    return;
  }

  clog("d", "Script finished.");
}

//execute a function in a lua script into the specified lua state
void loadAndCallFromLuaScript(lua_State* L, std::string scriptNameStr, const std::string& funcName) {
  if ((scriptNameStr.length() <= 4) ||
      (scriptNameStr.substr(scriptNameStr.length() - 4) != ".lua"))
    scriptNameStr += ".lua";

  std::filesystem::path scriptDirPref = getDDD_GS().conf.customLuaScriptsDir;
  std::filesystem::path scriptPath = scriptDirPref / scriptNameStr;
  if (!std::filesystem::exists(scriptPath)) {
    clog("w", "Unable to find '", scriptNameStr, "' in ",
         scriptDirPref, " dir; double check the path?");
    return;
  }

  int len = 0;
  const int maxLength = 2048;
  byte* bytes = read_script(scriptPath.string().c_str(), &len);
  char buffer[maxLength + 1];

  if (len > maxLength) {
    len = maxLength;
    clog("w", "Script length for '", scriptNameStr,
         "' exceeds char limit of ", maxLength,
         "; script may be truncated and invalidated.");
  }
  memcpy(buffer, bytes, len);
  buffer[len + 1] = '\0';

  clog("ok", "Selected script '", scriptNameStr, "' (len = ", len, ").");


  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "lua_typename = ", lua_typename(L, -1));
  clog("d", "Before anything\n");

  luaL_openlibs(L);
  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "lua_typename = ", lua_typename(L, -1));
  clog("d", "After loading debug library.\n");


  //load functions (actually a chunk with all functions)
  int status = (luaL_loadbuffer(L, buffer, len, scriptNameStr.c_str()));
  if (status != LUA_OK) {
    clog("e", "Something went wrong...");
    return;
  }
  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "luaL_typename = ", luaL_typename(L, -1));
  clog("d", "After luaL_loadbuffer()\n");

  //run the chunk
  status = lua_pcall(L, 0, LUA_MULTRET, 0);
  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "luaL_typename = ", luaL_typename(L, -1));
  clog("d", "After lua_pcall()\n");

  //get a func
  lua_getglobal(L, funcName.c_str());
  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "luaL_typename = ", luaL_typename(L, -1));
  clog("d", "After lua_getglobal()\n");

  //push an arg
  int arg = 57;
  lua_pushinteger(L, arg);
  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "luaL_typename = ", luaL_typename(L, -1));
  clog("d", "After lua_pushinteger()\n");

  //call func
  lua_pcall(L, 1, 1, 0);
  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "luaL_typename = ", luaL_typename(L, -1));
  clog("d", "After lua_pcall()\n");

  //get value at top of stack
  int result_i = lua_tonumber(L, -1);
  std::string result_s = lua_tostring(L, -1);
  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "luaL_typename = ", luaL_typename(L, -1));
  clog("d", "result_i = ", result_i);
  clog("d", "result_s = ", result_s);
  clog("d", "After lua_tonumber()\n");

  //pop once
  lua_pop(L, 1);
  clog("d", "lua stack size = ", lua_gettop(L));
  clog("d", "luaL_typename = ", luaL_typename(L, -1));
  clog("d", "After lua_pop()\n");


  clog("d", "Script finished.");
}

//get the rightmost number in a string if it exists
// (returns 0 if no number found)
int getLastNumberFromString(const std::string& input) {
  std::deque<char> buffer;
  bool foundDigit = false;

  for (auto i = input.rbegin(); i != input.rend(); ++i) {
    switch (*i) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      {
        foundDigit = true;
        buffer.push_front(*i);
        break;
      }
    default:
      if (foundDigit)
        break;
    }
  }

  if (!foundDigit)
    return 0;

  std::string s = std::string(buffer.begin(), buffer.end());
  int result = 0;

  try {
    result = std::stoi(s);
  } catch (std::exception& e) {
    clog("e", e.what());
    return 0;
  }

  return result;
}

//parses a string into a bool value
// (accepting a number, "true" or "false";
//  otherwise throws an exception if cannot be parsed)
bool parseStringToBool(const std::string& s) {
  if (dequoteString(s) == "true")
    return true;
  else if (dequoteString(s) == "false")
    return false;
  else
    return !!std::stoi(s);
}

//loads a dcs script, parses it for commands (while ignoring comments)
// and saves the raw commands in the singleton's command cache
// (raw commands as a string to be parsed later)
void loadDcsCommandsFromScript(std::filesystem::path scriptPath) {

  if (scriptPath.empty()) {
    std::string inputStr;
    std::cout << "Enter dcs script to load: ";
    std::getline(std::cin, inputStr);

    scriptPath = inputStr;
  }

  std::filesystem::path testPath;
  std::filesystem::path confParentPath =
    getDDD_GS().conf.confPath.parent_path();
  std::vector<std::filesystem::path> candidatePaths;

  testPath = scriptPath;
  candidatePaths.push_back(testPath);
  testPath = scriptPath;
  testPath.concat(".dcs");
  candidatePaths.push_back(testPath);

  if (!scriptPath.is_absolute()) {
    testPath = getDDD_GS().conf.dcsScriptsDir / scriptPath;
    candidatePaths.push_back(testPath);
    testPath = getDDD_GS().conf.dcsScriptsDir / scriptPath;
    testPath.concat(".dcs");
    candidatePaths.push_back(testPath);
    testPath = confParentPath / scriptPath;
    candidatePaths.push_back(testPath);
    testPath = confParentPath / scriptPath;
    testPath.concat(".dcs");
    candidatePaths.push_back(testPath);
    testPath = confParentPath / SANDBOX_DIR / scriptPath;
    candidatePaths.push_back(testPath);
    testPath = confParentPath / SANDBOX_DIR / scriptPath;
    testPath.concat(".dcs");
    candidatePaths.push_back(testPath);
  }

  for (const auto &p: candidatePaths) {
    if (std::filesystem::exists(p)) {
      scriptPath = p;
      break;
    }
  }

  if (!std::filesystem::exists(scriptPath)) {
    clog("e", "Unable to find script '", scriptPath, "'.");
    return;
  }

  if (!std::filesystem::is_regular_file(scriptPath)) {
    clog("e", "Script '", scriptPath, "' does not appear to be a file.");
    return;
  }

  std::deque<std::pair<std::string, int>> deq;

  try {
    std::ifstream scriptContents(scriptPath);
    std::string line;
    bool inMultilineComment = false;

    for (int i = 1; std::getline(scriptContents, line); i++) {

      if (inMultilineComment) {
        auto endMultilineCommentMark = line.find("*/");
        if (endMultilineCommentMark != std::string::npos) {
          line = line.substr(endMultilineCommentMark + 2);
          inMultilineComment = false;
        } else {
          continue;
        }
      }

      auto commentMark = line.find("//");
      line = (commentMark != std::string::npos)
        ? line.substr(0, commentMark) : line;

      auto startMulilineCommentMark = line.find("/*");
      if (startMulilineCommentMark != std::string::npos) {
        line = (startMulilineCommentMark != std::string::npos)
          ? line.substr(0, startMulilineCommentMark) : line;
        inMultilineComment = true;
      }

      line = trimString(line);

      if (line.empty())
        continue;

      //can in theory split line by ';' to allow for multiple
      // commands in a line if delimited by that

      deq.push_back(std::make_pair(line, i));
    }

    getDDD_GS().conf.dcsCommandsCache.clear();
    getDDD_GS().conf.dcsCommandsCache = deq;

    if (deq.size() > 0) {
      clog("ok", "Loaded dcs script contents from '", scriptPath, "'"
           " (found ", deq.size(), " commands).");

    } else {
      clog("w", "Loaded dcs script contents but found 0 commands.");
    }

  } catch (std::exception& e) {
    clog("e", e.what());
    clog("An error occured while loading dcs script contents.");
  }
}

//get the next dcs command and the line number of the command
// (returns an empty string and -1 if no more commands)
std::pair<std::string, int> getDcsCommand() {
  if (getDDD_GS().conf.dcsCommandsCache.empty()) {
    return std::make_pair("", -1);
  } else {
    auto cmd = getDDD_GS().conf.dcsCommandsCache.front();
    getDDD_GS().conf.dcsCommandsCache.pop_front();
    return cmd;
  }
}

//parses a raw dcs command from a string into a tuple separating
// the actual command along with any arguments and/or flags
std::tuple<std::string, std::unordered_map<std::string, std::string>, std::unordered_set<std::string>> parseDcsCommand(const std::string& s) {
  std::tuple<std::string, std::unordered_map<std::string, std::string>,
             std::unordered_set<std::string>> tpl;

  std::vector<std::string> tokens;
  bool foundLast = false;
  bool inSquote = false;
  bool inDquote = false;
  std::string tempStr = trimString(s);

  while (!foundLast) {
    if ((inSquote) && (!foundLast)) {
      auto squoteMark = tempStr.find('\'');
      if (squoteMark != std::string::npos) {
        tokens.push_back('"' + tempStr.substr(0, squoteMark) + '"');
        tempStr = tempStr.substr(squoteMark + 1);
        inSquote = false;
        continue;
      } else {
        tokens.push_back(tempStr);
        foundLast = true;
        break;
      }
    }
    if ((inDquote) && (!foundLast)) {
      auto dquoteMark = tempStr.find('"');
      if (dquoteMark != std::string::npos) {
        tokens.push_back('"' + tempStr.substr(0, dquoteMark) + '"');
        tempStr = tempStr.substr(dquoteMark + 1);
        inDquote = false;
        continue;
      } else {
        tokens.push_back(tempStr);
        foundLast = true;
        break;
      }
    }
    if (!foundLast) {
      for (int i = 0; i <= tempStr.length(); i++) {
        if (i == tempStr.length()) {
          tokens.push_back(tempStr);
          foundLast = true;
          break;
        }
        if (tempStr[i] == ' ') {
          tokens.push_back(tempStr.substr(0, i));
          tempStr = tempStr.substr(i + 1);
          break;
        } else if ((tempStr[i] == '=') ||
                   (tempStr[i] == ',')) {
          tokens.push_back(tempStr.substr(0, i));
          tokens.push_back(tempStr.substr(i, 1));
          tempStr = tempStr.substr(i + 1);
          break;
        } else if (tempStr[i] == '\'') {
          tokens.push_back(tempStr.substr(0, i));
          tempStr = tempStr.substr(i + 1);
          inSquote = true;
          break;
        } else if (tempStr[i] == '"') {
          tokens.push_back(tempStr.substr(0, i));
          tempStr = tempStr.substr(i + 1);
          inDquote = true;
          break;
        }
      }
      if (tempStr.empty())
        foundLast = true;
    }
  }

  std::string cmd;
  std::vector<std::string> argsTokens;
  std::unordered_map<std::string, std::string> args;
  std::unordered_set<std::string> flags;

  //find cmd+flags and non empty args tokens
  for (const auto &t: tokens) {
    auto tst = trimString(t);
    if (!tst.empty())
      if (cmd.empty())
        cmd = tst;
      else
        argsTokens.push_back(tst);
  }

  //separate cmd and flags
  std::unordered_set<char> flagsList = {'!'};
  for (int i = 0; i < cmd.length(); i++) {
    //treat everything past first valid flag as a flag
    if (flagsList.find(cmd[i]) != flagsList.end()) {
      for (int j = i; j < cmd.length(); j++)
        flags.insert(std::string(1, cmd[j]));
      cmd = cmd.substr(0, i);
      break;
    }
  }

  //parse args tokens into args map
  std::deque<std::string> deq;
  auto parseDeque = [](std::deque<std::string>& deq) {
    std::string k;
    std::string v;
    bool expectsVal = false;
    for (const auto &dv: deq) {
      if (dv == "=") {
        expectsVal = true;
      } else if (!expectsVal) {
        k = dv;
      } else {
        v = dv;
      }
    }
    if (v.empty())
      v = (expectsVal) ? "0" : "1";
    deq.clear();
    return std::make_pair(k, v);
  };

  for (const auto &t: argsTokens) {
    if (t != ",") {
      deq.push_back(t);
    } else if (!deq.empty()) {
      args.insert(parseDeque(deq));
    }
  }
  if (!deq.empty())
    args.insert(parseDeque(deq));

  std::get<0>(tpl) = cmd;
  std::get<1>(tpl) = args;
  std::get<2>(tpl) = flags;

  return tpl;
}
