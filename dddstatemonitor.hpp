/*
 * dddstatemonitor.hpp
 */
//definition of the StateMonitorTest class and its functions
// used to store and monitor certain values in a pDuel and monitor
// their changes


#include "dddutil.hpp"
#include "ddddebug.hpp"
#include "ocgapi.h"


#ifndef __DDDSTATEMONITOR_HPP
#define __DDDSTATEMONITOR_HPP

class StateMonitorTest {
private:
  struct LCard {
    card c;

    LCard(card* pc) : c(nullptr) {
      if (pc) {
        c = *pc;

        if (pc->indexer.size() > 0) {
          //demonstration of (imo) the redundency of the indexer member if true,
          // should never print anything but will warn if an assumption (of two)
          // is wrong

          for (const auto &ep: pc->indexer) {
            if (ep.first != ep.second->second) {
              clog("w", "Expected key (", ep.first, ") for indexer at c(", pc,
                   ").indexer[", ep.first, "] to be equal (indexer[", ep.first,
                   "]->second: (expected: e(", ep.first, "); got: e(",
                   ep.second->second, "))");
              printEffectDebug(ep.first);
              printEffectDebug(ep.second->second);
            }

            if (ep.first->code != ep.second->first)
              clog("w", "Expected status at e(", ep.first,
                   ")->code for indexer at c(", pc, ").indexer[", ep.first,
                   "] to be equal (indexer[", ep.first, "]->first: (expected: ",
                   ep.first->code, "; got: ", ep.second->first, "))");
          }
        }
      }
    }
  };
  struct LGroup {
    group g;
    card* lit;

    LGroup(group* pg) : g(nullptr) {
      if (pg) {
        g = *pg;

        //no telling when iterator will be invalidated; store result instead
        lit = (pg->it != pg->container.end()) ? *(pg->it) : nullptr;

      } else {
        lit = nullptr;
      }
    }
  };
  struct LEffect {
    effect e;
    LEffect(effect* pe) : e(nullptr) {
      if (pe) {
        e = *pe;
      }
    }
  };
  struct LuaStateInfo {
    //custom struct to store lua_State info
    lua_State* L;

    struct LuaStackEntry {
      const void* ptr;
      std::string type;
      std::string value;
    };

    std::vector<LuaStackEntry> luaStackEntries;

    LuaStateInfo(lua_State* pL) {
      L = pL;
      if (pL) {
        //populate luaStackEntries here
        int top = lua_gettop(L);

        for (int i = 1; i <= top; ++i) {
          LuaStackEntry lse = {
            .ptr = lua_topointer(L, i),
            .type = lua_typename(L, lua_type(L, i)),
            .value = luaValueAtIndexToString(L, i, false) //from ddddebug.cpp
          };
          luaStackEntries.push_back(lse);
        }
      }
    }
  };
  struct LInterpreter {
    struct LInterpreterInterpreter {
      //custom struct to avoid new memory allocated calls from luaL_newstate()
      duel* pduel;
      lua_State* lua_state;
      lua_State* current_state;
      std::list<std::pair<interpreter::lua_param, LuaParamType>> params;
      std::list<std::pair<interpreter::lua_param, LuaParamType>> resumes;
      int32 no_action;
      int32 call_depth;
    };

    LInterpreterInterpreter i;
    LuaStateInfo llua_state;
    LuaStateInfo lcurrent_state;

    std::unordered_map<int32, std::pair<LuaStateInfo, int32>> lcoroutines;

    LInterpreter(interpreter* pi) :
      llua_state((pi) ? pi->lua_state : nullptr),
      lcurrent_state((pi) ? pi->current_state : nullptr)
    {
      if (pi) {
        i.pduel = pi->pduel;
        i.lua_state = pi->lua_state;
        i.current_state = pi->current_state;
        i.params = pi->params;
        i.resumes = pi->resumes;
        i.no_action = pi->no_action;
        i.call_depth = pi->call_depth;

        for (const auto &cr: pi->coroutines)
          lcoroutines.emplace(cr.first, std::make_pair(cr.second.first,
                                                       cr.second.second));

      } else {
        i.pduel = nullptr;
        i.lua_state = nullptr;
        i.current_state = nullptr;
        i.no_action = 0;
        i.call_depth = 0;
      }
    }
  };
  struct LField {
    field f;
    LField(field* pf) : f(nullptr) {
      if (pf) {
        f = *pf;
        //also need special logic for pf->effects.indexer
      }
    }
  };
  struct LDuel {
    struct LDuelDuel {
      std::vector<byte> message_buffer;
      interpreter* lua;
      field* game_field;
    };
    duel* originalPDuel;

    //duel d; //can't because uses mt19937 class which can't use default '=' op
    LDuelDuel d;

    LInterpreter llua;
    LField lgame_field;

    std::unordered_map<card*, LCard> lcards;
    std::unordered_map<card*, LCard> lassumes;
    std::unordered_map<group*, LGroup> lgroups;
    std::unordered_map<group*, LGroup> lsgroups;
    std::unordered_map<effect*, LEffect> leffects;
    std::unordered_map<effect*, LEffect> luncopy;
    LDuel(duel* pDuel) :
      lgame_field((pDuel) ? pDuel->game_field : nullptr),
      llua((pDuel) ? pDuel->lua : nullptr)
    {
      if (pDuel) {
        originalPDuel = pDuel;

        d = LDuelDuel();
        d.message_buffer = pDuel->message_buffer;
        d.lua = pDuel->lua;
        d.game_field = pDuel->game_field;

        for (const auto &c: pDuel->cards)
          lcards.emplace(c, c); //lcards[c] = LCard(c);
        for (const auto &a: pDuel->assumes)
          lassumes.emplace(a, a); //lassumes[a] = LCard(a);
        for (const auto &g: pDuel->groups)
          lgroups.emplace(g, g); //lgroups[g] = LGroup(g);
        for (const auto &s: pDuel->sgroups)
          lsgroups.emplace(s, s); //lsgroups[s] = LGroup(s);
        for (const auto &e: pDuel->effects)
          leffects.emplace(e, e); //leffects[e] = LEffect(e);
        for (const auto &u: pDuel->uncopy)
          luncopy.emplace(u, u); //luncopy[u] = LEffect(u);

      } else {
        originalPDuel = nullptr;

        d = LDuelDuel();
        d.lua = nullptr;
        d.game_field = nullptr;
      }
    }
  };

  bool initialized;

public:
  intptr_t lpDuel;

  LDuel lduel;

  StateMonitorTest();
  StateMonitorTest(intptr_t pDuel);
  void captureGamestate(intptr_t);
  void reset();

private:
  template <typename T, typename U>
  std::string toChgStr(std::string pref, T t, U u) {
    return toChgStr(pref, t, u, true);
  }
  template <typename T, typename U>
  std::string toChgStr(std::string pref, T t, U u, bool wrapBrackets) {
    std::stringstream buffer;
    buffer << pref << ": \t";

    if (wrapBrackets)
      buffer << "(";

    if constexpr((std::is_same_v<T, uint8>) || (std::is_same_v<T, int8>))
      buffer << (int) t;
    else
      buffer << t;

    if (wrapBrackets)
      buffer << ")";

    buffer << " -> ";

    if (wrapBrackets)
      buffer << "(";

    if constexpr((std::is_same_v<U, uint8>) || (std::is_same_v<U, int8>))
      buffer << (int) u;
    else
      buffer << u;

    if (wrapBrackets)
      buffer << ")";

    return buffer.str();
  }

  std::vector<std::string> getLduelChanges(LDuel*, LDuel*);
  std::vector<std::string> getLinterpreterChanges(std::string, LInterpreter*, LInterpreter*);
  std::vector<std::string> getLfieldChanges(std::string, LField*, LField*);


  template <template<typename, typename> typename T, typename U, typename V>
  std::vector<std::string> get_MapChanges(std::string, T<U, V>*, T<U, V>*);
  std::vector<std::string> get_EffectContainerChanges(std::string, std::multimap<uint32, effect*>*, std::multimap<uint32, effect*>*);
  template <template<typename> typename T, typename U, typename... Uoptional>
  std::vector<std::string> get_VectorChanges(std::string, T<U, Uoptional...>*, T<U, Uoptional...>*);
  template <template<typename> typename T, typename U, typename... Uoptional>
  std::vector<std::string> get_SetChanges(std::string, T<U, Uoptional...>*, T<U, Uoptional...>*);
  std::vector<std::string> get_LuaStateChanges(std::string, LuaStateInfo*, LuaStateInfo*);

  std::vector<std::string> getCardChanges(std::string, card*, card*);
  std::vector<std::string> getCardStateChanges(std::string, card_state*, card_state*);
  std::vector<std::string> getGroupChanges(std::string, group*, group*);
  std::vector<std::string> getEffectChanges(std::string, effect*, effect*);
  std::vector<std::string> getPlayerInfosChanges(std::string, player_info*, player_info*);
  std::vector<std::string> getFieldInfosChanges(std::string, field_info*, field_info*);
  std::vector<std::string> getFieldEffectsChanges(std::string, field_effect*, field_effect*);
  std::vector<std::string> getProcessorChanges(std::string, processor*, processor*);
  std::vector<std::string> getProcessorUnitChanges(std::string, const processor_unit*, const processor_unit*);
  std::vector<std::string> getTeventChanges(std::string, const tevent*, const tevent*);
  std::vector<std::string> getChainChanges(std::string, const chain*, const chain*);
};

#endif
