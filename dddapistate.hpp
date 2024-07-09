/*
 * dddapistate.hpp
 */

#ifndef __DDDAPISTATE_HPP
#define __DDDAPISTATE_HPP

#include "ocgapi.h"
#include "duel.h"
#include "card.h"
#include "group.h"
#include "effect.h"
#include "interpreter.h"

#include "dddsingleton.hpp"
#include "dddutil.hpp"
#include "dddapi.hpp"
#include "ddddebug.hpp"


namespace ddd {
  unsigned long long createDuelState(bool, bool, unsigned int);

  intptr_t duplicateDuel(intptr_t);
  intptr_t duplicateDuel(intptr_t, intptr_t);
  bool isDuelDuplicatable(intptr_t);
  bool isEffectEquivalent(effect*, effect*);

  bool copyAndMapCards(intptr_t, intptr_t, std::unordered_map<card*, card*>&, std::unordered_map<card*, card*>&, std::unordered_map<effect*, effect*>&, std::unordered_map<effect*, effect*>&, std::unordered_map<int32, int32>&, std::unordered_set<card*>&);
  bool mapEffectsFromCardEffectContainers(const std::unordered_set<card*>&, const std::unordered_map<card*, card*>&, std::unordered_map<effect*, effect*>&, std::unordered_map<effect*, effect*>&, std::unordered_map<int32, int32>&, bool, std::unordered_set<card*>&);
  bool copyAndMapGroups(intptr_t, intptr_t, std::unordered_map<group*, group*>&, std::unordered_map<group*, group*>&, std::unordered_map<int32, int32>&);
  bool copyAndMapEffects(intptr_t, intptr_t, const std::unordered_map<card*, card*>&, std::unordered_map<effect*, effect*>&, std::unordered_map<effect*, effect*>&, std::unordered_map<int32, int32>&, std::unordered_set<card*>&);
  bool mapAssumes(intptr_t, intptr_t, const std::unordered_map<card*, card*>&);
  bool mapSgroups(intptr_t, intptr_t, const std::unordered_map<group*, group*>&);
  bool mapUncopy(intptr_t, intptr_t, const std::unordered_map<effect*, effect*>&);
  bool mapCardMembers(intptr_t, intptr_t, const std::unordered_map<card*, card*>&, const std::unordered_map<card*, card*>&, const std::unordered_map<effect*, effect*>&, const std::unordered_map<effect*, effect*>&, const std::unordered_map<int32, int32>&);
  bool mapGroupMembers(intptr_t, intptr_t, const std::unordered_map<card*, card*>&, const std::unordered_map<group*, group*>&);
  bool mapEffectMembers(intptr_t, const std::unordered_map<card*, card*>&, const std::unordered_map<card*, card*>&, const std::unordered_map<effect*, effect*>&);
  bool copyAndMapInterpreter(intptr_t, intptr_t, const std::unordered_map<card*, card*>&, const std::unordered_map<group*, group*>&, const std::unordered_map<effect*, effect*>&, const std::unordered_map<int32, int32>&);
  bool copyAndMapInterpreterLuaState(lua_State*&, lua_State*&, const std::unordered_map<int32, int32>&);
  bool copyAndMapFieldPlayerInfo(intptr_t, intptr_t, const std::unordered_map<card*, card*>&);
  bool copyAndMapFieldFieldEffect(intptr_t, intptr_t, const std::unordered_map<card*, card*>&, const std::unordered_map<effect*, effect*>&);
  bool copyAndMapFieldProcessor(intptr_t, intptr_t, const std::unordered_map<card*, card*>&, const std::unordered_map<group*, group*>&, const std::unordered_map<effect*, effect*>&);

  std::tuple<unsigned long long, DuelState> duplicateDuelState(const DuelState&);
  std::tuple<unsigned long long, DuelState> duplicateDuelStateFromShadow(const intptr_t, const DuelState&, const ShadowDuelStateResponses&);
  std::tuple<unsigned long long, DuelState> duplicateDuelStateFromShadow(const intptr_t, const DuelState&, const ShadowDuelStateResponses&, const intptr_t);
  std::tuple<unsigned long long, DuelState> assumeDuelState(DuelState&);
  bool isDuelStateActive(const DuelState&);
  bool deactivateDuelState(DuelState&);
  bool reactivateDuelState(const unsigned long long, DuelState&, const intptr_t, const ShadowDuelStateResponses&);
  bool removeDuelState(const unsigned long long);
  void setDuelStateResponseI(DuelState&, const int32);
  void setDuelStateResponseB(DuelState&, const byte*);
  std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> getFieldVisualGamestate(unsigned long long);
}
#endif

