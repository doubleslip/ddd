/*
 * ddddebug.hpp
 */
//collection of various helper functions to help understand the
// present state of things
// (to be called while debugging or in a test)


#ifndef __DDDDEBUG_HPP
#define __DDDDEBUG_HPP

#include "dddutil.hpp"
#include "lua.hpp"

#include "duel.h"
#include "card.h" //for some reason not in ocgapi.h but in ocgapi.cpp
#include "group.h"
#include "effect.h"
#include "interpreter.h"


struct LuaGlobalEntry {
  std::string name;
  std::string type;
  std::string value;
  const void* ptr;
};
struct LuaRegistryEntry {
  int32 ref_handle;
  std::string type;
  std::string value;
  const void* ptr;
};
struct DuelVarEntry {
  std::string type;
  std::string origin;
  const void* ptr;
};

void printLuaGlobalsByPtr(lua_State*);
bool printLuaGlobalsByPtr(lua_State*, const void*, const int, const bool);
std::vector<LuaGlobalEntry> getLuaGlobalsByPtr(lua_State*, const void*);
void printLuaGlobalsByRef(lua_State*);
bool printLuaGlobalsByRef(lua_State*, const int32, const int, const bool);
void printLuaRegistryEntryByPtr(lua_State*);
bool printLuaRegistryEntryByPtr(lua_State*, const void*, const int, const bool);
std::vector<LuaRegistryEntry> getRegistryEntryByPtr(lua_State*, const void*);
void printLuaRegistryEntryByRef(lua_State*);
bool printLuaRegistryEntryByRef(lua_State*, const int32, const int, const bool);
LuaRegistryEntry getRegistryEntryByRef(lua_State*, const int32, const bool);
void printDuelVarsByPtr(intptr_t);
bool printDuelVarsByPtr(intptr_t, const void*, const int, const bool);
void printDuelVarsByRef(intptr_t);
bool printDuelVarsByRef(intptr_t, const int32, const int, const bool);
std::vector<std::pair<void*, std::string>> getDuelVarsByRef(intptr_t, const int32);
void printLuaStackEntriesByPtr(lua_State*);
bool printLuaStackEntriesByPtr(lua_State*, const void*, const int, const bool);
std::vector<std::pair<std::pair<int, int>, std::pair<std::string, std::string>>> getLuaStackEntriesByPtr(lua_State*, const void*, const bool);

void ptrSearch(intptr_t);
void ptrSearch(intptr_t, const void*, const int);
void refSearch(intptr_t);
void refSearch(intptr_t, const int32, const int);

void cardNameSearch(intptr_t, const bool);
void cardNameSearch(intptr_t, const std::string, const int, const bool);

std::string luaValueAtIndexToString(lua_State*, const int);
std::string luaValueAtIndexToString(lua_State*, const int, bool);

void printCardDebug(card*);
void printCardDebug(card*, const int);
void printCardDebug(card*, const int, std::vector<const void*>);
void printGroupDebug(group*);
void printGroupDebug(group*, const int);
void printGroupDebug(group*, const int, std::vector<const void*>);
void printEffectDebug(effect*);
void printEffectDebug(effect*, const int);
void printEffectDebug(effect*, const int, std::vector<const void*>);

const std::unordered_multimap<const void*, LuaGlobalEntry> luaGlobalsToMap(lua_State*);
const std::unordered_multimap<const void*, LuaGlobalEntry> luaGlobalsToMap(lua_State*, const int, const int);
const std::unordered_map<int32, LuaRegistryEntry> luaRegistryToMap(lua_State*);
const std::unordered_multimap<const void*, DuelVarEntry> duelVarsToMap(intptr_t);

#endif

