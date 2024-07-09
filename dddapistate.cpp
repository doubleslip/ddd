/*
 * dddapistate.cpp file
 */
#include "dddapistate.hpp"


//Creates a new duel state with a random seed (unless otherwise
// specified by conf file in which case, will use seed in conf file)
// and adds any needed references to and from its shadow state
// (bit of a helper function used by both create_duel_state() and
//  create_duel_state_from_seed())
unsigned long long ddd::createDuelState(bool showNonErrors, bool useSpecifiedSeed, unsigned int seed) {
  unsigned long long dsId = 0;

  //attempt to create and start duel
  intptr_t pDuel
    = setUpAndStartDuel(showNonErrors, useSpecifiedSeed, seed);

  //check if successful
  if (pDuel) {
    //successfully created and started duel

    //attempt to create a new shadow state based on created duel
    intptr_t pShadowDuel = ddd::duplicateDuel(pDuel);

    if (!pShadowDuel) {
      clog("e", "Unable to create shadow duel state for created duel"
           " state.");
      end_duel(pDuel); //remove the created duel
      return 0;
    }

    //set up duel state (and shadow duel state) variables for singleton
    DuelState ds;
    ShadowDuelState sds;

    dsId = ddd::generateId();
    unsigned long long sdsId = ddd::generateId();

    ds.pDuel = pDuel;
    ds.shadowDsId = sdsId;
    ds.active = true;

    sds.pDuel = pShadowDuel;
    sds.responsesMap[dsId]; //create entry with empty response

    //insert to maps in singleton
    // std::scoped_lock<std::mutex> dsLock(getDDD_GS()
    //                                  .dddapi.duelStatesMutex);
    getDDD_GS().dddapi
      .duelStates.insert(std::make_pair(dsId, std::move(ds)));
    // std::scoped_lock<std::mutex> sdsLock(getDDD_GS().dddapi
    //                                   .shadowDuelStatesMutex);
    getDDD_GS().dddapi
      .shadowDuelStates.insert(std::make_pair(sdsId, std::move(sds)));

    //if (showNonErrors) {
      clog("d", "created state: ", dsId, " (", (duel*) pDuel, ")");
      clog("d", "shadow state: ", sdsId, " (", (duel*) pShadowDuel, ")");
	//}
  }

  return dsId;
}

//Creates a copy of a duel so that any responses set or process
// iterations performed on one duel do not affect the other
//Unlike duplicate_duel_state(), this function handles a pDuel
// as opposed to a duel state, which means this function does
// not update any singleton variables and is more for internal use
intptr_t ddd::duplicateDuel(intptr_t pDuelSource) {
  return ddd::duplicateDuel(pDuelSource, 0);
}
intptr_t ddd::duplicateDuel(intptr_t pDuelSource, intptr_t pDuelReuse) {
  /*
   * Preliminary checks
   */
  //check source duel valid
  if (!pDuelSource) {
    clog("e", "Duel currently not set up yet or has invalid ptr.");
    return 0;
  }

  intptr_t pDuelNew = 0;
  duel* pDSource = (duel*) pDuelSource;

  /*
   * Create the new duel/state
   */
  if ((pDuelReuse != 0) && (ddd::isDuelDuplicatable(pDuelReuse))) {
    //reusable pDuel specified and is duplicatable
    // (clear the duel and use it)
    duel* pDReuse = (duel*) pDuelReuse;
    pDReuse->clear_buffer();
    pDReuse->clear();

    std::scoped_lock<std::mutex> gen(getDDD_GS().dddapi.genMutex);
    //might not be necessary?

    lua_pop(pDReuse->lua->lua_state, lua_gettop(pDReuse->lua->lua_state));

    if (pDReuse->lua->current_state != pDReuse->lua->lua_state) {
      lua_pop(pDReuse->lua->current_state,
              lua_gettop(pDReuse->lua->current_state));
      pDReuse->lua->current_state = pDReuse->lua->lua_state;
    }

    pDuelNew = pDuelReuse;

  } else {
    //reusable pDuel not specified or is not duplicatable
    // (construct a new pDuel)

    std::scoped_lock<std::mutex> gen(getDDD_GS().dddapi.genMutex);
    //more specifically it's the duel constructor that cannot be
    // run in parallel and probably inserting the duel_set as well

    pDuelNew = create_duel(0);
  }

  //check valid ptr created from create_duel
  if (!pDuelNew) {
    clog("e", "Unable to create duel (invalid ptr (", pDuelNew,
         ") returned for create_duel())");
    end_duel(pDuelNew);
    return 0;
  }

  duel* pDNew = (duel*) pDuelNew;


  /*
   * Set up duel vars (card, group, effect) in new duel/state by
   *  copying their contents (from source duel/state) and mapping them
   */
  //attempt to map cards (and some effects)
  std::unordered_map<card*, card*> cardMappings;
  std::unordered_map<card*, card*> cardReverseMappings;
  std::unordered_map<effect*, effect*> effectMappings;
  std::unordered_map<effect*, effect*> effectReverseMappings;
  std::unordered_map<int32, int32> rhMappings;

  //cards with effects in effect container that cannot be mapped yet
  // (until after all effects are mapped/created)
  std::unordered_set<card*> deferredMappingsCards;

  if (!ddd::copyAndMapCards(pDuelSource, pDuelNew, cardMappings,
                            cardReverseMappings, effectMappings,
                            effectReverseMappings, rhMappings,
                            deferredMappingsCards)) {
    clog("e", "Unable to copy and map cards or an effect in card's"
         " effect_container.");
    end_duel(pDuelNew);
    return 0;
  }

  //attempt to map groups
  std::unordered_map<group*, group*> groupMappings;
  std::unordered_map<group*, group*> groupReverseMappings;
  if (!ddd::copyAndMapGroups(pDuelSource, pDuelNew, groupMappings,
                             groupReverseMappings, rhMappings)) {
    clog("e", "Unable to copy and map groups.");
    end_duel(pDuelNew);
    return 0;
  }

  //attempt to map effects
  // (at least the ones not already mapped from card's effect container)
  {
    std::scoped_lock<std::mutex> gen(getDDD_GS().dddapi.genMutex);

    if (!ddd::copyAndMapEffects(pDuelSource, pDuelNew, cardMappings,
                                effectMappings, effectReverseMappings,
                                rhMappings, deferredMappingsCards)) {
      clog("e", "Unable to copy and map effects.");
      end_duel(pDuelNew);
      return 0;
    }
  }

  /*
   * Map other duel vars (assumes, sgroups, uncopy) to duel vars that
   *  should exist in new duel/state
   */
  if (!ddd::mapAssumes(pDuelSource, pDuelNew, cardMappings)) {
    clog("e", "Unable to map assumes.");
    end_duel(pDuelNew);
    return 0;
  }
  if (!ddd::mapSgroups(pDuelSource, pDuelNew, groupMappings)) {
    clog("e", "Unable to map sgroups.");
    end_duel(pDuelNew);
    return 0;
  }
  if (!ddd::mapUncopy(pDuelSource, pDuelNew, effectMappings)) {
    clog("e", "Unable to map uncopy.");
    end_duel(pDuelNew);
    return 0;
  }


  /*
   * Map duel var members by replacing them with their mapped equivalents
   */
  if (!ddd::mapCardMembers(pDuelSource, pDuelNew, cardMappings,
                           cardReverseMappings, effectMappings,
                           effectReverseMappings, rhMappings)) {
    clog("e", "Unable to map card members.");
    end_duel(pDuelNew);
    return 0;
  }
  if (!ddd::mapGroupMembers(pDuelSource, pDuelNew,
                            cardMappings, groupReverseMappings)) {
    clog("e", "Unable to map group members.");
    end_duel(pDuelNew);
    return 0;
  }
  if (!ddd::mapEffectMembers(pDuelNew, cardMappings, cardReverseMappings,
                             effectReverseMappings)) {
    clog("e", "Unable to map effect members.");
    end_duel(pDuelNew);
    return 0;
  }

  /*
   * Copy duel immediate members
   */
  memcpy(&(pDNew->strbuffer), &(pDSource->strbuffer), MAX_STR_BUFFER_SIZE);
  pDNew->message_buffer = pDSource->message_buffer;
  ddd::copyRandomState(pDSource->random, pDNew->random);

  /*
   * Copy and map interpreter
   */
  {
    std::scoped_lock<std::mutex> gen(getDDD_GS().dddapi.genMutex);
    if (!ddd::copyAndMapInterpreter(pDuelSource, pDuelNew,
                                    cardMappings, groupMappings,
                                    effectMappings, rhMappings)) {
      clog("e", "Unable to map interpreter.");
      end_duel(pDuelNew);
      return 0;
    }
  }

  /*
   * Copy struct+class immediate members of source duel/state to new duel/state
   */
  //game_field->player_info player[] (both)
  if (!ddd::copyAndMapFieldPlayerInfo(pDuelSource, pDuelNew, cardMappings)) {
    clog("e", "Unable to map field player infos.");
    end_duel(pDuelNew);
    return 0;
  }

  //game_field->temp_card
  pDNew->game_field->temp_card = pDSource->game_field->temp_card;
  if (pDNew->game_field->temp_card) {
    if (cardMappings.find(pDNew->game_field->temp_card) ==
        cardMappings.end()) {
      clog("w", "Could not find mapping for card c(",
           pDNew->game_field->temp_card,
           ") for pDNew->game_field->temp_card");
      clog("e", "Unable to map field temp_card.");
      end_duel(pDuelNew);
      return 0;
    }
    pDNew->game_field->temp_card
      = cardMappings.at(pDNew->game_field->temp_card);
  }

  //game_field->infos (only copy; nothing to map)
  pDNew->game_field->infos = pDSource->game_field->infos;

  //game_field->effects
  if (!ddd::copyAndMapFieldFieldEffect(pDuelSource, pDuelNew,
                                       cardMappings, effectMappings)) {
    clog("e", "Unable to map field effects.");
    end_duel(pDuelNew);
    return 0;
  }

  //game_field->core
  if (!ddd::copyAndMapFieldProcessor(pDuelSource, pDuelNew, cardMappings,
                                     groupMappings, effectMappings)) {
    clog("e", "Unable to map field processor.");
    end_duel(pDuelNew);
    return 0;
  }

  //game_field->returns
  std::memcpy(&(pDNew->game_field->returns),
              &(pDSource->game_field->returns), 64);

  //game_field->nil_event (maybe should be its own function?)
  card* nilEventC = pDNew->game_field->nil_event.trigger_card;
  group* nilEventG = pDNew->game_field->nil_event.event_cards;
  effect* nilEventE = pDNew->game_field->nil_event.reason_effect;
  if (nilEventC) {
    if (cardMappings.find(nilEventC) == cardMappings.end()) {
      clog("w", "Could not find mapping for card c(", nilEventC,
           ") for pDNew->game_field->nil_event.trigger_card");
      clog("e", "Unable to map field nil_event.");
      end_duel(pDuelNew);
      return 0;
    } else {
      pDNew->game_field->nil_event.trigger_card = cardMappings.at(nilEventC);
    }
  }
  if (nilEventG) {
    if (groupMappings.find(nilEventG) == groupMappings.end()) {
      clog("w", "Could not find mapping for group g(", nilEventG,
           ") for pDNew->game_field->nil_event.event_cards");
      clog("e", "Unable to map field nil_event.");
      end_duel(pDuelNew);
      return 0;
    } else {
      pDNew->game_field->nil_event.event_cards = groupMappings.at(nilEventG);
    }
  }
  if (nilEventE) {
    if (effectMappings.find(nilEventE) == effectMappings.end()) {
      clog("w", "Could not find mapping for effect e(", nilEventE,
           ") for pDNew->game_field->nil_event.reason_effect");
      clog("e", "Unable to map field nil_event.");
      end_duel(pDuelNew);
      return 0;
    } else {
      pDNew->game_field->nil_event.reason_effect = effectMappings.at(nilEventE);
    }
  }

  return pDuelNew;
}

//determine if an effect is considered equivalent to another effect
// (between different pDuels)
bool ddd::isEffectEquivalent(effect* e1, effect* e2) {
  bool result = true;

  auto isCardEquivalent = [](card* c1, card* c2) {
      if (!!c1 != !!c2)
        return false;

      if ((c1 != nullptr) && (c2 != nullptr)) {
        if (c1->data.code != c2->data.code)
          return false;
        if (c1->current.controler != c2->current.controler)
          return false;
        if (c1->current.location != c2->current.location)
          return false;

        // sequence may differ because function may be called before
        //  card has been properly mapped
        //if (c1->current.sequence != c2->current.sequence)
        //  return false
      }
      return true;
  };

  if (!isCardEquivalent(e1->owner, e2->owner)) return false;
  if (!isCardEquivalent(e1->handler, e2->handler)) return false;

  if (e1->description != e2->description) return false;
  if (!!e1->condition != !!e2->condition) return false;
  if (!!e1->cost != !!e2->cost) return false;
  if (!!e1->target != !!e2->target) return false;
  if (!!e1->value != !!e2->value) return false;
  if (!!e1->operation != !!e2->operation) return false;

  //flag[0] may differ
  //if (e1->flag[0] != e2->flag[0]) return false;
  if (e1->flag[1] != e2->flag[1]) return false;
  if (e1->type != e2->type) return false;
  if (e1->code != e2->code) return false;

  return result;
}

//check if a pDuel can be reliably duplicated in its present state
bool ddd::isDuelDuplicatable(intptr_t pDuel) {

  duel* pD = (duel*) pDuel;

  if (pD->lua->lua_state != pD->lua->current_state)
    return false;
  if (pD->lua->coroutines.size() > 0)
    return false;

  //perhaps not necessary but for extra safety, consider safe
  // to duplicate only if current message in buffer is one of
  // 4 known safe messages
  if (pD->message_buffer.size() < 1)
    return false;

  int8 msg = pD->message_buffer[0];
  if ((msg == 40) ||  //MSG_NEW_TURN
      (msg == 41) ||  //MSG_NEW_PHASE
      (msg == 11) ||  //MSG_IDLECMD
      (msg == 10))    //MSG_BATTLECMD
    return true;

  return false;
}

//determine card equivalents and copy contents to equivalents
// (despite its name, some effects also get mapped here;
//  specifically where the mapEffectsFromCardEffectContainers
//  function is called)
bool ddd::copyAndMapCards(intptr_t pDuelSource, intptr_t pDuelNew, std::unordered_map<card*, card*>& cardMappings, std::unordered_map<card*, card*>& cardReverseMappings, std::unordered_map<effect*, effect*>& effectMappings, std::unordered_map<effect*, effect*>& effectReverseMappings, std::unordered_map<int32, int32>& rhMappings, std::unordered_set<card*>& deferredMappingsCards) {

  bool status = true;
  duel* pDSource = (duel*) pDuelSource;
  duel* pDNew = (duel*) pDuelNew;

  card* tempCardNew = nullptr;
  int tempCardNewCount = 0;

  //check for and remember the temp_card/code 0 card
  // (not sure what the card actually is but there should only be
  //  exactly 1 either way created when duel was constructed)
  if (pDNew->cards.size() != 1) {
    clog("e", "Expected exactly 1 card (temp_card) to exist in new"
         " duel before mapping (got ", pDNew->cards.size(), " instead).");
    return false;
  }
  tempCardNew = *(pDNew->cards.begin());

  std::vector<card*> cardsToRegister;
  cardsToRegister.reserve(pDSource->cards.size() - 1);

  //iterate through all cards in source duel/state
  for (const auto &sc: pDSource->cards) {
    card* pCardNew = nullptr;

    bool isZeroCard = (sc->data.code == 0);

    //create new card UNLESS determined to be temp_card/code 0 card
    if (isZeroCard) {
      pCardNew = tempCardNew;
      ++tempCardNewCount;
    } else {
      pCardNew = new card(0);
    }

    //update members of new card
    if (isZeroCard) {
      int32 rh = pCardNew->ref_handle; //temporary store
      *pCardNew = *sc; //copy contents of source card to new card
      pCardNew->ref_handle = rh; //restore original ref handle
    } else {
      *pCardNew = *sc; //copy contents of source card to new card
    }
    pCardNew->pduel = pDNew; //use new duel pointer

    //insert to new duel (possibly check if exists before inserting?)
    pDNew->cards.insert(pCardNew);

    //update card mappings
    cardMappings.insert(std::make_pair(sc, pCardNew));
    cardReverseMappings.insert(std::make_pair(pCardNew, sc));

    //clear effect containers before registering
    pCardNew->single_effect.clear();
    pCardNew->field_effect.clear();
    pCardNew->equip_effect.clear();
    pCardNew->target_effect.clear();
    pCardNew->xmaterial_effect.clear();

    //register card (also registers some effects related to card)
     if (!isZeroCard) //zero card should already be registered
       cardsToRegister.push_back(pCardNew);

    //map card ref handle
    rhMappings.insert(std::make_pair(sc->ref_handle, pCardNew->ref_handle));
  }

  {
    std::scoped_lock<std::mutex> gen(getDDD_GS().dddapi.genMutex);
    for (const auto& nc: cardsToRegister)
      pDNew->lua->register_card(nc);
  }


  if (tempCardNewCount == 0) {
    clog("e", "No temp_card mappings were created where exactly 1"
         " was expected.");
    status = false;
  } else if (tempCardNewCount > 1) {
    clog("e", "Multiple (", tempCardNewCount, ") temp_card mappings"
         " were created where exactly 1 was expected.");
    status = false;
  }

  if (status) {
    status = ddd::
      mapEffectsFromCardEffectContainers(pDSource->cards, cardMappings,
                                         effectMappings,
                                         effectReverseMappings,
                                         rhMappings, true,
                                         deferredMappingsCards);
  }

  return status;
}

//determine effect container and effect equivalents and copy contents
//  of any effects to their equivalents
bool ddd::mapEffectsFromCardEffectContainers(const std::unordered_set<card*>& cardsSource, const std::unordered_map<card*, card*>& cardMappings, std::unordered_map<effect*, effect*>& effectMappings, std::unordered_map<effect*, effect*>& effectReverseMappings, std::unordered_map<int32, int32>& rhMappings, bool allowDefferedMappings, std::unordered_set<card*>& deferredMappingsCards) {
  bool status = true;

  //lambda declaration to be used for each effect container in card
  auto mapEffectContainer = [&](const card::effect_container& sec, card::effect_container& nec, card* c, std::string memberStr) {
    bool ecStatus = status;
    card::effect_container newEc;

    for (const auto &ecp: sec) {
      effect* se = ecp.second;
      std::vector<effect*> foundMappings;

      //get equivalent effect_container effects and find potential mappings
      auto ner = nec.equal_range(ecp.first);

      //check if no candidates returned; if not, might have to
      // possibly be added later when mapping effects
      if ((allowDefferedMappings) && (ner.first == ner.second)) {
        deferredMappingsCards.insert(c);
        continue;
      }

      for (auto i = ner.first; i != ner.second; ++i)
        if (ddd::isEffectEquivalent(se, i->second))
          foundMappings.push_back(i->second);

      if (foundMappings.size() == 0) {

        //if no candidate in new effect container, check if mapping
        // for current effect found and if so, add it to new effect
        // container
        if (effectMappings.find(se) != effectMappings.end()) {
          //source effect container has an effect with a mapped
          // equivalent but is not in new effect container
          effect* ne = effectMappings.at(se);
          newEc.insert(std::make_pair(ecp.first, ne));

        } else {
          clog("w", "Unable to find appropriate mapping for e(", se,
               ") for effect_container in ", memberStr);
          clog("d", "candidates:");
          clog("d", "(ner.first==ner.second: ",
               ((ner.first == ner.second) ? "true" : "false"),
               "; if true, no results were returned for equal_range())");
          for (auto i = ner.first; i != ner.second; ++i)
            clog("d", i->second);

          ecStatus = false;
        }

      } else if (foundMappings.size() > 1) {
        clog("w", "Unable to find unique mapping for e(", se,
             ") (found ", foundMappings.size(), " mappings)");
        ecStatus = false;

      } else if (foundMappings.size() != 1) {
        clog("e", "foundMappings.size() = ", foundMappings.size());
        ecStatus = false;

      } else {
        //successfully found 1 unique mapping
        effect* ne = foundMappings[0];
        effectMappings.insert(std::make_pair(se, ne));
        effectReverseMappings.insert(std::make_pair(ne, se));
        newEc.insert(std::make_pair(ecp.first, ne));
        rhMappings.insert(std::make_pair(se->ref_handle, ne->ref_handle));
      }
    }

    if (ecStatus)
      nec = newEc;

    return ecStatus;
  };

  //iterate all cards (in source duel/state)
  //iterate all cards in set
  //for (const auto &sc: pDSource->cards) {
  for (const auto &sc: cardsSource) {
    if (cardMappings.find(sc) == cardMappings.end()) {
      clog("w", "Could not find mapping for card c(", sc,
           ") while mapping card effect container.");
      status = false;
      continue;
    }

    card* nc = cardMappings.at(sc);

    std::stringstream buffer;
    buffer << "pDuelSource->cards[c(" << sc << ")]";
    std::string pref = buffer.str();

    //check each effect container
    status = mapEffectContainer(sc->single_effect, nc->single_effect,
                                sc, pref + ".single_effect");
    status = mapEffectContainer(sc->field_effect, nc->field_effect,
                                sc, pref + ".field_effect");
    status = mapEffectContainer(sc->equip_effect, nc->equip_effect,
                                sc, pref + ".equip_effect");
    status = mapEffectContainer(sc->target_effect, nc->target_effect,
                                sc, pref + ".target_effect");
    status = mapEffectContainer(sc->xmaterial_effect, nc->xmaterial_effect,
                                sc, pref + ".xmaterial_effect");
  }

  return status;
}

//determine group equivalents and copy contents to equivalents
bool ddd::copyAndMapGroups(intptr_t pDuelSource, intptr_t pDuelNew, std::unordered_map<group*, group*>& groupMappings, std::unordered_map<group*, group*>& groupReverseMappings, std::unordered_map<int32, int32>& rhMappings) {
  bool status = true;
  duel* pDSource = (duel*) pDuelSource;
  duel* pDNew = (duel*) pDuelNew;

  group* zeroGroupNew = nullptr;
  int zeroGroupNewCount = 0;

  //check for and remember the empty group created when duel
  // was constructed
  // (not sure what this group actually is but there should only
  //  be exactly 1 either way; ...seems there are some supposedly
  //  valid cases where there are none...?)
  if (pDNew->groups.size() > 1) {
    clog("e", "Expected 1 or 0 (zero_group) group to exist in new duel"
         " before mapping (got ", pDNew->groups.size(), " instead).");
    return false;

  } else if (pDNew->groups.size() == 1)
    zeroGroupNew = *(pDNew->groups.begin());

  //iterate through all cards in source duel
  for (const auto &pg: pDSource->groups) {

    group* pGroupNew = nullptr;

    //create new group UNLESS determined to be zero_group/code 0 group
    if (pg->container.size() == 0) {
      pGroupNew = zeroGroupNew;
      ++zeroGroupNewCount;
    } else {
      pGroupNew = new group(0);
    }

    //update members of new group
    *pGroupNew = *pg; //copy contents of source group to new group
    pGroupNew->pduel = pDNew; //use new duel pointer

    //insert to new duel (possibly check if exists before inserting?)
    pDNew->groups.insert(pGroupNew);

    //update group mappings
    groupMappings.insert(std::make_pair(pg, pGroupNew));
    groupReverseMappings.insert(std::make_pair(pGroupNew, pg));

    //register group
    pDNew->lua->register_group(pGroupNew);

    //map ref handle
    rhMappings.insert(std::make_pair(pg->ref_handle, pGroupNew->ref_handle));
  }

  if (zeroGroupNewCount == 0) {
    //seemingly, this can happen in some valid states

  } else if (zeroGroupNewCount > 1) {
    clog("e", "Multiple (", zeroGroupNewCount, ") zero_group mappings"
         " were created where exactly 1 was expected.");
    status = false;
  }

  return status;
}

//determine effect equivalents and copy contents to equivalents
// (note that some effects were previously mapped in the
//  mapEffectsFromCardEffectContainers() function;
//  unfortunately, this function cannot be completely reliable as
//  functions cannot be compared and each pDuel has their own instances
//  of the same functions under different pointers)
bool ddd::copyAndMapEffects(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings, std::unordered_map<effect*, effect*>& effectMappings, std::unordered_map<effect*, effect*>& effectReverseMappings, std::unordered_map<int32, int32>& rhMappings, std::unordered_set<card*>& deferredMappingsCards) {
  bool status = true;
  duel* pDSource = (duel*) pDuelSource;
  duel* pDNew = (duel*) pDuelNew;

  std::unordered_set<effect*> sEffectsWithMappings;
  std::unordered_set<effect*> sEffectsWithoutMappings;
  std::unordered_set<effect*> nEffectsWithoutMappings;

  //iterate through all effects in source duel
  for (const auto &pe: pDSource->effects) {
    //check effect was not already added via copyAndMapCards()
    if (effectMappings.find(pe) == effectMappings.end()) {
      //get effects that still don't have a mapping yet
      sEffectsWithoutMappings.insert(pe);
    } else {
      sEffectsWithMappings.insert(pe);
    }
  }

  for (const auto &e: pDNew->effects)
    if (sEffectsWithMappings.find(e) == sEffectsWithMappings.end())
      nEffectsWithoutMappings.insert(e);

  for (const auto &se: sEffectsWithoutMappings) {
    std::vector<effect*> foundMappings;
    for (const auto &ne: nEffectsWithoutMappings)
      if (ddd::isEffectEquivalent(se, ne))
        foundMappings.push_back(ne);

    if (foundMappings.size() == 0) {
      //create, register then map new effect here

      effect* pEffectNew = new effect(0);

      //update members of new effect
      // (nonexistant values for cctvo might be assigned here)
      *pEffectNew = *se;
      pEffectNew->pduel = pDNew;

      //insert to new duel
      pDNew->effects.insert(pEffectNew);

      //register effect
      pDNew->lua->register_effect(pEffectNew);

      //update effect mappings
      effectMappings.insert(std::make_pair(se, pEffectNew));
      effectReverseMappings.insert(std::make_pair(pEffectNew, se));

      //map ref handle
      rhMappings.insert(std::make_pair(se->ref_handle, pEffectNew->ref_handle));

    } else if (foundMappings.size() > 1) {
      clog("w", "Unable to find unique mapping for e(", se,
           ") (found ", foundMappings.size(), " mappings)");
      status = false;

    } else if (foundMappings.size() != 1) {
      clog("e", "foundMappings.size() = ", foundMappings.size());
      status = false;

    } else {
      //successfully found 1 unique mapping
      effect* ne = foundMappings[0];
      effectMappings.insert(std::make_pair(se, ne));
      effectReverseMappings.insert(std::make_pair(ne, se));
      rhMappings.insert(std::make_pair(se->ref_handle, ne->ref_handle));
    }
  }


  if (status) {
    status = ddd::
      mapEffectsFromCardEffectContainers(deferredMappingsCards,
                                         cardMappings, effectMappings,
                                         effectReverseMappings,
                                         rhMappings, false,
                                         deferredMappingsCards);
  }

  //helper struct to help log any functions where their
  // equivalence cannot be guaranteed
  struct BlindTrustTemp {
    int32 si;
    const void* siPtr;
    effect* se;
    int32 ni;
    const void* niPtr;
    effect* ne;
  };
  std::vector<BlindTrustTemp> bttVec;

  if (status) {
    //map all effect condition, cost, target, value and operation
    // if they are non zero

    for (const auto &se: pDSource->effects) {
      if (effectMappings.find(se) == effectMappings.end()) {
        clog("e", "Unable to find equivalent effect* for e(", se,
             ") in effects (when mapping cctvo values).");
        status = false;
        continue;
      }

      effect* ne = effectMappings.at(se);

      //lambda declarations
      auto mapAndRegisterCorrespondingRhFunction = [&](int32 si, int32& ni) {
        //check if ni exists in registry and is also the
        // corresponding function
        //if so, changes the value of ni (passed by reference)
        // and returns true; returns false otherwise


        bool marcrf = status;
        lua_State* LSource = pDSource->lua->lua_state;
        lua_State* LNew = pDNew->lua->lua_state;


        //get si from registry and verify it is a function
        lua_rawgeti(LSource, LUA_REGISTRYINDEX, si);
        if (!lua_isfunction(LSource, -1)) {
          clog("w", "Expected ref handle ", si, " in lua registry"
               " to be of type function (was of type '",
               lua_typename(LSource, lua_type(LSource, -1)),
               "' instead).");
          lua_pop(LSource, 1);
          return false;
        }

        //get ptr and pop from stack
        const void* siPtr = lua_topointer(LSource, -1);
        lua_pop(LSource, 1);

        //get card code to look up in globals table
        uint32 code = 0;
        if ((se->owner) &&
            (pDSource->cards.find(se->owner) != pDSource->cards.end())) {
          code = se->owner->get_code();
        }

        //(inner) lambda declaration
        auto checkFuncInLuaGlobalTable = [&](const char* tableName) {
          std::string funcName;

          //if table name not specified, check in whole
          // globals table; otherwise check in specified table name
          if (tableName == nullptr)
            lua_pushglobaltable(LSource); //-0/+1
          else
            lua_getglobal(LSource, tableName); //-0/+1

          if (lua_isnil(LSource, -1)) {
            //couldn't find table
            lua_pop(LSource, 1);
            return std::string();

          } else if (!lua_istable(LSource, -1)) {
            clog("w", "Expected lua global variable '", tableName,
                 "' in source duel/state ", pDuelSource, " (",
                 (duel*) pDuelSource, ") to be of type table (was of"
                 " type '", lua_typename(LSource, lua_type(LSource, -1)),
                 "' instead).");
            lua_pop(LSource, 1);
            return std::string();
          }

          //iterate table and check and compare pointer
          int indexOfTable = lua_gettop(LSource); //get index of table
          lua_pushnil(LSource); //push nil as first key
          std::string func;
          while (lua_next(LSource, indexOfTable) != 0) {
            //ignore if not function
            if (lua_type(LSource, -1) != LUA_TFUNCTION) {
              lua_pop(LSource, 1); //pop only value; keeping key
              continue;
            }
            //check if pointer matches that of si in registry
            if (lua_topointer(LSource, -1) == siPtr) {
              funcName = lua_tostring(LSource, -2); //save key as string
              lua_pop(LSource, 2); //pop key and value
              break;
            }

            lua_pop(LSource, 1); //pop only value; keeping key
          }
          lua_pop(LSource, 1); //pop table
          return funcName;
        };

        //then check source lua global table

        std::vector<std::string> gVarCandidates;
        std::string gVarCandidate;
        std::string func;

        //get candidates of table names to search, based on
        // whether the effect's owner (a card*) has a code or not
        if (code) {
          gVarCandidates = {
            'c' + std::to_string(code)
            ,'c' + std::to_string(code - 1)
            ,"Auxiliary"
          };
        } else {
          std::unordered_set<std::string> gVarCandidatesSet;
          gVarCandidatesSet.insert("Auxiliary");
          for (const auto &c: pDSource->cards)
            gVarCandidatesSet.insert('c' + std::to_string(c->get_code()));
          gVarCandidates
            = std::vector<std::string>(gVarCandidatesSet.begin(),
                                       gVarCandidatesSet.end());
        }
        for (const auto &gvc: gVarCandidates) {
          std::string f = checkFuncInLuaGlobalTable(gvc.c_str());
          if (!f.empty()) {
            func = f;
            gVarCandidate = (gvc.empty()) ? "_G" : gvc;
            break;
          }
        }


        if (func.empty()) {
          //...if mapping in new exists and is a function, might
          // just have to trust it is indeed correct...?
          lua_rawgeti(LNew, LUA_REGISTRYINDEX, ni);
          if (lua_type(LNew, -1) == LUA_TFUNCTION) {

            auto lgm = luaGlobalsToMap(((duel*) pDuelSource)->lua->lua_state);
            for (const auto &lge: lgm) {
              if (siPtr == lge.second.ptr) {
                clog("e", "!!\t", lge.second.name, "  (func ptr: (",
                     lge.second.ptr, "))");
                clog("e", "!!\tcode: ", se->owner->data.code,
                     "  code (via get_code()):", code,
                     "  gVarCandidate: '",
                     gVarCandidate, "'");
                clog("e", "!!\tpDuelSource: ", pDuelSource,
                     "  se: e(", se, ")");
                printEffectDebug(se);
                auto asdf = se->owner->get_code();
                //throw std::exception();
              }
            }

            const void* niPtr = lua_topointer(LNew, -1);

            BlindTrustTemp btt;
            btt.se = se;
            btt.si = si;
            btt.siPtr = siPtr;
            btt.ne = ne;
            btt.ni = ni;
            btt.niPtr = niPtr;
            bttVec.push_back(btt);

            lua_pop(LNew, 1);
            return marcrf;

          } else {
            clog("w", "Unable to find function ptr (", siPtr,
                 ") within '", ((code) ? gVarCandidate : "all globals"),
                 "' and 'Auxiliary' in lua globals for ", pDuelSource,
                 " (", (duel*) pDuelSource, ").");
            lua_pop(LNew, 1);
            return false;
          }
        }

        if ((code) || (gVarCandidate != "_G")) {
          //get table from other lua state
          lua_getglobal(LNew, gVarCandidate.c_str()); //(-0/+1)
        } else {
          //get lua global table from other lua state
          lua_pushglobaltable(LNew); //(-0/+1)
        }

        if (!lua_istable(LNew, -1)) {
          clog("w", "Expected lua global variable '", gVarCandidate,
               "' in new duel/state ", pDuelNew, " (",
               (duel*) pDuelNew, ") to be of type table (was of"
               " type '", lua_typename(LNew, lua_type(LNew, -1)),
               "' instead).");
          lua_pop(LNew, 1);
          return false;
        }

        //push table to top of stack
        lua_pushstring(LNew, func.c_str()); //push key to top
        lua_rawget(LNew, -2); //use key to get table value (-1/+1)
        if (lua_type(LNew, -1) != LUA_TFUNCTION) {
          clog("w", "Expected lua global variable '",
               ((code) ? (gVarCandidate + "." + func) : func),
               "' in new duel/state ", pDuelNew, " (",
               (duel*) pDuelNew, ") to be of type function (was of"
               " type '", lua_typename(LNew, lua_type(LNew, -1)),
               "' instead).");
          lua_pop(LNew, 2);
          return false;
        }

        //save ptr
        const void* niPtr = lua_topointer(LNew, -1);

        //get registry entry (push to top of stack) and compare it
        // against ni by checking if it matches found ptr
        lua_rawgeti(LNew, LUA_REGISTRYINDEX, ni);
        if (niPtr != lua_topointer(LNew, -1)) {

          //register found function (currently at top of stack)
          // (while also setting ni to new ref handle)
          lua_pop(LNew, 1); //pop ni registry entry
          ni = luaL_ref(LNew, LUA_REGISTRYINDEX); // (-1/+0)

        } else {
          //ni was already set to correct ref handle
          lua_pop(LNew, 2); //pop ni registry entry and card table value
        }

        lua_pop(LNew, 1); //pop table

        return marcrf;
      };
      auto checkAndMapRh = [&](int32 si, int32& ni, uint32 flag, const std::string& memberStr) {
        //attempts to find the equivalent ref handle of si

        bool camrhStatus = status;

        if (!!si != !!ni) {
          clog("w", "Differing values for ", memberStr, " where both"
               " values should be 0 or non 0 (", si , " vs ", ni, ").");
          return false;
        }

        //ignore if both are 0
        if (!si)
          return camrhStatus;

        //ignore value if flag not set
        if (!(flag & 0x0002)) //EFFECT_FLAG_FUNC_VALUE
          return camrhStatus;

        //if both values are same, strong hint to further check if ni
        // exists in registry and is also the corresponding function
        if (si == ni) {
          camrhStatus = mapAndRegisterCorrespondingRhFunction(si, ni);
        }

        if (camrhStatus)
          rhMappings.insert(std::make_pair(si, ni));

        return camrhStatus;
      };

      uint32 flag = 0x0002; //EFFECT_FLAG_FUNC_VALUE bit set to true

      std::stringstream buffer;
      buffer << "e[Source|New](" << se << " | " << ne << ")->";
      std::string pref = buffer.str();

      status = checkAndMapRh(se->condition, ne->condition, flag,
                             pref + "condition");
      status = checkAndMapRh(se->cost, ne->cost, flag,
                             pref + "cost");
      status = checkAndMapRh(se->target, ne->target, flag,
                             pref + "target");
      status = checkAndMapRh(se->value, ne->value, se->flag[0],
                             pref + "value");
      status = checkAndMapRh(se->operation, ne->operation, flag,
                             pref + "operation");
    }
  }

  //output any functions if their equivalence cannot be guaranteed
  if (bttVec.size() > 0) {
    clog("d", "Blindly trusting ", bttVec.size(), " ref handles in ",
         pDuelSource, " (", (duel*) pDuelSource, ") to be"
         " equivalent functions:");
    for (const auto &btt: bttVec) {
      clog("d", "    ", "ref handle: ", btt.si, " (", btt.siPtr,
           "); new ref handle: ", btt.ni, " (", btt.niPtr, ")");
    }
  }


  return status;
}

//determine assumes equivalents from card mappings and map them
bool ddd::mapAssumes(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings) {
  bool status = true;
  duel* pDNew = (duel*) pDuelNew;
  duel* pDSource = (duel*) pDuelSource;

  for (const auto &a: pDSource->assumes) {
    auto equivalent = cardMappings.find(a);
    if (equivalent == cardMappings.end()) {
      clog("e", "Unable to find equivalent card* for c(", a,
           ") in assumes.");
      status = false;
      continue;
    }
    pDNew->assumes.insert(equivalent->second);
  }


  return status;
}

//determine sgroups equivalents from group mappings and map them
bool ddd::mapSgroups(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<group*, group*>& groupMappings) {
  bool status = true;
  duel* pDNew = (duel*) pDuelNew;
  duel* pDSource = (duel*) pDuelSource;

  for (const auto &s: pDSource->sgroups) {
    auto equivalent = groupMappings.find(s);
    if (equivalent == groupMappings.end()) {
      clog("e", "Unable to find equivalent group* for g(", s,
           ") in sgroups.");
      status = false;
      continue;
    }
    pDNew->sgroups.insert(equivalent->second);
  }

  return status;
}

//determine uncopy equivalents from effect mappings and map them
bool ddd::mapUncopy(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<effect*, effect*>& effectMappings) {
  bool status = true;
  duel* pDNew = (duel*) pDuelNew;
  duel* pDSource = (duel*) pDuelSource;

  for (const auto &u: pDSource->uncopy) {
    auto equivalent = effectMappings.find(u);
    if (equivalent == effectMappings.end()) {
      clog("e", "Unable to find equivalent effect* for e(", u,
           ") in uncopy.");
      status = false;
      continue;
    }
    pDNew->uncopy.insert(equivalent->second);
  }

  return status;
}

//remap or rebuild applicable members of cards in the new pDuel to
// their equivalents in the new pDuel
bool ddd::mapCardMembers(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings, const std::unordered_map<card*, card*>& cardReverseMappings, const std::unordered_map<effect*, effect*>& effectMappings, const std::unordered_map<effect*, effect*>& effectReverseMappings, const std::unordered_map<int32, int32>& rhMappings) {
  bool status = true;
  duel* pDNew = (duel*) pDuelNew;

  //lambda declarations
  auto mapCard = [&](card*& c, const std::string& memberStr) {
    if (cardMappings.find(c) == cardMappings.end()) {
      clog("w", "Could not find mapping for card c(", c,
           ") in ", memberStr);
      return false;
    }
    c = cardMappings.at(c);
    return status;
  };
  auto mapEffect = [&](effect*& e, const std::string& memberStr) {
    if (effectMappings.find(e) == effectMappings.end()) {
      //unable to find any mappings (for e)
      //do nothing; see note for equivalent function in dddstate.cpp
    } else {
      e = effectMappings.at(e);
    }
    return status;
  };
  auto mapRelationMap = [&](std::unordered_map<card*, uint32>& rm, const std::string& memberStr) {
    bool mrmStatus = status;
    std::unordered_map<card*, uint32> newRm;

    for (const auto &r: rm) {
      if (cardMappings.find(r.first) == cardMappings.end()) {
        clog("w", "Could not find mapping for card c(", r.first,
             ") for relation map in ", memberStr);
        mrmStatus = false;
        continue;
      }
      newRm.insert(std::make_pair(cardMappings.at(r.first), r.second));
    }
    if (mrmStatus)
      rm = newRm;

    return mrmStatus;
  };
  auto mapAttackerMap = [&](card::attacker_map& am, const std::string& memberStr) {
    bool amStatus = status;
    card::attacker_map newAm;

    for (const auto &a: am) {
      card* c = a.second.first;
      if (cardMappings.find(c) == cardMappings.end()) {
        clog("w", "Could not find mapping for card c(", a.first,
             ") for attacker map in ", memberStr);
        amStatus = false;
        continue;
      }
      newAm.insert(std::make_pair(a.first, std::make_pair(cardMappings.at(c),
                                                          a.second.second)));
    }
    if (amStatus)
      am = newAm;

    return amStatus;
  };
  auto mapCardSet = [&](card::card_set& cs, const std::string& memberStr) {
    bool csStatus = status;
    card::card_set newCs;
    for (const auto &c: cs) {
      if (cardMappings.find(c) == cardMappings.end()) {
        clog("w", "Could not find mapping for card c(", c,
             ") for card_set in ", memberStr);
        csStatus = false;
        continue;
      }
      newCs.insert(cardMappings.at(c));
    }
    if (csStatus)
      cs = newCs;
    return csStatus;
  };
  auto mapCardVector = [&](std::vector<card*>& cv, const std::string& memberStr) {
    bool cvStatus = status;
    for (auto &c: cv) {
      if (cardMappings.find(c) == cardMappings.end()) {
        clog("w", "Could not find mapping for card c(", c,
             ") for card vector in ", memberStr);
        cvStatus = false;
      } else {
        c = cardMappings.at(c);
      }
    }
    return cvStatus;
  };
  auto mapEffectContainer = [&](std::multimap<uint32, effect*>& ec, const std::string& memberStr) {
    bool ecStatus = status;
    std::multimap<uint32, effect*> newEc;

    for (const auto &e: ec) {
      if (effectMappings.find(e.second) != effectMappings.end()) {
        //should technically never end up here
        newEc.insert(std::make_pair(e.first, effectMappings.at(e.second)));
      } else if (effectReverseMappings.find(e.second) !=
                 effectReverseMappings.end()) {
        //already correctly mapped; just copy it over
        newEc.insert(e);
      } else {
        clog("w", "Could not find mapping for effect e(", e.second,
             ") for effect container in ", memberStr);
        ecStatus = false;
      }
    }
    if ((ecStatus) && (ec.size() != newEc.size())) {
      clog("w", "Size discrepency between source and new effect"
           " container (", ec.size(), " vs ", newEc.size(), ")");
      ecStatus = false;
    }
    if (ecStatus)
      ec = newEc;

    return ecStatus;
  };
  auto rebuildEffectIndexer = [&](card::effect_indexer& ei, card* nc, const std::string& memberStr) {
    //rather than map from source indexer, rebuild indexer without regard
    // to what the source indexer actually contains (other than maybe to verify)
    bool eiStatus = status;
    duel* pDSource = (duel*) pDuelSource;
    card* sc = nullptr;

    //get source card (for its effect_containers to use for comparisons)
    if (cardReverseMappings.find(nc) == cardReverseMappings.end()) {
      clog("w", "Could not map indexer because unable to find"
           " (reverse) mapping for card* c(", nc, ")");
      eiStatus = false;
    }
    sc = cardReverseMappings.at(nc);

    //clear effect_indexer
    ei.clear();

    auto insertIntoEffectIndexer = [&](card::effect_container& sec, card::effect_container& nec, const std::string& memberStr) {
      if (sec.size() != nec.size()) {
        clog("w", "Discrepency in size for effect container (",
             sec.size(), " vs ", nec.size(), ") in ", memberStr);
        eiStatus = false;
      }

      for (auto i = nec.begin(); i != nec.end(); ++i) {
        ei.insert(std::make_pair(i->second, i));
        //maybe also check reverse effect mappings equivalence here...?
      }
    };

    insertIntoEffectIndexer(sc->single_effect, nc->single_effect,
                            memberStr + ".single_effect");
    insertIntoEffectIndexer(sc->field_effect, nc->field_effect,
                            memberStr + ".field_effect");
    insertIntoEffectIndexer(sc->equip_effect, nc->equip_effect,
                            memberStr + ".equip_effect");
    insertIntoEffectIndexer(sc->target_effect, nc->target_effect,
                            memberStr + ".target_effect");
    insertIntoEffectIndexer(sc->xmaterial_effect, nc->xmaterial_effect,
                            memberStr + ".xmaterial_effect");

    if (ei.size() != sc->indexer.size()) {
      clog("w", "Discrepency in indexer size (", ei.size(), " vs ",
           sc->indexer.size(), ") in ", memberStr);
      eiStatus = false;
    }

    return eiStatus;
  };
  auto mapEffectRelation = [&](card::effect_relation& er, const std::string& memberStr) {
    bool erStatus = status;
    card::effect_relation newEr;

    for (const auto &r: er) {
      if (effectMappings.find(r.first) == effectMappings.end()) {
        clog("w", "Could not find mapping for effect e(", r.first,
             ") for effect relation in ", memberStr);
        erStatus = false;
        continue;
      }
      newEr.insert(std::make_pair(effectMappings.at(r.first), r.second));
    }
    if (erStatus)
      er = newEr;

    return erStatus;
  };
  auto mapEffectSetV = [&](effect_set_v& esv, const std::string& memberStr) {
    bool esvStatus = status;
    effect_set_v newEsv;

    for (int i = 0; i < esv.size(); ++i) {
      if (effectMappings.find(esv.at(i)) == effectMappings.end()) {
        clog("w", "Could not find mapping for effect e(", esv.at(i),
             ") for effect_set_v in ", memberStr);
        esvStatus = false;
        continue;
      }
      newEsv.add_item(effectMappings.at(esv.at(i)));
    }
    if (esvStatus)
      esv = newEsv;

    return esvStatus;
  };
  auto mapCardId = [&](card* nc, const std::string& memberStr) {
    bool ciStatus = status;
    if (cardReverseMappings.find(nc) == cardReverseMappings.end()) {
      clog("w", "Could not map cardid because unable to find (reverse)"
           " mapping for card c(", nc, ") in ", memberStr);
      return false;
    }
    nc->cardid = cardReverseMappings.at(nc)->cardid;

    return status;
  };


  //iterate through card members (that mostly involve card* or effect*)
  // and need mappings
  for (auto &c: pDNew->cards) {

    std::stringstream buffer;
    buffer << "pDuelNew->cards[c(" << c << ")]->";
    const std::string pref = buffer.str();

    //raw members
    if (c->previous.reason_card)
      status = mapCard(c->previous.reason_card,
                       pref + "previous.reason_card");
    //if (c->previous.reason_effect)
    status = mapEffect(c->previous.reason_effect,
                       pref + "previous.reason_effect");
    if (c->temp.reason_card)
      status = mapCard(c->temp.reason_card,
                       pref + "temp.reason_card");
    //if (c->temp.reason_effect)
    status = mapEffect(c->temp.reason_effect,
                       pref + "temp.reason_effect");
    if (c->current.reason_card)
      status = mapCard(c->current.reason_card,
                       pref + "current.reason_card");
    //if (c->current.reason_effect)
    status = mapEffect(c->current.reason_effect,
                       pref + "current.reason_effect");
    //if (c->unique_effect)
    status = mapEffect(c->unique_effect, pref + "unique_effect");
    if (c->equiping_target)
      status = mapCard(c->equiping_target, pref + "equiping_target");
    if (c->pre_equip_target)
      status = mapCard(c->pre_equip_target, pref + "pre_equip_target");
    if (c->overlay_target)
      status = mapCard(c->overlay_target, pref + "overlay_target");
    status = mapCardId(c, pref + "cardid");

    //relation_map
    if (c->relations.size() > 0)
      status = mapRelationMap(c->relations, pref + "relations");

    //attacker_maps
    if (c->announced_cards.size() > 0)
      status = mapAttackerMap(c->announced_cards,
                              pref + "announced_cards");
    if (c->attacked_cards.size() > 0)
      status = mapAttackerMap(c->attacked_cards,
                              pref + "attacked_cards");
    if (c->battled_cards.size() > 0)
      status = mapAttackerMap(c->battled_cards,
                              pref + "battled_cards");

    //card_sets
    if (c->equiping_cards.size() > 0)
      status = mapCardSet(c->equiping_cards, pref + "equiping_cards");
    if (c->material_cards.size() > 0)
      status = mapCardSet(c->material_cards, pref + "material_cards");
    if (c->effect_target_owner.size() > 0)
      status = mapCardSet(c->effect_target_owner,
                          pref + "effect_target_owner");
    if (c->effect_target_cards.size() > 0)
      status = mapCardSet(c->effect_target_cards,
                          pref + "effect_target_cards");

    //card_vector
    if (c->xyz_materials.size() > 0)
      status = mapCardVector(c->xyz_materials, pref + "xyz_materials");

    //effect_containers
    if (c->single_effect.size() > 0)
      status = mapEffectContainer(c->single_effect,
                                  pref + "single_effect");
    if (c->field_effect.size() > 0)
      status = mapEffectContainer(c->field_effect,
                                  pref + "field_effect");
    if (c->equip_effect.size() > 0)
      status = mapEffectContainer(c->equip_effect,
                                  pref + "equip_effect");
    if (c->target_effect.size() > 0)
      status = mapEffectContainer(c->target_effect,
                                  pref + "target_effect");
    if (c->xmaterial_effect.size() > 0)
      status = mapEffectContainer(c->xmaterial_effect,
                                  pref + "xmaterial_effect");

    //effect_indexer
    // (definitely gotta make sure effect_containers
    //  mappings aren't invalid here)
    if (status)
      if (c->indexer.size() > 0)
        status = rebuildEffectIndexer(c->indexer, c, pref + "indexer");

    //effect_relation
    if (c->relate_effect.size() > 0)
      status = mapEffectRelation(c->relate_effect, pref + "relate_effect");

    //effect_set_v
    if (c->immune_effect.size() > 0)
      status = mapEffectSetV(c->immune_effect, pref + "immune_effect");
  }

  return status;
}

//remap or rebuild applicable members of groups in the new pDuel to
// their equivalents in the new pDuel
bool ddd::mapGroupMembers(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings, const std::unordered_map<group*, group*>& groupReverseMappings) {
  bool status = true;
  duel* pDNew = (duel*) pDuelNew;

  //lambda declarations
  auto mapCardSet = [&](group::card_set& cs, const std::string& memberStr) {
    bool csStatus = status;
    group::card_set newCs;

    for (const auto &c: cs) {
      if (cardMappings.find(c) == cardMappings.end()) {
        clog("w", "Could not find mapping for card c(", c,
             ") for container in group ", memberStr);
        csStatus = false;
        continue;
      }
      newCs.insert(cardMappings.at(c));
    }
    if(csStatus)
      cs = newCs;

    return csStatus;
  };
  auto mapCardSetIterator = [&](group::card_set::iterator& itr, group* g, const std::string& memberStr) {
    bool mciStatus = status;
    group* og = nullptr;

    if (groupReverseMappings.find(g) == groupReverseMappings.end()) {
      clog("w", "Could not map group iterator because unable to find"
           " (reverse) mapping in group ", memberStr);
      return false;
    }

    og = groupReverseMappings.at(g);
    auto foundItr = og->container.end();

    if (itr == og->container.end()) {
      //determined iterator points to container end
      itr = g->container.end(); //mapping here
      return mciStatus;
    }

    for (auto i = og->container.begin(); i != og->container.end(); ++i) {
      //should be able to directly compare itrs here since there's no
      // longer multiple possible containers itr can be from
      if (i == itr) {
        foundItr = i;
        break;
      }
    }

    if (foundItr == og->container.end()) {
      clog("w", "Itr does not appear to belong to container in ", memberStr);
      return false;
    }

    if (cardMappings.find(*itr) == cardMappings.end()) {
      clog("w", "Could not find mapping for card c(", *itr,
           ") for iterator in group for ", memberStr);
      return false;
    }

    auto foundEquivalentItr = g->container.find(cardMappings.at(*itr));

    if (foundEquivalentItr == g->container.end()) {
      clog("w", "Could not find equivalent card c(", *foundEquivalentItr,
           ") in container in group for ", memberStr);
      return false;
    } else {
      itr = foundEquivalentItr;
    }

    return mciStatus;
  };


  //iterate through group members that involve card* and need mappings
  for (auto &g: pDNew->groups) {

    std::stringstream buffer;
    buffer << "pDuelNew->groups[g(" << g << ")]->";
    const std::string pref = buffer.str();


    //container
    if (g->container.size() > 0)
      status = mapCardSet(g->container, pref + "container");

    //it(erator)
    if (status)
      status = mapCardSetIterator(g->it, g, pref + "it");
  }
  return status;
}

//remap or rebuild applicable members of effects in the new pDuel to
// their equivalents in the new pDuel
bool ddd::mapEffectMembers(intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings, const std::unordered_map<card*, card*>& cardReverseMappings, const std::unordered_map<effect*, effect*>& effectReverseMappings) {
  bool status = true;
  duel* pDNew = (duel*) pDuelNew;

  //lambda declarations
  auto mapCard = [&](card*& c, const std::string& memberStr) {
    if (cardReverseMappings.find(c) != cardReverseMappings.end()) {
      //already correctly mapped; no need to change anything
      return status;

    } else if (cardMappings.find(c) != cardMappings.end()) {
      c = cardMappings.at(c);
      return status;

    } else {
      clog("w", "Could not find (reverse) mapping for card c(", c,
           ") in ", memberStr);
      return false;
    }
  };

  //iterate through effect members
  for (auto &e: pDNew->effects) {

    std::stringstream buffer;
    buffer << "pDuelNew->effects[e(" << e << ")]->";
    const std::string pref = buffer.str();

    //map members that involve card* and need mappings
    if (e->owner)
      status = mapCard(e->owner, pref + "owner");

    if (e->handler)
      status = mapCard(e->handler, pref + "handler");

    if (e->active_handler)
      status = mapCard(e->active_handler, pref + "active_handler");

    //update flag[0]
    if (effectReverseMappings.find(e) == effectReverseMappings.end()) {
      clog("w", "Could not find (reverse) mapping for effect e(", e,
           ") (for ", pref + "flag[0])");
      status = false;
    } else {
      e->flag[0] = effectReverseMappings.at(e)->flag[0];
    }

  }

  return status;
}

//remap or rebuild applicable members of the interpreter in the new
// pDuel to their equivalents in the new pDuel
bool ddd::copyAndMapInterpreter(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings, const std::unordered_map<group*, group*>& groupMappings, const std::unordered_map<effect*, effect*>& effectMappings, const std::unordered_map<int32, int32>& rhMappings) {
  bool status = true;
  duel* pDSource = (duel*) pDuelSource;
  duel* pDNew = (duel*) pDuelNew;

  //lambda declarations
  auto mapAndRebuildLuaState = [&](lua_State*& sL, lua_State*& nL, const std::string& memberStr) {
    bool lsStatus = status;

    //pop everything off nL first?
    lua_pop(nL, lua_gettop(nL));

    lsStatus = ddd::copyAndMapInterpreterLuaState(sL, nL, rhMappings);

    return lsStatus;
  };
  auto getPtrOfCurrentStateInLuaState = [&](lua_State*& csL, lua_State*& lsL) {
    lua_State* foundCs = nullptr;

    if (!csL)
      return foundCs;

    int lsTop = lua_gettop(lsL);
    for (int i = lsTop; i >= 1; --i) {
      const void* ptr = lua_topointer(lsL, i);
      if (csL == ptr) {
        foundCs = (lua_State*) ptr;
        break;
      }
    }

    return foundCs;
  };
  auto mapAndRebuildParamList = [&](std::list<std::pair<interpreter::lua_param, LuaParamType>>& spl, std::list<std::pair<interpreter::lua_param, LuaParamType>>& npl, const std::string& memberStr) {

    bool plStatus = status;
    std::list<std::pair<interpreter::lua_param, LuaParamType>> newPl;

    if (spl.size() > 0) {

      for (const auto &plp: spl) {
        if (plp.second == PARAM_TYPE_CARD) {
          card* c = (card*) plp.first.ptr;
          if (cardMappings.find(c) == cardMappings.end()) {
              clog("w", "Could not find mapping for card c(", c,
                   ") for card in ", memberStr);
            plStatus = false;
            continue;
          }
          interpreter::lua_param lp;
          lp.ptr = cardMappings.at(c);
          newPl.emplace_back(std::move(lp), plp.second);

        } else if (plp.second == PARAM_TYPE_GROUP) {
          group* g = (group*) plp.first.ptr;
          if (groupMappings.find(g) == groupMappings.end()) {
              clog("w", "Could not find mapping for group g(", g,
                   ") for group in ", memberStr);
            plStatus = false;
            continue;
          }

          interpreter::lua_param lp;
          lp.ptr = groupMappings.at(g);
          newPl.emplace_back(std::move(lp), plp.second);

        } else if (plp.second == PARAM_TYPE_EFFECT) {
          effect* e = (effect*) plp.first.ptr;
          if (effectMappings.find(e) == effectMappings.end()) {
              clog("w", "Could not find mapping for effect e(", e,
                   ") for effect in ", memberStr);
            plStatus = false;
            continue;
          }

          interpreter::lua_param lp;
          lp.ptr = effectMappings.at(e);
          newPl.emplace_back(std::move(lp), plp.second);

        } else if (plp.second == PARAM_TYPE_FUNCTION) {
          clog("w", "Unknown function (", plp.first.ptr,
               ") to map in ", memberStr);
          newPl.emplace_back(plp);

        } else {
          newPl.emplace_back(plp);

        }
      }

      if (plStatus) {
        npl = std::move(newPl);
      }
    }

    return plStatus;
  };
  auto mapAndRebuildCoroutineMap = [&](std::unordered_map<int32, std::pair<lua_State*, int32>>& scm, std::unordered_map<int32, std::pair<lua_State*, int32>>& ncm, const std::string& memberStr) {

    bool cmStatus = status;

    if (scm.size() > 0) {
      /*
      std::unordered_map<int32, std::pair<lua_State*, int32>> newCm;

      for (const auto &pcm: scm) {
      }

      if (cmStatus)
        ncm = std::move(newCm);
      */
      clog("w", "Unable to map non-empty coroutine map in ", memberStr);
      cmStatus = false;
    }

    return cmStatus;
  };

  std::stringstream buffer;
  buffer << "pDNew->lua->";
  std::string pref = buffer.str();

  //lua_state
  status = mapAndRebuildLuaState(pDSource->lua->lua_state,
                                 pDNew->lua->lua_state,
                                 pref + "lua_state");

  //current_state
  if (pDSource->lua->current_state == pDSource->lua->lua_state) {
    pDNew->lua->current_state == pDNew->lua->lua_state;
  } else {
    lua_State* foundCs
      = getPtrOfCurrentStateInLuaState(pDSource->lua->current_state,
                                       pDSource->lua->lua_state);
    if (foundCs) {
      pDNew->lua->current_state = foundCs;
    } else {
      status = mapAndRebuildLuaState(pDSource->lua->current_state,
                                     pDNew->lua->current_state,
                                     pref + "current_state");
    }
  }

  //msgbuf
  memcpy(&(pDNew->lua->msgbuf), &(pDSource->lua->msgbuf), 64);

  //params
  status = mapAndRebuildParamList(pDSource->lua->params,
                                  pDNew->lua->params, pref + "params");

  //resumes
  status = mapAndRebuildParamList(pDSource->lua->resumes,
                                  pDNew->lua->resumes, pref + "resumes");

  //coroutines
  if (status)
    status = mapAndRebuildCoroutineMap(pDSource->lua->coroutines,
                                       pDNew->lua->coroutines,
                                       pref + "coroutines");

  pDNew->lua->no_action = pDSource->lua->no_action;
  pDNew->lua->call_depth = pDSource->lua->call_depth;

  return status;
}

//attempt to copy the source interpreter's stack contents to
// to the new pDuel's interpreter's stack
bool ddd::copyAndMapInterpreterLuaState(lua_State*& sL, lua_State*& nL, const std::unordered_map<int32, int32>& rhMappings) {

  bool lsStatus = true;

  int sLTop = lua_gettop(sL);

  for (int i = 1; i <= sLTop; ++i) {
    auto type = lua_type(sL, i);
    switch (type) {
    case LUA_TNUMBER:
      {
        lua_pushnumber(nL, lua_tonumber(sL, i));
      }
      break;
    case LUA_TBOOLEAN:
      {
        lua_pushboolean(nL, lua_toboolean(sL, i));
      }
      break;
    case LUA_TNIL:
      {
        lua_pushnil(nL);
      }
      break;
    case LUA_TTHREAD:
      {
        lua_State* st = lua_tothread(sL, i);
        if (lua_status(st) != LUA_OK) {
          clog("w", "Unable to handle type 'thread' with status of ",
               lua_status(st), " (at index ", i, ").");
          lsStatus = false;
          break;
        }
        if (lua_gettop(st) == 0) {
          lua_newthread(nL); //-0,+1 (push empty thread to top of stack)
        } else {
          lua_State* nt = lua_newthread(nL);
          lsStatus = ddd::copyAndMapInterpreterLuaState(st, nt, rhMappings);
        }
      }
      break;
    //intentionally unimplemented as state probably cannot duplicated anyway
    //case LUA_TUSERDATA:
    //case LUA_TFUNCTION:
    default:
      clog("w", "Unable to handle type '", lua_typename(sL, type),
           "' (at index ", i, ") when rebuilding lua state.");
      lsStatus = false;
    }

    if (!lsStatus)
      break;
  }

  return lsStatus;
}

//remap or rebuild applicable members of the field in the new
// pDuel to their equivalents in the new pDuel
bool ddd::copyAndMapFieldPlayerInfo(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings) {
  bool status = true;
  duel* pDSource = (duel*) pDuelSource;
  duel* pDNew = (duel*) pDuelNew;

  //lambda declarations
  auto mapCardVector = [&](std::vector<card*>& cv, const std::string& memberStr) {
    bool cvStatus = status;
    for (auto &c: cv) {
      if (c == nullptr) //implementation difference from copyAndMapCards()
        continue;

      if (cardMappings.find(c) == cardMappings.end()) {
        clog("w", "Could not find mapping for card c(", c,
             ") for card vector in ", memberStr);
        cvStatus = false;
      } else {
        c = cardMappings.at(c);
      }
    }
    return cvStatus;
  };

  for (int i = 0; i < 2; ++i) {
    std::string pref = "pDuelNew->game_field.player["
      + std::to_string(i) + "].";

    //copy player_info members
    pDNew->game_field->player[i] = pDSource->game_field->player[i];

    //map player_info members
    player_info* pi = &(pDNew->game_field->player[i]);

    if (pi->list_mzone.size() > 0)
      status = mapCardVector(pi->list_mzone, pref + "list_mzone");
    if (pi->list_szone.size() > 0)
      status = mapCardVector(pi->list_szone, pref + "list_szone");
    if (pi->list_main.size() > 0)
      status = mapCardVector(pi->list_main, pref + "list_main");
    if (pi->list_grave.size() > 0)
      status = mapCardVector(pi->list_grave, pref + "list_grave");
    if (pi->list_hand.size() > 0)
      status = mapCardVector(pi->list_hand, pref + "list_hand");
    if (pi->list_remove.size() > 0)
      status = mapCardVector(pi->list_remove, pref + "list_remove");
    if (pi->list_extra.size() > 0)
      status = mapCardVector(pi->list_extra, pref + "list_extra");
    if (pi->tag_list_main.size() > 0)
      status = mapCardVector(pi->tag_list_main, pref + "tag_list_main");
    if (pi->tag_list_hand.size() > 0)
      status = mapCardVector(pi->tag_list_hand, pref + "tag_list_hand");
    if (pi->tag_list_extra.size() > 0)
      status = mapCardVector(pi->tag_list_extra, pref + "tag_list_extra");
  }

  return status;
}

//remap or rebuild applicable members of the new pDuel's field's
// effects member (pDuel->game_field->effects) to their equivalents
bool ddd::copyAndMapFieldFieldEffect(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings, const std::unordered_map<effect*, effect*>& effectMappings) {
  bool status = true;
  duel* pDSource = (duel*) pDuelSource;
  duel* pDNew = (duel*) pDuelNew;

  //lambda declarations
  auto mapEffectContainer = [&](std::multimap<uint32, effect*>& ec, const std::string& memberStr) { //copied from copyAndMapCards()
    bool ecStatus = status;
    std::multimap<uint32, effect*> newEc;

    for (const auto &e: ec) {
      if (effectMappings.find(e.second) == effectMappings.end()) {
        clog("w", "Could not find mapping for effect e(", e.second,
             ") for effect container in ", memberStr);
        ecStatus = false;
        continue;
      }
      newEc.insert(std::make_pair(e.first, effectMappings.at(e.second)));
    }
    if (ecStatus)
      ec = newEc;

    return ecStatus;
  };
  auto rebuildEffectIndexer = [&](field_effect::effect_indexer& ei, field_effect* nfe, const std::string& memberStr) {
    bool eiStatus = status;
    duel* pDSource = (duel*) pDuelSource;
    field_effect* sfe = &(pDSource->game_field->effects);

    //clear effect_indexer
    ei.clear();

    auto insertIntoEffectIndexer = [&](card::effect_container& sec, card::effect_container& nec, const std::string& memberStr) {
      if (sec.size() != nec.size()) {
        clog("w", "Discrepency in size for effect container (",
             sec.size(), " vs ", nec.size(), ") in ", memberStr);
        eiStatus = false;
      }

      for (auto i = nec.begin(); i != nec.end(); ++i) {
        ei.insert(std::make_pair(i->second, i));
        //maybe also check reverse effect mappings equivalence here...?
      }
    };

    insertIntoEffectIndexer(sfe->aura_effect, nfe->aura_effect,
                            memberStr + ".aura_effect");
    insertIntoEffectIndexer(sfe->ignition_effect, nfe->ignition_effect,
                            memberStr + ".ignition_effect");
    insertIntoEffectIndexer(sfe->activate_effect, nfe->activate_effect,
                            memberStr + ".activate_effect");
    insertIntoEffectIndexer(sfe->trigger_o_effect, nfe->trigger_o_effect,
                            memberStr + ".trigger_o_effect");
    insertIntoEffectIndexer(sfe->trigger_f_effect, nfe->trigger_f_effect,
                            memberStr + ".trigger_f_effect");
    insertIntoEffectIndexer(sfe->quick_o_effect, nfe->quick_o_effect,
                            memberStr + ".quick_o_effect");
    insertIntoEffectIndexer(sfe->quick_f_effect, nfe->quick_f_effect,
                            memberStr + ".quick_f_effect");
    insertIntoEffectIndexer(sfe->continuous_effect, nfe->continuous_effect,
                            memberStr + ".continuous_effect");

    if (ei.size() != sfe->indexer.size()) {
      clog("w", "Discrepency in indexer size (", ei.size(), " vs ",
           sfe->indexer.size(), ") in ", memberStr);
      eiStatus = false;
    }

    return eiStatus;
  };
  auto mapOathEffects = [&](std::unordered_map<effect*, effect*>& oe, const std::string& memberStr) {
    bool oeStatus = status;
    std::unordered_map<effect*, effect*> newOe;

    for (const auto &o: oe) {
      effect* oathKey = nullptr;
      effect* oathVal = nullptr;

      if (effectMappings.find(o.first) == effectMappings.end()) {
        clog("w", "Unable to find equivalent effect* for e(", o.first,
             ") as key in oath_effects in ", memberStr);
        oeStatus = false;
        continue;
      } else {
        oathKey = effectMappings.at(o.first);
      }
      if (o.second != nullptr) { //allow nullptr for val but not key
        if (effectMappings.find(o.second) == effectMappings.end()) {
          clog("w", "Unable to find equivalent effect* for e(",
               o.second, ") as value (key was e(", o.first,
               ")) in oath_effects in ", memberStr);
          oeStatus = false;
          continue;
        } else {
          oathVal = effectMappings.at(o.second);
        }
      }

      newOe.insert(std::make_pair(oathKey, oathVal));
    }

    if (oeStatus)
      oe = newOe;

    return oeStatus;
  };
  auto mapEffectCollection = [&](std::unordered_set<effect*>& ec, const std::string& memberStr) {
    bool ecStatus = status;
    std::unordered_set<effect*> newEc;

    for (const auto &e: ec) {
      if (effectMappings.find(e) == effectMappings.end()) {
        clog("w", "Unable to find equivalent effect* for e(", e,
             ") for effect_collection in ", memberStr);
        ecStatus = false;
        continue;
      }
      newEc.insert(effectMappings.at(e));
    }
    if (ecStatus)
      ec = newEc;

    return ecStatus;
  };
  auto mapCardList = [&](std::list<card*>& cl, const std::string& memberStr) {
    bool clStatus = status;
    std::list<card*> newCl;

    for (const auto &c: cl) {
      if (cardMappings.find(c) == cardMappings.end()) {
        clog("w", "Unable to find equivalent card* for c(", c,
             ") for card* list in ", memberStr);
        clStatus = false;
        continue;
      }
      newCl.push_back(cardMappings.at(c));
    }
    if (clStatus)
      cl = newCl;

    return clStatus;
  };
  auto mapCardUnorderedSet = [&](std::unordered_set<card*> cus, const std::string& memberStr) {
    bool cusStatus = status;
    std::unordered_set<card*> newCus;

    for (const auto &c: cus) {
      if (cardMappings.find(c) == cardMappings.end()) {
        clog("w", "Unable to find equivalent card* for c(", c,
             ") for card* (unordered) set in ", memberStr);
        cusStatus = false;
        continue;
      }
      newCus.insert(cardMappings.at(c));
    }
    if (cusStatus)
      cus = newCus;

    return cusStatus;
  };
  auto mapGrantEffectContainer = [&](std::unordered_map<effect*, field_effect::gain_effects>& gec, const std::string& memberStr) {
    bool gecStatus = status;
    std::unordered_map<effect*, field_effect::gain_effects> newGec;

    for (const auto &gep: gec) {
      if (effectMappings.find(gep.first) == effectMappings.end()) {
        clog("w", "Unable to find equivalent effect* for e(", gep.first,
             ") for grant_effect_collection key in ", memberStr);
        gecStatus = false;
        continue;
      }

      bool geStatus = gecStatus;
      field_effect::gain_effects newGe;
      for (const auto &ge: gep.second) {
        if (cardMappings.find(ge.first) == cardMappings.end()) {
          clog("w", "Unable to find equivalent card* for c(",
               ge.first, ") for grant_effect in"
               " grant_effect_container in ", memberStr);
          geStatus = false;
          continue;
        }
        if (effectMappings.find(ge.second) == effectMappings.end()) {
          clog("w", "Unable to find equivalent effect* for e(",
               ge.second, ") for grant_effect in"
               " grant_effect_container in ", memberStr);
          geStatus = false;
          continue;
        }
        newGe.insert(std::make_pair(cardMappings.at(ge.first),
                                    effectMappings.at(ge.second)));
      }

      if (geStatus)
        newGec.insert(std::make_pair(effectMappings.at(gep.first), newGe));
      else
        gecStatus = false;
    }
    if (gecStatus)
      gec = newGec;

    return gecStatus;
  };



  //copy members
  pDNew->game_field->effects = pDSource->game_field->effects;


  //map members
  field_effect* fe = &(pDNew->game_field->effects);
  std::string pref = "pDNew->game_field.effects.";

  //effect_container
  if (fe->aura_effect.size() > 0)
    status = mapEffectContainer(fe->aura_effect,
                                pref + "aura_effect");
  if (fe->ignition_effect.size() > 0)
    status = mapEffectContainer(fe->ignition_effect,
                                pref + "ignition_effect");
  if (fe->activate_effect.size() > 0)
    status = mapEffectContainer(fe->activate_effect,
                                pref + "activate_effect");
  if (fe->trigger_o_effect.size() > 0)
    status = mapEffectContainer(fe->trigger_o_effect,
                                pref + "trigger_o_effect");
  if (fe->trigger_f_effect.size() > 0)
    status = mapEffectContainer(fe->trigger_f_effect,
                                pref + "trigger_f_effect");
  if (fe->quick_o_effect.size() > 0)
    status = mapEffectContainer(fe->quick_o_effect,
                                pref + "quick_o_effect");
  if (fe->quick_f_effect.size() > 0)
    status = mapEffectContainer(fe->quick_f_effect,
                                pref + "quick_f_effect");
  if (fe->continuous_effect.size() > 0)
    status = mapEffectContainer(fe->continuous_effect,
                                pref + "continuous_effect");

  //effect_indexer
  if (status)
    status = rebuildEffectIndexer(fe->indexer, fe, pref + "indexer");

  //oath_effects
  if (fe->oath.size() > 0)
    status = mapOathEffects(fe->oath, pref + "oath");

  //effect_collection
  if (fe->pheff.size() > 0)
    status = mapEffectCollection(fe->pheff, pref + "pheff");
  if (fe->cheff.size() > 0)
    status = mapEffectCollection(fe->cheff, pref + "cheff");
  if (fe->rechargeable.size() > 0)
    status = mapEffectCollection(fe->rechargeable,
                                 pref + "rechargeable");
  if (fe->spsummon_count_eff.size() > 0)
    status = mapEffectCollection(fe->spsummon_count_eff,
                                 pref + "spsummon_count_eff");

  //std::list<card*>
  if (fe->disable_check_list.size() > 0)
    status = mapCardList(fe->disable_check_list,
                         pref + "disable_check_list");

  //std::unordered_set<card*>
  if (fe->disable_check_set.size() > 0)
    status = mapCardUnorderedSet(fe->disable_check_set,
                                 pref + "disable_check_set");

  //grant_effect_container
  if (fe->grant_effect.size() > 0)
    status = mapGrantEffectContainer(fe->grant_effect,
                                     pref + "grant_effect");

  return status;
}

//remap or rebuild applicable members of the processor in the new
// pDuel to their equivalents in the new pDuel
bool ddd::copyAndMapFieldProcessor(intptr_t pDuelSource, intptr_t pDuelNew, const std::unordered_map<card*, card*>& cardMappings, const std::unordered_map<group*, group*>& groupMappings, const std::unordered_map<effect*, effect*>& effectMappings) {
  bool status = true;
  duel* pDNew = (duel*) pDuelNew;
  duel* pDSource = (duel*) pDuelSource;
  std::string pref = "pDNew->game_field.core.";

  //lambda declarations
  auto mapCard = [&](card*& c, const std::string& memberStr) {
    if (cardMappings.find(c) == cardMappings.end()) {
      clog("w", "Could not find mapping for card c(", c,
           ") in ", memberStr);
      return false;
    }
    c = cardMappings.at(c);
    return status;
  };
  auto mapGroup = [&](group*& g, const std::string& memberStr) {
    if (groupMappings.find(g) == groupMappings.end()) {
      clog("w", "Could not find mapping for group g(", g,
           ") in ", memberStr);
      return false;
    }
    g = groupMappings.at(g);
    return status;
  };
  auto mapEffect = [&](effect*& e, const std::string& memberStr) {
    if (effectMappings.find(e) == effectMappings.end()) {
      clog("w", "Could not find mapping for effect e(", e,
           ") in ", memberStr);
      return false;
    }
    e = effectMappings.at(e);
    return status;
  };
  auto mapProcessorUnit = [&](processor_unit& pu, std::unordered_set<group*>& sptarget, const std::string& memberStr) {
    bool puStatus = status;

    if (pu.peffect) {
      if (!mapEffect(pu.peffect,
                     memberStr + " (in processor_unit.peffect)")) {
        puStatus = false;
      }
    }
    if (pu.ptarget) {
      //despite the member being of type group*, it seems under some
      //  circumstances this member may be of type card* instead
      //(e.g. when normal summoning and then being asked the place to
      // normal summon the card to)
      // ...or at the start of the duel(?), can also seem to be a
      //  card_set (probably field:: but might be processor::)
      // (not sure where if anywhere this variable is stored so
      //  can't check if this exists or what it's supposed
      //  to be without risking a segfault unless there's some way
      //  to determine if ptarget is indeed a card_set;
      //  also perhaps as bad is assuming it is a card_set and then
      //  allocating for it anyway but then having it never be freed)
      // ...could (pu.type == PROCESSOR_DRAW) be a thing...?

      if (puStatus) {
        bool foundPtargetMapping = false;

        //check if ptarget is a group
        if (groupMappings.find(pu.ptarget) != groupMappings.end()) {
          foundPtargetMapping = true;
          if (!mapGroup(pu.ptarget, memberStr +
                        " (in processor_unit.ptarget (as group*))")) {
            puStatus = false;
          }
        }
        //check if ptarget is a card
        card* ptargetAsCard = (card*) pu.ptarget;
        if (cardMappings.find(ptargetAsCard) != cardMappings.end()) {
          foundPtargetMapping = true;
          if (!mapCard(ptargetAsCard, memberStr +
                       " (in processor_unit.ptarget (as card*))")) {
            puStatus = false;
          } else {
            //set ptarget back as group if successfully mapped
            pu.ptarget = (group*) ptargetAsCard;
          }
        }
        //check if ptarget is a field::card_set
        if ((!foundPtargetMapping) && (puStatus) &&
            (pu.type == PROCESSOR_DRAW)) {
          foundPtargetMapping = true;

          field::card_set* ptargetAsCardSet = (field::card_set*) pu.ptarget;

          //scary shit that might segfault...
          if (sptarget.size() > 0) {
            pu.ptarget = *(sptarget.begin());
          } else {
            //potentially scary memory leaks.....!
            // clog("w", ""); //maybe warn about this?
            pu.ptarget = (group*) new field::card_set;
          }

          //save old contents of card_set before clearing
          field::card_set scs = field::card_set(*ptargetAsCardSet);
          ptargetAsCardSet->clear();

          for (const auto &c: scs) {
            card* nc = c;
            if (mapCard(nc, memberStr +
                        " (in processor_unit.ptarget (as card_set*))")) {
              //insert back mapped card
              ptargetAsCardSet->insert(nc);
            } else {
              puStatus = false;
            }
          }
        }
        //check if ptarget successfully mapped or not
        if ((puStatus) && (!foundPtargetMapping)) {
          clog("w", "Unable to determine ptarget type for"
               " processor_unit.ptarget in ", memberStr);
          puStatus = false;
        }
      }
    }
    return puStatus;
  };
  auto mapProcessorList = [&](std::list<processor_unit>& pl, std::unordered_set<group*>& sptarget, const std::string& memberStr) {
    bool plStatus = status;
    std::list<processor_unit> newPl;

    for (const auto &pu: pl) {
      newPl.push_back(pu);
      if (!mapProcessorUnit(newPl.back(), sptarget, memberStr))
        plStatus = false;
    }
    if (plStatus)
      pl = newPl;

    return plStatus;
  };
  auto mapCardVector = [&](std::vector<card*>& cv, const std::string& memberStr) {
    bool cvStatus = status;
    for (auto &c: cv) {
      if (c)
        if (!mapCard(c, memberStr + " (in card_vector)"))
          cvStatus = false;
    }
    return cvStatus;
  };
  auto mapEffectVector = [&](std::vector<effect*>& ev, const std::string& memberStr) {
    bool evStatus = status;
    for (auto &e: ev) {
      if (e)
        if (!mapEffect(e, memberStr + " (in effect_vector)"))
          evStatus = false;
    }
    return evStatus;
  };
  auto mapTevent = [&](tevent& te, const std::string& memberStr) {
    bool teStatus = status;

    if (te.trigger_card)
      if (!mapCard(te.trigger_card,
                   memberStr + " (in tevent.trigger_card)"))
        teStatus = false;

    if (te.event_cards)
      if (!mapGroup(te.event_cards,
                    memberStr + " (in tevent.event_cards)"))
        teStatus = false;

    if (te.reason_effect)
      if (!mapEffect(te.reason_effect,
                     memberStr + " (in tevent.reason_effect)"))
        teStatus = false;

    return teStatus;
  };
  auto mapEventList = [&](std::list<tevent>& el, const std::string& memberStr) {
    bool elStatus = status;
    std::list<tevent> newEl;

    for (const auto &te: el) {
      newEl.push_back(te);
      if (!mapTevent(newEl.back(), memberStr))
        elStatus = false;
    }
    if (elStatus)
      el = newEl;

    return elStatus;
  };
  auto mapAndRebuildEffectSet = [&](effect_set& es, const std::string& memberStr) {
    bool esStatus = status;
    effect_set newEs;

    for (int i = 0; i < es.size(); ++i) {
      effect* newEff = es.at(i);

      if (!mapEffect(newEff, memberStr + " (in effect_set)"))
        esStatus = false;
      else
        newEs.add_item(newEff);
    }

    if (esStatus) {
      newEs.sort(); //...necessary...?
      es = newEs;
    }

    return esStatus;
  };
  auto mapAndRebuildEffectSetV = [&](effect_set_v& esv, const std::string& memberStr) {
    bool esvStatus = status;
    effect_set_v newEsv;

    for (int i = 0; i < esv.size(); ++i) {
      effect* newEff = esv.at(i);

      if (!mapEffect(newEff, memberStr + " (in effect_set_v)"))
        esvStatus = false;
      else
        newEsv.add_item(newEff);
    }
    if (esvStatus) {
      newEsv.sort(); //...necessary...?
      esv = newEsv;
    }

    return esvStatus;
  };
  auto mapChain = [&](chain& chn, const std::string& memberStr) {
    bool chnStatus = status;

    if (chn.triggering_effect)
      if (!mapEffect(chn.triggering_effect,
                     memberStr + " (in chain.triggering_effect)"))
        chnStatus = false;

    if (chn.target_cards)
      if (!mapGroup(chn.target_cards,
                    memberStr + " (in chain.target_cards)"))
        chnStatus = false;

    if (chn.disable_reason)
      if (!mapEffect(chn.disable_reason,
                     memberStr + " (in chain.disable_reason)"))
        chnStatus = false;

    if (chn.triggering_state.reason_card)
      if (!mapCard(chn.triggering_state.reason_card, memberStr +
                   " (in chain.triggering_state.reason_card)"))
        chnStatus = false;

    if (chn.triggering_state.reason_effect)
      if (!mapEffect(chn.triggering_state.reason_effect, memberStr +
                     " (in chain.triggering_state.reason_effect)"))
        chnStatus = false;

    if (!mapTevent(chn.evt, memberStr + " (in chain.evt)"))
      chnStatus = false;

    //special handling for opmap
    for (auto &opt: chn.opinfos)
      if (opt.second.op_cards)
        if (!mapGroup(opt.second.op_cards, memberStr +
                      " (in chain.opinfos (optarget))"))
          chnStatus = false;

    if (!mapAndRebuildEffectSet(chn.required_handorset_effects,
                                memberStr +
                                " (in chain.required_handorset_effects)"))
      chnStatus = false;

    return chnStatus;
  };
  auto mapChainArray = [&](std::vector<chain>& ca, const std::string& memberStr) {
    bool caStatus = status;

    for (auto &chn: ca)
      if (!mapChain(chn, memberStr))
        caStatus = false;

    return caStatus;
  };
  auto mapChainList = [&](std::list<chain>& cl, const std::string& memberStr) {
    bool clStatus = status;

    for (auto &chn: cl)
      if (!mapChain(chn, memberStr))
        clStatus = false;

    return clStatus;
  };
  auto mapDelayedEffectCollection = [&](std::set<std::pair<effect*, tevent>>& dec, const std::string& memberStr) {
    bool decStatus = status;
    std::set<std::pair<effect*, tevent>> newDec;

    for (const auto &de: dec) {
      bool deStatus = decStatus;
      effect* newEff = de.first;
      tevent newTe = de.second;

      if (!mapEffect(newEff, memberStr +
                     " (in delayed_effect_collection.first)"))
        deStatus = false;

      if (!mapTevent(newTe, memberStr +
                     " (in delayed_effect_collection.second)"))
        deStatus = false;

      if (deStatus)
        newDec.insert(std::make_pair(newEff, newTe));
      else
        decStatus = false;
    }
    if (decStatus)
      dec = newDec;

    return decStatus;
  };
  auto mapInstantFList = [&](std::map<effect*, chain>& ifl, const std::string& memberStr) {
    bool iflStatus = status;
    std::map<effect*, chain> newIfl;

    for (const auto &instantf: ifl) {
      bool instantfStatus = iflStatus;
      effect* newEff = instantf.first;
      chain newChain = instantf.second;

      if (!mapEffect(newEff, memberStr + " (in instant_f_list)"))
        instantfStatus = false;

      if (!mapChain(newChain, memberStr + " (in instant_f_list"))
        instantfStatus = false;

      if (instantfStatus)
        newIfl.insert(std::make_pair(newEff, newChain));
      else
        iflStatus = false;
    }
    if (iflStatus)
      ifl = newIfl;

    return iflStatus;
  };
  auto mapCardSet = [&](processor::card_set& cs, const std::string& memberStr) {
    bool csStatus = status;
    processor::card_set newCs;

    for (const auto &c: cs) {
      card* newCard = c;

      if (!mapCard(newCard, memberStr + " (in card_set)"))
        csStatus = false;
      else
        newCs.insert(newCard);
    }
    if (csStatus)
      cs = newCs;

    return csStatus;
  };
  auto mapEffectSet = [&](std::set<effect*>& es, const std::string& memberStr) {
    bool esStatus = status;
    std::set<effect*> newEs;

    for (const auto &e: es) {
      effect* newEff = e;
      if (!mapEffect(newEff, memberStr + " (in set<effect*>)"))
        esStatus = false;
      else
        newEs.insert(newEff);
    }
    if (esStatus)
      es = newEs;

    return esStatus;
  };
  auto mapReadjustMap = [&](std::unordered_map<card*, uint32>& ram, const std::string& memberStr) {
    bool ramStatus = status;
    std::unordered_map<card*, uint32> newRam;

    for (const auto &ra: ram) {
      card* newCard = ra.first;

      if (!mapCard(newCard,
                   memberStr + " (in unordered_map<card*, uint32>)"))
        ramStatus = false;
      else
        newRam.insert(std::make_pair(newCard, ra.second));
    }
    if (ramStatus)
      ram = newRam;

    return ramStatus;
  };
  auto mapUniqueCards = [&](std::unordered_set<card*>& uc, const std::string& memberStr) {
    bool ucStatus = status;
    std::unordered_set<card*> newUc;

    for (const auto &c: uc) {
      card* newCard = c;

      if (!mapCard(newCard,
                   memberStr + " (in std::unordered_set<card*>)"))
        ucStatus = false;
      else
        newUc.insert(newCard);
    }
    if (ucStatus)
      uc = newUc;

    return ucStatus;
  };
  auto mapXyzMaterialList = [&](std::multimap<int32, card*, std::greater<int32>>& xml, const std::string& memberStr) {
    bool xmlStatus = status;
    std::multimap<int32, card*, std::greater<int32>> newXml;

    for (const auto &xm: xml) {
      card* newCard = xm.second;

      if (!mapCard(newCard, memberStr + " (in"
                   " std::multimap<int32, card*, std::greater<int32>>)"))
        xmlStatus = false;
      else
        newXml.insert(std::make_pair(xm.first, newCard));
    }
    if (xmlStatus)
      xml = newXml;

    return xmlStatus;
  };


  //copy processor_list members' ptargets to temporary variable
  // before copying processor members over
  // (this is because while the ptargets may need to be changed,
  //  the variable itself is a ptr and should not be changed which
  //  the default assignment operator would overwrite)
  std::unordered_set<group*> cardSetPtargetSet;
  for (const auto &pu: pDNew->game_field->core.units)
    if ((pu.type == PROCESSOR_DRAW) && (pu.ptarget))
      cardSetPtargetSet.insert(pu.ptarget);
  for (const auto &pu: pDNew->game_field->core.subunits)
    if ((pu.type == PROCESSOR_DRAW) && (pu.ptarget))
      cardSetPtargetSet.insert(pu.ptarget);
  for (const auto &pu: pDNew->game_field->core.recover_damage_reserve)
    if ((pu.type == PROCESSOR_DRAW) && (pu.ptarget))
      cardSetPtargetSet.insert(pu.ptarget);
  if ((pDNew->game_field->core.damage_step_reserved.type
       == PROCESSOR_DRAW) &&
      (pDNew->game_field->core.damage_step_reserved.ptarget))
    cardSetPtargetSet
      .insert(pDNew->game_field->core.damage_step_reserved.ptarget);
  if ((pDNew->game_field->core.summon_reserved.type
       == PROCESSOR_DRAW) &&
      (pDNew->game_field->core.summon_reserved.ptarget))
    cardSetPtargetSet
      .insert(pDNew->game_field->core.summon_reserved.ptarget);

  if (cardSetPtargetSet.size() > 1) {
    clog("e", "Found multiple (possible card_sets?) where 0 or 1 was"
         " expected.");
    for (const auto &pt: cardSetPtargetSet)
      clog("d", "  (", pt, ")");
    return false;
  }


  //copy processor members
  processor* p = &(pDNew->game_field->core);
  *p = pDSource->game_field->core;


  //map processor members

  //processor_list
  if (p->units.size() > 0)
    status = mapProcessorList(p->units, cardSetPtargetSet,
                              pref + "units");
  if (p->subunits.size() > 0)
    status = mapProcessorList(p->subunits, cardSetPtargetSet,
                              pref + "subunits");
  if (p->recover_damage_reserve.size() > 0)
    status = mapProcessorList(p->recover_damage_reserve, cardSetPtargetSet,
                              pref + "recover_damage_reserve");

  //processor_unit
  status = mapProcessorUnit(p->damage_step_reserved, cardSetPtargetSet,
                            pref + "damage_step_reserved");
  status = mapProcessorUnit(p->summon_reserved, cardSetPtargetSet,
                            pref + "summon_reserved");

  //card_vector
  if (p->select_cards.size() > 0)
    status = mapCardVector(p->select_cards, pref + "select_cards");
  if (p->unselect_cards.size() > 0)
    status = mapCardVector(p->unselect_cards, pref + "unselect_cards");
  if (p->summonable_cards.size() > 0)
    status = mapCardVector(p->summonable_cards,
                           pref + "summonable_cards");
  if (p->spsummonable_cards.size() > 0)
    status = mapCardVector(p->spsummonable_cards,
                           pref + "spsummonable_cards");
  if (p->repositionable_cards.size() > 0)
    status = mapCardVector(p->repositionable_cards,
                           pref + "repositionable_cards");
  if (p->msetable_cards.size() > 0)
    status = mapCardVector(p->msetable_cards, pref + "msetable_cards");
  if (p->ssetable_cards.size() > 0)
    status = mapCardVector(p->ssetable_cards, pref + "ssetable_cards");
  if (p->attackable_cards.size() > 0)
    status = mapCardVector(p->attackable_cards,
                           pref + "attackable_cards");
  if (p->must_select_cards.size() > 0)
    status = mapCardVector(p->must_select_cards,
                           pref + "must_select_cards");

  //effect_vector
  if (p->select_effects.size() > 0)
    status = mapEffectVector(p->select_effects,
                             pref + "select_effects");
  if (p->dec_count_reserve.size() > 0)
    status = mapEffectVector(p->dec_count_reserve,
                             pref + "dec_count_reserve");

  //option_vector (nothing to map?)

  //event_list
  if (p->point_event.size() > 0)
    status = mapEventList(p->point_event, pref + "point_event");
  if (p->instant_event.size() > 0)
    status = mapEventList(p->instant_event, pref + "instant_event");
  if (p->queue_event.size() > 0)
    status = mapEventList(p->queue_event, pref + "queue_event");
  if (p->delayed_activate_event.size() > 0)
    status = mapEventList(p->delayed_activate_event,
                          pref + "delayed_activate_event");
  if (p->full_event.size() > 0)
    status = mapEventList(p->full_event, pref + "full_event");
  if (p->used_event.size() > 0)
    status = mapEventList(p->used_event, pref + "used_event");
  if (p->single_event.size() > 0)
    status = mapEventList(p->single_event, pref + "single_event");
  if (p->solving_event.size() > 0)
    status = mapEventList(p->solving_event, pref + "solving_event");
  if (p->sub_solving_event.size() > 0)
    status = mapEventList(p->sub_solving_event,
                          pref + "sub_solving_event");

  //chain_array
  if (p->select_chains.size() > 0)
    status = mapChainArray(p->select_chains, pref + "select_chains");
  if (p->current_chain.size() > 0)
    status = mapChainArray(p->current_chain, pref + "current_chain");
  if (p->ignition_priority_chains.size() > 0)
    status = mapChainArray(p->ignition_priority_chains,
                           pref + "ignition_priority_chains");

  //chain_list
  if (p->continuous_chain.size() > 0)
    status = mapChainList(p->continuous_chain,
                          pref + "continuous_chain");
  if (p->solving_continuous.size() > 0)
    status = mapChainList(p->solving_continuous,
                          pref + "solving_continuous");
  if (p->sub_solving_continuous.size() > 0)
    status = mapChainList(p->sub_solving_continuous,
                          pref + "sub_solving_continuous");
  if (p->delayed_continuous_tp.size() > 0)
    status = mapChainList(p->delayed_continuous_tp,
                          pref + "delayed_continuous_tp");
  if (p->delayed_continuous_ntp.size() > 0)
    status = mapChainList(p->delayed_continuous_ntp,
                          pref + "delayed_continuous_ntp");
  if (p->desrep_chain.size() > 0)
    status = mapChainList(p->desrep_chain, pref + "desrep_chain");
  if (p->new_fchain.size() > 0)
    status = mapChainList(p->new_fchain, pref + "new_fchain");
  if (p->new_fchain_s.size() > 0)
    status = mapChainList(p->new_fchain_s, pref + "new_fchain_s");
  if (p->new_ochain.size() > 0)
    status = mapChainList(p->new_ochain, pref + "new_ochain");
  if (p->new_ochain_s.size() > 0)
    status = mapChainList(p->new_ochain_s, pref + "new_ochain_s");
  if (p->new_fchain_b.size() > 0)
    status = mapChainList(p->new_fchain_b, pref + "new_fchain_b");
  if (p->new_ochain_b.size() > 0)
    status = mapChainList(p->new_ochain_b, pref + "new_ochain_b");
  if (p->new_ochain_h.size() > 0)
    status = mapChainList(p->new_ochain_h, pref + "new_ochain_h");
  if (p->new_chains.size() > 0)
    status = mapChainList(p->new_chains, pref + "new_chains");

  //delayed_effect_collection
  if (p->delayed_quick_tmp.size() > 0)
    status = mapDelayedEffectCollection(p->delayed_quick_tmp,
                                        pref + "delayed_quick_tmp");
  if (p->delayed_quick.size() > 0)
    status = mapDelayedEffectCollection(p->delayed_quick,
                                        pref + "delayed_quick");

  //instant_f_list
  if (p->quick_f_chain.size() > 0)
    status = mapInstantFList(p->quick_f_chain, pref + "quick_f_chain");

  //card_set
  if (p->leave_confirmed.size() > 0)
    status = mapCardSet(p->leave_confirmed, pref + "leave_confirmed");
  if (p->special_summoning.size() > 0)
    status = mapCardSet(p->special_summoning,
                        pref + "special_summoning");
  if (p->unable_tofield_set.size() > 0)
    status = mapCardSet(p->unable_tofield_set,
                        pref + "unable_tofield_set");
  if (p->equiping_cards.size() > 0)
    status = mapCardSet(p->equiping_cards, pref + "equiping_cards");
  if (p->control_adjust_set[0].size() > 0)
    status = mapCardSet(p->control_adjust_set[0],
                        pref + "control_adjust_set[0]");
  if (p->control_adjust_set[1].size() > 0)
    status = mapCardSet(p->control_adjust_set[1],
                        pref + "control_adjust_set[1]");
  if (p->unique_destroy_set.size() > 0)
    status = mapCardSet(p->unique_destroy_set,
                        pref + "unique_destroy_set");
  if (p->self_destroy_set.size() > 0)
    status = mapCardSet(p->self_destroy_set, pref + "self_destroy_set");
  if (p->self_tograve_set.size() > 0)
    status = mapCardSet(p->self_tograve_set, pref + "self_tograve_set");
  if (p->trap_monster_adjust_set[0].size() > 0)
    status = mapCardSet(p->trap_monster_adjust_set[0],
                        pref + "trap_monster_adjust_set[0]");
  if (p->trap_monster_adjust_set[1].size() > 0)
    status = mapCardSet(p->trap_monster_adjust_set[1],
                        pref + "trap_monster_adjust_set[1]");
  if (p->release_cards.size() > 0)
    status = mapCardSet(p->release_cards, pref + "release_cards");
  if (p->release_cards_ex.size() > 0)
    status = mapCardSet(p->release_cards_ex, pref + "release_cards_ex");
  if (p->release_cards_ex_oneof.size() > 0)
    status = mapCardSet(p->release_cards_ex_oneof,
                        pref + "release_cards_ex_oneof");
  if (p->battle_destroy_rep.size() > 0)
    status = mapCardSet(p->battle_destroy_rep,
                        pref + "battle_destroy_rep");
  if (p->fusion_materials.size() > 0)
    status = mapCardSet(p->fusion_materials,
                        pref + "fusion_materials");
  if (p->synchro_materials.size() > 0)
    status = mapCardSet(p->synchro_materials,
                        pref + "synchro_materials");
  if (p->operated_set.size() > 0)
    status = mapCardSet(p->operated_set, pref + "operated_set");
  if (p->discarded_set.size() > 0)
    status = mapCardSet(p->discarded_set, pref + "discarded_set");
  if (p->destroy_canceled.size() > 0)
    status = mapCardSet(p->destroy_canceled, pref + "destroy_canceled");
  if (p->indestructable_count_set.size() > 0)
    status = mapCardSet(p->destroy_canceled,
                        pref + "indestructable_count_set");
  if (p->delayed_enable_set.size() > 0)
    status = mapCardSet(p->delayed_enable_set,
                        pref + "delayed_enable_set");
  if (p->set_group_pre_set.size() > 0)
    status = mapCardSet(p->set_group_pre_set,
                        pref + "set_group_pre_set");
  if (p->set_group_set.size() > 0)
    status = mapCardSet(p->set_group_set, pref + "set_group_set");

  //effect_set_v
  if (p->disfield_effects.size() > 0)
    status = mapAndRebuildEffectSetV(p->disfield_effects,
                                     pref + "disfield_effects");
  if (p->extra_mzone_effects.size() > 0)
    status = mapAndRebuildEffectSetV(p->extra_mzone_effects,
                                     pref + "extra_mzone_effects");
  if (p->extra_szone_effects.size() > 0)
    status = mapAndRebuildEffectSetV(p->extra_szone_effects,
                                     pref + "extra_szone_effects");

  //std::set<effect*>
  if (p->reseted_effects.size() > 0)
    status = mapEffectSet(p->reseted_effects, pref + "reseted_effects");

  //std::unordered_map<card*, uint32>
  if (p->readjust_map.size() > 0)
    status = mapReadjustMap(p->readjust_map, pref + "readjust_map");

  //std::unordered_set<card*>
  if (p->unique_cards[0].size() > 0)
    status = mapUniqueCards(p->unique_cards[0], pref + "unique_cards[0]");
  if (p->unique_cards[1].size() > 0)
    status = mapUniqueCards(p->unique_cards[1], pref + "unique_cards[1]");

  //std::unordered_map<uint32, uint32> (nothing to map?)

  //std::multimap<int32, card*, std::greater<int32>>
  if (p->xmaterial_lst.size() > 0)
    status = mapXyzMaterialList(p->xmaterial_lst, pref + "xmaterial_lst");

  //chain_limit_list (nothing to map?)

  //std::unordered_map<uint32, std::pair<uint32, uint32>> (nothing to map?)

  //card*
  if (p->summoning_card)
    status = mapCard(p->summoning_card, pref + "summoning_card");
  if (p->attacker)
    status = mapCard(p->attacker, pref + "attacker");
  if (p->attack_target)
    status = mapCard(p->attack_target, pref + "attack_target");
  if (p->limit_tuner)
    status = mapCard(p->limit_tuner, pref + "limit_tuner");
  if (p->limit_link_card)
    status = mapCard(p->limit_link_card, pref + "limit_link_card");
  if (p->chain_attack_target)
    status = mapCard(p->chain_attack_target, pref + "chain_attack_target");

  //group*
  if (p->limit_syn)
    status = mapGroup(p->limit_syn, pref + "limit_syn");
  if (p->limit_xyz)
    status = mapGroup(p->limit_xyz, pref + "limit_xyz");
  if (p->limit_link)
    status = mapGroup(p->limit_link, pref + "limit_link");

  //effect*
  if (p->reason_effect)
    status = mapEffect(p->reason_effect, pref + "reason_effect");

  return status;
}


//duplicate duel state object and its pDuel, then return the
// duel state id and a copy of the duel state
std::tuple<unsigned long long, DuelState> ddd::duplicateDuelState(const DuelState& ds) {

  //duplicate duel state object and its pDuel
  DuelState newDs = DuelState(ds);

  //actually duplicate the duel
  // (caller to this function needs to ensure passed DuelState
  //  contains an active duel state as no attempts to reactivate
  //  duel state here)
  newDs.pDuel = ddd::duplicateDuel(ds.pDuel);

  //generate new id
  unsigned long long newDsId = ddd::generateId();

  return std::make_tuple(newDsId, newDs);
}

//duplicate duel state object and its pDuel, using a provided
// shadow duel state and then return the duel state id and a copy
// of the duel state
//optionally accepts another pDuel as the last argument to reuse
// for the duplicateDuel function
std::tuple<unsigned long long, DuelState> ddd::duplicateDuelStateFromShadow(const intptr_t pShadowDuel, const DuelState& ds, const ShadowDuelStateResponses& sdsr) {
  return ddd::duplicateDuelStateFromShadow(pShadowDuel, ds, sdsr, 0);
}
std::tuple<unsigned long long, DuelState> ddd::duplicateDuelStateFromShadow(const intptr_t pShadowDuel, const DuelState& ds, const ShadowDuelStateResponses& sdsr, const intptr_t pDuelReuse) {
  DuelState newDs = DuelState(ds);

  //actually duplicate the duel (from shadow duel state)
  {
    newDs.pDuel = ddd::duplicateDuel(pShadowDuel, pDuelReuse);
  }

  //advance new duel state to current state
  if (!ddd::advanceDuelState(newDs.pDuel, sdsr)) {
    clog("e", "Unable to advance duplicate of duel state ",
         newDs.pDuel, " (", (duel*) newDs.pDuel, ") to match"
         " original duel state.");
    end_duel(newDs.pDuel);
    newDs.pDuel = 0;
    return std::make_tuple(0, newDs);
  }

  newDs.active = true;

  //generate new id
  unsigned long long newDsId = ddd::generateId();

  return std::make_tuple(newDsId, newDs);
}

//functionally equivalent to calling the duplicateDuelState function
// followed by the deactivateDuelState and then returning the newly
// duplicated duel state id and a copy of the duel state
// (but more efficient in that no new duel state is actually
//  created but merely reassigned (or "assumed"))
std::tuple<unsigned long long, DuelState> ddd::assumeDuelState(DuelState& ds) {

  //duplicate duel state object and its pDuel
  DuelState newDs = DuelState(ds);

  //remove association to pDuel in original duel state
  ds.pDuel = 0;
  ds.active = false;

  //generate new id
  unsigned long long newDsId = ddd::generateId();

  return std::make_tuple(newDsId, newDs);
}

//return whether a duel state is active
bool ddd::isDuelStateActive(const DuelState& ds) {
  return ((ds.active) && (ds.pDuel));
}

//deactivates a duel state
// (deactivated duel states destroy the pDuel containing the duel
//  state's state, saving memory but cannot be used unless
//  reactivated first)
bool ddd::deactivateDuelState(DuelState& ds) {
  bool status = true;

  //maybe can utilize a helper function to check duel_set?
  end_duel(ds.pDuel);

  ds.pDuel = 0;
  ds.active = false;

  return status;
}

//attempts to reactivates a duel state and returns whether
// successful or not
// (unlike dddapi's reactivate_duel_state function which wraps this
//  function, no checks are done here to determine if duel state
//  was already active and if was indeed already active, creates a new
//  duel anyway and overwrites old pDuel, essentially resulting
//  in a memory leak (though technically still exists in duel_set))
bool ddd::reactivateDuelState(const unsigned long long dsId, DuelState& ds, const intptr_t pShadowDuel, const ShadowDuelStateResponses& sdsr) {
  bool status = true;

  intptr_t pDuelNew = ddd::duplicateDuel(pShadowDuel);
  if (!pDuelNew) {
    clog("e", "Unable to reactivate duel state ", dsId,
         "; duplication of shadow duel state ", pShadowDuel, " (",
         (duel*) pShadowDuel, ") did not succeed.");
    return false;
  }

  ds.pDuel = pDuelNew;
  ds.active = true;

  if (!advanceDuelState(pDuelNew, sdsr)) {
    clog("e", "Unable to reactivate duel state ", dsId,
         "; advancement of duel state (restored from shadow duel state ",
         pShadowDuel, " (", (duel*) pShadowDuel, ") did not succeed.");

    end_duel(pDuelNew);
    ds.pDuel = 0;
    ds.active = false;
    return false;
  }

  return status;
}

//attempts to find a duel state from the singleton and if found,
// destroys its pDuel and removes all references in the singleton
// to the duel state
bool ddd::removeDuelState(const unsigned long long dsId) {
  bool status = true;

  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("e", "Unable to find duel state ", dsId, ".");
    if (getDDD_GS().dddapi.shadowDuelStates.find(dsId) !=
      getDDD_GS().dddapi.shadowDuelStates.end()) {
      clog("w", "  (note: ", dsId, " appears to be a shadow duel"
           " state)");
    }
    return false;
  }

  //delete the duel itself
  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  end_duel(pDuel);
  getDDD_GS().dddapi.duelStates.at(dsId).pDuel = 0;

  //remove reference to duel state from shadow
  unsigned long long sdsId =
    getDDD_GS().dddapi.duelStates.at(dsId).shadowDsId;
  if (getDDD_GS().dddapi.shadowDuelStates.find(sdsId) ==
      getDDD_GS().dddapi.shadowDuelStates.end()) {
    clog("e", "Unable to find shadow duel state ", sdsId,
         " (for duel state ", dsId, ").");
    return false;
  }

  auto& sds = getDDD_GS().dddapi.shadowDuelStates.at(sdsId);
  sds.responsesMap.erase(dsId);

  //also remove shadow state if no longer referenced by any duel states
  if (sds.responsesMap.size() < 1) {
    end_duel(sds.pDuel);
    getDDD_GS().dddapi.shadowDuelStates.erase(sdsId);
  }

  //remove the duel state itself from map
  getDDD_GS().dddapi.duelStates.erase(dsId);

  return status;
}

//sets the last response for the passed duel state as an int32
void ddd::setDuelStateResponseI(DuelState& ds, const int32 i) {
  ds.lastResponse.response.u.ivalue = i;
  ds.lastResponse.response.iactive = true;
  ds.lastResponse.responseSet = true;
}

//sets the last response for the passed duel state as a byte array
void ddd::setDuelStateResponseB(DuelState& ds, const byte* b) {
  std::memcpy(ds.lastResponse.response.u.bvaluebytes, b, 64);
  ds.lastResponse.response.iactive = false;
  ds.lastResponse.responseSet = true;
}

//generate and return a text-based representation (over multiple lines)
// of the common elements of the present game state
std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> ddd::getFieldVisualGamestate(unsigned long long dsId) {

  std::tuple<std::vector<std::string>, //field
             std::vector<std::string>, //specific player 0 field info
             std::vector<std::string>, //specific player 1 field info
             std::vector<std::string>, //player 0 info
             std::vector<std::string>, //player 1 info
             std::vector<std::string>> //chain information
    tpl;

  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    //clog("e", "");
    return tpl; //maybe just return empty tuple?
  }

  auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
  intptr_t pDuel = ds.pDuel;
  duel* pD = (duel*) pDuel;
  bool isActive = ds.active;

  if ((!isActive) && (!reactivate_duel_state(dsId))) {
    return tpl;
  }

  //set up and get field gamestate via buffer
  byte rawBuffer[1024];
  unsigned int bufferLen = query_field_info(pDuel, rawBuffer);
  Buffer b = Buffer(rawBuffer, bufferLen);
  b.inc8(2);

  //set up vars to store certain information
  int32 lifePoints[2]; //playerId
  int8 monsterZoneCards[2][7][2]; //playerId; sequence/zone; occupied/position+xyzMats
  int8 stZoneCards[2][8][1]; //playerId; sequence/zone; occupied/position
  int8 mainDeckSize[2]; //playerId
  int8 handSize[2]; //playerId
  int8 gySize[2]; //playerId
  int8 banishedSize[2]; //playerId
  int8 extraDeckSize[2]; //playerId
  int8 faceUpPendulumExtraDeckSize[2]; //playerId

  std::vector<std::string> handCards[2]; //playerId
  std::vector<std::string> mzoneCardDetails[2]; //playerId
  std::vector<std::string> stzoneCardDetails[2]; //playerId

  //fill out information on cards on field
  for (int playerId = 0; playerId <= 1; ++playerId) {
    lifePoints[playerId] = b.get32();

    //iterate mzones
    for (int i = 1; i <= 7; ++i) {
      bool isCardInZone = (bool) b.get8();
      if (!isCardInZone) {
        monsterZoneCards[playerId][i - 1][0] = 0;
        monsterZoneCards[playerId][i - 1][1] = 0;

      } else {
        int8 cardPosition = b.get8();
        int8 xyzMatsAttached = b.get8();
        monsterZoneCards[playerId][i - 1][0] = cardPosition;
        monsterZoneCards[playerId][i - 1][1] = xyzMatsAttached;

        //set up buffer for getting information on specific card
        // in current mzone
        byte cardBuffer[1024];
        unsigned int cardBufferLen =
          query_card(pDuel, playerId, LOCATION_MZONE, (i - 1),
                     QUERY_CODE | QUERY_TYPE | QUERY_LEVEL |
                     QUERY_RANK | QUERY_ATTRIBUTE | QUERY_RACE |
                     QUERY_ATTACK | QUERY_DEFENSE | QUERY_REASON |
                     QUERY_REASON_CARD | QUERY_LSCALE, cardBuffer, 0);
        Buffer mzoneBuffer = Buffer(cardBuffer, cardBufferLen);
        mzoneBuffer.inc32(2);

        //extract information from buffer
        uint32 code = mzoneBuffer.get32();
        uint32 type = mzoneBuffer.get32();
        uint32 level = mzoneBuffer.get32();
        uint32 rank = mzoneBuffer.get32();
        uint32 attribute = mzoneBuffer.get32();
        uint32 race = mzoneBuffer.get32();
        int32 atk = mzoneBuffer.get32();
        int32 def = mzoneBuffer.get32();
        //int32 extraval1 = mzoneBuffer.get32();
        //int32 extraval2 = mzoneBuffer.get32();
        //int32 extraval3 = mzoneBuffer.get32();
        mzoneBuffer.inc32(3);

        std::string s = ((i > 5) ? "[EM" : "[M") +
          std::to_string(i - 1);
        s += "] " + decodeCode(code) + " (" + std::to_string(code);
        s += ") [" + decodeType(type) + "] ";

        if ((level != 0) && (rank == 0))
          s += "Level:" + std::to_string(level);
        else if ((level == 0) && (rank != 0))
          s += "Rank:" + std::to_string(rank);

        s += " " + decodeAttribute(attribute) + " "
          + decodeRace(race) + " ";

        if (cardPosition & POS_ATTACK)
          s += "(ATK " + std::to_string(atk) +
            ")/DEF " + std::to_string(def);
        else if (cardPosition & POS_DEFENSE)
          s += "ATK " + std::to_string(atk) +
            "/(DEF " + std::to_string(def) + ")";
        else
          s += "[ATK " + std::to_string(atk) +
            "/DEF " + std::to_string(def) + "]?";

        if (cardPosition & POS_FACEDOWN)
          s += " (facedown)";

        mzoneCardDetails[playerId].push_back(s);
      }
    }

    //iterate szones
    for (int i = 1; i <= 8; ++i) {
      bool isCardInZone = (bool) b.get8();
      if (!isCardInZone) {
        stZoneCards[playerId][i - 1][0] = 0;
      } else {
        int8 cardPosition = b.get8();
        stZoneCards[playerId][i - 1][0] = cardPosition;

        //set up buffer for getting information on specific card
        // in current szone
        byte cardBuffer[1024];
        unsigned int cardBufferLen =
          query_card(pDuel, playerId, LOCATION_SZONE, (i - 1),
                     QUERY_CODE | QUERY_TYPE | QUERY_REASON |
                     QUERY_REASON_CARD | QUERY_LSCALE, cardBuffer, 0);
        Buffer stzoneBuffer = Buffer(cardBuffer, cardBufferLen);
        stzoneBuffer.inc32(2);

        uint32 code = stzoneBuffer.get32();
        uint32 type = stzoneBuffer.get32();
        //int32 extraval1 = stzoneBuffer.get32();
        //int32 extraval2 = stzoneBuffer.get32();
        stzoneBuffer.inc32(2);
        uint32 lscale = stzoneBuffer.get32();

        //std::string s = "[P" + std::to_string(playerId) + "-";
        std::string s = "[";
        if ((i - 1) != 5)
          s += "S/T" + std::to_string(i - 1) + "] ";
        else
          s += "F-ZONE] ";
        s += decodeCode(code) + " (" + std::to_string(code) + ") ";
        s += "[" + decodeType(type) + "]";

        if (cardPosition & POS_FACEDOWN)
          s += " (facedown)";

        stzoneCardDetails[playerId].push_back(s);
      }
    }

    //iterate other zones
    mainDeckSize[playerId] = b.get8();
    handSize[playerId] = b.get8();
    gySize[playerId] = b.get8();
    banishedSize[playerId] = b.get8();
    extraDeckSize[playerId] = b.get8();
    faceUpPendulumExtraDeckSize[playerId] = b.get8();

    //iterate hands
    if (handSize[playerId] > 0) {
      for (int i = 0; i < handSize[playerId]; ++i) {
        //set up buffer for getting information on specific card
        // in hand
        byte cardBuffer[1024];
        unsigned int cardBufferLen =
          query_card(pDuel, playerId, LOCATION_HAND,
                     i, QUERY_CODE, cardBuffer, 0);
        Buffer handBuffer = Buffer(cardBuffer, cardBufferLen);
        handBuffer.inc32(2);
        int32 pcb = handBuffer.get32();

        //get name of card in hand
        handCards[playerId].push_back(decodeCode(pcb));
      }
    }
  }

  //construct field lines
  std::vector<std::string> fieldLines = std::vector<std::string>(34);

  //build player 1's backrow
  fieldLines[0] += " ____  ";
  fieldLines[1] += "|    | ";
  fieldLines[2] += "|" +
    std::string(3 - std::to_string(mainDeckSize[1]).length(), ' ')
    + std::to_string(mainDeckSize[1]) + " | ";
  fieldLines[3] += "|DECK| ";
  fieldLines[4] += "|____| ";

  for (int i = 4; i >= 0; --i) {
    if (stZoneCards[1][i][0] == 0) {
      fieldLines[0] += "  _  _  ";
      fieldLines[1] += " |    | ";
      fieldLines[2] += "        ";
      fieldLines[3] += "        ";
      fieldLines[4] += " |_  _| ";
    } else {
      fieldLines[0] += "  ____  ";
      fieldLines[1] += " |    | ";
      fieldLines[2] += " |S/T | ";
      fieldLines[3] += " |  " + std::to_string(i) + " | ";
      fieldLines[4] += " |____| ";
    }
  }

  fieldLines[0] += "  ____ ";
  fieldLines[1] += " |    |";
  fieldLines[2] += " |"
    + std::string(3 - std::to_string(extraDeckSize[1]).length(), ' ')
    + std::to_string(extraDeckSize[1]) + " |";
  fieldLines[3] += " | ED |";
  fieldLines[4] += " |____|";


  //build player 1's front row
  fieldLines[7] += " ____  ";
  fieldLines[8] += "|    | ";
  fieldLines[9] += "|"
    + std::string(3 - std::to_string(gySize[1]).length(), ' ')
    + std::to_string(gySize[1]) + " | ";
  fieldLines[10] += "| GY | ";
  fieldLines[11] += "|____| ";
  fieldLines[12] += "       ";

  for (int i = 4; i >= 0; --i) {
    if (monsterZoneCards[1][i][0] == 0) {
      fieldLines[7] += "  _  _  ";
      fieldLines[8] += " |    | ";
      fieldLines[9] += "        ";
      fieldLines[10] += "        ";
      fieldLines[11] += " |_  _| ";
      fieldLines[12] += "        ";
    } else {
      int tempLen = 0;
      byte tempBytes[1024];
      tempLen = query_card(pDuel, 1, LOCATION_MZONE, (uint8) i,
                           QUERY_ATTACK | QUERY_DEFENSE |
                           QUERY_POSITION, tempBytes, false);
      Buffer tempB = Buffer(tempBytes, tempLen);
      tempB.inc32(2);
      tempB.inc8(3);
      int8 pos = tempB.get8();
      int32 atk = tempB.get32();
      int32 def = tempB.get32();

      if (monsterZoneCards[1][i][0] & POS_ATTACK) {
        fieldLines[7] += "  ____  ";
        fieldLines[8] += " |    | ";
        fieldLines[9] += " | M" + std::to_string(i) + " | ";
        fieldLines[10] += " |    | ";
        fieldLines[11] += " |____| ";
      } else {
        fieldLines[7] += "        ";
        fieldLines[8] += " ______ ";
        fieldLines[9] += "|      |";
        fieldLines[10] += "|  M" + std::to_string(i) + "  |";
        fieldLines[11] += "|______|";
      }

      if (pos & POS_FACEUP_ATTACK) {
        fieldLines[12]
          += std::string(6 - std::to_string(atk).length(), ' ')
          + std::to_string(atk) + "/ ";
      } else if (pos & POS_FACEUP_DEFENSE) {
        fieldLines[12] += " /" + std::to_string(def)
          + std::string(6 - std::to_string(def).length(), ' ');
      } else if (pos & POS_FACEDOWN_DEFENSE) {
        fieldLines[12] += "   /?   ";
      } else {
        fieldLines[12] += "        ";
      }
    }
  }

  if (stZoneCards[1][5][0] == 0) {
    fieldLines[7] += "  _  _  ";
    fieldLines[8] += " |    | ";
    fieldLines[9] += "        ";
    fieldLines[10] += "        ";
    fieldLines[11] += " |_  _| ";
  } else {
    fieldLines[7] += "  ____ ";
    fieldLines[8] += " |    |";
    fieldLines[9] += " | F- |";
    fieldLines[10] += " |ZONE|";
    fieldLines[11] += " |____|";
  }

  //build extra zones + banished
  fieldLines[14] += " ______        ";
  fieldLines[15] += "|      |       ";
  fieldLines[16] += "|BAN"
    + std::string(3 - std::to_string(banishedSize[1]).length(), ' ')
    + std::to_string(banishedSize[1]) + "|       ";
  fieldLines[17] += "|______|       ";
  fieldLines[18] += "               ";
  fieldLines[19] += "               ";

  int p0em5 = monsterZoneCards[0][5][0];
  int p0em6 = monsterZoneCards[0][6][0];
  int p1em5 = monsterZoneCards[1][5][0];
  int p1em6 = monsterZoneCards[1][6][0];

  int tempLen = 0;
  byte tempBytes[1024];
  Buffer tempB;

  if ((p0em5 != 0) && (p0em5 & POS_ATTACK)) {
    fieldLines[14] += "  ____  ";
    fieldLines[15] += " |    | ";
    fieldLines[16] += " | P0 | ";
    fieldLines[17] += " |EM5 | ";
    fieldLines[18] += " |____| ";

    tempLen = query_card(pDuel, 0, LOCATION_MZONE, 5,
                         QUERY_ATTACK, tempBytes, false);
    tempB = Buffer(tempBytes, tempLen);
    tempB.inc32(2);
    int32 atk = tempB.get32();
    fieldLines[19] +=
      std::string(6 - std::to_string(atk).length(), ' ')
      + std::to_string(atk) + "/ ";

  } else if ((p1em6 != 0) && (p1em6 & POS_ATTACK)) {
    fieldLines[14] += "  ____  ";
    fieldLines[15] += " |    | ";
    fieldLines[16] += " | P1 | ";
    fieldLines[17] += " |EM6 | ";
    fieldLines[18] += " |____| ";

    tempLen = query_card(pDuel, 1, LOCATION_MZONE, 6,
                         QUERY_DEFENSE, tempBytes, false);
    tempB = Buffer(tempBytes, tempLen);
    tempB.inc32(2);
    int32 atk = tempB.get32();
    fieldLines[19] +=
      std::string(6 - std::to_string(atk).length(), ' ')
      + std::to_string(atk) + "/ ";

  } else if ((p0em5 != 0) && (p0em5 & POS_DEFENSE)) {
    fieldLines[14] += "        ";
    fieldLines[15] += " ______ ";
    fieldLines[16] += "|      |";
    fieldLines[17] += "|P0 EM5|";
    fieldLines[18] += "|______|";

    tempLen = query_card(pDuel, 0, LOCATION_MZONE, 5,
                         QUERY_DEFENSE, tempBytes, false);
    tempB = Buffer(tempBytes, tempLen);
    tempB.inc32(2);
    int32 def = tempB.get32();
    fieldLines[19] += "/"
      + std::string(6 - std::to_string(def).length(), ' ')
      + std::to_string(def) + " ";

  } else if ((p1em6 != 0) && (p1em6 & POS_DEFENSE)) {
    fieldLines[14] += "        ";
    fieldLines[15] += " ______ ";
    fieldLines[16] += "|      |";
    fieldLines[17] += "|P1 EM6|";
    fieldLines[18] += "|______|";

    tempLen = query_card(pDuel, 1, LOCATION_MZONE, 6,
                         QUERY_ATTACK, tempBytes, false);
    tempB = Buffer(tempBytes, tempLen);
    tempB.inc32(2);
    int32 def = tempB.get32();
    fieldLines[19] += "/"
      + std::string(6 - std::to_string(def).length(), ' ')
      + std::to_string(def) + " ";

  } else {
    fieldLines[14] += "  _  _  ";
    fieldLines[15] += " |    | ";
    fieldLines[16] += "        ";
    fieldLines[17] += "        ";
    fieldLines[18] += " |_  _| ";
    fieldLines[19] += "        ";
  }

  fieldLines[14] += "        ";
  fieldLines[15] += "        ";
  fieldLines[16] += "        ";
  fieldLines[17] += "        ";
  fieldLines[18] += "        ";
  fieldLines[19] += "        ";

  if ((p0em6 != 0) && (p0em6 & POS_ATTACK)) {
    fieldLines[14] += "  ____  ";
    fieldLines[15] += " |    | ";
    fieldLines[16] += " | P0 | ";
    fieldLines[17] += " |EM6 | ";
    fieldLines[18] += " |____| ";

    tempLen = query_card(pDuel, 0, LOCATION_MZONE, 6,
                         QUERY_ATTACK, tempBytes, false);
    tempB = Buffer(tempBytes, tempLen);
    tempB.inc32(2);
    int32 atk = tempB.get32();
    fieldLines[19] +=
      std::string(6 - std::to_string(atk).length(), ' ')
      + std::to_string(atk) + "/ ";

  } else if ((p1em5 != 0) && (p1em5 & POS_ATTACK)) {
    fieldLines[14] += "  ____  ";
    fieldLines[15] += " |    | ";
    fieldLines[16] += " | P1 | ";
    fieldLines[17] += " |EM5 | ";
    fieldLines[18] += " |____| ";

    tempLen = query_card(pDuel, 1, LOCATION_MZONE, 5,
                         QUERY_ATTACK, tempBytes, false);
    tempB = Buffer(tempBytes, tempLen);
    tempB.inc32(2);
    int32 atk = tempB.get32();
    fieldLines[19]
      += std::string(6 - std::to_string(atk).length(), ' ')
      + std::to_string(atk) + "/ ";

  } else if ((p0em6 != 0) && (p0em6 & POS_DEFENSE)) {
    fieldLines[14] += "        ";
    fieldLines[15] += " ______ ";
    fieldLines[16] += "|      |";
    fieldLines[17] += "|P0 EM6|";
    fieldLines[18] += "|______|";

    tempLen = query_card(pDuel, 0, LOCATION_MZONE, 6, QUERY_DEFENSE,
                         tempBytes, false);
    tempB = Buffer(tempBytes, tempLen);
    tempB.inc32(2);
    int32 def = tempB.get32();
    fieldLines[19] += "/"
      + std::string(6 - std::to_string(def).length(), ' ')
      + std::to_string(def) + " ";

  } else if ((p1em5 != 0) && (p1em5 & POS_DEFENSE)) {
    fieldLines[14] += "        ";
    fieldLines[15] += " ______ ";
    fieldLines[16] += "|      |";
    fieldLines[17] += "|P1 EM5|";
    fieldLines[18] += "|______|";

    tempLen = query_card(pDuel, 1, LOCATION_MZONE, 5,
                         QUERY_DEFENSE, tempBytes, false);
    tempB = Buffer(tempBytes, tempLen);
    tempB.inc32(2);
    int32 def = tempB.get32();
    fieldLines[19] += "/"
      + std::string(6 - std::to_string(def).length(), ' ')
      + std::to_string(def) + " ";

  } else {
    fieldLines[14] += "  _  _  ";
    fieldLines[15] += " |    | ";
    fieldLines[16] += "        ";
    fieldLines[17] += "        ";
    fieldLines[18] += " |_  _| ";
  }

  fieldLines[14] += "               ";
  fieldLines[15] += "        ______ ";
  fieldLines[16] += "       |      |";
  fieldLines[17] += "       |BAN"
    + std::string(3 - std::to_string(banishedSize[0]).length(), ' ')
    + std::to_string(banishedSize[0]) + "|  ";
  fieldLines[18] += "       |______|";


  //build player 0's front row
  if (stZoneCards[0][5][0] == 0) {
    fieldLines[21] += " _  _  ";
    fieldLines[22] += "|    | ";
    fieldLines[23] += "       ";
    fieldLines[24] += "       ";
    fieldLines[25] += "|_  _| ";
    fieldLines[26] += "       ";
  } else {
    fieldLines[21] += " ____  ";
    fieldLines[22] += "|    | ";
    fieldLines[23] += "| F- | ";
    fieldLines[24] += "|ZONE| ";
    fieldLines[25] += "|____| ";
    fieldLines[26] += "       ";
  }

  for (int i = 0; i < 5; ++i) {
    if (monsterZoneCards[0][i][0] == 0) {
      fieldLines[21] += "  _  _  ";
      fieldLines[22] += " |    | ";
      fieldLines[23] += "        ";
      fieldLines[24] += "        ";
      fieldLines[25] += " |_  _| ";
      fieldLines[26] += "        ";
    } else {
      int tempLen = 0;
      byte tempBytes[1024];
      tempLen = query_card(pDuel, 0, LOCATION_MZONE, (uint8) i,
                           QUERY_ATTACK | QUERY_DEFENSE |
                           QUERY_POSITION, tempBytes, false);
      Buffer tempB = Buffer(tempBytes, tempLen);
      tempB.inc32(2); //...?
      tempB.inc8(3);
      int8 pos = tempB.get8();
      int32 atk = tempB.get32();
      int32 def = tempB.get32();

      if (monsterZoneCards[0][i][0] & POS_ATTACK) {
        fieldLines[21] += "  ____  ";
        fieldLines[22] += " |    | ";
        fieldLines[23] += " | M" + std::to_string(i) + " | ";
        fieldLines[24] += " |    | ";
        fieldLines[25] += " |____| ";
      } else {
        fieldLines[21] += "        ";
        fieldLines[22] += " ______ ";
        fieldLines[23] += "|      |";
        fieldLines[24] += "|  M" + std::to_string(i) + "  |";
        fieldLines[25] += "|______|";
      }

      if (pos & POS_FACEUP_ATTACK) {
        fieldLines[26] +=
          std::string(6 - std::to_string(atk).length(), ' ')
          + std::to_string(atk) + "/ ";
      } else if (pos & POS_FACEUP_DEFENSE) {
        fieldLines[26] += " /" + std::to_string(def)
          + std::string(6 - std::to_string(def).length(), ' ');
      } else if (pos & POS_FACEDOWN_DEFENSE) {
        fieldLines[26] += "   /?   ";
      } else {
        fieldLines[26] += "        ";
      }
    }
  }

  fieldLines[21] += "  ____ ";
  fieldLines[22] += " |    |";
  fieldLines[23] += " |"
    + std::string(3 - std::to_string(gySize[0]).length(), ' ')
    + std::to_string(gySize[0]) + " |";
  fieldLines[24] += " | GY |";
  fieldLines[25] += " |____|";


  //build player 0's back row
  fieldLines[28] += " ____  ";
  fieldLines[29] += "|    | ";
  fieldLines[30] += "|"
    + std::string(3 - std::to_string(extraDeckSize[0]).length(), ' ')
    + std::to_string(extraDeckSize[0]) + " | ";
  fieldLines[31] += "| ED | ";
  fieldLines[32] += "|____| ";


  for (int i = 0; i < 5; ++i) {
    if (stZoneCards[0][i][0] == 0) {
      fieldLines[28] += "  _  _  ";
      fieldLines[29] += " |    | ";
      fieldLines[30] += "        ";
      fieldLines[31] += "        ";
      fieldLines[32] += " |_  _| ";
    } else {
      fieldLines[28] += "  ____  ";
      fieldLines[29] += " |    | ";
      fieldLines[30] += " |S/T | ";
      fieldLines[31] += " |  " + std::to_string(i) + " | ";
      fieldLines[32] += " |____| ";
    }
  }

  fieldLines[28] += "  ____ ";
  fieldLines[29] += " |    |";
  fieldLines[30] += " |"
    + std::string(3 - std::to_string(mainDeckSize[0]).length(), ' ')
    + std::to_string(mainDeckSize[0]) + " | ";
  fieldLines[31] += " |DECK|";
  fieldLines[32] += " |____|";



  //construct lines for player information including cards in hand
  std::vector<std::string> playerLines[2];

  for (int playerId = 0; playerId <= 1; ++playerId) {
    std::string playerLine = "Player " + std::to_string(playerId)
      + " Lifepoints: " + std::to_string((int) lifePoints[playerId])
      + "  Cards in hand (" + std::to_string((int) handSize[playerId])
      + ")";

    if (handSize[playerId] > 0)
      playerLine += ":";

    playerLines[playerId].push_back(playerLine);
    playerLines[playerId]
      .push_back("[" + joinString(handCards[playerId], "], [") + "]");
  }

  //construct lines for chain information
  std::vector<std::string> chainLines;
  int8 clSize = b.get8();
  chainLines.push_back("Current chain link size: "
                       + std::to_string((int) clSize));
  for (int i = 0; i < clSize; ++i) {
    int32 code = b.get32();
    uint32 location = b.get32();
    int8 triggeringController = b.get8();
    uint8 triggeringLocation = b.get8();
    int8 triggeringSequence = b.get8();
    int32 description = b.get32();

    std::string chainLine = "[CL" + std::to_string(i + 1) + "] "
      + decodeCode(code) + " (" + std::to_string(code)
      + ") " + decodeLocation(location);

    std::string descriptionDecoded = decodeDesc(description);
    //if (descriptionDecoded != "") {
    if (!descriptionDecoded.empty()) {
      chainLine += " (" + descriptionDecoded + ")";
    }

    chainLine += " by player " + std::to_string(triggeringController);

    chainLines.push_back(chainLine);

  }
  //print most recent chain first (+1 offset for chain size hint)
  std::reverse(chainLines.begin() + 1, chainLines.end());


  //construct lines for specific field information
  std::vector<std::string> fieldInformationLines[2];
  for (int playerId = 0; playerId <= 1; ++playerId) {
    if ((stzoneCardDetails[playerId].size() > 0) ||
        (mzoneCardDetails[playerId].size() > 0)) {
      fieldInformationLines[playerId]
        .push_back("Player " + std::to_string(playerId)
                   + " field details:");

      for (int i = 0; i < mzoneCardDetails[playerId].size(); ++i)
        //if (mzoneCardDetails[playerId][i] != "")
        if (!mzoneCardDetails[playerId][i].empty())
          fieldInformationLines[playerId]
            .push_back(mzoneCardDetails[playerId][i]);

      for (int i = 0; i < stzoneCardDetails[playerId].size(); ++i)
        //if (stzoneCardDetails[playerId][i] != "")
        if (!stzoneCardDetails[playerId][i].empty())
          fieldInformationLines[playerId]
            .push_back(stzoneCardDetails[playerId][i]);

    }
  }

  if ((!isActive) && (!deactivate_duel_state(dsId))) {
    //clog("e", "");
    return tpl;
  }

  //build tuple
  std::get<0>(tpl) = fieldLines;
  std::get<1>(tpl) = fieldInformationLines[0];
  std::get<2>(tpl) = fieldInformationLines[1];
  std::get<3>(tpl) = playerLines[0];
  std::get<4>(tpl) = playerLines[1];
  std::get<5>(tpl) = chainLines;

  return tpl;
}
