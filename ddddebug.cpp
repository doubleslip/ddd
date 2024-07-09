/*
 * ddddebug.cpp
 */
//collection of various helper functions to help understand the
// present state of things
// (to be called while debugging or in a test)


#include "ddddebug.hpp"


void printLuaGlobalsByPtr(lua_State* L) {
  try {
    auto searchPtr = getPtrFromInput();
    printLuaGlobalsByPtr(L, searchPtr, 1, true);
  } catch (std::exception& e) {
    clog("e", e.what());
  }
}
bool printLuaGlobalsByPtr(lua_State* L, const void* searchPtr, const int depth, const bool showNotFound) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  bool found = false;

  auto lgm = luaGlobalsToMap(L);
  auto er = lgm.equal_range(searchPtr);
  std::vector<LuaGlobalEntry> lgv;
  for (auto i = er.first; i != er.second; ++i)
    lgv.push_back(i->second);

  if (lgv.size() > 0) {
    found = true;

    if (lgv.size() == 1) {
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") is associated with the following lua global: ");
    } else {
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") is associated with the following (", lgv.size(), ") lua globals:");
    }

    for (const auto &lg: lgv) {
      std::string nameDecoded = decodeCodeFromLuaGlobalString(lg.name);
      if (!nameDecoded.empty())
        nameDecoded = "  (from: '" + nameDecoded + "')";

      clog("lf", depthIndent, indent, lg.name, nameDecoded, " \ttype: ", lg.type);
    }

  } else {
    if (showNotFound)
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") does not seem to be associated with any lua globals"
           " (max depth of table recurssion = 1).");
  }

  return found;
}

std::vector<LuaGlobalEntry> getLuaGlobalsByPtr(lua_State* L, const void* searchPtr) {
  auto lgm = luaGlobalsToMap(L);
  auto er = lgm.equal_range(searchPtr);
  std::vector<LuaGlobalEntry> results;

  for (auto i = er.first; i != er.second; ++i)
    results.push_back(i->second);

  return results;
}

void printLuaGlobalsByRef(lua_State* L) {
  try {
    int32 searchRh = getRefFromInput();
    printLuaGlobalsByRef(L, searchRh, 0, true);
  } catch (std::exception& e) {
    clog("e", e.what());
  }
}

bool printLuaGlobalsByRef(lua_State* L, const int32 searchRh, const int depth, const bool showNotFound) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  bool found = false;

  auto lre =
    getRegistryEntryByRef(L, searchRh, false); //forced to look up registry

  if ((lre.ptr == nullptr) && (lre.type == "nil")) {
    if (showNotFound)
      clog("if", depthIndent, "ref_handle '", searchRh,
           "' is nil or was not found in the lua registry.");

  } else if (lre.ptr == nullptr) {
    if (showNotFound) {
      clog("w", depthIndent, "Value for ref_handle '", searchRh,
           "' was found in lua registry but its pointer was null.");
      clog("lf", depthIndent, indent, "(type: ", lre.type, "; value: ",
           lre.value, ")");
    }

  } else {

    auto lgResults = getLuaGlobalsByPtr(L, lre.ptr);

    if (lgResults.size() == 0) {
      if (showNotFound) {
        clog("if", depthIndent, "ref_handle ", searchRh,
             " found in lua registry but was not associated with any"
             " known lua globals.");
        clog("lf", depthIndent, indent, "Ptr for lua registry entry: (",
             lre.ptr, ")");
      }

    } else if (lgResults.size() > 0) {
      found = true;
      if (lgResults.size() == 1) {
        clog("if", depthIndent, "ref_handle ", searchRh, " (type: '", lre.type,
             "') associated with the following lua global:");
      } else {
        clog("if", depthIndent, "ref_handle ", searchRh, " (type: '", lre.type,
             "') associated with the following (", lgResults.size(),
             ") lua globals:");
      }
      for (const auto& lg: lgResults) {
        std::string codeStr = decodeCodeFromLuaGlobalString(lg.name);
        if (!codeStr.empty())
          codeStr = "  (from: '" + codeStr + "')";

        clog("lf", depthIndent, indent, lg.name, codeStr);
      }
    } else {
      clog("e", depthIndent, "Vector size was ", lgResults.size());
    }
  }
  return found;
}


void printLuaRegistryEntryByPtr(lua_State* L) {
  try {
    auto searchPtr = getPtrFromInput();
    printLuaRegistryEntryByPtr(L, searchPtr, 0, true);
  } catch (std::exception& e) {
    clog("e", e.what());
  }
}
bool printLuaRegistryEntryByPtr(lua_State* L, const void* searchPtr, const int depth, const bool showNotFound) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  bool found = false;

  auto lrm = luaRegistryToMap(L);
  std::vector<LuaRegistryEntry> lrv;
  for (const auto &lre: lrm) {
    if (lre.second.ptr == searchPtr)
      lrv.push_back(lre.second);
  }

  if (lrv.size() == 0) {
    if (showNotFound)
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") does not seem to be associated with any lua registry entries.");
  } else {
    found = true;

    if (lrv.size() == 1) {
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") is associated with the lua registry entry:");
    } else {
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") is associated with the following (", lrv.size(),
           ") lua registry entries:");
    }

    for (const auto &lre: lrv) {
      clog("lf", depthIndent, indent, "ref_handle: ", lre.ref_handle,
           " \ttype: ", lre.type," \tvalue: ", lre.value);
    }
  }

  return found;


}
std::vector<LuaRegistryEntry> getRegistryEntryByPtr(lua_State* L, const void* searchPtr) {
  std::vector<LuaRegistryEntry> results;
  auto lrm = luaRegistryToMap(L);

  for (const auto &lre: lrm) {
    if (lre.second.ptr == searchPtr)
      results.push_back(lre.second);
  }

  return results;
}
void printLuaRegistryEntryByRef(lua_State* L) {
  try {
    int32 searchRh = getRefFromInput();
    printLuaRegistryEntryByRef(L, searchRh, 0, true);
  } catch (std::exception& e) {
    clog("e", e.what());
  }
}
bool printLuaRegistryEntryByRef(lua_State* L, const int32 searchRh, const int depth, const bool showNotFound) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  bool found = false;

  auto lre = getRegistryEntryByRef(L, searchRh, true);
  if ((lre.ptr == nullptr) && (lre.type == "nil")) {
    if (showNotFound)
      clog("if", depthIndent, "ref_handle ", searchRh,
           " is nil or was not found in the lua registry.");

  } else {
    found = true;
    clog("if", depthIndent, "ref_handle ", searchRh,
         " in the lua registry entry is of type '", lre.type, "'.");
    clog("lf", depthIndent, indent, "ptr: (", lre.ptr, ")  value: ", lre.value);
  }

  return found;
}

LuaRegistryEntry getRegistryEntryByRef(lua_State* L, const int32 searchRh, const bool enableFormatting) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, searchRh); //0, +1

  std::string type = std::string(lua_typename(L, lua_type(L, -1)));
  std::string value = luaValueAtIndexToString(L, -1, enableFormatting);
  const void* ptr = lua_topointer(L, -1);

  LuaRegistryEntry lre = {
      .ref_handle = searchRh,
      .type = type,
      .value = value,
      .ptr = ptr
    };

  lua_pop(L, 1); //-1, 0
  return lre;
}
void printDuelVarsByPtr(intptr_t pDuel) {
  try {
    auto searchPtr = getPtrFromInput();
    printDuelVarsByPtr(pDuel, searchPtr, 0, true);
  } catch (std::exception& e) {
    clog("e", e.what());
  }
}
bool printDuelVarsByPtr(intptr_t pDuel, const void* searchPtr, const int depth, const bool showNotFound) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  bool found = false;

  auto dvm = duelVarsToMap(pDuel);
  auto dvsm = std::map<const void*, DuelVarEntry>(dvm.begin(), dvm.end());
  auto dver = dvsm.equal_range(searchPtr);
  std::vector<DuelVarEntry> results;
  for (auto i = dver.first; i != dver.second; ++i) {
    results.push_back(i->second);
  }

  if (results.size() == 0) {
    if (showNotFound)
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") does not seem to be associated with any known duel vars.");

    return false;
  } else {
    found = true;
  }

  if (results.size() == 1) {
    clog("if", depthIndent, "Ptr (", searchPtr,
         ") is associated with the following duel var:");
  } else {
    clog("if", depthIndent, "Ptr (", searchPtr,
         ") is associated with the following (", results.size(), ") duel vars:");
  }

  std::set<const void*> alreadyPrintedSet;

  for (const auto& dv: results) {
    clog("lf", depthIndent, indent, "type: ", dv.type, "  from: ", dv.origin,
         "  ptr: (", dv.ptr, ")");

    if (alreadyPrintedSet.find(dv.ptr) != alreadyPrintedSet.end()) {
      clog("lf", depthIndent, indent, indent, "(was previously printed; ptr: (",
           dv.ptr, "))");
    } else {
      alreadyPrintedSet.insert(dv.ptr);
      if (dv.type == "card") {
        printCardDebug((card*) dv.ptr, (depth + 2));
      } else if (dv.type == "group") {
        printGroupDebug((group*) dv.ptr, (depth + 2));
      } else if (dv.type == "effect") {
        printEffectDebug((effect*) dv.ptr, (depth + 2));
      } else {
        clog("w", "Unknown type '", dv.type, "'; ignoring.");
      }
    }
  }

  return found;
}

void printDuelVarsByRef(intptr_t pDuel) {
  try {
    int32 searchRh = getRefFromInput();
    printDuelVarsByRef(pDuel, searchRh, 0, true);
  } catch (std::exception& e) {
    clog("e", e.what());
  }
}
bool printDuelVarsByRef(intptr_t pDuel, const int32 searchRh, const int depth, const bool showNotFound) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  bool found = false;

  auto dvs = getDuelVarsByRef(pDuel, searchRh);

  if (dvs.size() == 0) {
    if (showNotFound)
      clog("if", depthIndent, "ref_handle ", searchRh,
           " does not seem to be associated with any known duel vars.");
  } else {
    found = true;

    if (dvs.size() == 1) {
      clog("if", depthIndent, "ref_handle ", searchRh,
           " is associated with the following duel var:");
    } else {
      clog("if", depthIndent, "ref_handle ", searchRh,
           " is associated with the following (", dvs.size(), ") duel vars:");
    }

    for (const auto& dv: dvs) {
      if ((dv.second == "cards") || (dv.second == "assumes")) {
        card* c = (card*) dv.first;
        clog("lf", depthIndent, indent, "type: 'card' (from pduel->", dv.second, ")");
        printCardDebug(c, (depth + 1));

      } else if ((dv.second == "groups") || (dv.second == "sgroups")) {
        group* g = (group*) dv.first;
        clog("lf", depthIndent, indent, "type: 'group' (from pduel->", dv.second, ")");
        printGroupDebug(g, (depth + 1));

      } else if ((dv.second == "effects") || (dv.second == "uncopy")) {
        effect* e = (effect*) dv.first;
        clog("lf", depthIndent, indent, "type: 'effect' (from pduel->", dv.second, ")");
        printEffectDebug(e, (depth + 1));

      } else if (dv.second == "coroutines") {
        clog("lf", depthIndent, indent, "type: 'effect' (from pduel->lua->coroutines)");

      } else {
        clog("w", depthIndent, indent, "Unknown type: '", dv.second, "'.");
      }
    }
  }
  return found;
}
std::vector<std::pair<void*,std::string>> getDuelVarsByRef(intptr_t pDuel, const int32 searchRh) {
  duel* pD = (duel*) pDuel;
  lua_State* L = pD->lua->lua_state;
  std::vector<std::pair<void*, std::string>> results;

  for (card* const &c: pD->cards) {
    int32 refHandle = c->ref_handle;
    if (refHandle == searchRh) {
      results.push_back(std::make_pair((void*) c, "cards"));
    }
  }
  for (card* const &a: pD->assumes) {
    int32 refHandle = a->ref_handle;
    if (refHandle == searchRh) {
      results.push_back(std::make_pair((void*) a, "assumes"));
    }
  }
  for (group* const &g: pD->groups) {
    int32 refHandle = g->ref_handle;
    if (refHandle == searchRh) {
      results.push_back(std::make_pair((void*) g, "groups"));
    }
  }
  for (group* const &s: pD->sgroups) {
    int32 refHandle = s->ref_handle;
    if (refHandle == searchRh) {
      results.push_back(std::make_pair((void*) s, "sgroups"));
    }
  }
  for (effect* const &e: pD->effects) {
    int32 refHandle = e->ref_handle;
    if (refHandle == searchRh) {
      results.push_back(std::make_pair((void*) e, "effects"));
    }
  }
  for (effect* const &u: pD->uncopy) {
    int32 refHandle = u->ref_handle;
    if (refHandle == searchRh) {
      results.push_back(std::make_pair((void*) u, "uncopy"));
    }
  }
  for (std::pair<int32, std::pair<lua_State*, int32>> const &co:
         pD->lua->coroutines) {
    int32 refHandle = co.first;
    if (refHandle == searchRh) {
      results.push_back(std::make_pair((void*) co.second.first,
                                       "coroutines"));
    }
  }

  return results;
}
void printLuaStackEntriesByPtr(lua_State* L) {
  try {
    void* searchPtr = getPtrFromInput();
    printLuaStackEntriesByPtr(L, searchPtr, 0, true);
  } catch (std::exception& e) {
    clog("e", e.what());
  }
}
bool printLuaStackEntriesByPtr(lua_State* L, const void* searchPtr, const int depth, const bool showNotFound) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  bool found = false;

  auto lse = getLuaStackEntriesByPtr(L, searchPtr, true);

  if (lse.size() == 0) {
    if (showNotFound)
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") does not seem to be associated with any (immediate) lua stack vars.");
  } else {
    found = true;

    if (lse.size() == 1) {
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") is associated with the following lua stack var:");
    } else {
      clog("if", depthIndent, "Ptr (", searchPtr,
           ") is associated with the following (", lse.size(), ") lua stack vars:");
    }

    for (const auto &e: lse) {
      int index = e.first.first;
      int innerIndex = e.first.second;
      std::string type = e.second.first;
      std::string val = e.second.second;

      if (innerIndex == 0) {
        clog("lf", depthIndent, indent, "index: ", index, "  type: ",
             type, "\tval: ", val);
      } else {
        clog("lf", depthIndent, indent, "(in ",
             lua_typename(L, lua_type(L, index)), " at index: ", index, ")");
        clog("lf", depthIndent, indent, indent, "index: ",
             innerIndex, "  type: ", type, "\tval: ", val);
      }
    }
  }

  return found;
}
std::vector<std::pair<std::pair<int, int>, std::pair<std::string, std::string>>> getLuaStackEntriesByPtr(lua_State* L, const void* searchPtr, const bool shouldRecur) {
  std::vector<std::pair<std::pair<int, int>, std::pair<std::string, std::string>>> results;

  int top = lua_gettop(L);

  for (int i = 1; i <= top; ++i) {
    std::string type = lua_typename(L, lua_type(L, i));
    std::string val = luaValueAtIndexToString(L, i);
    const void* ptr = lua_topointer(L, i);

    if (searchPtr == ptr)
      results.push_back(std::make_pair(std::make_pair(i, 0),
                                       std::make_pair(type, val)));

    if ((type == "thread") && (shouldRecur)) {

      auto innerEntries =
        getLuaStackEntriesByPtr((lua_State*) ptr, searchPtr, false);

      for (const auto& ie: innerEntries) {
        results.push_back(std::make_pair
                          (std::make_pair(i, ie.first.first),
                           std::make_pair(ie.second.first, ie.second.second)));
      }

    //} else if ((type == "table") && (shouldRecur)) {
    }
  }

  return results;
}


void ptrSearch(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;
  void* searchPtr = nullptr;

  try {
    searchPtr = getPtrFromInput();
    ptrSearch(pDuel, searchPtr, 0);
  } catch (std::exception& e) {
    clog("e", e.what());
    return;
  }
}
void ptrSearch(intptr_t pDuel, const void* searchPtr, const int depth) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  duel* pD = (duel*) pDuel;

  bool foundInLuaRegistry =
    printLuaRegistryEntryByPtr(pD->lua->lua_state, searchPtr, depth, false);
  bool foundInDuelVars = printDuelVarsByPtr(pDuel, searchPtr, depth, false);
  bool foundInLuaGlobals =
    printLuaGlobalsByPtr(pD->lua->lua_state, searchPtr, depth, false);
  bool foundInLuaStateStack =
    printLuaStackEntriesByPtr(pD->lua->lua_state, searchPtr, depth, false);

  if ((!foundInLuaRegistry) && (!foundInDuelVars) &&
      (!foundInLuaGlobals) && (!foundInLuaStateStack)) {
    clog("if", depthIndent, "Ptr (", searchPtr,
         ") does not seem to be associated with any known registry entries,"
         " duel vars or lua globals.");
  }
}

void refSearch(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;
  int32 searchRh = 0;

  try {
    searchRh = getRefFromInput();
    refSearch(pDuel, searchRh, 0);
  } catch (std::exception& e) {
    clog("e", e.what());
    return;
  }
}
void refSearch(intptr_t pDuel, const int32 searchRh, const int depth) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  duel* pD = (duel*) pDuel;

  bool foundInLuaRegistry =
    printLuaRegistryEntryByRef(pD->lua->lua_state, searchRh, depth, false);
  bool foundInDuelVars = printDuelVarsByRef(pDuel, searchRh, depth, false);
  bool foundInLuaGlobals =
    printLuaGlobalsByRef(pD->lua->lua_state, searchRh, depth, false);
  //search in lua_state stack as well?

  if ((!foundInLuaRegistry) && (!foundInDuelVars) && (!foundInLuaGlobals)) {
    clog("if", depthIndent, "Ref handle ", searchRh,
         " does not seem to be associated with any known registry entries,"
         " duel vars or lua globals.");
  }
}

void cardNameSearch(intptr_t pDuel, const bool showOthers) {
  std::string searchStr;
  std::cout << "Enter a card name or part of a card name: ";
  std::getline(std::cin, searchStr);

  if (trimString(searchStr).empty()) {
    clog("w", "Empty string; ignoring.");
  } else {
    cardNameSearch(pDuel, searchStr, 0, showOthers);
  }
}

void cardNameSearch(intptr_t pDuel, const std::string searchStr, const int depth, const bool showOthers) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');
  duel* pD = (duel*) pDuel;

  auto dvm = duelVarsToMap(pDuel);
  auto lgm = luaGlobalsToMap(pD->lua->lua_state);
  auto lrm = luaRegistryToMap(pD->lua->lua_state);

  std::set<const void*> cardSet;
  std::set<effect*> cardEffectSet;
  std::set<uint32> cardCodeSet;
  std::set<uint32> cardRhSet;

  for (const auto &dv: dvm) {
    if (dv.second.type == "card") {
      card* c = (card*) dv.second.ptr;
      uint32 code = c->data.code;

      if (searchString(decodeCode(code), searchStr)) {
        cardSet.insert(dv.second.ptr);
        cardCodeSet.insert(code);

        if (c->ref_handle != 0)
          cardRhSet.insert(c->ref_handle);

        if (c->single_effect.size() > 0)
          for (const auto &e: c->single_effect)
            cardEffectSet.insert(e.second);

        if (c->field_effect.size() > 0)
          for (const auto &e: c->field_effect)
            cardEffectSet.insert(e.second);

        if (c->equip_effect.size() > 0)
          for (const auto &e: c->equip_effect)
            cardEffectSet.insert(e.second);

        if (c->target_effect.size() > 0)
          for (const auto &e: c->target_effect)
            cardEffectSet.insert(e.second);

        if (c->xmaterial_effect.size() > 0)
          for (const auto &e: c->xmaterial_effect)
            cardEffectSet.insert(e.second);

      }
    }
  }

  if (cardSet.size() == 0) {
    clog("d", depthIndent, "No results for '", searchStr, "'.");
    return;
  } else {
    clog("d", depthIndent, "'", searchStr, "' seems to be related to these results:");
  }

  clog("if", "Cards (", cardSet.size(), "):");
  for (const auto &c: cardSet) {
    printCardDebug((card*) c, (depth + 1));
  }

  if (cardCodeSet.size() > 1) { //check for dupes but account for different card arts
    clog("d", "cardCodeSet size = ", cardCodeSet.size());
    std::set<std::string> cardDecodedSet;
    for (const auto &ccs: cardCodeSet) {
      std::string cardName = decodeCode(ccs);
      cardDecodedSet.insert(cardName);
    }
    if (cardDecodedSet.size() > 1) {
      clog("if", depthIndent,
           "Note: filter card name to 1 result to show further details.");
      return; //exit
    }
  }

  uint32 code = ((card*) *(cardSet.begin()))->data.code;
  std::string codeStr = "c" + std::to_string(code) + ".";
  std::vector<LuaGlobalEntry> lgResults;
  for (const auto &lg: lgm) {
    if (lg.second.name.find(codeStr) == 0) { //found and also at very beginning
      //if (lg.second.type == "function") {
        lgResults.push_back(lg.second);
      //}
    }
  }
  if (lgResults.size() > 0) {
    clog("if", depthIndent, "Lua globals (", lgResults.size(), "):");

    for (const auto &lg: lgResults) {
      clog("lf", depthIndent, indent, lg.name, " \ttype: ", lg.type,
           " \t", lg.value);

      std::vector<std::string> lrev;
      for (const auto &lre: lrm) {
        if (lre.second.ptr == lg.ptr) {
          lrev.push_back(std::to_string(lre.second.ref_handle));
        }
      }

      if (lrev.size() > 0)
        clog("lf", depthIndent, indent, indent, "ref handle",
             ((lrev.size() != 1) ? "s: " : ": "), joinString(lrev, ", "));
    }
  }

  std::multimap<uint32, effect*> rhOfInterestSet;
  std::vector<LuaRegistryEntry> lreOfInterest;
  for (const auto &e: cardEffectSet) {
    //if (e->ref_handle != 0) rhOfInterestSet.insert(std::make_pair(e->ref_handle, e));
    if (e->condition != 0) rhOfInterestSet.insert(std::make_pair(e->condition, e));
    if (e->cost != 0) rhOfInterestSet.insert(std::make_pair(e->cost, e));
    if (e->target != 0) rhOfInterestSet.insert(std::make_pair(e->target, e));
    if ((e->value != 0) && (e->flag[0] & EFFECT_FLAG_FUNC_VALUE))
      rhOfInterestSet.insert(std::make_pair(e->value, e));
    if (e->operation != 0) rhOfInterestSet.insert(std::make_pair(e->operation, e));
  }
  for (const auto &rh: rhOfInterestSet) {
    if (lrm.find(rh.first) != lrm.end()) { //should be guaranteed to be unique
      auto lre = lrm[rh.first];

      if (lre.type != "nil")
        lreOfInterest.push_back(lrm[rh.first]);

    }
  }
  if (lreOfInterest.size() > 0) {
    clog("if", depthIndent, "Lua registry entries (", lreOfInterest.size(), "):");
    for (const auto &lre: lreOfInterest) {

      std::vector<std::string> lgstrv;
      std::string lgstr = "";
      auto er = lgm.equal_range(lre.ptr);
      for (auto i = er.first; i != er.second; ++i) {
        lgstrv.push_back(i->second.name);
      }
      if (lgstrv.size() > 0) {
        lgstr = "  lua global" + std::string((lgstrv.size() == 1) ? "" : "s")
          + ": [" + joinString(lgstrv, ",") + "]";
      }
      auto eer = rhOfInterestSet.equal_range(lre.ref_handle);
      std::vector<std::string> estrv;
      std::string estr = "";
      if (eer.first != eer.second) {
        for (auto i = eer.first; i != eer.second; ++i) {
          std::stringstream buffer;
          buffer << i->second;
          estrv.push_back(buffer.str());
        }
        if (estrv.size() > 0) {
          estr = "  (effect*) ptr" + std::string((estrv.size() == 1) ? "" : "s")
            + ": [" + joinString(estrv, ", ") + "]";
        }
      }

      if ((!estr.empty()) || (!lgstr.empty())) {
        clog("lf", depthIndent, indent, "ref_handle: ", lre.ref_handle,
             "  type: ", lre.type, "  ptr: (", lre.ptr, ")  value: ", lre.value);
        clog("lf", depthIndent, indent, indent, "associated variables: ", estr, lgstr);
      } else {
        clog("w", depthIndent, indent, "ref_handle: ", lre.ref_handle,
             "  type: ", lre.type, "  ptr: (", lre.ptr, ")  value: ", lre.value);
      }
    }
  }

}


std::string luaValueAtIndexToString(lua_State* L, const int index) {
  return luaValueAtIndexToString(L, index, true);
}
std::string luaValueAtIndexToString(lua_State* L, const int index, bool enableFormatting) {
  switch (lua_type(L, index)) {
  case LUA_TNUMBER:
    {
      double d = lua_tonumber(L, index);
      if (d == (int) d) {
        return std::to_string((int) d);
      } else {
        return std::to_string(d);
      }
    }
    break;
  case LUA_TSTRING:
    {
      if (enableFormatting)
        return "'" + std::string(lua_tostring(L, index)) + "'";
      else
        return std::string(lua_tostring(L, index));
    }
    break;
  case LUA_TBOOLEAN:
    {
      return (lua_toboolean(L, index) ? "true" : "false");
    }
    break;
  case LUA_TNIL:
    {
      return "nil";
    }
    break;
  default:
    {
      intptr_t i = (intptr_t) lua_topointer(L, index);
      std::stringstream ss;
      ss << "0x" << std::hex << i;

      if (enableFormatting)
        return "(" + ss.str() + ")";
      else
        return ss.str();
    }
    break;
  }
}

void printCardDebug(card* c) {
  printCardDebug(c, 0, {});
}
void printCardDebug(card* c, const int depth) {
  printCardDebug(c, depth, {});
}
void printCardDebug(card* c, const int depth, std::vector<const void*> prevPtrs) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');

  if ((c == nullptr) || (c == 0)) {
    clog("lf", depthIndent, "[card] is nullptr.");
    return;
  }

  if (std::find(prevPtrs.begin(), prevPtrs.end(), c) != prevPtrs.end()) {
    clog("lf", depthIndent, "[card] circular ref to ptr: (", c, ").");
    return;
  }

  if (depth > 15) {
    clog("w", "Excessive depth reached; aborting print"
         " (infinite recurssion?; passed ptr = (", c, ")).");
    return;
  }

  int32 refHandle = c->ref_handle;
  clog("lf", depthIndent, "[card] ref_handle: ", refHandle, "  ptr: (", c, ")");


  const auto pca = getDDD_GS().conf.debug.printCardAttributes;
  prevPtrs.push_back(c);

  if (pca.find("code") != pca.end()) {
    uint32 code = c->data.code;
    std::string codeDecoded = decodeCode(code);

    if (!codeDecoded.empty())
      codeDecoded = " (" + codeDecoded + ")";

    clog("lf", depthIndent, indent, "code: ", code, codeDecoded);
  }

  if (pca.find("pduel") != pca.end()) {
    duel* pduel = c->pduel;
    clog("lf", depthIndent, indent, "pduel: duel*(", pduel, ")");
  }

  if ((pca.find("controler") != pca.end()) ||
      (pca.find("controller") != pca.end())) {
    uint8 controler = c->current.controler; //current, but temp and previous also exist
    std::string controlerDecoded = decodePlayer(controler);

    if (!controlerDecoded.empty())
      controlerDecoded = " (" + controlerDecoded + ")";

    clog("lf", depthIndent, indent, "controler (current): ",
         (int) controler, controlerDecoded);
  }

  if (pca.find("location") != pca.end()) {
    uint8 location = c->current.location; //current, but temp and previous also exist
    std::string locationDecoded = decodeLocation(location);

    if (!locationDecoded.empty())
      locationDecoded = " (" + locationDecoded + ")";

    clog("lf", depthIndent, indent, "location (current): ",
         (int) location, locationDecoded);
  }

  if (pca.find("sequence") != pca.end()) {
    uint8 sequence = c->current.sequence; //current, but temp and previous also exist
    clog("lf", depthIndent, indent, "sequence (current): ", (int) sequence);
  }

  if (pca.find("status") != pca.end()) {
    uint32 status = c->status;
    std::string statusDecoded = decodeStatus(status);

    if (!statusDecoded.empty())
      statusDecoded = " (" + statusDecoded + ")";

    clog("lf", depthIndent, indent, "status: ", (int) status, statusDecoded);
  }

  if (pca.find("reason") != pca.end()) {
    uint32 reason = c->current.reason; //current, but temp and previous also exist
    card* card_reason = c->current.reason_card;
    uint8 reason_player = c->current.reason_player;
    effect* reason_effect = c->current.reason_effect;

    std::string reasonDecoded = decodeReason(reason);
    if (!reasonDecoded.empty())
      reasonDecoded = " [" + reasonDecoded + "]";

    std::string reason_playerDecoded = decodePlayer(reason_player);
    if (!reason_playerDecoded.empty())
      reason_playerDecoded = " (" + reason_playerDecoded + ")";

    clog("lf", depthIndent, indent, "reason_player: ", (int) reason_player,
         reason_playerDecoded, "  reason: ", (int) reason, reasonDecoded);

    if ((card_reason == nullptr) || (card_reason == 0)) {
      clog("lf", depthIndent, indent, indent, "card_reason (card*) = nullptr");
    } else if (c == card_reason) {
      clog("lf", depthIndent, indent, indent, "card_reason (card*) = this");
    } else {
      clog("lf", depthIndent, indent, indent, "card_reason (card*) = (",
           card_reason, ") (this = (", c, "))");
      printCardDebug(card_reason, (depth + 3), prevPtrs);
    }

    if ((reason_effect == nullptr) || (reason_effect == 0)) {
      clog("lf", depthIndent, indent, indent, "reason_effect (effect*) = nullptr");
    } else {
      clog("lf", depthIndent, indent, indent, "reason_effect (effect*) = (",
           reason_effect, ")");
      printEffectDebug(reason_effect, (depth + 3), prevPtrs);
    }
  }

  if (pca.find("unique_effect") != pca.end()) {
    if (c->unique_effect)
      clog("lf", depthIndent, indent, "unique_effect = (", c->unique_effect, ")");
    else
      clog("lf", depthIndent, indent, "unique_effect = nullptr");
  }

  if (pca.find("effect_container") != pca.end()) {
    unsigned int single_effect_size = c->single_effect.size();
    unsigned int field_effect_size = c->field_effect.size();
    unsigned int equip_effect_size = c->equip_effect.size();
    unsigned int target_effect_size = c->target_effect.size();
    unsigned int xmaterial_effect_size = c->xmaterial_effect.size();

    clog("lf", depthIndent, indent, "effect_container var sizes:");

    if (single_effect_size > 0) {
      clog("lf", depthIndent, indent, indent, "single_effect: ",
           single_effect_size);
      for (const auto &e: c->single_effect) {
        std::string effectCodeStr = decodeEffectCode(e.first);
        if (!effectCodeStr.empty())
          effectCodeStr = "  (" + effectCodeStr + ")";
        clog("lf", depthIndent, indent, indent, indent, "(effect*) ptr: (",
             e.second, ")  status: ", e.first, effectCodeStr);
      }
    }
    if (field_effect_size > 0) {
      clog("lf", depthIndent, indent, indent, "field_effect: ", field_effect_size);
      for (const auto &e: c->field_effect) {
        std::string effectCodeStr = decodeEffectCode(e.first);
        if (!effectCodeStr.empty())
          effectCodeStr = "  (" + effectCodeStr + ")";

        clog("lf", depthIndent, indent, indent, indent, "(effect*) ptr: (",
             e.second, ")  status: ", e.first, effectCodeStr);
      }
    }
    if (equip_effect_size > 0) {
      clog("lf", depthIndent, indent, indent, "equip_effect: ", equip_effect_size);
      for (const auto &e: c->equip_effect) {
        std::string effectCodeStr = decodeEffectCode(e.first);
        if (!effectCodeStr.empty())
          effectCodeStr = "  (" + effectCodeStr + ")";

        clog("lf", depthIndent, indent, indent, indent, "(effect*) ptr: (",
             e.second, ")  status: ", e.first, effectCodeStr);
      }
    }
    if (target_effect_size > 0) {
      clog("lf", depthIndent, indent, indent, "target_effect: ", target_effect_size);
      for (const auto &e: c->target_effect) {
        std::string effectCodeStr = decodeEffectCode(e.first);
        if (!effectCodeStr.empty())
          effectCodeStr = "  (" + effectCodeStr + ")";

        clog("lf", depthIndent, indent, indent, indent, "(effect*) ptr: (",
             e.second, ")  status: ", e.first, effectCodeStr);
      }
    }
    if (xmaterial_effect_size > 0) {
      clog("lf", depthIndent, indent, indent, "xmaterial_effect: ",
           xmaterial_effect_size);
      for (const auto &e: c->xmaterial_effect) {
        std::string effectCodeStr = decodeEffectCode(e.first);
        if (!effectCodeStr.empty())
          effectCodeStr = "  (" + effectCodeStr + ")";

        clog("lf", depthIndent, indent, indent, indent, "(effect*) ptr: (",
             e.second, ")  status: ", e.first, effectCodeStr);
      }
    }
  }
}
void printGroupDebug(group* g) {
  printGroupDebug(g, 0, {});
}
void printGroupDebug(group* g, const int depth) {
  printGroupDebug(g, depth, {});
}
void printGroupDebug(group* g, const int depth, std::vector<const void*> prevPtrs) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');

  if ((g == nullptr) || (g == 0)) {
    clog("lf", depthIndent, "[group] is nullptr.");
    return;
  }

  if (std::find(prevPtrs.begin(), prevPtrs.end(), g) != prevPtrs.end()) {
    clog("lf", depthIndent, "[group] circular ref to ptr: (", g, ").");
    return;
  }

  if (depth > 15) {
    clog("w", "Excessive depth reached; aborting print"
         " (infinite recurssion?; passed ptr = (", g, ")).");
    return;
  }

  int32 refHandle = g->ref_handle;
  clog("lf", depthIndent, "[group] ref_handle: ", refHandle, "  ptr: (", g, ")");

  const auto pga = getDDD_GS().conf.debug.printGroupAttributes;
  prevPtrs.push_back(g);

  if (pga.find("pduel") != pga.end()) {
    duel* pduel = g->pduel;
    clog("lf", depthIndent, indent, "pduel: duel*(", pduel, ")");
  }

  if (pga.find("container") != pga.end()) {
    if (g->container.size() == 0) {
      clog("lf", depthIndent, indent, "container contents (",
           g->container.size(), " of card*): <empty>");
    } else {
      clog("lf", depthIndent, indent, "container contents (",
           g->container.size(), " of card*):");
      for (const auto& c: g->container) {
        printCardDebug(c, (depth + 2), prevPtrs);
      }
    }
  }
}
void printEffectDebug(effect* e) {
  printEffectDebug(e, 0, {});
}
void printEffectDebug(effect* e, const int depth) {
  printEffectDebug(e, depth, {});
}
void printEffectDebug(effect* e, const int depth, std::vector<const void*> prevPtrs) {
  const std::string depthIndent = std::string((depth * 4), ' ');
  const std::string indent = std::string(4, ' ');

  if ((e == nullptr) || (e == 0)) {
    clog("lf", depthIndent, "[effect] is nullptr.");
    return;
  }

  if (std::find(prevPtrs.begin(), prevPtrs.end(), e) != prevPtrs.end()) {
    clog("lf", depthIndent, "[effect] circular ref to ptr: (", e, ").");
    return;
  }

  if (depth > 15) {
    clog("w", "Excessive depth reached; aborting print"
         " (infinite recurssion?; passed ptr = (", e, ")).");
    return;
  }

  int32 refHandle = e->ref_handle;
  uint32 id = e->id;
  clog("lf", depthIndent, "[effect] ref_handle: ", refHandle, "  id: ", id,
       "  ptr: (", e, ")");

  const auto pea = getDDD_GS().conf.debug.printEffectAttributes;
  prevPtrs.push_back(e);

  if (pea.find("pduel") != pea.end()) {
    duel* pduel = e->pduel;
    clog("lf", depthIndent, indent, "pduel: duel*(", pduel, ")");
  }

  if (pea.find("owner") != pea.end()) {
    card* owner = e->get_owner();
    uint8 owner_player = e->get_owner_player();
    clog("lf", depthIndent, indent, "effect owner (card*) properties:");
    printCardDebug(owner, (depth + 2), prevPtrs);

  }

  if (pea.find("handler") != pea.end()) {
    card* handler = e->get_handler();
    uint8 handler_player = e->get_handler_player();
    clog("lf", depthIndent, indent, "effect handler (card*) properties:");
    if ((pea.find("owner") != pea.end()) && (handler == e->get_owner())) {
      clog("lf", depthIndent, indent, indent,
           "[card] (same as effect owner)  ptr: (", handler, ")");
    } else {
      printCardDebug(handler, (depth + 2), prevPtrs);
    }
  }

  bool printCountLimit = (pea.find("count_limit") != pea.end());
  bool printCountLimitMax = (pea.find("count_limit_max") != pea.end());
  bool printCountCode = (pea.find("count_code") != pea.end());
  if ((printCountLimit) || (printCountLimitMax) || (printCountCode)) {
    std::string count_limit = (!printCountLimit) ? "":
      ("count_limit: " + std::to_string((int) e->count_limit));
    std::string count_limit_max = (!printCountLimitMax) ? "":
      ("count_limit_max: " + std::to_string((int) e->count_limit_max));

    std::string count_code = "count_code: " + std::to_string(e->count_code);
    std::string countCodeDecoded = decodeEffectCountCode(e->count_code);
    //if (countCodeDecoded != "")
    if (!countCodeDecoded.empty())
      count_code += " (" + countCodeDecoded + ")";

    if (printCountLimit)
      count_limit_max = "  " + count_limit_max;

    if ((printCountLimit) || (printCountLimitMax))
        count_code = "  " + count_code;

    clog("lf", depthIndent, indent, count_limit, count_limit_max, count_code);
  }

  if (pea.find("description") != pea.end()) {
    uint32 description = e->description;
    std::string descriptionDecoded = decodeDesc(description);

    //if (descriptionDecoded != "")
    if (!descriptionDecoded.empty())
      descriptionDecoded = " (" + descriptionDecoded + ")";

    clog("lf", depthIndent, indent, "description: ",
         description, descriptionDecoded);
  }

  if (pea.find("code") != pea.end()) {
    //not to be confused with card code
    uint32 rawEffectCode = e->code;
    std::string rawEffectCodeDecoded = decodeEffectCode(rawEffectCode);

    if (!rawEffectCodeDecoded.empty())
      rawEffectCodeDecoded = " (" + rawEffectCodeDecoded + ")";

    clog("lf", depthIndent, indent, "effect code: ",
         rawEffectCode, rawEffectCodeDecoded);
  }

  if (pea.find("type") != pea.end()) {
    //not to be confused with card type
    uint16 type = e->type;
    std::string typeDecoded = decodeEffectType(type);

    if (!typeDecoded.empty())
      typeDecoded = " (" + typeDecoded + ")";

    clog("lf", depthIndent, indent, "effect type: ", type, typeDecoded);
  }

  if (pea.find("status") != pea.end()) {
    //not to be confused with card status
    uint16 status = e->status;
    std::string statusDecoded = decodeEffectStatus(status);

    if (!statusDecoded.empty())
      statusDecoded = " (" + statusDecoded + ")";

    clog("lf", depthIndent, indent, "status: ", status, statusDecoded);
  }

  if (pea.find("flag") != pea.end()) {
    uint32 flag = e->flag[0];
    uint32 flag2 = e->flag[1];

    std::string flagDecoded = decodeEffectFlag(flag);
    std::string flag2Decoded = decodeEffectFlag2(flag2);

    clog("lf", depthIndent, indent, "flag: [", flag, ", ", flag2, "]");
    if (!flagDecoded.empty())
      clog("lf", depthIndent, indent, indent, "flag[0] (flag): (", flagDecoded, ")");
    if (!flag2Decoded.empty())
      clog("lf", depthIndent, indent, indent, "flag[1] (flag2): (", flag2Decoded, ")");
  }

  if (pea.find("reset") != pea.end()) {
    uint32 reset_flag = e->reset_flag;
    uint16 reset_count = e->reset_count;
    std::string resetDecoded = decodeReset(reset_flag);

    if (!resetDecoded.empty())
      resetDecoded = " (" + resetDecoded + ")";

    clog("lf", depthIndent, indent, "reset_flag: ", reset_flag, resetDecoded,
         "  reset_count: ", reset_count);
  }

  if (pea.find("condition") != pea.end()) {
    int32 condition = e->condition; //ref handle
    std::string lgNames = "";

    if (condition != 0) {
      duel* pD = e->pduel;
      auto re = getRegistryEntryByRef(pD->lua->lua_state, condition, false);
      if (re.type == "function") {
        auto lgv = getLuaGlobalsByPtr(pD->lua->lua_state, re.ptr);

        std::vector<std::string> lgsv;
        for (const auto &lg: lgv)
          lgsv.push_back(lg.name);

        lgNames = joinString(lgsv, ", ");
      }
      if (!lgNames.empty())
        lgNames = "  (func: " + lgNames + ")";
    }
    clog("lf", depthIndent, indent, "condition: ", condition, lgNames);
  }

  if (pea.find("cost") != pea.end()) {
    int32 cost = e->cost; //ref handle
    uint8 cost_checked = e->cost_checked;
    std::string lgNames = "";

    if (cost != 0) {
      duel* pD = e->pduel;
      auto re = getRegistryEntryByRef(pD->lua->lua_state, cost, false);
      if (re.type == "function") {
        auto lgv = getLuaGlobalsByPtr(pD->lua->lua_state, re.ptr);

        std::vector<std::string> lgsv;
        for (const auto &lg: lgv)
          lgsv.push_back(lg.name);

        lgNames = joinString(lgsv, ", ");
      }
      if (!lgNames.empty())
        lgNames = "  (func: " + lgNames + ")";
    }
    clog("lf", depthIndent, indent, "cost: ", cost, "  cost_checked: ",
         (int) cost_checked, lgNames);
  }

  if (pea.find("target") != pea.end()) {
    int32 target = e->target; //ref handle
    std::string lgNames = "";

    if (target != 0) {
      duel* pD = e->pduel;
      auto re = getRegistryEntryByRef(pD->lua->lua_state, target, false);
      if (re.type == "function") {
        auto lgv = getLuaGlobalsByPtr(pD->lua->lua_state, re.ptr);

        std::vector<std::string> lgsv;
        for (const auto &lg: lgv)
          lgsv.push_back(lg.name);

        lgNames = joinString(lgsv, ", ");
      }
      if (!lgNames.empty())
        lgNames = "  (func: " + lgNames + ")";
    }
    clog("lf", depthIndent, indent, "target: ", target, lgNames);
  }

  if (pea.find("operation") != pea.end()) {
    int32 operation = e->operation; //ref handle
    std::string lgNames = "";

    if (operation != 0) {
      duel* pD = e->pduel;
      auto re = getRegistryEntryByRef(pD->lua->lua_state, operation, false);
      if (re.type == "function") {
        auto lgv = getLuaGlobalsByPtr(pD->lua->lua_state, re.ptr);

        std::vector<std::string> lgsv;
        for (const auto &lg: lgv)
          lgsv.push_back(lg.name);

        lgNames = joinString(lgsv, ", ");
      }
      if (!lgNames.empty())
        lgNames = "  (func: " + lgNames + ")";
    }
    clog("lf", depthIndent, indent, "operation: ", operation, lgNames);
  }

  if (pea.find("value") != pea.end()) {
    int32 value = e->value; //ref handle
    std::string lgNames = "";
    std::string effvStr = "";

    if (value != 0) {
      duel* pD = e->pduel;
      auto re = getRegistryEntryByRef(pD->lua->lua_state, value, false);
      if (re.type == "function") {
        auto lgv = getLuaGlobalsByPtr(pD->lua->lua_state, re.ptr);

        std::vector<std::string> lgsv;
        for (const auto &lg: lgv)
          lgsv.push_back(lg.name);

        lgNames = joinString(lgsv, ", ");
      }
      if (!lgNames.empty())
        lgNames = "  (func: " + lgNames + ")";

      effvStr = "  (is EFFECT_FLAG_FUNC_VALUE: "
        + std::string((e->flag[0] & EFFECT_FLAG_FUNC_VALUE) ? "true" : "false")
        + ")";
    }

    clog("lf", depthIndent, indent, "value: ", value, effvStr, lgNames);
  }
}


const std::unordered_multimap<const void*, LuaGlobalEntry> luaGlobalsToMap(lua_State* L) {
  return luaGlobalsToMap(L, 3, 0);
}

const std::unordered_multimap<const void*, LuaGlobalEntry> luaGlobalsToMap(lua_State* L, const int maxDepth, const int depth) {
  std::unordered_multimap<const void*, LuaGlobalEntry> results;

  if (depth == 0)
    lua_pushglobaltable(L);

  int indexOfTable = lua_gettop(L); //get index of table

  lua_pushnil(L); //push nil as first key
  while (lua_next(L, indexOfTable) != 0) {
    std::string keyType = lua_typename(L, lua_type(L, -2));
    std::string valType = lua_typename(L, lua_type(L, -1));
    std::string keyVal;
    std::string valVal;
    const void* keyPtr = lua_topointer(L, -2);
    const void* valPtr = lua_topointer(L, -1);


    lua_pushvalue(L, -2); //make copy of key
    keyVal = lua_tostring(L, -1);
    lua_pop(L, 1); //remove copy of key

    lua_pushvalue(L, -1); //make copy of val
    if ((valType == "number") || (valType == "string")) {
      valVal = lua_tostring(L, -1);
    } else {
      valVal = luaValueAtIndexToString(L, -1);
    }
    lua_pop(L, 1); //remove copy of val

    LuaGlobalEntry lge = {
      .name = keyVal,
      .type = valType,
      .value = valVal,
      .ptr = valPtr
    };

    results.insert(std::make_pair(valPtr, lge));

    if ((valType == "table") && (depth < maxDepth) &&
        (keyVal != "_G") && (keyVal != "__index")) {
      auto stm = luaGlobalsToMap(L, maxDepth, (depth + 1));

      for (auto &m: stm) {
        std::string keyStr = keyVal + "." + m.second.name;
        m.second.name = keyStr;
        results.insert(std::make_pair(m.second.ptr, m.second));
      }
    }

    lua_pop(L, 1); //pop only the value, keeping the key on stack
  }

  if (depth == 0)
    lua_pop(L, 1); //pop global table at end

  return results;
}

const std::unordered_map<int32, LuaRegistryEntry> luaRegistryToMap(lua_State* L) {
  std::unordered_map<int32, LuaRegistryEntry> resultsMap;

  lua_pushnil(L); //push nil as first key
  while (lua_next(L, LUA_REGISTRYINDEX) != 0) {
    std::string valType = lua_typename(L, lua_type(L, -1));
    int32 keyVal;
    std::string valVal;
    const void* valPtr = lua_topointer(L, -1);

    if (lua_type(L, -2) != LUA_TNUMBER) {
      //only care about ref handles which are numbers
      //other internal tables and userdata are present though with
      // strings as keys
      //(since our LuaRegistryEntry ref_handle member is a int32,
      // can't store the string keys in map here)

      //prepare and then go to next iteration
      lua_pop(L, 1); //pop only the value, keeping the key on stack
      continue;
    }

    lua_pushvalue(L, -2); //make copy of key
    keyVal = lua_tonumber(L, -1);
    lua_pop(L, 1); //remove copy of key

    LuaRegistryEntry lre = {
      .ref_handle = keyVal,
      .type = valType,
      .value = luaValueAtIndexToString(L, -1),
      .ptr = valPtr
    };

    resultsMap[keyVal] = lre;

    lua_pop(L, 1); //pop only the value, keeping the key on stack
  }

  return resultsMap;
}

const std::unordered_multimap<const void*, DuelVarEntry> duelVarsToMap(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;
  std::unordered_multimap<const void*, DuelVarEntry> resultsMap;

  for (card* const &c: pD->cards) {
    DuelVarEntry dve = {.type = "card", .origin = "cards", .ptr = c};
    resultsMap.insert(std::make_pair(c, dve));
  }
  for (card* const &a: pD->assumes) {
    DuelVarEntry dve = {.type = "card", .origin = "assumes", .ptr = a};
    resultsMap.insert(std::make_pair(a, dve));
  }
  for (group* const &g: pD->groups) {
    DuelVarEntry dve = {.type = "group", .origin = "groups", .ptr = g};
    resultsMap.insert(std::make_pair(g, dve));
  }
  for (group* const &s: pD->sgroups) {
    DuelVarEntry dve = {.type = "group", .origin = "sgroups", .ptr = s};
    resultsMap.insert(std::make_pair(s, dve));
  }
  for (effect* const &e: pD->effects) {
    DuelVarEntry dve = {.type = "effect", .origin = "effects", .ptr = e};
    resultsMap.insert(std::make_pair(e, dve));
  }
  for (effect* const &u: pD->uncopy) {
    DuelVarEntry dve = {.type = "effect", .origin = "uncopy", .ptr = u};
    resultsMap.insert(std::make_pair(u, dve));
  }
  for (std::pair<int32, std::pair<lua_State*, int32>> const &co:
         pD->lua->coroutines) {
    DuelVarEntry dve = {
      .type = "coroutine",
      .origin = "coroutines",
      .ptr = co.second.first
    };
    resultsMap.insert(std::make_pair(co.second.first, dve));
  }

  return resultsMap;
}
