/*
 * dddstatemonitor.cpp
 */
//definition of the StateMonitorTest class and its functions
// used to store and monitor certain values in a pDuel and monitor
// their changes


#include "dddstatemonitor.hpp"


StateMonitorTest::StateMonitorTest() : StateMonitorTest(0) {
}

StateMonitorTest::StateMonitorTest(intptr_t pDuel)
  : lpDuel(pDuel),
    lduel((duel*) pDuel) {

  initialized = !!lpDuel;
}
void StateMonitorTest::captureGamestate(intptr_t pDuel) {
  duel* pD = (duel*) pDuel;
  LDuel newlduel = LDuel(pD);

  if ((initialized) && (pDuel == lpDuel)) {
    std::vector<std::string> changes = getLduelChanges(&lduel, &newlduel);

    if (changes.size() > 1) {
      clog("d", "State changes (", changes.size(), "):");
      for (const auto &chg: changes)
        clog("lf", "    ", chg);
    }

  } else {
    initialized = true;
  }
  lpDuel = pDuel;
  lduel = newlduel;
}
void StateMonitorTest::reset() {
  initialized = false;
}
std::vector<std::string> StateMonitorTest::getLduelChanges(LDuel* olduel, LDuel* nlduel) {

  std::vector<std::string> changes;
  const std::string pref = "duel";

  //ignore duel_strbuffer;
  //ignore buffer

  //bufferlen
  if (nlduel->d.message_buffer.size() != olduel->d.message_buffer.size())
    changes.push_back(toChgStr(pref + ".message_buffer.size()",
                               olduel->d.message_buffer.size(),
                               nlduel->d.message_buffer.size()));

  //ignore bufferp

  //lua
  if (nlduel->d.lua != olduel->d.lua)
    changes.push_back(toChgStr(pref + "->lua", olduel->d.lua, nlduel->d.lua, true));
  auto interpreterChanges = getLinterpreterChanges(pref + "->lua",
                                                   &(olduel->llua),
                                                   &(nlduel->llua));
  changes.insert(changes.end(), interpreterChanges.begin(), interpreterChanges.end());

  //game_field
  if (nlduel->d.game_field != olduel->d.game_field)
    changes.push_back(toChgStr(pref + "->game_field",
                               olduel->d.game_field,
                               nlduel->d.game_field, true));
  auto gameFieldChanges = getLfieldChanges(pref + "->game_field",
                                           &(olduel->lgame_field),
                                           &(nlduel->lgame_field));
  changes.insert(changes.end(), gameFieldChanges.begin(), gameFieldChanges.end());

  //ignore random (mt19937)

  //cards
  auto cardsChanges = get_MapChanges(pref + ".cards",
                                     &(olduel->lcards),
                                     &(nlduel->lcards));
  changes.insert(changes.end(), cardsChanges.begin(), cardsChanges.end());
  //assumes
  auto assumesChanges = get_MapChanges(pref + ".assumes",
                                       &(olduel->lassumes),
                                       &(nlduel->lassumes));
  changes.insert(changes.end(), assumesChanges.begin(), assumesChanges.end());
  //groups
  auto groupsChanges = get_MapChanges(pref + ".groups",
                                      &(olduel->lgroups),
                                      &(nlduel->lgroups));
  changes.insert(changes.end(), groupsChanges.begin(), groupsChanges.end());
  //sgroups
  auto sgroupsChanges = get_MapChanges(pref + ".sgroups",
                                       &(olduel->lsgroups),
                                       &(nlduel->lsgroups));
  changes.insert(changes.end(), sgroupsChanges.begin(), sgroupsChanges.end());
  //effects
  auto effectsChanges = get_MapChanges(pref + ".effects",
                                       &(olduel->leffects),
                                       &(nlduel->leffects));
  changes.insert(changes.end(), effectsChanges.begin(), effectsChanges.end());
  //uncopy
  auto uncopyChanges = get_MapChanges(pref + ".uncopy",
                                      &(olduel->luncopy),
                                      &(nlduel->luncopy));
  changes.insert(changes.end(), uncopyChanges.begin(), uncopyChanges.end());

  return changes;
}

std::vector<std::string> StateMonitorTest::getLinterpreterChanges(std::string pref, LInterpreter* oi, LInterpreter* ni) {
  std::vector<std::string> changes;

  //lua_state
  if (oi->i.lua_state != ni->i.lua_state)
    changes.push_back(toChgStr(pref + ".lua_State",
                               oi->i.lua_state,
                               ni->i.lua_state, true));
  auto luaStateChanges = get_LuaStateChanges(pref + "->lua_State",
                                             &(oi->llua_state),
                                             &(ni->llua_state));
  changes.insert(changes.end(), luaStateChanges.begin(), luaStateChanges.end());

  //current_state
  if ((oi->i.current_state == oi->i.lua_state) &&
      (ni->i.current_state == ni->i.lua_state) &&
      (luaStateChanges.size() > 0)) {
    std::stringstream buffer;
    buffer << pref << ".current_state   (same changes as " << pref << ".lua_state)";
    changes.push_back(buffer.str());
  } else {
    if (oi->i.current_state != ni->i.current_state)
      changes.push_back(toChgStr(pref + ".current_state",
                                 oi->i.current_state,
                                 ni->i.current_state, true));
    auto currentStateChanges = get_LuaStateChanges(pref + "->current_state",
                                                   &(oi->lcurrent_state),
                                                   &(ni->lcurrent_state));
    changes.insert(changes.end(),
                   currentStateChanges.begin(),
                   currentStateChanges.end());
  }

  //param_list params;
  auto paramsChanges = get_VectorChanges(pref + ".params",
                                         &(oi->i.params),
                                         &(ni->i.params));
  changes.insert(changes.end(), paramsChanges.begin(), paramsChanges.end());

  //param_list resumes;
  auto resumesChanges = get_VectorChanges(pref + ".resumes",
                                          &(oi->i.resumes),
                                          &(ni->i.resumes));
  changes.insert(changes.end(), resumesChanges.begin(), resumesChanges.end());

  //coroutines
  auto coroutinesChanges = get_MapChanges(pref + ".coroutines",
                                          &(oi->lcoroutines),
                                          &(ni->lcoroutines));
  changes.insert(changes.end(), coroutinesChanges.begin(), coroutinesChanges.end());

  //no_action
  if (oi->i.no_action != ni->i.no_action)
    changes.push_back(toChgStr(pref + ".no_action",
                               oi->i.no_action,
                               ni->i.no_action));

  //call_depth
  if (oi->i.call_depth != ni->i.call_depth)
    changes.push_back(toChgStr(pref + ".call_depth",
                               oi->i.call_depth,
                               ni->i.call_depth));

  return changes;
}

std::vector<std::string> StateMonitorTest::getLfieldChanges(std::string pref, LField* of, LField* nf) {
  std::vector<std::string> changes;


  //player
  auto playerInfos0Changes = getPlayerInfosChanges(pref + ".player[0]",
                                                   &(of->f.player[0]),
                                                   &(nf->f.player[0]));
  changes.insert(changes.end(),
                 playerInfos0Changes.begin(),
                 playerInfos0Changes.end());
  auto playerInfos1Changes = getPlayerInfosChanges(pref + ".player[1]",
                                                   &(of->f.player[1]),
                                                   &(nf->f.player[1]));
  changes.insert(changes.end(),
                 playerInfos1Changes.begin(),
                 playerInfos1Changes.end());

  //temp_card (is this already checked in pduel->cards?)
  if (of->f.temp_card != nf->f.temp_card)
    changes.push_back(toChgStr(pref + "->temp_card",
                               of->f.temp_card,
                               nf->f.temp_card, true));
  auto tempCardChanges = getCardChanges(pref + "->temp_card",
                                        of->f.temp_card,
                                        nf->f.temp_card);
  changes.insert(changes.end(),
                 tempCardChanges.begin(),
                 tempCardChanges.end());

  //infos
  auto fieldInfosChanges = getFieldInfosChanges(pref + ".infos",
                                                &(of->f.infos),
                                                &(nf->f.infos));
  changes.insert(changes.end(), fieldInfosChanges.begin(), fieldInfosChanges.end());

  //effects
  auto fieldEffectsChanges = getFieldEffectsChanges(pref + ".effects",
                                                    &(of->f.effects),
                                                    &(nf->f.effects));
  changes.insert(changes.end(),
                 fieldEffectsChanges.begin(),
                 fieldEffectsChanges.end());

  //core
  auto processorChanges = getProcessorChanges(pref + ".core",
                                              &(of->f.core),
                                              &(nf->f.core));
  changes.insert(changes.end(), processorChanges.begin(), processorChanges.end());

  //returns

  //nil_event;
  auto nilEventChanges = getTeventChanges(pref + ".nil_event",
                                          &(of->f.nil_event),
                                          &(nf->f.nil_event));
  changes.insert(changes.end(), nilEventChanges.begin(), nilEventChanges.end());

  return changes;
}


template <template<typename, typename> typename T, typename U, typename V>
std::vector<std::string> StateMonitorTest::get_MapChanges(std::string pref, T<U, V>* om, T<U, V>* nm) {
  std::vector<std::string> changes;

  //lambda declarations
  std::function<std::string(const U*)> printKey = [](const U* u) {
    std::stringstream buffer;
    buffer << *u;
    return buffer.str();
  };
  std::function<std::string(std::string, const U*)> appendPref = [&printKey](std::string pref, const U* u) {
    std::stringstream buffer;
    buffer << pref << "[" << printKey(u) << "]";
    return buffer.str();
  };
  std::function<std::vector<std::string>(StateMonitorTest*, std::string, V*, V*)> getChanges = [](StateMonitorTest* smt, std::string pref, V* ov, V* nv) {
    return std::vector<std::string>();
  };

  //redefine lambdas for special handling based on types passed
  if constexpr((std::is_same_v<U, card*>) && (std::is_same_v<V, LCard>)) {
    //LDuel.lcards and LDuel.lassumes
    printKey = [](const U* c) {
      std::stringstream buffer;
      buffer << "c(" << *c << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* oc, V* nc) {
      return smt->getCardChanges(pref, &(oc->c), &(nc->c));
    };
  } else if constexpr((std::is_same_v<U, group*>) && (std::is_same_v<V, LGroup>)) {
    //LDuel.lgroups and LDuel.lsgroups
    printKey = [](const U* g) {
      std::stringstream buffer;
      buffer << "g(" << *g << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* og, V* ng) {
      auto groupChanges = smt->getGroupChanges(pref, &(og->g), &(ng->g));

      //special logic for group->lit
      if (og->lit != ng->lit) {
        std::stringstream itr1buffer;
        if (og->lit != nullptr)
          itr1buffer << "<itr c(" << og->lit << ")>";
        else
          itr1buffer << "<itr end>";

        std::stringstream itr2buffer;
        if (ng->lit != nullptr)
          itr2buffer << "<itr c(" << ng->lit << ")>";
        else
          itr2buffer << "<itr end>";

        if (itr1buffer.str() != itr2buffer.str())
          groupChanges.push_back(smt->toChgStr(pref + ".it",
                                               itr1buffer.str(),
                                               itr2buffer.str(), false));
      }
      return groupChanges;
    };
  } else if constexpr((std::is_same_v<U, effect*>) && (std::is_same_v<V, LEffect>)) {
    //LDuel.leffects and LDuel.luncopy
    printKey = [](const U* e) {
      std::stringstream buffer;
      buffer << "e(" << *e << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* oe, V* ne) {
      return smt->getEffectChanges(pref, &(oe->e), &(ne->e));
    };
  } else if constexpr((std::is_same_v<U, int32>) && (std::is_same_v<V, LuaStateInfo>)) {
    //interpreter::coroutine_map
    getChanges = [](StateMonitorTest* smt, std::string pref, V* ocmv, V* ncmv) {
      return smt->get_LuaStateChanges(pref, ocmv, ncmv);
    };
  } else if constexpr((std::is_same_v<U, card*>) && (std::is_same_v<V, uint32>)) {
    //card::relation_map
    printKey = [](const U* rm) {
      std::stringstream buffer;
      buffer << "c(" << *rm << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* ormv, V* nrmv) {
      std::vector<std::string> changes;
      if (*ormv != *nrmv) {
        changes.push_back(smt->toChgStr(pref, *ormv, *nrmv));
      }
      return changes;
    };
  } else if constexpr((std::is_same_v<U, uint16>) && (std::is_same_v<V, std::array<uint16, 2>>)) {
    //card::counter_map
    getChanges = [](StateMonitorTest* smt, std::string pref, V* ocmv, V* ncmv) {
      std::vector<std::string> changes;

      if (*ocmv != *ncmv) {
        std::stringstream buffer1;
        std::stringstream buffer2;

        for (auto i = ocmv->begin(); i != ocmv->end(); ++i) {
          if (i != ocmv->begin())
            buffer1 << ", ";

          buffer1 << *i;
        }
        for (auto i = ncmv->begin(); i != ncmv->end(); ++i) {
          if (i != ncmv->begin())
            buffer2 << ", ";

          buffer2 << *i;
        }

        changes.push_back(smt->toChgStr(pref,
                                        "[" + buffer1.str() + "]",
                                        "[" + buffer2.str() + "]" , false));
      }

      return changes;
    };
  } else if constexpr((std::is_same_v<U, uint16>) && (std::is_same_v<V, std::pair<card*, uint32>>)) {
    //card::attacker_map
    getChanges = [](StateMonitorTest* smt, std::string pref, V* oamv, V* namv) {
      std::vector<std::string> changes;

      if ((oamv->first != namv->first) ||
          (oamv->second != namv->second)) {

        std::stringstream buffer1;
        std::stringstream buffer2;
        buffer1 << "[" << oamv->first << ", " << oamv->second << "]";
        buffer2 << "[" << namv->first << ", " << namv->second << "]";

        changes.push_back(smt->toChgStr(pref, buffer1.str(), buffer2.str(), false));
      }

      return changes;
    };
  } else if constexpr ((std::is_same_v<U, effect*>) && (std::is_same_v<V, field_effect::effect_container::iterator>)) {
    //field_effect::effect_indexer
    printKey = [](const U* ei) {
      std::stringstream buffer;
      buffer << "e(" << *ei << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* oei, V* nei) {
      //if shit crashes here, probably tried to dereference/get address of an .end()
      // ...which can neither be dereferenced or compared to without ub
      std::vector<std::string> changes;

      if (((*oei)->first != (*nei)->first) ||
          ((*oei)->second != (*nei)->second)) {

        std::stringstream buffer1;
        std::stringstream buffer2;
        buffer1 << "<itr [" << (*oei)->first << "]: e(" << (*oei)->second << ")>";
        buffer2 << "<itr [" << (*nei)->first << "]: e(" << (*nei)->second << ")>";

        changes.push_back(smt->toChgStr(pref, buffer1.str(), buffer2.str(), false));
      }

      return changes;
    };
  } else if constexpr ((std::is_same_v<U, effect*>) && (std::is_same_v<V, effect*>)) {
    //field_effect::oath_effects
    printKey = [](const U* oe) {
      std::stringstream buffer;
      buffer << "e(" << *oe << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* ooe, V* noe) {
      std::vector<std::string> changes;

      std::stringstream buffer1;
      std::stringstream buffer2;
      buffer1 << "e(" << *ooe << ")";
      buffer2 << "e(" << *noe << ")";

      if (*ooe != *noe)
        changes.push_back(smt->toChgStr(pref, buffer1.str(), buffer2.str()));

      return changes;
    };
  } else if constexpr ((std::is_same_v<U, card*>) && (std::is_same_v<V, effect*>)) {
    //field_effect::grant_effect
    printKey = [](const U* ge) {
      std::stringstream buffer;
      buffer << "c(" << *ge << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* oge, V* nge) {
      std::vector<std::string> changes;

      std::stringstream buffer1;
      std::stringstream buffer2;
      buffer1 << "e(" << *oge << ")";
      buffer2 << "e(" << *nge << ")";

      if (*oge != *nge)
        changes.push_back(smt->toChgStr(pref, buffer1.str(), buffer2.str()));

      return changes;
    };
  } else if constexpr ((std::is_same_v<U, effect*>) && (std::is_same_v<V, std::unordered_map<card*, effect*>>)) {
    //field_effect::grant_effect_container
    printKey = [](const U* gec) {
      std::stringstream buffer;
      buffer << "e(" << *gec << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* ogec, V* ngec) {
      return smt->get_MapChanges(pref, ogec, ngec);
    };

  } else if constexpr ((std::is_same_v<U, uint32>) && (std::is_same_v<V, std::pair<uint32, uint32>>)) {
    //processor::*_counter
    getChanges = [](StateMonitorTest* smt, std::string pref, V* ocmv, V* ncmv) {
      std::vector<std::string> changes;

      if ((ocmv->first != ncmv->first) ||
          (ocmv->second != ncmv->second)) {

        std::stringstream buffer1;
        std::stringstream buffer2;
        buffer1 << "[" << ocmv->first << ", " << ocmv->second << "]";
        buffer2 << "[" << ncmv->first << ", " << ncmv->second << "]";

        changes.push_back(smt->toChgStr(pref, buffer1.str(), buffer2.str(), false));
      }

      return changes;
    };
  } else if constexpr ((std::is_same_v<U, effect*>) && (std::is_same_v<V, chain>)) {
    //processor::instant_f_list
    printKey = [](const U* ifl) {
      std::stringstream buffer;
      buffer << "c(" << *ifl << ")";
      return buffer.str();
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, V* oc, V* nc) {
      return smt->getChainChanges(pref, oc, nc);
    };
  }


  //actually check for map differences
  if (nm->size() != om->size())
    changes.push_back(toChgStr(pref + ".size", om->size(), nm->size()));

  std::vector<std::string> addedStrV;
  //iterate new map
  for (auto &nt: *nm) {
    if (om->find(nt.first) == om->end()) {
      addedStrV.push_back(printKey(&(nt.first)));

    } else {
      //found key in both old and new map
      //check for any changes in key's values (no need to do again when iterating old map)

      auto valuesChanges = getChanges(this, appendPref(pref, &(nt.first)), &(om->find(nt.first)->second), &(nt.second));
      changes.insert(changes.end(), valuesChanges.begin(), valuesChanges.end());
    }
  }
  std::vector<std::string> removedStrV;
  //iterate old map
  for (const auto &ot: *om) {
    if (nm->find(ot.first) == nm->end()) {
      removedStrV.push_back(printKey(&(ot.first)));
    }
  }
  if ((addedStrV.size() > 0) || (removedStrV.size() > 0)) {
    std::stringstream buffer;
    buffer << pref << " (changes (-" << removedStrV.size() << "/+" << addedStrV.size() << ")): ";
    if (removedStrV.size() > 0) {
      if (removedStrV.size() == om->size())
        buffer << "-<all elements> -[";
      else
        buffer << "-[";

      buffer << joinString(removedStrV, "] -[") << "]";
    }

    if (addedStrV.size() > 0) {
      if (removedStrV.size() > 0)
        buffer << ' ';

      buffer << "+[" << joinString(addedStrV, "] +[") << "]";
    }

    changes.push_back(buffer.str());
  }

  return changes;
}

std::vector<std::string> StateMonitorTest::get_EffectContainerChanges(std::string pref, std::multimap<uint32, effect*>* omm, std::multimap<uint32, effect*>* nmm) {
  std::vector<std::string> changes;

  if (omm->size() != nmm->size())
    changes.push_back(toChgStr(pref + ".size", omm->size(), nmm->size()));

  std::vector<std::string> addedKeyStrV;
  std::vector<std::string> addedStrV;
  std::vector<std::string> removedKeyStrV;
  std::vector<std::string> removedStrV;

  std::set<uint32> ommUniqueKeys;
  std::set<uint32> nmmUniqueKeys;

  for (const auto &e: *omm)
    ommUniqueKeys.insert(e.first);
  for (const auto &e: *nmm)
    nmmUniqueKeys.insert(e.first);

  //get all old map keys and check if they also exist in new map
  for (const auto &e: ommUniqueKeys) {
    if (nmm->find(e) != nmm->end()) {
      auto oer = omm->equal_range(e);
      auto ner = nmm->equal_range(e);

      //iterate all old values for given key and check if also present in new map
      for (auto i = oer.first; i != oer.second; ++i) {
        bool found = false;
        for (auto j = ner.first; j != ner.second; ++j) {
          if (*i == *j) {
            found = true;
            break;
          }
        }
        if (!found) {
          std::stringstream buffer;
          buffer << "[" << i->first << "] " << i->second;
          removedStrV.push_back(buffer.str());
        }
      }

    } else {
      //key in old map not found in new map
      {
        std::stringstream buffer;
        buffer << e;
        removedKeyStrV.push_back(buffer.str());
      }

      auto ar = omm->equal_range(e);
      for (auto i = ar.first; i != ar.second; ++i) {
        std::stringstream buffer;
        buffer << "[" << i->first << "] " << i->second;
        removedStrV.push_back(buffer.str());
      }
    }
  }
  //get all new map keys and check if they also exist in old map
  for (const auto &e: nmmUniqueKeys) {
    if (omm->find(e) != omm->end()) {
      auto oer = omm->equal_range(e);
      auto ner = nmm->equal_range(e);

      //iterate all new values for given key and check if also present in old map
      for (auto i = ner.first; i != ner.second; ++i) {
        bool found = false;
        for (auto j = oer.first; j != oer.second; ++j) {
          if (*i == *j) {
            found = true;
            break;
          }
        }
        if (!found) {
          std::stringstream buffer;
          buffer << "[" << i->first << "] " << i->second;
          addedStrV.push_back(buffer.str());
        }
      }

    } else {
      //key in new map not found in old map
      {
        std::stringstream buffer;
        buffer << e;
        addedKeyStrV.push_back(buffer.str());
      }
      auto ar = nmm->equal_range(e);
      for (auto i = ar.first; i != ar.second; ++i) {
        std::stringstream buffer;
        buffer << "[" << i->first << "] " << i->second;
        addedStrV.push_back(buffer.str());
      }
    }
  }
  if ((addedKeyStrV.size() > 0) || (removedKeyStrV.size() > 0)) {
    std::stringstream buffer;
    buffer << pref << " (key changes (-" << removedKeyStrV.size() <<
      "/+" << addedKeyStrV.size() << ")): ";
    if (removedKeyStrV.size() > 0)
      buffer << "-[" << joinString(removedKeyStrV, "] -[") << "]";

    if (addedKeyStrV.size() > 0) {
      if (removedKeyStrV.size() > 0)
        buffer << ' ';

      buffer << "+[" << joinString(addedKeyStrV, "] +[") << "]";
    }

    changes.push_back(buffer.str());
  }
  if ((addedStrV.size() > 0) || (removedStrV.size() > 0)) {
    std::stringstream buffer;
    buffer << pref << " (changes (-" << removedStrV.size() << "/+" <<
      addedStrV.size() << ")): ";
    if (removedStrV.size() > 0) {
      if (removedStrV.size() == omm->size())
        buffer << "-<all elements> -(";
      else
        buffer << "-(";

      buffer << joinString(removedStrV, ") -(") << ")";
    }

    if (addedStrV.size() > 0) {
      if (removedStrV.size() > 0)
        buffer << ' ';

      buffer << "+(" << joinString(addedStrV, ") +(") << ")";
    }

    changes.push_back(buffer.str());
  }

  return changes;
}
template <template<typename> typename T, typename U, typename... Uoptional>
std::vector<std::string> StateMonitorTest::get_VectorChanges(std::string pref, T<U, Uoptional...>* ov, T<U, Uoptional...>* nv) {
  //despite the name, works on lists and probably other containers too
  std::vector<std::string> changes;

  //lambdas
  std::function<std::string(const U*)> printElement = [](const U* u) {
    return std::string();
  };
  std::function<bool(const U*, const U*)> isEqual = [](const U* u1, const U* u2) {
    return false;
  };
  std::function<std::vector<std::string>(StateMonitorTest*, std::string, const U*, const U*)> getChanges = [](StateMonitorTest* smt, std::string pref, const U* ou, const U* nu) {
    return std::vector<std::string>();
  };

  if constexpr (std::is_same_v<U, card*>) {
    printElement = [](const U* c) {
      std::stringstream buffer;
      buffer << "c(" << *c << ")";
      return buffer.str();
    };
    isEqual = [](const U* c1, const U* c2) {
      return *c1 == *c2;
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, const U* oc, const U* nc) {
      std::vector<std::string> changes;

      if (*oc != *nc)
        changes.push_back(smt->toChgStr(pref, *oc, *nc));

      return changes;
    };
  } else if constexpr (std::is_same_v<U, group*>) {
    printElement = [](const U* g) {
      std::stringstream buffer;
      buffer << "g(" << *g << ")";
      return buffer.str();
    };
    isEqual = [](const U* g1, const U* g2) {
      return *g1 == *g2;
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, const U* og, const U* ng) {
      std::vector<std::string> changes;

      if (*og != *ng)
        changes.push_back(smt->toChgStr(pref, *og, *ng));

      return changes;
    };
  } else if constexpr (std::is_same_v<U, effect*>) {
    printElement = [](const U* e) {
      std::stringstream buffer;
      buffer << "e(" << *e << ")";
      return buffer.str();
    };
    isEqual = [](const U* e1, const U* e2) {
      return *e1 == *e2;
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, const U* oe, const U* ne) {
      std::vector<std::string> changes;

      if (*oe != *ne)
        changes.push_back(smt->toChgStr(pref, *oe, *ne));

      return changes;
    };
  } else if constexpr (std::is_same_v<U, std::pair<interpreter::lua_param, LuaParamType>>) {

    //interpreter::param_list
    printElement = [](const U* u) {
      std::stringstream buffer;
      buffer << "(" << u->first.ptr << ") " << u->second;
      return buffer.str();
    };
    isEqual = [](const U* u1, const U* u2) {
      return ((u1->first.ptr == u2->first.ptr) && (u1->second == u2->second));
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, const U* ou, const U* nu) {
      std::vector<std::string> changes;

      if ((ou->first.ptr != nu->first.ptr) || (ou->second != nu->second)) {
        std::stringstream buffer1;
        buffer1 << "[(" << ou->first.ptr << "), " << ou->second << "]";
        std::stringstream buffer2;
        buffer2 << "[(" << nu->first.ptr << "), " << nu->second << "]";
        changes.push_back(smt->toChgStr(pref, buffer1.str(), buffer2.str(), false));
      }

      return changes;
    };
  } else if constexpr (std::is_same_v<U, processor_unit>) {
    //processor::processor_list
    printElement = [](const U* u) {
      std::vector<std::string> nonZeroAttributes;

      if (u->type) {
        std::stringstream buffer;
        buffer << "type: " << u->type;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (u->step) {
        std::stringstream buffer;
        buffer << "step: " << u->step;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (u->peffect != nullptr) {
        std::stringstream buffer;
        buffer << "peffect: (" << u->peffect << ")";
        nonZeroAttributes.push_back(buffer.str());
      }
      if (u->ptarget != nullptr) {
        std::stringstream buffer;
        buffer << "ptarget: (" << u->ptarget << ")";
        nonZeroAttributes.push_back(buffer.str());
      }
      if ((u->arg1) || (u->arg2) || (u->arg3) || (u->arg4)) {
        std::stringstream buffer;
        buffer << "arg: [" << u->arg1 << ", " << u->arg2 << ", "
               << u->arg3 << ", " << u->arg4 << "]";
        nonZeroAttributes.push_back(buffer.str());
      }
      if (u->ptr1 != nullptr) {
        std::stringstream buffer;
        buffer << "ptr1: (" << u->ptr1 << ")";
        nonZeroAttributes.push_back(buffer.str());
      }
      if (u->ptr2 != nullptr) {
        std::stringstream buffer;
        buffer << "ptr2: (" << u->ptr2 << ")";
        nonZeroAttributes.push_back(buffer.str());
      }

      if (nonZeroAttributes.size() == 0)
        nonZeroAttributes.push_back("<all zero/nullptr>");

      return joinString(nonZeroAttributes, ", ");
    };
    isEqual = [](const U* opu, const U* npu) {
      return ((opu->type == npu->type) &&
              (opu->step == npu->step) &&
              (opu->peffect == npu->peffect) &&
              (opu->ptarget == npu->ptarget) &&
              (opu->arg1 == npu->arg1) &&
              (opu->arg2 == npu->arg2) &&
              (opu->arg3 == npu->arg3) &&
              (opu->arg4 == npu->arg4) &&
              (opu->ptr1 == npu->ptr1) &&
              (opu->ptr2 == npu->ptr2));
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, const U* opu, const U* npu) {
      return smt->getProcessorUnitChanges(pref, opu, npu);
    };

  } else if constexpr (std::is_same_v<U, tevent>) {
    //processor::event_list
    printElement = [](const U* te) {
      std::vector<std::string> nonZeroAttributes;

      if (te->trigger_card != nullptr) {
        std::stringstream buffer;
        buffer << "trigger_card: " << te->trigger_card;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (te->event_cards != nullptr) {
        std::stringstream buffer;
        buffer << "event_cards: " << te->event_cards;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (te->reason_effect != nullptr) {
        std::stringstream buffer;
        buffer << "reason_effect: " << te->reason_effect;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (te->event_code) {
        std::stringstream buffer;
        buffer << "event_code: " << te->event_code;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (te->event_value) {
        std::stringstream buffer;
        buffer << "event_value: " << te->event_value;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (te->reason) {
        std::stringstream buffer;
        buffer << "reason: " << te->reason;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (te->event_player) {
        std::stringstream buffer;
        buffer << "event_player: " << (int) te->event_player;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (te->reason_player) {
        std::stringstream buffer;
        buffer << "reason_player: " << (int) te->reason_player;
        nonZeroAttributes.push_back(buffer.str());
      }

      if (nonZeroAttributes.size() == 0)
        nonZeroAttributes.push_back("<all zero/nullptr>");

      return joinString(nonZeroAttributes, ", ");
    };
    isEqual = [](const U* ote, const U* nte) {
      return ((ote->trigger_card == nte->trigger_card) &&
              (ote->event_cards == nte->event_cards) &&
              (ote->reason_effect == nte->reason_effect) &&
              (ote->event_code == nte->event_code) &&
              (ote->reason == nte->reason) &&
              (ote->event_player == nte->event_player) &&
              (ote->reason_player == nte->reason_player));
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, const U* ote, const U* nte) {
      return smt->getTeventChanges(pref, ote, nte);
    };
  } else if constexpr (std::is_same_v<U, chain>) {
    //chain
    printElement = [](const U* c) {
      std::vector<std::string> nonZeroAttributes;

      if (c->chain_id) {
        std::stringstream buffer;
        buffer << "chain_id: " << c->chain_id;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->chain_count) {
        std::stringstream buffer;
        buffer << "chain_count: " << (int) c->chain_count;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->triggering_player) {
        std::stringstream buffer;
        buffer << "triggering_player: " << (int) c->triggering_player;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->triggering_controler) {
        std::stringstream buffer;
        buffer << "triggering_controler: " << (int) c->triggering_controler;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->triggering_location) {
        std::stringstream buffer;
        buffer << "triggering_location: " << c->triggering_location;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->triggering_sequence) {
        std::stringstream buffer;
        buffer << "triggering_sequence: " << (int) c->triggering_sequence;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->triggering_position) {
        std::stringstream buffer;
        buffer << "triggering_position: " << (int) c->triggering_position;
        nonZeroAttributes.push_back(buffer.str());
      }

      //ignore card_state triggering_state

      if (c->triggering_effect != nullptr) {
        std::stringstream buffer;
        buffer << "triggering_effect: (" << c->triggering_effect << ")";
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->target_cards != nullptr) {
        std::stringstream buffer;
        buffer << "target_cards: (" << c->target_cards << ")";
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->replace_op) {
        std::stringstream buffer;
        buffer << "replace_op: " << c->replace_op;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->target_player) {
        std::stringstream buffer;
        buffer << "target_player: " << (int) c->target_player;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->target_param) {
        std::stringstream buffer;
        buffer << "target_param: " << c->target_param;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->disable_reason != nullptr) {
        std::stringstream buffer;
        buffer << "disable_reason: (" << c->disable_reason << ")";
        nonZeroAttributes.push_back(buffer.str());
      }
      if (c->disable_player) {
        std::stringstream buffer;
        buffer << "disable_player: " << (int) c->disable_player;
        nonZeroAttributes.push_back(buffer.str());
      }

      //ignore tevent evt
      //ignore opmap opinfos

      if (c->flag) {
        std::stringstream buffer;
        buffer << "flag: " << (int) c->flag;
        nonZeroAttributes.push_back(buffer.str());
      }

      return joinString(nonZeroAttributes, ", ");
    };
    isEqual = [](const U* oc, const U* nc) {
      return ((oc->chain_id == nc->chain_id) &&
              (oc->chain_count == nc->chain_count) &&
              (oc->triggering_player == nc->triggering_player) &&
              (oc->triggering_controler == nc->triggering_controler) &&
              (oc->triggering_location == nc->triggering_location) &&
              (oc->triggering_sequence == nc->triggering_sequence) &&
              (oc->triggering_position == nc->triggering_position) &&
              //(oc->triggering_state == nc->triggering_state) &&
              (oc->triggering_effect == nc->triggering_effect) &&
              (oc->target_cards == nc->target_cards) &&
              (oc->replace_op == nc->replace_op) &&
              (oc->target_player == nc->target_player) &&
              (oc->target_param == nc->target_param) &&
              (oc->disable_reason == nc->disable_reason) &&
              (oc->disable_player == nc->disable_player) &&
              //(oc->evt == nc->evt) &&
              //(oc->opinfos == nc->opinfos) &&
              (oc->flag == nc->flag));
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, const U* oc, const U* nc) {
      std::vector<std::string> changes;

      if (oc->chain_id != nc->chain_id)
        changes.push_back(smt->toChgStr(pref, oc->chain_id, nc->chain_id));
      if (oc->chain_count != nc->chain_count)
        changes.push_back(smt->toChgStr(pref, oc->chain_count, nc->chain_count));
      if (oc->triggering_player != nc->triggering_player)
        changes.push_back(smt->toChgStr(pref,
                                        oc->triggering_player,
                                        nc->triggering_player));
      if (oc->triggering_controler != nc->triggering_controler)
        changes.push_back(smt->toChgStr(pref,
                                        oc->triggering_controler,
                                        nc->triggering_controler));
      if (oc->triggering_location != nc->triggering_location)
        changes.push_back(smt->toChgStr(pref,
                                        oc->triggering_location,
                                        nc->triggering_location));
      if (oc->triggering_sequence != nc->triggering_sequence)
        changes.push_back(smt->toChgStr(pref,
                                        oc->triggering_sequence,
                                        nc->triggering_sequence));
      if (oc->triggering_position != nc->triggering_position)
        changes.push_back(smt->toChgStr(pref,
                                        oc->triggering_position,
                                        nc->triggering_position));
      //ignore triggering_state
      if (oc->triggering_effect != nc->triggering_effect)
        changes.push_back(smt->toChgStr(pref,
                                        oc->triggering_effect,
                                        nc->triggering_effect));
      if (oc->target_cards != nc->target_cards)
        changes.push_back(smt->toChgStr(pref,
                                        oc->target_cards,
                                        nc->target_cards));
      if (oc->replace_op != nc->replace_op)
        changes.push_back(smt->toChgStr(pref,
                                        oc->replace_op,
                                        nc->replace_op));
      if (oc->target_player != nc->target_player)
        changes.push_back(smt->toChgStr(pref,
                                        oc->target_player,
                                        nc->target_player));
      if (oc->target_param != nc->target_param)
        changes.push_back(smt->toChgStr(pref,
                                        oc->target_param,
                                        nc->target_param));
      if (oc->disable_reason != nc->disable_reason)
        changes.push_back(smt->toChgStr(pref,
                                        oc->disable_reason,
                                        nc->disable_reason));
      if (oc->disable_player != nc->disable_player)
        changes.push_back(smt->toChgStr(pref,
                                        oc->disable_player,
                                        nc->disable_player));
      //ignore evt
      //ignore opinfos
      if (oc->flag != nc->flag)
        changes.push_back(smt->toChgStr(pref, oc->flag, nc->flag));

      return changes;
    };
  } else {
    //'default' case

    printElement = [](const U* u) {
      std::stringstream buffer;
      buffer << *u;
      return buffer.str();
    };
    isEqual = [](const U* u1, const U* u2) {
      return *u1 == *u2;
    };
    getChanges = [](StateMonitorTest* smt, std::string pref, const U* ou, const U* nu) {
      std::vector<std::string> changes;

      if (*ou != *nu)
        changes.push_back(smt->toChgStr(pref, ou, nu));

      return changes;
    };
  }

  if (ov->size() != nv->size())
    changes.push_back(toChgStr(pref + ".size", ov->size(), nv->size()));

  std::vector<std::string> addedStrV;
  std::vector<std::string> removedStrV;

  //if vector and order of elements are different...print both vectors...?
  //reason being unlike (un/)ordered sets, elements are not unique and should be checked

  for (const auto &oe: *ov) {
    bool found = false;
    for (const auto &ne: *nv) {
      if (isEqual(&oe, &ne)) {
        found = true;
        break;
      }
    }
    if (!found)
      removedStrV.push_back(printElement(&oe));
  }
  for (const auto &ne: *nv) {
    bool found = false;
    for (const auto &oe: *ov) {
      if (isEqual(&oe, &ne)) {
        found = true;
        break;
      }
    }
    if (!found)
      addedStrV.push_back(printElement(&ne));
  }
  if ((addedStrV.size() > 0) || (removedStrV.size() > 0)) {
    //print removed and added
    std::stringstream buffer;
    buffer << pref << " (changes (-" << removedStrV.size() << "/+" <<
      addedStrV.size() << ")): ";
    if (removedStrV.size() > 0) {
      if (removedStrV.size() == ov->size())
        buffer << "-<all elements> -(";
      else
        buffer << "-(";

      buffer << joinString(removedStrV, ") -(") << ")";
    }

    if (addedStrV.size() > 0) {
      if (removedStrV.size() > 0)
        buffer << ' ';

      buffer << "+(" << joinString(addedStrV, ") +(") << ")";
    }
    changes.push_back(buffer.str());

    //print updated vectors
    if ((addedStrV.size() > 0) && (removedStrV.size() > 0)
        && (removedStrV.size() != ov->size()) && (addedStrV.size() != nv->size())
        ) {
      //...only if elements were both added and removed
      //...and completely changed...?
      std::vector<std::string> oStrV;
      std::vector<std::string> nStrV;

      for (const auto &oe: *ov)
        oStrV.push_back(printElement(&oe));
      for (const auto &ne: *nv)
        nStrV.push_back(printElement(&ne));

      changes.push_back(toChgStr(pref + " (container comparison)", "\n\t[("
                                 + joinString(oStrV, "), (") + ")]", "\n\t[("
                                 + joinString(nStrV, "), (") + ")]", false));
    }
  }
  //auto elementChanges = getChanges(this, pref, &oe, &ne);
  //changes.insert(changes.end(), elementChanges.begin(), elementChanges.end());
  return changes;
}
template <template<typename> typename T, typename U, typename... Uoptional>
std::vector<std::string> StateMonitorTest::get_SetChanges(std::string pref, T<U, Uoptional...>* ost, T<U, Uoptional...>* nst) {
  //Uoptional because custom hash functions
  std::vector<std::string> changes;

  //default lambda to output key
  std::function<std::string(const U*)> printKey = [](const U* u) {
    std::stringstream buffer;
    buffer << u;
    return buffer.str();
  };

  if constexpr (std::is_same_v<U, card*>) {
    printKey = [](const U* c) {
      std::stringstream buffer;
      buffer << "c(" << *c << ")";
      return buffer.str();
    };
  } else if constexpr (std::is_same_v<U, group*>) {
    printKey = [](const U* g) {
      std::stringstream buffer;
      buffer << "g(" << *g << ")";
      return buffer.str();
    };
  } else if constexpr (std::is_same_v<U, effect*>) {
    printKey = [](const U* e) {
      std::stringstream buffer;
      buffer << "e(" << *e << ")";
      return buffer.str();
    };
  } else if constexpr (std::is_same_v<U, std::pair<effect*, uint16>>) {
    //card::effect_relation
    printKey = [](const U* u) {
      std::stringstream buffer;
      buffer << u->first << ", " << u->second;
      return buffer.str();
    };
  } else if constexpr (std::is_same_v<U, std::pair<effect*, tevent>>) {
    //processor::delayed_effect_collection
    printKey = [](const U* dae) {
      std::string eStr;
      std::vector<std::string> nonZeroAttributes;

      {
        std::stringstream buffer;
        buffer << dae->first;
        eStr = buffer.str();
      }
      if (dae->second.trigger_card != nullptr) {
        std::stringstream buffer;
        buffer << "trigger_card: " << dae->second.trigger_card;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (dae->second.event_cards != nullptr) {
        std::stringstream buffer;
        buffer << "event_cards: " << dae->second.event_cards;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (dae->second.reason_effect != nullptr) {
        std::stringstream buffer;
        buffer << "reason_effect: " << dae->second.reason_effect;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (dae->second.event_code) {
        std::stringstream buffer;
        buffer << "event_code: " << dae->second.event_code;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (dae->second.event_value) {
        std::stringstream buffer;
        buffer << "event_value: " << dae->second.event_value;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (dae->second.reason) {
        std::stringstream buffer;
        buffer << "reason: " << dae->second.reason;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (dae->second.event_player) {
        std::stringstream buffer;
        buffer << "event_player: " << (int) dae->second.event_player;
        nonZeroAttributes.push_back(buffer.str());
      }
      if (dae->second.reason_player) {
        std::stringstream buffer;
        buffer << "reason_player: " << (int) dae->second.reason_player;
        nonZeroAttributes.push_back(buffer.str());
      }

      if (nonZeroAttributes.size() == 0)
        nonZeroAttributes.push_back("<all zero/nullptr>");

      return "{" + eStr + ", {" + joinString(nonZeroAttributes, ", ") + "}}";
    };
  }

  if (ost->size() != nst->size())
    changes.push_back(toChgStr(pref + ".size", ost->size(), nst->size()));

  std::vector<std::string> addedStrV;
  std::vector<std::string> removedStrV;

  for (const auto &ot: *ost) {
    if (nst->find(ot) == nst->end()) {
      removedStrV.push_back(printKey(&ot));
    }
  }
  for (const auto &nt: *nst) {
    if (ost->find(nt) == ost->end()) {
      addedStrV.push_back(printKey(&nt));
    }
  }
  if ((addedStrV.size() > 0) || (removedStrV.size() > 0)) {
    std::stringstream buffer;
    buffer << pref << " (changes (-" << removedStrV.size() << "/+" <<
      addedStrV.size() << ")): ";
    if (removedStrV.size() > 0) {
      if (removedStrV.size() == ost->size())
        buffer << "-<all elements> -[";
      else
        buffer << "-[";

      buffer << joinString(removedStrV, "] -[") << "]";
    }

    if (addedStrV.size() > 0) {
      if (removedStrV.size() > 0)
        buffer << ' ';

      buffer << "+[" << joinString(addedStrV, "] +[") << "]";
    }

    changes.push_back(buffer.str());
  }

  return changes;
}

std::vector<std::string> StateMonitorTest::get_LuaStateChanges(std::string pref, LuaStateInfo* olsi, LuaStateInfo* nlsi) {
  std::vector<std::string> changes;
  bool isEqual = true;

  if (olsi->luaStackEntries.size() != nlsi->luaStackEntries.size()) {
    changes.push_back(toChgStr(pref + " => lua stack size",
                               olsi->luaStackEntries.size(),
                               nlsi->luaStackEntries.size()));
    isEqual = false;
  }

  int firstDiscrepency = 0;
  int higherMax = (olsi->luaStackEntries.size() > nlsi->luaStackEntries.size())
    ? olsi->luaStackEntries.size()
    : nlsi->luaStackEntries.size();
  int lowerMax = (olsi->luaStackEntries.size() <= nlsi->luaStackEntries.size())
    ? olsi->luaStackEntries.size()
    : nlsi->luaStackEntries.size();

  for (int i = 0; i < higherMax; ++i) {
    firstDiscrepency = i;
    if ((i >= lowerMax) ||
        (olsi->luaStackEntries[i].ptr != nlsi->luaStackEntries[i].ptr) ||
        (olsi->luaStackEntries[i].type != nlsi->luaStackEntries[i].type) ||
        (olsi->luaStackEntries[i].value != nlsi->luaStackEntries[i].value)) {
      isEqual = false;
      break;
    }
  }

  if (!isEqual) {
    std::stringstream buffer;
    buffer << pref << " [changes from idx " << (firstDiscrepency + 1) << "]:";
    for (int i = firstDiscrepency; i < higherMax; ++i) {
      buffer << "\n\t[idx " << (i + 1) << "] ";
      if (i < olsi->luaStackEntries.size()) {
        buffer << "(type: " << olsi->luaStackEntries[i].type;

        if ((olsi->luaStackEntries[i].type == "number") ||
            (olsi->luaStackEntries[i].type == "string") ||
            (olsi->luaStackEntries[i].type == "nil")) {
          buffer << "  value: " << olsi->luaStackEntries[i].value;
        } else {
          buffer << "  ptr: (" << olsi->luaStackEntries[i].ptr << ")";
        }
        buffer << ")";
      } else {
        buffer << "(nil)\t\t\t\t";
      }
      buffer << "\t-> ";
      if (i < nlsi->luaStackEntries.size()) {
        buffer << "(type: " << nlsi->luaStackEntries[i].type;

        if ((nlsi->luaStackEntries[i].type == "number") ||
            (nlsi->luaStackEntries[i].type == "string") ||
            (nlsi->luaStackEntries[i].type == "nil")) {
          buffer << "  value: " << nlsi->luaStackEntries[i].value;
        } else {
          buffer << "  ptr: (" << nlsi->luaStackEntries[i].ptr << ")";
        }
        buffer << ")";
      } else {
        buffer << "(nil)";
      }
    }
    changes.push_back(buffer.str()); //no need to use toChgStr()

  }

  return changes;
}


std::vector<std::string> StateMonitorTest::getCardChanges(std::string pref, card* oc, card* nc) {
  std::vector<std::string> changes;

  if ((!oc) && (!nc)) {
    return changes;
  } else if ((!oc) && (nc)) {
    changes.push_back(toChgStr(pref, "nullptr", nc, true));
    return changes;
  } else if ((oc) && (!nc)) {
    changes.push_back(toChgStr(pref, oc, "nullptr", true));
    return changes;
  }

  //ref_handle
  if (oc->ref_handle != nc->ref_handle)
    changes.push_back(toChgStr(pref + "->ref_handle",
                               oc->ref_handle,
                               nc->ref_handle));

  //current
  auto currentCardStateChanges = getCardStateChanges(pref + "->current",
                                                     &(oc->current),
                                                     &(nc->current));
  changes.insert(changes.end(),
                 currentCardStateChanges.begin(),
                 currentCardStateChanges.end());

  //temp
  auto tempCardStateChanges = getCardStateChanges(pref + "->temp",
                                                  &(oc->temp),
                                                  &(nc->temp));
  changes.insert(changes.end(),
                 tempCardStateChanges.begin(),
                 tempCardStateChanges.end());

  //previous
  auto previousCardStateChanges = getCardStateChanges(pref + "->previous",
                                                      &(oc->previous),
                                                      &(nc->previous));
  changes.insert(changes.end(),
                 previousCardStateChanges.begin(),
                 previousCardStateChanges.end());

  //owner
  if (oc->owner != nc->owner)
    changes.push_back(toChgStr(pref + "->owner", oc->owner, nc->owner));

  //summon_player
  if (oc->summon_player != nc->summon_player)
    changes.push_back(toChgStr(pref + "->summon_player",
                               oc->summon_player,
                               nc->summon_player));

  //summon_info
  if (oc->summon_info != nc->summon_info)
    changes.push_back(toChgStr(pref + "->summon_info",
                               oc->summon_info,
                               nc->summon_info));

  //status
  if (oc->status != nc->status)
    changes.push_back(toChgStr(pref + "->status", oc->status, nc->status));

  //send_to_param (need slightly specialized handling for this)

  //release_param
  if (oc->release_param != nc->release_param)
    changes.push_back(toChgStr(pref + "->release_param",
                               oc->release_param,
                               nc->release_param));

  //sum_param
  if (oc->sum_param != nc->sum_param)
    changes.push_back(toChgStr(pref + "->sum_param",
                               oc->sum_param,
                               nc->sum_param));

  //position_param
  if (oc->position_param != nc->position_param)
    changes.push_back(toChgStr(pref + "->position_param",
                               oc->position_param,
                               nc->position_param));

  //spsummon_param
  if (oc->spsummon_param != nc->spsummon_param)
    changes.push_back(toChgStr(pref + "->spsummon_param",
                               oc->spsummon_param,
                               nc->spsummon_param));

  //to_field_param
  if (oc->to_field_param != nc->to_field_param)
    changes.push_back(toChgStr(pref + "->to_field_param",
                               oc->to_field_param,
                               nc->to_field_param));

  //attack_announce_count
  if (oc->attack_announce_count != nc->attack_announce_count)
    changes.push_back(toChgStr(pref + "->attack_announce_count",
                               oc->attack_announce_count,
                               nc->attack_announce_count));

  //direct_attackable
  if (oc->direct_attackable != nc->direct_attackable)
    changes.push_back(toChgStr(pref + "->direct_attackable",
                               oc->direct_attackable,
                               nc->direct_attackable));

  //announce_count
  if (oc->announce_count != nc->announce_count)
    changes.push_back(toChgStr(pref + "->announce_count",
                               oc->announce_count,
                               nc->announce_count));

  //attacked_count
  if (oc->attacked_count != nc->attacked_count)
    changes.push_back(toChgStr(pref + "->attacked_count",
                               oc->attacked_count,
                               nc->attacked_count));

  //attack_all_target
  if (oc->attack_all_target != nc->attack_all_target)
    changes.push_back(toChgStr(pref + "->attack_all_target",
                               oc->attack_all_target,
                               nc->attack_all_target));

  //attack_controler
  if (oc->attack_controler != nc->attack_controler)
    changes.push_back(toChgStr(pref + "->attack_controler",
                               oc->attack_controler,
                               nc->attack_controler));

  //cardid
  if (oc->cardid != nc->cardid)
    changes.push_back(toChgStr(pref + "->cardid", oc->cardid, nc->cardid));

  //fieldid
  if (oc->fieldid != nc->fieldid)
    changes.push_back(toChgStr(pref + "->fieldid", oc->fieldid, nc->fieldid));

  //fieldid_r
  if (oc->fieldid_r != nc->fieldid_r)
    changes.push_back(toChgStr(pref + "->fieldid_r", oc->fieldid_r, nc->fieldid_r));

  //turnid
  if (oc->turnid != nc->turnid)
    changes.push_back(toChgStr(pref + "->turnid", oc->turnid, nc->turnid));

  //turn_counter
  if (oc->turn_counter != nc->turn_counter)
    changes.push_back(toChgStr(pref + "->turn_counter",
                               oc->turn_counter,
                               nc->turn_counter));

  //unique_pos
  if ((oc->unique_pos[0] != nc->unique_pos[0]) ||
      (oc->unique_pos[1] != nc->unique_pos[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << oc->unique_pos[0] << ", " << oc->unique_pos[1] << "]";
    buffer2 << "[" << nc->unique_pos[0] << ", " << nc->unique_pos[1] << "]";
    changes.push_back(toChgStr(pref + ".unique_pos",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //unique_fieldid
  if (oc->unique_fieldid != nc->unique_fieldid)
    changes.push_back(toChgStr(pref + "->unique_fieldid",
                               oc->unique_fieldid,
                               nc->unique_fieldid));

  //unique_code
  if (oc->unique_code != nc->unique_code)
    changes.push_back(toChgStr(pref + "->unique_code",
                               oc->unique_code,
                               nc->unique_code));

  //unique_location
  if (oc->unique_location != nc->unique_location)
    changes.push_back(toChgStr(pref + "->unique_location",
                               oc->unique_location,
                               nc->unique_location));

  //unique_function
  if (oc->unique_function != nc->unique_function)
    changes.push_back(toChgStr(pref + "->unique_function",
                               oc->unique_function,
                               nc->unique_function));

  //unique_effect
  if (oc->unique_effect != nc->unique_effect)
    changes.push_back(toChgStr(pref + "->unique_effect",
                               oc->unique_effect,
                               nc->unique_effect,
                               true));

  //spsummon_code
  if (oc->spsummon_code != nc->spsummon_code)
    changes.push_back(toChgStr(pref + "->spsummon_code",
                               oc->spsummon_code,
                               nc->spsummon_code));

  //spsummon_counter
  if ((oc->spsummon_counter[0] != nc->spsummon_counter[0]) ||
      (oc->spsummon_counter[1] != nc->spsummon_counter[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << oc->spsummon_counter[0] << ", " <<
      oc->spsummon_counter[1] << "]";
    buffer2 << "[" << nc->spsummon_counter[0] << ", " <<
      nc->spsummon_counter[1] << "]";
    changes.push_back(toChgStr(pref + ".spsummon_counter",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //assume_type
  if (oc->assume_type != nc->assume_type)
    changes.push_back(toChgStr(pref + "->assume_type",
                               oc->assume_type,
                               nc->assume_type));

  //assume_value
  if (oc->assume_value != nc->assume_value)
    changes.push_back(toChgStr(pref + "->assume_value",
                               oc->assume_value,
                               nc->assume_value));

  //equiping_target
  if (oc->equiping_target != nc->equiping_target)
    changes.push_back(toChgStr(pref + "->equiping_target",
                               oc->equiping_target,
                               nc->equiping_target,
                               true));

  //pre_equip_target
  if (oc->pre_equip_target != nc->pre_equip_target)
    changes.push_back(toChgStr(pref + "->pre_equip_target",
                               oc->pre_equip_target,
                               nc->pre_equip_target,
                               true));

  //overlay_target
  if (oc->overlay_target != nc->overlay_target)
    changes.push_back(toChgStr(pref + "->overlay_target",
                               oc->overlay_target,
                               nc->overlay_target, true));

  //relation_map relations
  auto relationsChanges = get_MapChanges(pref + ".relations",
                                         &(oc->relations),
                                         &(nc->relations));
  changes.insert(changes.end(),
                 relationsChanges.begin(),
                 relationsChanges.end());

  //counter_map counters
  auto countersChanges = get_MapChanges(pref + ".counters",
                                        &(oc->counters),
                                        &(nc->counters));
  changes.insert(changes.end(),
                 countersChanges.begin(),
                 countersChanges.end());

  //indestructable_effects;
  auto indestructableEffectsChanges = get_MapChanges(pref + ".indestructable_effects",
                                                     &(oc->indestructable_effects),
                                                     &(nc->indestructable_effects));
  changes.insert(changes.end(),
                 indestructableEffectsChanges.begin(),
                 indestructableEffectsChanges.end());

  //announced_cards;
  auto announcedCardsChanges = get_MapChanges(pref + ".announced_cards",
                                              &(oc->announced_cards),
                                              &(nc->announced_cards));
  changes.insert(changes.end(),
                 announcedCardsChanges.begin(),
                 announcedCardsChanges.end());
  //attacked_cards;
  auto attackedCardsChanges = get_MapChanges(pref + ".attacked_cards",
                                             &(oc->attacked_cards),
                                             &(nc->attacked_cards));
  changes.insert(changes.end(),
                 attackedCardsChanges.begin(),
                 attackedCardsChanges.end());

  //battled_cards;
  auto battledCardsChanges = get_MapChanges(pref + ".battled_cards",
                                            &(oc->battled_cards),
                                            &(nc->battled_cards));
  changes.insert(changes.end(),
                 battledCardsChanges.begin(),
                 battledCardsChanges.end());

  //equiping_cards
  auto equipingCardsChanges = get_SetChanges(pref + ".equiping_cards",
                                             &(oc->equiping_cards),
                                             &(nc->equiping_cards));
  changes.insert(changes.end(),
                 equipingCardsChanges.begin(),
                 equipingCardsChanges.end());

  //material_cards
  auto materialCardsChanges = get_SetChanges(pref + ".material_cards",
                                             &(oc->material_cards),
                                             &(nc->material_cards));
  changes.insert(changes.end(),
                 materialCardsChanges.begin(),
                 materialCardsChanges.end());

  //effect_target_owner
  auto effectTargetOwnerChanges = get_SetChanges(pref + ".effect_target_owner",
                                                 &(oc->effect_target_owner),
                                                 &(nc->effect_target_owner));
  changes.insert(changes.end(),
                 effectTargetOwnerChanges.begin(),
                 effectTargetOwnerChanges.end());

  //effect_target_cards
  auto effectTargetCardsChanges = get_SetChanges(pref + ".effect_target_cards",
                                                 &(oc->effect_target_cards),
                                                 &(nc->effect_target_cards));
  changes.insert(changes.end(),
                 effectTargetCardsChanges.begin(),
                 effectTargetCardsChanges.end());

  //xyz_materials
  auto xyzMaterialsChanges = get_VectorChanges(pref + ".xyz_materials",
                                               &(oc->xyz_materials),
                                               &(nc->xyz_materials));
  changes.insert(changes.end(),
                 xyzMaterialsChanges.begin(),
                 xyzMaterialsChanges.end());

  //single_effect
  auto singleEffectChanges = get_EffectContainerChanges(pref + "->single_effect",
                                                        &(oc->single_effect),
                                                        &(nc->single_effect));
  changes.insert(changes.end(),
                 singleEffectChanges.begin(),
                 singleEffectChanges.end());

  //field_effect
  auto fieldEffectChanges = get_EffectContainerChanges(pref + "->field_effect",
                                                       &(oc->field_effect),
                                                       &(nc->field_effect));
  changes.insert(changes.end(),
                 fieldEffectChanges.begin(),
                 fieldEffectChanges.end());

  //equip_effect
  auto equipEffectChanges = get_EffectContainerChanges(pref + "->equip_effect",
                                                       &(oc->equip_effect),
                                                       &(nc->equip_effect));
  changes.insert(changes.end(),
                 equipEffectChanges.begin(),
                 equipEffectChanges.end());

  //target_effect
  auto targetEffectChanges = get_EffectContainerChanges(pref + "->target_effect",
                                                        &(oc->target_effect),
                                                        &(nc->target_effect));
  changes.insert(changes.end(),
                 targetEffectChanges.begin(),
                 targetEffectChanges.end());

  //xmaterial_effect
  auto xmaterialEffectChanges =
    get_EffectContainerChanges(pref + "->xmaterial_effect",
                               &(oc->xmaterial_effect),
                               &(nc->xmaterial_effect));
  changes.insert(changes.end(),
                 xmaterialEffectChanges.begin(),
                 xmaterialEffectChanges.end());

  //indexer

  //effect_relation relate_effect
  auto relateEffectChanges = get_SetChanges(pref + ".relate_effect",
                                            &(oc->relate_effect),
                                            &(nc->relate_effect));
  changes.insert(changes.end(),
                 relateEffectChanges.begin(),
                 relateEffectChanges.end());


  //effect_set_v immune_effect;

  return changes;
}
std::vector<std::string> StateMonitorTest::getCardStateChanges(std::string pref, card_state* ocs, card_state* ncs) {
  std::vector<std::string> changes;

  if (ocs->type != ncs->type)
    changes.push_back(toChgStr(pref + ".type", ocs->type , ncs->type));

  if (ocs->level != ncs->level)
    changes.push_back(toChgStr(pref + ".level", ocs->level , ncs->level));

  if (ocs->rank != ncs->rank)
    changes.push_back(toChgStr(pref + ".rank", ocs->rank , ncs->rank));

  if (ocs->attribute != ncs->attribute)
    changes.push_back(toChgStr(pref + ".attribute", ocs->attribute , ncs->attribute));

  if (ocs->race != ncs->race)
    changes.push_back(toChgStr(pref + ".race", ocs->race , ncs->race));

  if (ocs->attack != ncs->attack)
    changes.push_back(toChgStr(pref + ".attack", ocs->attack , ncs->attack));

  if (ocs->defense != ncs->defense)
    changes.push_back(toChgStr(pref + ".defense", ocs->defense , ncs->defense));

  if (ocs->controler != ncs->controler)
    changes.push_back(toChgStr(pref + ".controler", ocs->controler , ncs->controler));

  if (ocs->location != ncs->location)
    changes.push_back(toChgStr(pref + ".location", ocs->location , ncs->location));

  if (ocs->sequence != ncs->sequence)
    changes.push_back(toChgStr(pref + ".sequence", ocs->sequence , ncs->sequence));

  if (ocs->position != ncs->position)
    changes.push_back(toChgStr(pref + ".position", ocs->position , ncs->position));

  if (ocs->reason != ncs->reason)
    changes.push_back(toChgStr(pref + ".reason", ocs->reason , ncs->reason));

  if (ocs->reason_card != ncs->reason_card)
    changes.push_back(toChgStr(pref + "->reason_card",
                               ocs->reason_card ,
                               ncs->reason_card, true));

  if (ocs->reason_player != ncs->reason_player)
    changes.push_back(toChgStr(pref + ".reason_player",
                               ocs->reason_player ,
                               ncs->reason_player));

  if (ocs->reason_effect != ncs->reason_effect)
    changes.push_back(toChgStr(pref + "->reason_effect",
                               ocs->reason_effect ,
                               ncs->reason_effect, true));

  return changes;
}

std::vector<std::string> StateMonitorTest::getGroupChanges(std::string pref, group* og, group* ng) {
  std::vector<std::string> changes;

  if ((!og) && (!ng)) {
    return changes;
  } else if ((!og) && (ng)) {
    changes.push_back(toChgStr(pref, "nullptr", ng, true));
    return changes;
  } else if ((og) && (!ng)) {
    changes.push_back(toChgStr(pref, og, "nullptr", true));
    return changes;
  }


  //ref_handle
  if (og->ref_handle != ng->ref_handle)
    changes.push_back(toChgStr(pref + ".ref_handle", og->ref_handle, ng->ref_handle));

  //ignore pduel

  //container
  auto containerChanges = get_VectorChanges(pref + ".container",
                                            &(og->container),
                                            &(ng->container));
  changes.insert(changes.end(), containerChanges.begin(), containerChanges.end());


  //ignore it(erator)
  //see LGroup constructor and group specific get_MapChanges() block

  //is_readonly
  if (og->is_readonly != ng->is_readonly)
    changes.push_back(toChgStr(pref + "->is_readonly", og->is_readonly, ng->is_readonly));

  return changes;
}
std::vector<std::string> StateMonitorTest::getEffectChanges(std::string pref, effect* oe, effect* ne) {
  std::vector<std::string> changes;

  if ((!oe) && (!ne)) {
    return changes;
  } else if ((!oe) && (ne)) {
    changes.push_back(toChgStr(pref, "nullptr", ne, true));
    return changes;
  } else if ((oe) && (!ne)) {
    changes.push_back(toChgStr(pref, oe, "nullptr", true));
    return changes;
  }

  //ref_handle
  if (oe->ref_handle != ne->ref_handle)
    changes.push_back(toChgStr(pref + ".ref_handle", oe->ref_handle, ne->ref_handle));

  //owner
  if (oe->owner != ne->owner)
    changes.push_back(toChgStr(pref + "->owner", oe->owner, ne->owner, true));

  //handler
  if (oe->handler != ne->handler)
    changes.push_back(toChgStr(pref + "->handler", oe->handler, ne->handler, true));

  //effect_owner
  if (oe->effect_owner != ne->effect_owner)
    changes.push_back(toChgStr(pref + ".effect_owner",
                               oe->effect_owner,
                               ne->effect_owner));

  //description
  if (oe->description != ne->description)
    changes.push_back(toChgStr(pref + ".description",
                               oe->description,
                               ne->description));

  //code
  if (oe->code != ne->code)
    changes.push_back(toChgStr(pref + ".code", oe->code, ne->code));

  //flag
  if ((oe->flag[0] != ne->flag[0]) || (oe->flag[1] != ne->flag[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << oe->flag[0] << ", " << oe->flag[1] << "]";
    buffer2 << "[" << ne->flag[0] << ", " << ne->flag[1] << "]";
    changes.push_back(toChgStr(pref + ".flag", buffer1.str(), buffer2.str(), false));
  }

  //id
  if (oe->id != ne->id)
    changes.push_back(toChgStr(pref + ".id", oe->id, ne->id));

  //type
  if (oe->type != ne->type)
    changes.push_back(toChgStr(pref + ".type", oe->type, ne->type));

  //copy_id
  if (oe->copy_id != ne->copy_id)
    changes.push_back(toChgStr(pref + ".copy_id", oe->copy_id, ne->copy_id));

  //range
  if (oe->range != ne->range)
    changes.push_back(toChgStr(pref + ".range", oe->range, ne->range));

  //s_range
  if (oe->s_range != ne->s_range)
    changes.push_back(toChgStr(pref + ".s_range", oe->s_range, ne->s_range));

  //o_range
  if (oe->o_range != ne->o_range)
    changes.push_back(toChgStr(pref + ".o_range", oe->o_range, ne->o_range));

  //count_limit
  if (oe->count_limit != ne->count_limit)
    changes.push_back(toChgStr(pref + ".count_limit",
                               oe->count_limit,
                               ne->count_limit));

  //count_limit_max
  if (oe->count_limit_max != ne->count_limit_max)
    changes.push_back(toChgStr(pref + ".count_limit_max",
                               oe->count_limit_max,
                               ne->count_limit_max));

  //reset_count
  if (oe->reset_count != ne->reset_count)
    changes.push_back(toChgStr(pref + ".reset_count",
                               oe->reset_count,
                               ne->reset_count));

  //reset_flag
  if (oe->reset_flag != ne->reset_flag)
    changes.push_back(toChgStr(pref + ".reset_flag",
                               oe->reset_flag,
                               ne->reset_flag));

  //count_code
  if (oe->count_code != ne->count_code)
    changes.push_back(toChgStr(pref + ".count_code",
                               oe->count_code,
                               ne->count_code));

  //category
  if (oe->category != ne->category)
    changes.push_back(toChgStr(pref + ".category", oe->category, ne->category));

  //hint_timing
  if ((oe->hint_timing[0] != ne->hint_timing[0]) ||
      (oe->hint_timing[1] != ne->hint_timing[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << oe->hint_timing[0] << ", " << oe->hint_timing[1] << "]";
    buffer2 << "[" << ne->hint_timing[0] << ", " << ne->hint_timing[1] << "]";
    changes.push_back(toChgStr(pref + ".hint_timing", buffer1.str(), buffer2.str(), false));
  }

  //card_type
  if (oe->card_type != ne->card_type)
    changes.push_back(toChgStr(pref + ".card_type", oe->card_type, ne->card_type));

  //active_type
  if (oe->active_type != ne->active_type)
    changes.push_back(toChgStr(pref + ".active_type",
                               oe->active_type,
                               ne->active_type));

  //active_location
  if (oe->active_location != ne->active_location)
    changes.push_back(toChgStr(pref + ".active_location",
                               oe->active_location,
                               ne->active_location));

  //active_sequence
  if (oe->active_sequence != ne->active_sequence)
    changes.push_back(toChgStr(pref + ".active_sequence",
                               oe->active_sequence,
                               ne->active_sequence));

  //active_handler
  if (oe->active_handler != ne->active_handler)
    changes.push_back(toChgStr(pref + ".active_handler",
                               oe->active_handler,
                               ne->active_handler, true));

  //status
  if (oe->status != ne->status)
    changes.push_back(toChgStr(pref + ".status", oe->status, ne->status));

  //label
  auto listMZoneChanges = get_VectorChanges(pref + ".label",
                                            &(oe->label),
                                            &(ne->label));
  changes.insert(changes.end(),
                 listMZoneChanges.begin(),
                 listMZoneChanges.end());

  //label_object
  if (oe->label_object != ne->label_object)
    changes.push_back(toChgStr(pref + ".label_object",
                               oe->label_object,
                               ne->label_object));

  //condition
  if (oe->condition != ne->condition)
    changes.push_back(toChgStr(pref + ".condition", oe->condition, ne->condition));

  //cost
  if (oe->cost != ne->cost)
    changes.push_back(toChgStr(pref + ".cost", oe->cost, ne->cost));

  //target
  if (oe->target != ne->target)
    changes.push_back(toChgStr(pref + ".target", oe->target, ne->target));

  //value
  if (oe->value != ne->value)
    changes.push_back(toChgStr(pref + ".value", oe->value, ne->value));

  //operation
  if (oe->operation != ne->operation)
    changes.push_back(toChgStr(pref + ".operation", oe->operation, ne->operation));

  //cost_checked
  if (oe->cost_checked != ne->cost_checked)
    changes.push_back(toChgStr(pref + ".cost_checked",
                               oe->cost_checked,
                               ne->cost_checked));


  return changes;
}
std::vector<std::string> StateMonitorTest::getPlayerInfosChanges(std::string pref, player_info* opi, player_info* npi) {
  std::vector<std::string> changes;

  //lp
  if (opi->lp != npi->lp)
    changes.push_back(toChgStr(pref + ".lp", opi->lp, npi->lp));

  //start_count
  if (opi->start_count != npi->start_count)
    changes.push_back(toChgStr(pref + ".start_count",
                               opi->start_count,
                               npi->start_count));

  //draw_count
  if (opi->draw_count != npi->draw_count)
    changes.push_back(toChgStr(pref + ".draw_count",
                               opi->draw_count,
                               npi->draw_count));

  //used_location
  if (opi->used_location != npi->used_location)
    changes.push_back(toChgStr(pref + ".used_location",
                               opi->used_location,
                               npi->used_location));

  //disabled_location
  if (opi->disabled_location != npi->disabled_location)
    changes.push_back(toChgStr(pref + ".disabled_location",
                               opi->disabled_location,
                               npi->disabled_location));

  //extra_p_count
  if (opi->extra_p_count != npi->extra_p_count)
    changes.push_back(toChgStr(pref + ".extra_p_count",
                               opi->extra_p_count,
                               npi->extra_p_count));

  //tag_extra_p_count
  if (opi->tag_extra_p_count != npi->tag_extra_p_count)
    changes.push_back(toChgStr(pref + ".tag_extra_p_count",
                               opi->tag_extra_p_count,
                               npi->tag_extra_p_count));

  //list_mzone
  auto listMZoneChanges = get_VectorChanges(pref + ".list_mzone",
                                            &(opi->list_mzone),
                                            &(npi->list_mzone));
  changes.insert(changes.end(),
                 listMZoneChanges.begin(),
                 listMZoneChanges.end());

  //list_szone
  auto listSZoneChanges = get_VectorChanges(pref + ".list_szone",
                                            &(opi->list_szone),
                                            &(npi->list_szone));
  changes.insert(changes.end(),
                 listSZoneChanges.begin(),
                 listSZoneChanges.end());

  //list_main
  auto listMainChanges = get_VectorChanges(pref + ".list_main",
                                           &(opi->list_main),
                                           &(npi->list_main));
  changes.insert(changes.end(),
                 listMainChanges.begin(),
                 listMainChanges.end());

  //list_grave
  auto listGraveChanges = get_VectorChanges(pref + ".list_grave",
                                            &(opi->list_grave),
                                            &(npi->list_grave));
  changes.insert(changes.end(),
                 listGraveChanges.begin(),
                 listGraveChanges.end());

  //list_hand
  auto listHandChanges = get_VectorChanges(pref + ".list_hand",
                                           &(opi->list_hand),
                                           &(npi->list_hand));
  changes.insert(changes.end(),
                 listHandChanges.begin(),
                 listHandChanges.end());

  //list_remove
  auto listRemoveChanges = get_VectorChanges(pref + ".list_remove",
                                             &(opi->list_remove),
                                             &(npi->list_remove));
  changes.insert(changes.end(),
                 listRemoveChanges.begin(),
                 listRemoveChanges.end());

  //list_extra
  auto listExtraChanges = get_VectorChanges(pref + ".list_extra",
                                            &(opi->list_extra),
                                            &(npi->list_extra));
  changes.insert(changes.end(),
                 listExtraChanges.begin(),
                 listExtraChanges.end());

  //tag_list_main
  auto tagListMainChanges = get_VectorChanges(pref + ".tag_list_main",
                                              &(opi->tag_list_main),
                                              &(npi->tag_list_main));
  changes.insert(changes.end(),
                 tagListMainChanges.begin(),
                 tagListMainChanges.end());

  //tag_list_hand
  auto tagListHandChanges = get_VectorChanges(pref + ".tag_list_hand",
                                              &(opi->tag_list_hand),
                                              &(npi->tag_list_hand));
  changes.insert(changes.end(),
                 tagListHandChanges.begin(),
                 tagListHandChanges.end());

  //tag_list_extra
  auto tagListExtraChanges = get_VectorChanges(pref + ".tag_list_extra",
                                               &(opi->tag_list_extra),
                                               &(npi->tag_list_extra));
  changes.insert(changes.end(),
                 tagListExtraChanges.begin(),
                 tagListExtraChanges.end());

  return changes;
}
std::vector<std::string> StateMonitorTest::getFieldInfosChanges(std::string pref, field_info* ofi, field_info* nfi) {
  std::vector<std::string> changes;

  //field_id
  if (ofi->field_id != nfi->field_id)
    changes.push_back(toChgStr(pref + ".field_id", ofi->field_id, nfi->field_id));

  //copy_id
  if (ofi->copy_id != nfi->copy_id)
    changes.push_back(toChgStr(pref + ".copy_id", ofi->copy_id, nfi->copy_id));

  //turn_id
  if (ofi->turn_id != nfi->turn_id)
    changes.push_back(toChgStr(pref + ".turn_id", ofi->turn_id, nfi->turn_id));

  //turn_id_by_player[0]
  if (ofi->turn_id_by_player[0] != nfi->turn_id_by_player[0])
    changes.push_back(toChgStr(pref + ".turn_id_by_player[0]",
                               ofi->turn_id_by_player[0],
                               nfi->turn_id_by_player[0]));
  //turn_id_by_player[1]
  if (ofi->turn_id_by_player[1] != nfi->turn_id_by_player[1])
    changes.push_back(toChgStr(pref + ".turn_id_by_player[1]",
                               ofi->turn_id_by_player[1],
                               nfi->turn_id_by_player[1]));

  //card_id
  if (ofi->card_id != nfi->card_id)
    changes.push_back(toChgStr(pref + ".card_id", ofi->card_id, nfi->card_id));

  //phase
  if (ofi->phase != nfi->phase)
    changes.push_back(toChgStr(pref + ".phase", ofi->phase, nfi->phase));

  //turn_player
  if (ofi->turn_player != nfi->turn_player)
    changes.push_back(toChgStr(pref + ".turn_player",
                               ofi->turn_player,
                               nfi->turn_player));

  //priorities[0]
  if (ofi->priorities[0] != nfi->priorities[0])
    changes.push_back(toChgStr(pref + ".priorities[0]",
                               ofi->priorities[0],
                               nfi->priorities[0]));
  //priorities[1]
  if (ofi->priorities[1] != nfi->priorities[1])
    changes.push_back(toChgStr(pref + ".priorities[1]",
                               ofi->priorities[1],
                               nfi->priorities[1]));

  //can_shuffle
  if (ofi->can_shuffle != nfi->can_shuffle)
    changes.push_back(toChgStr(pref + ".can_shuffle",
                               ofi->can_shuffle,
                               nfi->can_shuffle));

  return changes;
}
std::vector<std::string> StateMonitorTest::getFieldEffectsChanges(std::string pref, field_effect* ofe, field_effect* nfe) {
  std::vector<std::string> changes;

  //aura_effect
  auto auraEffectChanges = get_EffectContainerChanges(pref + ".aura_effect",
                                                      &(ofe->aura_effect),
                                                      &(nfe->aura_effect));
  changes.insert(changes.end(),
                 auraEffectChanges.begin(),
                 auraEffectChanges.end());

  //ignition_effect
  auto ignitionEffectChanges =
    get_EffectContainerChanges(pref + ".ignition_effect",
                               &(ofe->ignition_effect),
                               &(nfe->ignition_effect));
  changes.insert(changes.end(),
                 ignitionEffectChanges.begin(),
                 ignitionEffectChanges.end());

  //activate_effect
  auto activateEffectChanges =
    get_EffectContainerChanges(pref + ".activate_effect",
                               &(ofe->activate_effect),
                               &(nfe->activate_effect));
  changes.insert(changes.end(),
                 activateEffectChanges.begin(),
                 activateEffectChanges.end());

  //trigger_o_effect
  auto triggerOEffectChanges =
    get_EffectContainerChanges(pref + ".trigger_o_effect",
                               &(ofe->trigger_o_effect),
                               &(nfe->trigger_o_effect));
  changes.insert(changes.end(),
                 triggerOEffectChanges.begin(),
                 triggerOEffectChanges.end());

  //trigger_f_effect
  auto triggerFEffectChanges =
    get_EffectContainerChanges(pref + ".trigger_f_effect",
                               &(ofe->trigger_f_effect),
                               &(nfe->trigger_f_effect));
  changes.insert(changes.end(),
                 triggerFEffectChanges.begin(),
                 triggerFEffectChanges.end());

  //quick_o_effect
  auto quickOEffectChanges =
    get_EffectContainerChanges(pref + ".quick_o_effect",
                               &(ofe->quick_o_effect),
                               &(nfe->quick_o_effect));
  changes.insert(changes.end(),
                 quickOEffectChanges.begin(),
                 quickOEffectChanges.end());

  //quick_f_effect
  auto quickFEffectChanges =
    get_EffectContainerChanges(pref + ".quick_f_effect",
                               &(ofe->quick_f_effect),
                               &(nfe->quick_f_effect));
  changes.insert(changes.end(),
                 quickFEffectChanges.begin(),
                 quickFEffectChanges.end());

  //continuous_effect
  auto continuousEffectChanges =
    get_EffectContainerChanges(pref + ".continuous_effect",
                               &(ofe->continuous_effect),
                               &(nfe->continuous_effect));
  changes.insert(changes.end(),
                 continuousEffectChanges.begin(),
                 continuousEffectChanges.end());

  //indexer;
  auto indexerChanges = get_MapChanges(pref + ".indexer",
                                       &(ofe->indexer),
                                       &(nfe->indexer));
  changes.insert(changes.end(),
                 indexerChanges.begin(),
                 indexerChanges.end());

  //oath_effects oath;
  auto oathChanges = get_MapChanges(pref + ".oath",
                                    &(ofe->oath),
                                    &(ofe->oath));
  changes.insert(changes.end(),
                 oathChanges.begin(),
                 oathChanges.end());

  //pheff;
  auto pheffChanges = get_SetChanges(pref + ".pheff",
                                     &(ofe->pheff),
                                     &(nfe->pheff));
  changes.insert(changes.end(),
                 pheffChanges.begin(),
                 pheffChanges.end());

  //cheff;
  auto cheffChanges = get_SetChanges(pref + ".cheff",
                                     &(ofe->cheff),
                                     &(nfe->cheff));
  changes.insert(changes.end(),
                 cheffChanges.begin(),
                 cheffChanges.end());

  //rechargeable;
  auto rechargeableChanges = get_SetChanges(pref + ".rechargeable",
                                            &(ofe->rechargeable),
                                            &(nfe->rechargeable));
  changes.insert(changes.end(),
                 rechargeableChanges.begin(),
                 rechargeableChanges.end());

  //spsummon_count_eff;
  auto spsummonCountEffChanges = get_SetChanges(pref + ".spsummon_count_eff",
                                                &(ofe->spsummon_count_eff),
                                                &(nfe->spsummon_count_eff));
  changes.insert(changes.end(),
                 spsummonCountEffChanges.begin(),
                 spsummonCountEffChanges.end());

  //disable_check_list;
  auto disableCheckListChanges = get_VectorChanges(pref + ".disable_check_list",
                                                   &(ofe->disable_check_list),
                                                   &(nfe->disable_check_list));
  changes.insert(changes.end(),
                 disableCheckListChanges.begin(),
                 disableCheckListChanges.end());

  //disable_check_set;
  auto disableCheckSetChanges = get_SetChanges(pref + ".disable_check_set",
                                               &(ofe->disable_check_set),
                                               &(nfe->disable_check_set));
  changes.insert(changes.end(),
                 disableCheckSetChanges.begin(),
                 disableCheckSetChanges.end());

  //grant_effect;
  auto grantEffectChanges = get_MapChanges(pref + ".grant_effect",
                                           &(ofe->grant_effect),
                                           &(nfe->grant_effect));
  changes.insert(changes.end(),
                 grantEffectChanges.begin(),
                 grantEffectChanges.end());

  return changes;
}

std::vector<std::string> StateMonitorTest::getProcessorChanges(std::string pref, processor* op, processor* np) {
  std::vector<std::string> changes;

  //units;
  auto unitsChanges = get_VectorChanges(pref + ".units",
                                        &(op->units),
                                        &(np->units));
  changes.insert(changes.end(),
                 unitsChanges.begin(),
                 unitsChanges.end());

  //subunits;
  auto subunitsChanges = get_VectorChanges(pref + ".subunits",
                                           &(op->subunits),
                                           &(np->subunits));
  changes.insert(changes.end(),
                 subunitsChanges.begin(),
                 subunitsChanges.end());

  //damage_step_reserved;
  auto damage_step_reservedChanges =
    getProcessorUnitChanges(pref + ".damage_step_reserved",
                            &(op->damage_step_reserved),
                            &(np->damage_step_reserved));
  changes.insert(changes.end(),
                 damage_step_reservedChanges.begin(),
                 damage_step_reservedChanges.end());

  //summon_reserved;
  auto summon_reservedChanges = getProcessorUnitChanges(pref + ".summon_reserved",
                                                        &(op->summon_reserved),
                                                        &(np->summon_reserved));
  changes.insert(changes.end(),
                 summon_reservedChanges.begin(),
                 summon_reservedChanges.end());

  //select_cards
  auto selectCardsChanges = get_VectorChanges(pref + ".select_cards",
                                              &(op->select_cards),
                                              &(np->select_cards));
  changes.insert(changes.end(),
                 selectCardsChanges.begin(),
                 selectCardsChanges.end());

  //unselect_cards
  auto unselectCardsChanges = get_VectorChanges(pref + ".unselect_cards",
                                                &(op->unselect_cards),
                                                &(np->unselect_cards));
  changes.insert(changes.end(),
                 unselectCardsChanges.begin(),
                 unselectCardsChanges.end());

  //summonable_cards
  auto summonableCardsChanges = get_VectorChanges(pref + ".summonable_cards",
                                                  &(op->summonable_cards),
                                                  &(np->summonable_cards));
  changes.insert(changes.end(),
                 summonableCardsChanges.begin(),
                 summonableCardsChanges.end());

  //spsummonable_cards
  auto spSummonableCardsChanges = get_VectorChanges(pref + ".spsummonable_cards",
                                                    &(op->spsummonable_cards),
                                                    &(np->spsummonable_cards));
  changes.insert(changes.end(),
                 spSummonableCardsChanges.begin(),
                 spSummonableCardsChanges.end());

  //repositionable_cards
  auto repositionableCardsChanges =
    get_VectorChanges(pref + ".repositionable_cards",
                      &(op->repositionable_cards),
                      &(np->repositionable_cards));
  changes.insert(changes.end(),
                 repositionableCardsChanges.begin(),
                 repositionableCardsChanges.end());

  //msetable_cards
  auto mSetableCardsChanges = get_VectorChanges(pref + ".msetable_cards",
                                                &(op->msetable_cards),
                                                &(np->msetable_cards));
  changes.insert(changes.end(),
                 mSetableCardsChanges.begin(),
                 mSetableCardsChanges.end());

  //ssetable_cards
  auto sSetableCardsChanges = get_VectorChanges(pref + ".ssetable_cards",
                                                &(op->ssetable_cards),
                                                &(np->ssetable_cards));
  changes.insert(changes.end(),
                 sSetableCardsChanges.begin(),
                 sSetableCardsChanges.end());

  //attackable_cards
  auto attackableCardsChanges = get_VectorChanges(pref + ".attackable_cards",
                                                  &(op->attackable_cards),
                                                  &(np->attackable_cards));
  changes.insert(changes.end(),
                 attackableCardsChanges.begin(),
                 attackableCardsChanges.end());

  //select_effects
  auto selectEffectsChanges = get_VectorChanges(pref + ".select_effects",
                                                &(op->select_effects),
                                                &(np->select_effects));
  changes.insert(changes.end(),
                 selectEffectsChanges.begin(),
                 selectEffectsChanges.end());

  //select_options
  auto selectOptionsChanges = get_VectorChanges(pref + ".select_options",
                                                &(op->select_options),
                                                &(np->select_options));
  changes.insert(changes.end(),
                 selectOptionsChanges.begin(),
                 selectOptionsChanges.end());

  //must_select_cards
  auto mustSelectCardsChanges = get_VectorChanges(pref + ".must_select_cards",
                                                  &(op->must_select_cards),
                                                  &(np->must_select_cards));
  changes.insert(changes.end(),
                 mustSelectCardsChanges.begin(),
                 mustSelectCardsChanges.end());

  //point_event
  auto pointEventChanges = get_VectorChanges(pref + ".point_event",
                                             &(op->point_event),
                                             &(np->point_event));
  changes.insert(changes.end(),
                 pointEventChanges.begin(),
                 pointEventChanges.end());

  //instant_event
  auto instantEventChanges = get_VectorChanges(pref + ".instant_event",
                                               &(op->instant_event),
                                               &(np->instant_event));
  changes.insert(changes.end(),
                 instantEventChanges.begin(),
                 instantEventChanges.end());

  //queue_event
  auto queueEventChanges = get_VectorChanges(pref + ".queue_event",
                                             &(op->queue_event),
                                             &(np->queue_event));
  changes.insert(changes.end(),
                 queueEventChanges.begin(),
                 queueEventChanges.end());

  //delayed_activate_event
  auto delayedActivateEventChanges =
    get_VectorChanges(pref + ".delayed_activate_event",
                      &(op->delayed_activate_event),
                      &(np->delayed_activate_event));
  changes.insert(changes.end(),
                 delayedActivateEventChanges.begin(),
                 delayedActivateEventChanges.end());

  //full_event
  auto fullEventChanges = get_VectorChanges(pref + ".full_event",
                                            &(op->full_event),
                                            &(np->full_event));
  changes.insert(changes.end(),
                 fullEventChanges.begin(),
                 fullEventChanges.end());

  //used_event
  auto usedEventChanges = get_VectorChanges(pref + ".used_event",
                                            &(op->used_event),
                                            &(np->used_event));
  changes.insert(changes.end(),
                 usedEventChanges.begin(),
                 usedEventChanges.end());

  //single_event
  auto singleEventChanges = get_VectorChanges(pref + ".single_event",
                                              &(op->single_event),
                                              &(np->single_event));
  changes.insert(changes.end(),
                 singleEventChanges.begin(),
                 singleEventChanges.end());

  //solving_event
  auto solvingEventChanges = get_VectorChanges(pref + ".solving_event",
                                               &(op->solving_event),
                                               &(np->solving_event));
  changes.insert(changes.end(),
                 solvingEventChanges.begin(),
                 solvingEventChanges.end());

  //sub_solving_event
  auto subSolvingEventChanges = get_VectorChanges(pref + ".sub_solving_event",
                                                  &(op->sub_solving_event),
                                                  &(np->sub_solving_event));
  changes.insert(changes.end(),
                 subSolvingEventChanges.begin(),
                 subSolvingEventChanges.end());

  //select_chains
  auto selectChainsChanges = get_VectorChanges(pref + ".select_chains",
                                               &(op->select_chains),
                                               &(np->select_chains));
  changes.insert(changes.end(),
                 selectChainsChanges.begin(),
                 selectChainsChanges.end());

  //current_chain
  auto currentChainChanges = get_VectorChanges(pref + ".current_chain",
                                               &(op->current_chain),
                                               &(np->current_chain));
  changes.insert(changes.end(),
                 currentChainChanges.begin(),
                 currentChainChanges.end());

  //ignition_priority_chains
  auto ignitionPriorityChainsChanges =
    get_VectorChanges(pref + ".ignition_priority_chains",
                      &(op->ignition_priority_chains),
                      &(np->ignition_priority_chains));
  changes.insert(changes.end(),
                 ignitionPriorityChainsChanges.begin(),
                 ignitionPriorityChainsChanges.end());

  //continuous_chain
  auto continuousChainChanges = get_VectorChanges(pref + ".continuous_chain",
                                                  &(op->continuous_chain),
                                                  &(np->continuous_chain));
  changes.insert(changes.end(),
                 continuousChainChanges.begin(),
                 continuousChainChanges.end());

  //solving_continuous
  auto solvingContinuousChanges = get_VectorChanges(pref + ".solving_continuous",
                                                    &(op->solving_continuous),
                                                    &(np->solving_continuous));
  changes.insert(changes.end(),
                 solvingContinuousChanges.begin(),
                 solvingContinuousChanges.end());

  //sub_solving_continuous
  auto subSolvingContinuousChanges =
    get_VectorChanges(pref + ".sub_solving_continuous",
                      &(op->sub_solving_continuous),
                      &(np->sub_solving_continuous));
  changes.insert(changes.end(),
                 subSolvingContinuousChanges.begin(),
                 subSolvingContinuousChanges.end());

  //delayed_continuous_tp
  auto delayedContinuousTpChanges =
    get_VectorChanges(pref + ".delayed_continuous_tp",
                      &(op->delayed_continuous_tp),
                      &(np->delayed_continuous_tp));
  changes.insert(changes.end(),
                 delayedContinuousTpChanges.begin(),
                 delayedContinuousTpChanges.end());

  //delayed_continuous_ntp
  auto delayedContinuousNtpChanges =
    get_VectorChanges(pref + ".delayed_continuous_ntp",
                      &(op->delayed_continuous_ntp),
                      &(np->delayed_continuous_ntp));
  changes.insert(changes.end(),
                 delayedContinuousNtpChanges.begin(),
                 delayedContinuousNtpChanges.end());

  //desrep_chain
  auto desrepChainChanges = get_VectorChanges(pref + ".desrep_chain",
                                              &(op->desrep_chain),
                                              &(np->desrep_chain));
  changes.insert(changes.end(),
                 desrepChainChanges.begin(),
                 desrepChainChanges.end());

  //new_fchain
  auto newFchainChanges = get_VectorChanges(pref + ".new_fchain",
                                            &(op->new_fchain),
                                            &(np->new_fchain));
  changes.insert(changes.end(),
                 newFchainChanges.begin(),
                 newFchainChanges.end());

  //new_fchain_s
  auto newFchainSChanges = get_VectorChanges(pref + ".new_fchain_s",
                                             &(op->new_fchain_s),
                                             &(np->new_fchain_s));
  changes.insert(changes.end(),
                 newFchainSChanges.begin(),
                 newFchainSChanges.end());

  //new_ochain
  auto newOchainChanges = get_VectorChanges(pref + ".new_ochain",
                                            &(op->new_ochain),
                                            &(np->new_ochain));
  changes.insert(changes.end(),
                 newOchainChanges.begin(),
                 newOchainChanges.end());

  //new_ochain_s
  auto newOchainSChanges = get_VectorChanges(pref + ".new_ochain_s",
                                             &(op->new_ochain_s),
                                             &(np->new_ochain_s));
  changes.insert(changes.end(),
                 newOchainSChanges.begin(),
                 newOchainSChanges.end());

  //new_fchain_b
  auto newFchainBChanges = get_VectorChanges(pref + ".new_fchain_b",
                                             &(op->new_fchain_b),
                                             &(np->new_fchain_b));
  changes.insert(changes.end(),
                 newFchainBChanges.begin(),
                 newFchainBChanges.end());

  //new_ochain_b
  auto newOchainBChanges = get_VectorChanges(pref + ".new_ochain_b",
                                             &(op->new_ochain_b),
                                             &(np->new_ochain_b));
  changes.insert(changes.end(),
                 newOchainBChanges.begin(),
                 newOchainBChanges.end());

  //new_ochain_h
  auto newOchainHChanges = get_VectorChanges(pref + ".new_ochain_h",
                                             &(op->new_ochain_h),
                                             &(np->new_ochain_h));
  changes.insert(changes.end(),
                 newOchainHChanges.begin(),
                 newOchainHChanges.end());

  //new_chains
  auto newChainsChanges = get_VectorChanges(pref + ".new_chains",
                                            &(op->new_chains),
                                            &(np->new_chains));
  changes.insert(changes.end(),
                 newChainsChanges.begin(),
                 newChainsChanges.end());

  //delayed_effect_collection delayed_quick_tmp
  auto delayedQuickTmpChanges = get_SetChanges(pref + ".delayed_quick_tmp",
                                               &(op->delayed_quick_tmp),
                                               &(np->delayed_quick_tmp));
  changes.insert(changes.end(),
                 delayedQuickTmpChanges.begin(),
                 delayedQuickTmpChanges.end());

  //delayed_effect_collection delayed_quick
  auto delayedQuickChanges = get_SetChanges(pref + ".delayed_quick",
                                            &(op->delayed_quick),
                                            &(np->delayed_quick));
  changes.insert(changes.end(),
                 delayedQuickChanges.begin(),
                 delayedQuickChanges.end());

  //instant_f_list quick_f_chain
  auto quickFChainChanges = get_MapChanges(pref + ".quick_f_chain",
                                           &(op->quick_f_chain),
                                           &(np->quick_f_chain));
  changes.insert(changes.end(),
                 quickFChainChanges.begin(),
                 quickFChainChanges.end());

  //leave_confirmed
  auto leaveConfirmedChanges = get_SetChanges(pref + ".leave_confirmed",
                                              &(op->leave_confirmed),
                                              &(np->leave_confirmed));
  changes.insert(changes.end(),
                 leaveConfirmedChanges.begin(),
                 leaveConfirmedChanges.end());

  //special_summoning
  auto specialSummoningChanges = get_SetChanges(pref + ".special_summoning",
                                                &(op->special_summoning),
                                                &(np->special_summoning));
  changes.insert(changes.end(),
                 specialSummoningChanges.begin(),
                 specialSummoningChanges.end());

  //unable_tofield_set
  auto unableTofieldSetChanges = get_SetChanges(pref + ".unable_tofield_set",
                                                &(op->unable_tofield_set),
                                                &(np->unable_tofield_set));
  changes.insert(changes.end(),
                 unableTofieldSetChanges.begin(),
                 unableTofieldSetChanges.end());

  //equiping_cards
  auto equipingCardsChanges = get_SetChanges(pref + ".equiping_cards",
                                             &(op->equiping_cards),
                                             &(np->equiping_cards));
  changes.insert(changes.end(),
                 equipingCardsChanges.begin(),
                 equipingCardsChanges.end());

  //control_adjust_set[0]
  auto controlAdjustSetChanges1 = get_SetChanges(pref + ".control_adjust_set[0]",
                                                 &(op->control_adjust_set[0]),
                                                 &(np->control_adjust_set[0]));
  changes.insert(changes.end(),
                 controlAdjustSetChanges1.begin(),
                 controlAdjustSetChanges1.end());
  //control_adjust_set[1]
  auto controlAdjustSetChanges2 = get_SetChanges(pref + ".control_adjust_set[1]",
                                                 &(op->control_adjust_set[1]),
                                                 &(np->control_adjust_set[1]));
  changes.insert(changes.end(),
                 controlAdjustSetChanges2.begin(),
                 controlAdjustSetChanges2.end());

  //unique_destroy_set
  auto uniqueDestroySetChanges = get_SetChanges(pref + ".unique_destroy_set",
                                                &(op->unique_destroy_set),
                                                &(np->unique_destroy_set));
  changes.insert(changes.end(),
                 uniqueDestroySetChanges.begin(),
                 uniqueDestroySetChanges.end());

  //self_destroy_set
  auto selfDestroySetChanges = get_SetChanges(pref + ".self_destroy_set",
                                              &(op->self_destroy_set),
                                              &(np->self_destroy_set));
  changes.insert(changes.end(),
                 selfDestroySetChanges.begin(),
                 selfDestroySetChanges.end());

  //self_tograve_set
  auto selfTograveSetChanges = get_SetChanges(pref + ".self_tograve_set",
                                              &(op->self_tograve_set),
                                              &(np->self_tograve_set));
  changes.insert(changes.end(),
                 selfTograveSetChanges.begin(),
                 selfTograveSetChanges.end());

  //trap_monster_adjust_set[0]
  auto trapMonsterAdjustSetChanges1 =
    get_SetChanges(pref + ".trap_monster_adjust_set[0]",
                   &(op->trap_monster_adjust_set[0]),
                   &(np->trap_monster_adjust_set[0]));
  changes.insert(changes.end(),
                 trapMonsterAdjustSetChanges1.begin(),
                 trapMonsterAdjustSetChanges1.end());
  //trap_monster_adjust_set[1]
  auto trapMonsterAdjustSetChanges2 =
    get_SetChanges(pref + ".trap_monster_adjust_set[1]",
                   &(op->trap_monster_adjust_set[1]),
                   &(np->trap_monster_adjust_set[1]));
  changes.insert(changes.end(),
                 trapMonsterAdjustSetChanges2.begin(),
                 trapMonsterAdjustSetChanges2.end());

  //release_cards
  auto releaseCardsChanges = get_SetChanges(pref + ".release_cards",
                                            &(op->release_cards),
                                            &(np->release_cards));
  changes.insert(changes.end(),
                 releaseCardsChanges.begin(),
                 releaseCardsChanges.end());

  //release_cards_ex
  auto releaseCardsExChanges = get_SetChanges(pref + ".release_cards_ex",
                                              &(op->release_cards_ex),
                                              &(np->release_cards_ex));
  changes.insert(changes.end(),
                 releaseCardsExChanges.begin(),
                 releaseCardsExChanges.end());

  //release_cards_ex_oneof
  auto releaseCardsExOneofChanges = get_SetChanges(pref + ".release_cards_ex_oneof",
                                                   &(op->release_cards_ex_oneof),
                                                   &(np->release_cards_ex_oneof));
  changes.insert(changes.end(),
                 releaseCardsExOneofChanges.begin(),
                 releaseCardsExOneofChanges.end());

  //battle_destroy_rep
  auto battleDestroyRepChanges = get_SetChanges(pref + ".battle_destroy_rep",
                                                &(op->battle_destroy_rep),
                                                &(np->battle_destroy_rep));
  changes.insert(changes.end(),
                 battleDestroyRepChanges.begin(),
                 battleDestroyRepChanges.end());

  //fusion_materials
  auto fusionMaterialsChanges = get_SetChanges(pref + ".fusion_materials",
                                               &(op->fusion_materials),
                                               &(np->fusion_materials));
  changes.insert(changes.end(),
                 fusionMaterialsChanges.begin(),
                 fusionMaterialsChanges.end());

  //synchro_materials
  auto synchroMaterialsChanges = get_SetChanges(pref + ".synchro_materials",
                                                &(op->synchro_materials),
                                                &(np->synchro_materials));
  changes.insert(changes.end(),
                 synchroMaterialsChanges.begin(),
                 synchroMaterialsChanges.end());

  //operated_set
  auto operatedSetChanges = get_SetChanges(pref + ".operated_set",
                                           &(op->operated_set),
                                           &(np->operated_set));
  changes.insert(changes.end(),
                 operatedSetChanges.begin(),
                 operatedSetChanges.end());

  //discarded_set
  auto discardedSetChanges = get_SetChanges(pref + ".discarded_set",
                                            &(op->discarded_set),
                                            &(np->discarded_set));
  changes.insert(changes.end(),
                 discardedSetChanges.begin(),
                 discardedSetChanges.end());

  //destroy_canceled
  auto destroyCanceledChanges = get_SetChanges(pref + ".destroy_canceled",
                                               &(op->destroy_canceled),
                                               &(np->destroy_canceled));
  changes.insert(changes.end(),
                 destroyCanceledChanges.begin(),
                 destroyCanceledChanges.end());

  //delayed_enable_set
  auto delayedEnableSetChanges = get_SetChanges(pref + ".delayed_enable_set",
                                                &(op->delayed_enable_set),
                                                &(np->delayed_enable_set));
  changes.insert(changes.end(),
                 delayedEnableSetChanges.begin(),
                 delayedEnableSetChanges.end());

  //set_group_pre_set
  auto setGroupPreSetChanges = get_SetChanges(pref + ".set_group_pre_set",
                                              &(op->set_group_pre_set),
                                              &(np->set_group_pre_set));
  changes.insert(changes.end(),
                 setGroupPreSetChanges.begin(),
                 setGroupPreSetChanges.end());

  //set_group_set
  auto setGroupSetChanges = get_SetChanges(pref + ".set_group_set",
                                           &(op->set_group_set),
                                           &(np->set_group_set));
  changes.insert(changes.end(),
                 setGroupSetChanges.begin(),
                 setGroupSetChanges.end());

  //effect_set_v disfield_effects;
  //effect_set_v extra_mzone_effects;
  //effect_set_v extra_szone_effects;

  //reseted_effects
  auto resetedEffectsChanges = get_SetChanges(pref + ".reseted_effects",
                                              &(op->reseted_effects),
                                              &(np->reseted_effects));
  changes.insert(changes.end(),
                 resetedEffectsChanges.begin(),
                 resetedEffectsChanges.end());

  //readjust_map
  auto readjustMapChanges = get_MapChanges(pref + ".readjust_map",
                                           &(op->readjust_map),
                                           &(np->readjust_map));
  changes.insert(changes.end(),
                 readjustMapChanges.begin(),
                 readjustMapChanges.end());

  //unique_cards[0]
  auto uniqueCardsChanges1 = get_SetChanges(pref + ".unique_cards[0]",
                                            &(op->unique_cards[0]),
                                            &(np->unique_cards[0]));
  changes.insert(changes.end(),
                 uniqueCardsChanges1.begin(),
                 uniqueCardsChanges1.end());
  //unique_cards[1]
  auto uniqueCardsChanges2 = get_SetChanges(pref + ".unique_cards[1]",
                                            &(op->unique_cards[1]),
                                            &(np->unique_cards[1]));
  changes.insert(changes.end(),
                 uniqueCardsChanges2.begin(),
                 uniqueCardsChanges2.end());

  //effect_count_code
  auto effectCountCodeChanges = get_MapChanges(pref + ".effect_count_code",
                                               &(op->effect_count_code),
                                               &(np->effect_count_code));
  changes.insert(changes.end(),
                 effectCountCodeChanges.begin(),
                 effectCountCodeChanges.end());

  //effect_count_code_duel
  auto effectCountCodeDuelChanges =
    get_MapChanges(pref + ".effect_count_code_duel",
                   &(op->effect_count_code_duel),
                   &(np->effect_count_code_duel));
  changes.insert(changes.end(),
                 effectCountCodeDuelChanges.begin(),
                 effectCountCodeDuelChanges.end());

  //effect_count_code_chain
  auto effectCountCodeChainChanges =
    get_MapChanges(pref + ".effect_count_code_chain",
                   &(op->effect_count_code_chain),
                   &(np->effect_count_code_chain));
  changes.insert(changes.end(),
                 effectCountCodeChainChanges.begin(),
                 effectCountCodeChainChanges.end());

  //spsummon_once_map[0]
  auto spSummonOnceMapChanges1 = get_MapChanges(pref + ".spsummon_once_map[0]",
                                                &(op->spsummon_once_map[0]),
                                                &(np->spsummon_once_map[0]));
  changes.insert(changes.end(),
                 spSummonOnceMapChanges1.begin(),
                 spSummonOnceMapChanges1.end());
  //spsummon_once_map[1]
  auto spSummonOnceMapChanges2 = get_MapChanges(pref + ".spsummon_once_map[1]",
                                                &(op->spsummon_once_map[1]),
                                                &(np->spsummon_once_map[1]));
  changes.insert(changes.end(),
                 spSummonOnceMapChanges2.begin(),
                 spSummonOnceMapChanges2.end());

  //std::multimap<int32, card*, std::greater<int32>> xmaterial_lst;

  //temp_var
  if ((op->temp_var[0] != np->temp_var[0]) ||
      (op->temp_var[1] != np->temp_var[1]) ||
      (op->temp_var[2] != np->temp_var[2]) ||
      (op->temp_var[3] != np->temp_var[3])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << op->temp_var[0] << ", " << op->temp_var[1] << ", "
            << op->temp_var[2] << ", " << op->temp_var[3] << "]";
    buffer2 << "[" << np->temp_var[0] << ", " << np->temp_var[1] << ", "
            << np->temp_var[2] << ", " << np->temp_var[3] << "]";

    changes.push_back(toChgStr(pref + ".temp_var",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  if (op->global_flag != np->global_flag)
    changes.push_back(toChgStr(pref + ".global_flag",
                               op->global_flag,
                               np->global_flag));

  //pre_field
  if ((op->pre_field[0] != np->pre_field[0]) ||
      (op->pre_field[1] != np->pre_field[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << op->pre_field[0] << ", " << op->pre_field[1] << "]";
    buffer2 << "[" << np->pre_field[0] << ", " << np->pre_field[1] << "]";

    changes.push_back(toChgStr(pref + ".pre_field",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //opp_mzone
  auto oppMzoneChanges = get_SetChanges(pref + ".opp_mzone",
                                        &(op->opp_mzone),
                                        &(np->opp_mzone));
  changes.insert(changes.end(),
                 oppMzoneChanges.begin(),
                 oppMzoneChanges.end());

  //chain_limit_list chain_limit;
  // auto chainLimitChanges = get_VectorChanges(pref + ".chain_limit",
  //                                         &(op->chain_limit),
  //                                         &(np->chain_limit));
  // changes.insert(changes.end(),
  //             chainLimitChanges.begin(),
  //             chainLimitChanges.end());

  //chain_limit_list chain_limit_p;
  // auto chainLimitChangesP = get_VectorChanges(pref + ".chain_limit_p",
  //                                          &(op->chain_limit_p),
  //                                          &(np->chain_limit_p));
  // changes.insert(changes.end(),
  //             chainLimitChangesP.begin(),
  //             chainLimitChangesP.end());

  //chain_solving
  if (op->chain_solving != np->chain_solving)
    changes.push_back(toChgStr(pref + ".chain_solving",
                               op->chain_solving,
                               np->chain_solving));

  //conti_solving
  if (op->conti_solving != np->conti_solving)
    changes.push_back(toChgStr(pref + ".conti_solving",
                               op->conti_solving,
                               np->conti_solving));

  //win_player
  if (op->win_player != np->win_player)
    changes.push_back(toChgStr(pref + ".win_player",
                               op->win_player,
                               np->win_player));

  //win_reason
  if (op->win_reason != np->win_reason)
    changes.push_back(toChgStr(pref + ".win_reason",
                               op->win_reason,
                               np->win_reason));

  //re_adjust
  if (op->re_adjust != np->re_adjust)
    changes.push_back(toChgStr(pref + ".re_adjust",
                               op->re_adjust,
                               np->re_adjust));

  //reason_effect
  if (op->reason_effect != np->reason_effect)
    changes.push_back(toChgStr(pref + "->reason_effect",
                               op->reason_effect,
                               np->reason_effect,
                               true));

  //reason_player
  if (op->reason_player != np->reason_player)
    changes.push_back(toChgStr(pref + ".reason_player",
                               op->reason_player,
                               np->reason_player));

  //summoning_card
  if (op->summoning_card != np->summoning_card)
    changes.push_back(toChgStr(pref + "->summoning_card",
                               op->summoning_card,
                               np->summoning_card,
                               true));

  //summon_depth
  if (op->summon_depth != np->summon_depth)
    changes.push_back(toChgStr(pref + ".summon_depth",
                               op->summon_depth,
                               np->summon_depth));

  //summon_cancelable
  if (op->summon_cancelable != np->summon_cancelable)
    changes.push_back(toChgStr(pref + ".summon_cancelable",
                               op->summon_cancelable,
                               np->summon_cancelable));

  //attacker
  if (op->attacker != np->attacker)
    changes.push_back(toChgStr(pref + "->attacker",
                               op->attacker,
                               np->attacker,
                               true));

  //attack_target
  if (op->attack_target != np->attack_target)
    changes.push_back(toChgStr(pref + "->attack_target",
                               op->attack_target,
                               np->attack_target,
                               true));

  //limit_extra_summon_zone
  if (op->limit_extra_summon_zone != np->limit_extra_summon_zone)
    changes.push_back(toChgStr(pref + ".limit_extra_summon_zone",
                               op->limit_extra_summon_zone,
                               np->limit_extra_summon_zone));

  //limit_extra_summon_releasable
  if (op->limit_extra_summon_releasable != np->limit_extra_summon_releasable)
    changes.push_back(toChgStr(pref + ".limit_extra_summon_releasable",
                               op->limit_extra_summon_releasable,
                               np->limit_extra_summon_releasable));

  //limit_tuner
  if (op->limit_tuner != np->limit_tuner)
    changes.push_back(toChgStr(pref + "->limit_tuner",
                               op->limit_tuner,
                               np->limit_tuner,
                               true));

  //limit_syn
  if (op->limit_syn != np->limit_syn)
    changes.push_back(toChgStr(pref + "->limit_syn",
                               op->limit_syn,
                               np->limit_syn,
                               true));

  //limit_syn_minc
  if (op->limit_syn_minc != np->limit_syn_minc)
    changes.push_back(toChgStr(pref + ".limit_syn_minc",
                               op->limit_syn_minc,
                               np->limit_syn_minc));

  //limit_syn_maxc
  if (op->limit_syn_maxc != np->limit_syn_maxc)
    changes.push_back(toChgStr(pref + ".limit_syn_maxc",
                               op->limit_syn_maxc,
                               np->limit_syn_maxc));

  //limit_xyz
  if (op->limit_xyz != np->limit_xyz)
    changes.push_back(toChgStr(pref + "->limit_xyz",
                               op->limit_xyz,
                               np->limit_xyz,
                               true));

  //limit_xyz_minc
  if (op->limit_xyz_minc != np->limit_xyz_minc)
    changes.push_back(toChgStr(pref + ".limit_xyz_minc",
                               op->limit_xyz_minc,
                               np->limit_xyz_minc));

  //limit_xyz_maxc
  if (op->limit_xyz_maxc != np->limit_xyz_maxc)
    changes.push_back(toChgStr(pref + ".limit_xyz_maxc",
                               op->limit_xyz_maxc,
                               np->limit_xyz_maxc));

  //limit_link
  if (op->limit_link != np->limit_link)
    changes.push_back(toChgStr(pref + ".limit_link",
                               op->limit_link,
                               np->limit_link, true));

  //limit_link_card
  if (op->limit_link_card != np->limit_link_card)
    changes.push_back(toChgStr(pref + "->limit_link_card",
                               op->limit_link_card,
                               np->limit_link_card, true));

  //limit_link_minc
  if (op->limit_link_minc != np->limit_link_minc)
    changes.push_back(toChgStr(pref + ".limit_link_minc",
                               op->limit_link_minc,
                               np->limit_link_minc));

  //limit_link_maxc
  if (op->limit_link_maxc != np->limit_link_maxc)
    changes.push_back(toChgStr(pref + ".limit_link_maxc",
                               op->limit_link_maxc,
                               np->limit_link_maxc));

  //not_material
  if (op->not_material != np->not_material)
    changes.push_back(toChgStr(pref + ".not_material",
                               op->not_material,
                               np->not_material));

  //attack_cancelable
  if (op->attack_cancelable != np->attack_cancelable)
    changes.push_back(toChgStr(pref + ".attack_cancelable",
                               op->attack_cancelable,
                               np->attack_cancelable));

  //attack_rollback
  if (op->attack_rollback != np->attack_rollback)
    changes.push_back(toChgStr(pref + ".attack_rollback",
                               op->attack_rollback,
                               np->attack_rollback));

  //effect_damage_step
  if (op->effect_damage_step != np->effect_damage_step)
    changes.push_back(toChgStr(pref + ".effect_damage_step",
                               op->effect_damage_step,
                               np->effect_damage_step));

  //battle_damage
  if ((op->battle_damage[0] != np->battle_damage[0]) ||
      (op->battle_damage[1] != np->battle_damage[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->battle_damage[0] << ", " <<
      (int) op->battle_damage[1] << "]";
    buffer2 << "[" << (int) np->battle_damage[0] << ", " <<
      (int) np->battle_damage[1] << "]";

    changes.push_back(toChgStr(pref + ".battle_damage",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //summon_count
  if ((op->summon_count[0] != np->summon_count[0]) ||
      (op->summon_count[1] != np->summon_count[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->summon_count[0] << ", " <<
      (int) op->summon_count[1] << "]";
    buffer2 << "[" << (int) np->summon_count[0] << ", " <<
      (int) np->summon_count[1] << "]";

    changes.push_back(toChgStr(pref + ".summon_count",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //extra_summon
  if ((op->extra_summon[0] != np->extra_summon[0]) ||
      (op->extra_summon[1] != np->extra_summon[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->extra_summon[0] << ", " <<
      (int) op->extra_summon[1] << "]";
    buffer2 << "[" << (int) np->extra_summon[0] << ", " <<
      (int) np->extra_summon[1] << "]";

    changes.push_back(toChgStr(pref + ".extra_summon",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //spe_effect
  if ((op->spe_effect[0] != np->spe_effect[0]) ||
      (op->spe_effect[1] != np->spe_effect[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->spe_effect[0] << ", " <<
      (int) op->spe_effect[1] << "]";
    buffer2 << "[" << (int) np->spe_effect[0] << ", " <<
      (int) np->spe_effect[1] << "]";

    changes.push_back(toChgStr(pref + ".spe_effect",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //duel_options
  if (op->duel_options != np->duel_options)
    changes.push_back(toChgStr(pref + ".duel_options",
                               op->duel_options,
                               np->duel_options));

  //duel_rule
  if (op->duel_rule != np->duel_rule)
    changes.push_back(toChgStr(pref + ".duel_rule",
                               op->duel_rule,
                               np->duel_rule));

  //copy_reset
  if (op->copy_reset != np->copy_reset)
    changes.push_back(toChgStr(pref + ".copy_reset",
                               op->copy_reset,
                               np->copy_reset));

  //copy_reset_count
  if (op->copy_reset_count != np->copy_reset_count)
    changes.push_back(toChgStr(pref + ".copy_reset_count",
                               op->copy_reset_count,
                               np->copy_reset_count));

  //last_control_changed_id
  if (op->last_control_changed_id != np->last_control_changed_id)
    changes.push_back(toChgStr(pref + ".last_control_changed_id",
                               op->last_control_changed_id,
                               np->last_control_changed_id));

  //set_group_used_zones
  if (op->set_group_used_zones != np->set_group_used_zones)
    changes.push_back(toChgStr(pref + ".set_group_used_zones",
                               op->set_group_used_zones,
                               np->set_group_used_zones));

  //set_group_seq
  if ((op->set_group_seq[0] != np->set_group_seq[0]) ||
      (op->set_group_seq[1] != np->set_group_seq[1]) ||
      (op->set_group_seq[2] != np->set_group_seq[2]) ||
      (op->set_group_seq[3] != np->set_group_seq[3]) ||
      (op->set_group_seq[4] != np->set_group_seq[4]) ||
      (op->set_group_seq[5] != np->set_group_seq[5]) ||
      (op->set_group_seq[6] != np->set_group_seq[6])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->set_group_seq[0] << ", "
            << (int) op->set_group_seq[1] << ","
            << (int) op->set_group_seq[2] << ", " << (int) op->set_group_seq[3]
            << (int) op->set_group_seq[4] << ", " << (int) op->set_group_seq[5]
            << (int) op->set_group_seq[6] << "]";
    buffer2 << "[" << (int) np->set_group_seq[0]
            << ", " << (int) np->set_group_seq[1] << ","
            << (int) np->set_group_seq[2] << ", " << (int) np->set_group_seq[3]
            << (int) np->set_group_seq[4] << ", " << (int) np->set_group_seq[5]
            << (int) np->set_group_seq[6] << "]";

    changes.push_back(toChgStr(pref + ".set_group_seq",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //dice_result
  if ((op->dice_result[0] != np->dice_result[0]) ||
      (op->dice_result[1] != np->dice_result[1]) ||
      (op->dice_result[2] != np->dice_result[2]) ||
      (op->dice_result[3] != np->dice_result[3]) ||
      (op->dice_result[4] != np->dice_result[4])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->dice_result[0] << ", "
            << (int) op->dice_result[1] << ","
            << (int) op->dice_result[2] << ", " << (int) op->dice_result[3]
            << (int) op->dice_result[4] << "]";
    buffer2 << "[" << (int) np->dice_result[0] << ", "
            << (int) np->dice_result[1] << ","
            << (int) np->dice_result[2] << ", " << (int) np->dice_result[3]
            << (int) np->dice_result[4] << "]";

    changes.push_back(toChgStr(pref + ".dice_result",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //coin_result
  if ((op->coin_result[0] != np->coin_result[0]) ||
      (op->coin_result[1] != np->coin_result[1]) ||
      (op->coin_result[2] != np->coin_result[2]) ||
      (op->coin_result[3] != np->coin_result[3]) ||
      (op->coin_result[4] != np->coin_result[4])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->coin_result[0] << ", "
            << (int) op->coin_result[1] << ","
            << (int) op->coin_result[2] << ", " << (int) op->coin_result[3]
            << (int) op->coin_result[4] << "]";
    buffer2 << "[" << (int) np->coin_result[0] << ", "
            << (int) np->coin_result[1] << ","
            << (int) np->coin_result[2] << ", " << (int) np->coin_result[3]
            << (int) np->coin_result[4] << "]";

    changes.push_back(toChgStr(pref + ".coin_result",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //to_bp
  if (op->to_bp != np->to_bp)
    changes.push_back(toChgStr(pref + ".to_bp", op->to_bp, np->to_bp));

  //to_m2
  if (op->to_m2 != np->to_m2)
    changes.push_back(toChgStr(pref + ".to_m2", op->to_m2, np->to_m2));

  //to_ep
  if (op->to_ep != np->to_ep)
    changes.push_back(toChgStr(pref + ".to_ep", op->to_ep, np->to_ep));

  //skip_m2
  if (op->skip_m2 != np->skip_m2)
    changes.push_back(toChgStr(pref + ".skip_m2", op->skip_m2, np->skip_m2));

  //chain_attack
  if (op->chain_attack != np->chain_attack)
    changes.push_back(toChgStr(pref + ".chain_attack",
                               op->chain_attack,
                               np->chain_attack));

  //chain_attacker_id
  if (op->chain_attacker_id != np->chain_attacker_id)
    changes.push_back(toChgStr(pref + ".chain_attacker_id",
                               op->chain_attacker_id,
                               np->chain_attacker_id));

  //chain_attack_target
  if (op->chain_attack_target != np->chain_attack_target)
    changes.push_back(toChgStr(pref + "->chain_attack_target",
                               op->chain_attack_target,
                               np->chain_attack_target, true));

  //attack_player
  if (op->attack_player != np->attack_player)
    changes.push_back(toChgStr(pref + ".attack_player",
                               op->attack_player,
                               np->attack_player));

  //selfdes_disabled
  if (op->selfdes_disabled != np->selfdes_disabled)
    changes.push_back(toChgStr(pref + ".selfdes_disabled",
                               op->selfdes_disabled,
                               np->selfdes_disabled));

  //overdraw
  if ((op->overdraw[0] != np->overdraw[0]) ||
      (op->overdraw[1] != np->overdraw[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->overdraw[0] << ", "
            << (int) op->overdraw[1] << "]";
    buffer2 << "[" << (int) np->overdraw[0] << ", "
            << (int) np->overdraw[1] << "]";

    changes.push_back(toChgStr(pref + ".overdraw",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //check_level
  if (op->check_level != np->check_level)
    changes.push_back(toChgStr(pref + ".check_level",
                               op->check_level,
                               np->check_level));

  //shuffle_check_disabled
  if (op->shuffle_check_disabled != np->shuffle_check_disabled)
    changes.push_back(toChgStr(pref + ".shuffle_check_disabled",
                               op->shuffle_check_disabled,
                               np->shuffle_check_disabled));

  //shuffle_hand_check
  if ((op->shuffle_hand_check[0] != np->shuffle_hand_check[0]) ||
      (op->shuffle_hand_check[1] != np->shuffle_hand_check[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->shuffle_hand_check[0] << ", "
            << (int) op->shuffle_hand_check[1] << "]";
    buffer2 << "[" << (int) np->shuffle_hand_check[0] << ", "
            << (int) np->shuffle_hand_check[1] << "]";

    changes.push_back(toChgStr(pref + ".shuffle_hand_check",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //shuffle_deck_check
  if ((op->shuffle_deck_check[0] != np->shuffle_deck_check[0]) ||
      (op->shuffle_deck_check[1] != np->shuffle_deck_check[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->shuffle_deck_check[0] << ", " <<
      (int) op->shuffle_deck_check[1] << "]";
    buffer2 << "[" << (int) np->shuffle_deck_check[0] << ", " <<
      (int) np->shuffle_deck_check[1] << "]";

    changes.push_back(toChgStr(pref + ".shuffle_deck_check",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //deck_reversed
  if (op->deck_reversed != np->deck_reversed)
    changes.push_back(toChgStr(pref + ".deck_reversed",
                               op->deck_reversed,
                               np->deck_reversed));

  //remove_brainwashing
  if (op->remove_brainwashing != np->remove_brainwashing)
    changes.push_back(toChgStr(pref + ".remove_brainwashing",
                               op->remove_brainwashing,
                               np->remove_brainwashing));

  //flip_delayed
  if (op->flip_delayed != np->flip_delayed)
    changes.push_back(toChgStr(pref + ".flip_delayed",
                               op->flip_delayed,
                               np->flip_delayed));

  //damage_calculated
  if (op->damage_calculated != np->damage_calculated)
    changes.push_back(toChgStr(pref + ".damage_calculated",
                               op->damage_calculated,
                               np->damage_calculated));

  //hand_adjusted
  if (op->hand_adjusted != np->hand_adjusted)
    changes.push_back(toChgStr(pref + ".hand_adjusted",
                               op->hand_adjusted,
                               np->hand_adjusted));

  //summon_state_count
  if ((op->summon_state_count[0] != np->summon_state_count[0]) ||
      (op->summon_state_count[1] != np->summon_state_count[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->summon_state_count[0] << ", "
            << (int) op->summon_state_count[1] << "]";
    buffer2 << "[" << (int) np->summon_state_count[0] << ", "
            << (int) np->summon_state_count[1] << "]";

    changes.push_back(toChgStr(pref + ".summon_state_count",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //normalsummon_state_count
  if ((op->normalsummon_state_count[0] != np->normalsummon_state_count[0]) ||
      (op->normalsummon_state_count[1] != np->normalsummon_state_count[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->normalsummon_state_count[0] << ", " <<
      (int) op->normalsummon_state_count[1] << "]";
    buffer2 << "[" << (int) np->normalsummon_state_count[0] << ", " <<
      (int) np->normalsummon_state_count[1] << "]";

    changes.push_back(toChgStr(pref + ".normalsummon_state_count",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //flipsummon_state_count
  if ((op->flipsummon_state_count[0] != np->flipsummon_state_count[0]) ||
      (op->flipsummon_state_count[1] != np->flipsummon_state_count[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->flipsummon_state_count[0] << ", " <<
      (int) op->flipsummon_state_count[1] << "]";
    buffer2 << "[" << (int) np->flipsummon_state_count[0] << ", " <<
      (int) np->flipsummon_state_count[1] << "]";

    changes.push_back(toChgStr(pref + ".flipsummon_state_count",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //spsummon_state_count
  if ((op->spsummon_state_count[0] != np->spsummon_state_count[0]) ||
      (op->spsummon_state_count[1] != np->spsummon_state_count[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->spsummon_state_count[0] << ", " <<
      (int) op->spsummon_state_count[1] << "]";
    buffer2 << "[" << (int) np->spsummon_state_count[0] << ", " <<
      (int) np->spsummon_state_count[1] << "]";

    changes.push_back(toChgStr(pref + ".spsummon_state_count",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //attack_state_count
  if ((op->attack_state_count[0] != np->attack_state_count[0]) ||
      (op->attack_state_count[1] != np->attack_state_count[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->attack_state_count[0] << ", " <<
      (int) op->attack_state_count[1] << "]";
    buffer2 << "[" << (int) np->attack_state_count[0] << ", " <<
      (int) np->attack_state_count[1] << "]";

    changes.push_back(toChgStr(pref + ".attack_state_count",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //battle_phase_count
  if ((op->battle_phase_count[0] != np->battle_phase_count[0]) ||
      (op->battle_phase_count[1] != np->battle_phase_count[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->battle_phase_count[0] << ", " <<
      (int) op->battle_phase_count[1] << "]";
    buffer2 << "[" << (int) np->battle_phase_count[0] << ", " <<
      (int) np->battle_phase_count[1] << "]";

    changes.push_back(toChgStr(pref + ".battle_phase_count",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //battled_count
  if ((op->battled_count[0] != np->battled_count[0]) ||
      (op->battled_count[1] != np->battled_count[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->battled_count[0] << ", " <<
      (int) op->battled_count[1] << "]";
    buffer2 << "[" << (int) np->battled_count[0] << ", " <<
      (int) np->battled_count[1] << "]";

    changes.push_back(toChgStr(pref + ".battled_count",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //phase_action
  if (op->phase_action != np->phase_action)
    changes.push_back(toChgStr(pref + ".phase_action", op->phase_action, np->phase_action));

  //hint_timing
  if ((op->hint_timing[0] != np->hint_timing[0]) ||
      (op->hint_timing[1] != np->hint_timing[1])) {
    std::stringstream buffer1;
    std::stringstream buffer2;
    buffer1 << "[" << (int) op->hint_timing[0] << ", " <<
      (int) op->hint_timing[1] << "]";
    buffer2 << "[" << (int) np->hint_timing[0] << ", " <<
      (int) np->hint_timing[1] << "]";

    changes.push_back(toChgStr(pref + ".hint_timing",
                               buffer1.str(),
                               buffer2.str(), false));
  }

  //current_player
  if (op->current_player != np->current_player)
    changes.push_back(toChgStr(pref + ".current_player",
                               op->current_player,
                               np->current_player));

  //conti_player
  if (op->conti_player != np->conti_player)
    changes.push_back(toChgStr(pref + ".conti_player",
                               op->conti_player,
                               np->conti_player));

  //summon_counter;
  auto summonChanges = get_MapChanges(pref + ".summon_counter",
                                      &(op->summon_counter),
                                      &(np->summon_counter));
  changes.insert(changes.end(),
                 summonChanges.begin(),
                 summonChanges.end());

  //normalsummon_counter;
  auto normalsummonChanges = get_MapChanges(pref + ".normalsummon_counter",
                                            &(op->normalsummon_counter),
                                            &(np->normalsummon_counter));
  changes.insert(changes.end(),
                 normalsummonChanges.begin(),
                 normalsummonChanges.end());

  //spsummon_counter;
  auto spsummonChanges = get_MapChanges(pref + ".spsummon_counter",
                                        &(op->spsummon_counter),
                                        &(np->spsummon_counter));
  changes.insert(changes.end(),
                 spsummonChanges.begin(),
                 spsummonChanges.end());

  //flipsummon_counter;
  auto flipsummonChanges = get_MapChanges(pref + ".flipsummon_counter",
                                          &(op->flipsummon_counter),
                                          &(np->flipsummon_counter));
  changes.insert(changes.end(),
                 flipsummonChanges.begin(),
                 flipsummonChanges.end());

  //attack_counter;
  auto attackChanges = get_MapChanges(pref + ".attack_counter",
                                      &(op->attack_counter),
                                      &(np->attack_counter));
  changes.insert(changes.end(),
                 attackChanges.begin(),
                 attackChanges.end());

  //chain_counter;
  auto chainChanges = get_MapChanges(pref + ".chain_counter",
                                     &(op->chain_counter),
                                     &(np->chain_counter));
  changes.insert(changes.end(),
                 chainChanges.begin(),
                 chainChanges.end());

  //recover_damage_reserve;
  auto recoverDamageReserveChanges =
    get_VectorChanges(pref + ".recover_damage_reserve",
                      &(op->recover_damage_reserve),
                      &(np->recover_damage_reserve));
  changes.insert(changes.end(),
                 recoverDamageReserveChanges.begin(),
                 recoverDamageReserveChanges.end());

  //dec_count_reserve;
  auto decCountReserveChanges = get_VectorChanges(pref + ".dec_count_reserve",
                                                  &(op->dec_count_reserve),
                                                  &(np->dec_count_reserve));
  changes.insert(changes.end(),
                 decCountReserveChanges.begin(),
                 decCountReserveChanges.end());

  return changes;
}

std::vector<std::string> StateMonitorTest::getProcessorUnitChanges(std::string pref, const processor_unit* opu, const processor_unit* npu) {
  std::vector<std::string> changes;

  if (opu->type != npu->type)
    changes.push_back(toChgStr(pref + ".type", opu->type, npu->type));

  if (opu->step != npu->step)
    changes.push_back(toChgStr(pref + ".step", opu->step, npu->step));

  if (opu->peffect != npu->peffect)
    changes.push_back(toChgStr(pref + ".peffect", opu->peffect, npu->peffect));

  if (opu->ptarget != npu->ptarget)
    changes.push_back(toChgStr(pref + ".ptarget", opu->ptarget, npu->ptarget));

  if (opu->arg1 != npu->arg1)
    changes.push_back(toChgStr(pref + ".arg1", opu->arg1, npu->arg1));

  if (opu->arg2 != npu->arg2)
    changes.push_back(toChgStr(pref + ".arg2", opu->arg2, npu->arg2));

  if (opu->arg3 != npu->arg3)
    changes.push_back(toChgStr(pref + ".arg3", opu->arg3, npu->arg3));

  if (opu->arg4 != npu->arg4)
    changes.push_back(toChgStr(pref + ".arg4", opu->arg4, npu->arg4));

  if (opu->ptr1 != npu->ptr1)
    changes.push_back(toChgStr(pref + ".ptr1", opu->ptr1, npu->ptr1));

  if (opu->ptr2 != npu->ptr2)
    changes.push_back(toChgStr(pref + ".ptr2", opu->ptr2, npu->ptr2));

  return changes;
}

std::vector<std::string> StateMonitorTest::getTeventChanges(std::string pref, const tevent* ote, const tevent* nte) {
  std::vector<std::string> changes;

  if (ote->trigger_card != nte->trigger_card)
    changes.push_back(toChgStr(pref + ".trigger_card",
                               ote->trigger_card,
                               nte->trigger_card));

  if (ote->event_cards != nte->event_cards)
    changes.push_back(toChgStr(pref + ".event_cards",
                               ote->event_cards,
                               nte->event_cards));

  if (ote->reason_effect != nte->reason_effect)
    changes.push_back(toChgStr(pref + ".reason_effect",
                               ote->reason_effect,
                               nte->reason_effect));

  if (ote->event_code != nte->event_code)
    changes.push_back(toChgStr(pref + ".event_code",
                               ote->event_code,
                               nte->event_code));

  if (ote->event_value != nte->event_value)
    changes.push_back(toChgStr(pref + ".event_value",
                               ote->event_value,
                               nte->event_value));

  if (ote->reason != nte->reason)
    changes.push_back(toChgStr(pref + ".reason",
                               ote->reason,
                               nte->reason));

  if (ote->event_player != nte->event_player)
    changes.push_back(toChgStr(pref + ".event_player",
                               ote->event_player,
                               nte->event_player));

  if (ote->reason_player != nte->reason_player)
    changes.push_back(toChgStr(pref + ".reason_player",
                               ote->reason_player,
                               nte->reason_player));

  return changes;
}

std::vector<std::string> StateMonitorTest::getChainChanges(std::string pref, const chain* oc, const chain* nc) {
  std::vector<std::string> changes;

  if (oc->chain_id != nc->chain_id)
    changes.push_back(toChgStr(pref + ".chain_id",
                               oc->chain_id,
                               nc->chain_id));

  if (oc->chain_count != nc->chain_count)
    changes.push_back(toChgStr(pref + ".chain_count",
                               oc->chain_count,
                               nc->chain_count));

  if (oc->triggering_player != nc->triggering_player)
    changes.push_back(toChgStr(pref + ".triggering_player",
                               oc->triggering_player,
                               nc->triggering_player));

  if (oc->triggering_controler != nc->triggering_controler)
    changes.push_back(toChgStr(pref + ".triggering_controler",
                               oc->triggering_controler,
                               nc->triggering_controler));

  if (oc->triggering_location != nc->triggering_location)
    changes.push_back(toChgStr(pref + ".triggering_location",
                               oc->triggering_location,
                               nc->triggering_location));

  if (oc->triggering_sequence != nc->triggering_sequence)
    changes.push_back(toChgStr(pref + ".triggering_sequence",
                               oc->triggering_sequence,
                               nc->triggering_sequence));

  if (oc->triggering_position != nc->triggering_position)
    changes.push_back(toChgStr(pref + ".triggering_position",
                               oc->triggering_position,
                               nc->triggering_position));

  //triggering_state

  if (oc->triggering_effect != nc->triggering_effect)
    changes.push_back(toChgStr(pref + ".triggering_effect",
                               oc->triggering_effect,
                               nc->triggering_effect));

  if (oc->target_cards != nc->target_cards)
    changes.push_back(toChgStr(pref + ".target_cards",
                               oc->target_cards,
                               nc->target_cards));

  if (oc->replace_op != nc->replace_op)
    changes.push_back(toChgStr(pref + ".replace_op",
                               oc->replace_op,
                               nc->replace_op));

  if (oc->target_player != nc->target_player)
    changes.push_back(toChgStr(pref + ".target_player",
                               oc->target_player,
                               nc->target_player));

  if (oc->target_param != nc->target_param)
    changes.push_back(toChgStr(pref + ".target_param",
                               oc->target_param,
                               nc->target_param));

  if (oc->disable_reason != nc->disable_reason)
    changes.push_back(toChgStr(pref + ".disable_reason",
                               oc->disable_reason,
                               nc->disable_reason));

  if (oc->disable_player != nc->disable_player)
    changes.push_back(toChgStr(pref + ".disable_player",
                               oc->disable_player,
                               nc->disable_player));

  //evt

  //opinfos

  if (oc->flag != nc->flag)
    changes.push_back(toChgStr(pref + ".flag", oc->flag, nc->flag));

  return changes;
}
