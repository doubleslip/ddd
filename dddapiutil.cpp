/*
 * dddapiutil.cpp
 */

#include "dddapiutil.hpp"

//Short form overload for ddd::getChoicesFromDuelState() without
// passing a filter (equivalent to passing an empty filter)
std::tuple<std::vector<DDDReturn>, std::vector<int>, std::vector<std::string>, std::vector<std::string>, std::vector<int>> ddd::getChoicesFromDuelState(const DuelState& ds, const bool generateStrings) {
  return ddd::getChoicesFromDuelState(ds, generateStrings, {});
}
//Given a duel state id, return choices available to the current state
// as well as some extra data such as the message type and whether the
// choices match the filter
std::tuple<std::vector<DDDReturn>, std::vector<int>, std::vector<std::string>, std::vector<std::string>, std::vector<int>> ddd::getChoicesFromDuelState(const DuelState& ds, const bool generateStrings, const std::unordered_set<int>& filterSet) {
  std::tuple<
    std::vector<DDDReturn>,  //choices
    std::vector<int>,  //extras
    std::vector<std::string>,  //hintStrings
    std::vector<std::string>,  //choiceLabels
    std::vector<int>  //choiceFilterMatch
    > tpl;  //tuple to return

  //declare some more intuitive variable names for tuple members
  auto& choices = std::get<0>(tpl);
  auto& extras = std::get<1>(tpl);
  auto& hintStrings = std::get<2>(tpl);
  auto& choiceLabels = std::get<3>(tpl);
  auto& choiceFilterMatch = std::get<4>(tpl);

  //declare some choiceFilterMatch variables to classify
  // the type of match or action
  // (0x0 would be no match)
  enum CFM_TYPE {
    CFM_SELECT = 0x1,  //sort of the generic type
    CFM_NSUMMON = 0x2,
    CFM_SPSUMMON = 0x4,
    CFM_CHANGEPOS = 0x8,
    CFM_SETMZ = 0x10,
    CFM_SETST = 0x20,
    CFM_ACTIVATE = 0x40,  //'activate' but can also be 'use'
    CFM_ATTACK = 0x80,

    CFM_SHUFFLE_HAND = -0x1,
    CFM_ENTER_BP = -0x2,
    CFM_ENTER_MP2 = -0x4,
    CFM_ENTER_EP = -0x8
  };

  duel* pD = (duel*) ds.pDuel;
  const int len = pD->message_buffer.size();

  //check if buffer size is 0
  if (len == 0) {
    //no choices to get
    extras.push_back(0); //push back empty msg
    return tpl;
  }

  //create buffer
  Buffer b = Buffer(pD->message_buffer, MAX_OCGCORE_BUFFER_SIZE);
  b.resetItr(); //just in case; maybe not necessary

  //lambda declarations
  auto toChoiceI = [](const int32 i) {
    //returns a DDDReturn from an int32 value
    DDDReturn mr;
    mr.u.ivalue = i;
    mr.iactive = true;
    return mr;
  };
  auto toChoiceB = [](Buffer& buf) {
    //returns a DDDReturn from a Buffer object
    // (not really efficient but convenient; since function resets buffer,
    //  should only be called as last thing to do by caller)

    DDDReturn mr;
    buf.resetItr();
    for (int i = 0; i < 64; ++i) {
      if (buf.isItrAtEnd())
        mr.u.bvalue[i] = 0;
      else
        mr.u.bvalue[i] = buf.get8();
    }
    mr.iactive = false;
    return mr;
  };
  auto getFilterVal
    = [&pD, &filterSet](const int8 controller, const uint8 location,
                        const int8 sequence, const int defVal) {
    //checks if a choice involving a card where its controller,
    // location and sequence are known by checking if its card id
    // is in the filter
    //returns 0 if the card should be filtered, otherwise returns
    // defVal (1 if not otherwise specified)
    if (filterSet.size() < 1)
      return defVal;

    card* c
      = pD->game_field->get_field_card(controller, location, sequence);
    if (!c)
      return defVal;

    return (filterSet.find(c->cardid) == filterSet.end()) ? defVal : 0;
  };

  //get message and push it as first value in extras
  const uint8 msg = b.get8();
  extras.push_back(msg);

  //handle messages
  switch (msg) {
  case 1:  //MSG_RETRY
    {
      if (!generateStrings)
        return tpl;

      hintStrings.push_back("Invalid process result.");
      hintStrings.push_back("(interact again and double check your"
                            " input(s) before processing again)");
    }
    break;
  case 2:  //MSG_HINT
    {
      if (!generateStrings)
        return tpl;

      const int8 hint = b.get8();
      switch (hint) {
      case 1: //HINT_EVENT
        {
          const std::string hintStr = "Received hint HINT_EVENT.";
          hintStrings.push_back(hintStr);
        }
        break;
      case 2: //HINT_MESSAGE
        {
          const std::string hintStr = "Received hint HINT_MESSAGE.";
          hintStrings.push_back(hintStr);
        }
        break;
      case 3: //HINT_SELECTMSG
        {
          const std::string hintStr = "Received hint HINT_SELECTMSG.";
          hintStrings.push_back(hintStr);

          const int8 playerId = b.get8();
          const int32 target = b.get32();
          const std::string codeDecoded = decodeCode(target);

          if (!codeDecoded.empty()) {
            hintStrings.push_back("(selected " + codeDecoded +
                                  " for player " +
                                  std::to_string((int) playerId) + ")");
          } else {
            const std::string hintMsgDecoded = decodeMsgHint(target);

            if (!hintMsgDecoded.empty()) {
              hintStrings.push_back("(received hint " + hintMsgDecoded
                                    + " for player "
                                    + std::to_string((int) playerId)
                                    + ")");
            } else {
              hintStrings.push_back("(selected (" +
                                    std::to_string(target) +
                                    ") for player " +
                                    std::to_string((int) playerId) + ")");
            }
          }
        }
        break;
      case 4: //HINT_OPSELECTED
        {
          const std::string hintStr = "Received hint HINT_OPSELECTED.";
          hintStrings.push_back(hintStr);

          const int8 playerId = b.get8();
          const uint32 desc = b.get32();
          const std::string descDecoded = decodeDesc(desc);

          if (!descDecoded.empty()) {
            hintStrings.push_back("(player "
                                  + std::to_string((int) playerId)
                                  + " selected " + descDecoded + ")");
          } else {
            hintStrings.push_back("(player "
                                  + std::to_string((int) playerId)
                                  + " selected " + std::to_string(desc)
                                  + ")");
          }
        }
        break;
      case 5: //HINT_EFFECT
        {
          const std::string hintStr = "Received hint HINT_EFFECT.";
          hintStrings.push_back(hintStr);
        }
        break;
      case 6: //HINT_RACE
        {
          const std::string hintStr = "Received hint HINT_RACE.";
          hintStrings.push_back(hintStr);
        }
        break;
      case 7: //HINT_ATTRIB
        {
          const std::string hintStr = "Received hint HINT_ATTRIB.";
          hintStrings.push_back(hintStr);
        }
        break;
      case 8: //HINT_CODE
        {
          const std::string hintStr = "Received hint HINT_CODE.";
          hintStrings.push_back(hintStr);
        }
        break;
      case 9: //HINT_NUMBER
        {
          const std::string hintStr = "Received hint HINT_NUMBER.";
          hintStrings.push_back(hintStr);
        }
        break;
      case 10: //HINT_CARD
        {
          const std::string hintStr = "Received hint HINT_CARD.";
          hintStrings.push_back(hintStr);
        }
        break;
      case 11: //HINT_ZONE
        {
          const std::string hintStr = "Received hint HINT_ZONE.";
          hintStrings.push_back(hintStr);
        }
        break;
      default:
        {
          const std::string hintStr = "Received unknown hint '"
            + std::to_string((int) hint) + "'.";
          hintStrings.push_back(hintStr);
        }
      }
    }
    break;
  case 5:  //MSG_WIN
    {
      if (!generateStrings)
        return tpl;

      const int8 playerId = b.get8();
      const int8 reason = b.get8();
      hintStrings.push_back("Victory for player "
                            + std::to_string((int) playerId) + "!");
    }
    break;
  case 10: //MSG_SELECT_BATTLECMD
    {
      const int8 playerId = b.get8();
      if (generateStrings) {
        hintStrings.push_back("Idle now in battle phase for player "
                              + std::to_string((int) playerId) + ".");
        hintStrings.push_back("(open gamestate (in battle phase) for"
                              "player " +
                              std::to_string((int) playerId) + ")");
      }
      const int8 activatableCount = b.get8();
      for (int8 i = 0; i < activatableCount; ++i) {
        const uint32 code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        const int32 description = b.get32();

        choiceFilterMatch
          .push_back(getFilterVal(controller, location,
                                  sequence, CFM_ACTIVATE));

        choices.push_back(toChoiceI((i << 16) | 0));

        if (generateStrings) {
          const std::string label = "Activate " + decodeCode(code) + " ("
            + std::to_string(code) + ") " + decodeLocation(location)
            + "  (sequence = " + std::to_string(sequence);
          choiceLabels.push_back(label);
        }
      }

      const int8 attackableCount = b.get8();
      for (int8 i = 0; i < attackableCount; ++i) {
        const uint32 code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        const int8 canAttackDirectly = b.get8();

        choiceFilterMatch
          .push_back(getFilterVal(controller, location,
                                  sequence, CFM_ATTACK));

        choices.push_back(toChoiceI((i << 16) | 1));

        if (generateStrings) {
          const std::string label = "Attack with " + decodeCode(code)
            + " (" + std::to_string(code) + "; sequence = "
            + decodeLocation(location) + ")";
          choiceLabels.push_back(label);
        }
      }

      const int8 canGoMP2 = b.get8();
      const int8 canGoEP = b.get8();
      if (canGoMP2) {
        choices.push_back(toChoiceI(2));
        choiceFilterMatch
          .push_back((filterSet.find(-3) == filterSet.end())
                     ? CFM_ENTER_MP2 : 0);

        if (generateStrings)
          choiceLabels.push_back("Move to main phase 2");
      }
      if (canGoEP) {
        choices.push_back(toChoiceI(3));
        choiceFilterMatch
          .push_back((filterSet.find(-4) == filterSet.end())
                     ? CFM_ENTER_EP : 0);

        if (generateStrings)
          choiceLabels.push_back("Move to end phase");
      }
    }
    break;
  case 11: //MSG_SELECT_IDLECMD
    {
      const int8 playerId = b.get8();
      if (generateStrings) {
        hintStrings.push_back("Idle now for player "
                              + std::to_string((int) playerId) + ".");
        hintStrings.push_back("(open gamestate for player " +
                              std::to_string((int) playerId) + ").");
      }

      const std::string noChainCardActionsList[5] = {
        "Normal Summon", "Special Summon", "Change position",
        "Set (to monster zone)", "Set (to s/t zone)"
      };
      const CFM_TYPE cfmList[5] = {
        CFM_NSUMMON, CFM_SPSUMMON, CFM_CHANGEPOS,
        CFM_SETMZ, CFM_SETST
      };
      for (int8 i = 0; i < 5; ++i) {
        const std::string& action = noChainCardActionsList[i];
        const int8 ableCards = b.get8();

        for (int8 j = 0; j < ableCards; ++j) {
          const uint32 code = b.get32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();

          choiceFilterMatch
            .push_back(getFilterVal(controller, location,
                                    sequence, cfmList[i]));

          choices.push_back(toChoiceI((j << 16) | i));

          if (generateStrings) {
            std::string label = action + " " + decodeCode(code) + " ("
              + std::to_string(code) + ") " + decodeLocation(location);

            if ((noChainCardActionsList[i] == "Special Summon") &&
                (location == LOCATION_SZONE) &&
                (sequence == 0)) {
              label = "Pendulum summon";
            }
            choiceLabels.push_back(label);
          }
        }
      }

      const int8 activatableCards = b.get8();
      for (int8 i = 0; i < activatableCards; ++i) {
        const uint32 code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        const int32 description = b.get32();

        choiceFilterMatch
          .push_back(getFilterVal(controller, location,
                                  sequence, CFM_ACTIVATE));

        choices.push_back(toChoiceI((i << 16) | 5));

        if (generateStrings) {
          const std::string descriptionStr = decodeDesc(description);
          std::string label;


          if ((descriptionStr.find("EnablePendulumAttribute") !=
               std::string::npos) &&
              (getDDD_GS().cardDbs.cardDb[code].type &
               TYPE_PENDULUM) &&
              (location & LOCATION_HAND)) {
            //surely something less hacky than this exists...
            label = "Activate " + decodeCode(code) + " (" +
              std::to_string(code) + ") " + decodeLocation(location) +
              " (as pendulum scale)";

          } else if (descriptionStr.empty()) {
            //unfortunately some effects in hand might not
            //  have descriptions
            label = "Activate " + decodeCode(code) + " ("
              + std::to_string(code) + ") " + decodeLocation(location);

          } else {
            label = "Activate effect of " + decodeCode(code)
              + " (" + std::to_string(code) + ") "
              + decodeLocation(location) + " (" + descriptionStr + ")";

          }
          choiceLabels.push_back(label);
        }
      }

      const int8 canGoBP = b.get8();
      const int8 canGoEP = b.get8();
      const int8 canShuffleHand = b.get8();
      if (canGoBP) {
        choices.push_back(toChoiceI(6));
        choiceFilterMatch
          .push_back((filterSet.find(-2) == filterSet.end())
                     ? CFM_ENTER_BP : 0);

        if (generateStrings)
          choiceLabels.push_back("Move to battle phase");
      }
      if (canGoEP) {
        choices.push_back(toChoiceI(7));
        choiceFilterMatch
          .push_back((filterSet.find(-4) == filterSet.end())
                     ? CFM_ENTER_EP : 0);

        if (generateStrings)
          choiceLabels.push_back("Move to end phase");
      }
      if (canShuffleHand) {
        choices.push_back(toChoiceI(8));
        choiceFilterMatch
          .push_back((filterSet.find(-1) == filterSet.end())
                     ? CFM_SHUFFLE_HAND : 0);

        if (generateStrings)
          choiceLabels.push_back("Shuffle hand");
      }
    }
    break;
  case 12: //MSG_SELECT_EFFECTYN
    {
      const int8 playerId = b.get8();
      const uint32 code = b.get32();
      const int8 controller = b.get8();
      const uint8 location = b.get8();
      const int8 sequence = b.get8();
      b.inc8(); //position/overlay sequence
      const uint32 description = b.get32();

      //'yes' option; availability depends on filter
      // (despite mostly being used to activate effects, can
      //  also be used to 'use' effects)
      choices.push_back(toChoiceI(1));
      choiceFilterMatch
        .push_back(getFilterVal(controller, location,
                                sequence, CFM_ACTIVATE));

      //'no' option; always make available
      // (should this be CFM_SELECT or CFM_ACTIVATE...?)
      choices.push_back(toChoiceI(0));
      choiceFilterMatch.push_back(CFM_SELECT);

      if (generateStrings) {
        hintStrings.push_back("Asking if player "
                              + std::to_string((int) playerId)
                              + " wants to activate/use the effect of "
                              + decodeCode(code) + " ("
                              + std::to_string(code) + ") "
                              + decodeLocation(location) + ".");

        choiceLabels.push_back("Yes (activate/use the effect of "
                               + decodeCode(code) + ")");
        choiceLabels.push_back("No  (don't activate/use the effect of "
                               + decodeCode(code) + ")");
      }
    }
    break;
  case 13: //MSG_SELECT_YESNO
    {
      //'yes' option
      choices.push_back(toChoiceI(1));
      choiceFilterMatch.push_back(CFM_SELECT);

      //'no' option
      choices.push_back(toChoiceI(0));
      choiceFilterMatch.push_back(CFM_SELECT);

      if (generateStrings) {
        const uint8 playerId = b.get8();
        const uint32 description = b.get32();
        const std::string descriptionDecoded = decodeDesc(description);
        std::string hintStr = "Asking player "
          + std::to_string((int) playerId)
          + " to select either yes or no ";

        if (!descriptionDecoded.empty()) {
          hintStr += "for (" + descriptionDecoded + ").";
        } else {
          hintStr += "for (" + std::to_string(description) + ").";
        }
        hintStrings.push_back(hintStr);

        choiceLabels.push_back("Yes");
        choiceLabels.push_back("No");
      }
    }
    break;
  case 14: //MSG_SELECT_OPTION
    {
      const int8 playerId = b.get8();
      const int8 numOptions = b.get8();

      if (generateStrings) {
        hintStrings.push_back("Asking player "
                              + std::to_string((int) playerId)
                              + " to select an option ("
                              + std::to_string((int) numOptions)
                              + " available).");
      }

      for (int8 i = 0; i < numOptions; ++i) {
        const int32 option = b.get32();

        choices.push_back(toChoiceI(i));
        choiceFilterMatch.push_back(CFM_SELECT);

        if (generateStrings) {
          //not entirely sure what this is, check test.cpp as well
          const std::string descDecoded = decodeDesc(option);
          if (!descDecoded.empty())
            choiceLabels.push_back(descDecoded);
          else
            choiceLabels.push_back("? choice");
        }
      }
    }
    break;
  case 15: //MSG_SELECT_CARD
    {
      const int8 playerId = b.get8();
      const int8 cancelable = b.get8();
      const int8 minSelect = b.get8();
      const int8 maxSelect = b.get8();
      const int8 validSelectionsSize = b.get8();

      extras.push_back(minSelect);
      extras.push_back(maxSelect);
      extras.push_back(cancelable);

      if (generateStrings) {
        std::string hintStr = "Asking player "
          + std::to_string((int) playerId) + " to select (";
        if (minSelect != maxSelect) {
          hintStr += std::to_string((int) minSelect) + "-"
            + std::to_string((int) maxSelect) + ") cards";
        } else {
          hintStr += std::to_string((int) minSelect) + ") ";
          if (minSelect != 1)
            hintStr += "cards";
          else
            hintStr += "card";
        }
        if (cancelable) {
          hintStr += " (can be cancelled).";
        } else {
          hintStr += ".";
        }
        hintStrings.push_back(hintStr);
      }

      for (int8 i = 0; i < validSelectionsSize; ++i) {
        int code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        b.inc8(); //position/overlay sequence

        choiceFilterMatch
          .push_back(getFilterVal(controller, location,
                                  sequence, CFM_SELECT));

        Buffer vssb;
        vssb.w8(i);
        choices.push_back(toChoiceB(vssb));

        if (generateStrings) {
          const std::string locationDecoded = decodeLocation(location);
          std::string label = decodeCode(code);
          if (!locationDecoded.empty())
            label += " " + locationDecoded;
          choiceLabels.push_back(label);
        }
      }
    }
    break;
  case 16: //MSG_SELECT_CHAIN
    {
      const int8 playerId = b.get8();
      const int8 numChainable = b.get8(); //should be spe_count?
      const int8 speCount = b.get8();
      const int8 isForced = b.get8();
      const int32 hintTimingCurrP = b.get32();
      const int32 hintTimingOtherP = b.get32();

      if (generateStrings) {
        std::string hintStr = "Select chain for player "
          + std::to_string((int) playerId);
        if (hintTimingCurrP)
          hintStr += " (" + decodeTiming(hintTimingCurrP) + ")...";
        else if (hintTimingOtherP)
          hintStr += " (" + decodeTiming(hintTimingOtherP) + ")...";
        else
          hintStr += "...";

        hintStrings.push_back(hintStr);
      }

      if (numChainable > 0) {
        if (generateStrings) {
          std::string hintStr2 = "";
          if (isForced) {
            hintStr2 += "(Player " + std::to_string((int) playerId)
              + " required to activate/add to chain at least one"
              " mandatory effect)";
          } else {
            hintStr2 += "(Window for player "
              + std::to_string((int) playerId)
              + " to activate/add to chain or pass priority)";
          }
          hintStrings.push_back(hintStr2);
        }

        std::vector<std::string> activatableCardsVec;

        for (int8 i = 0; i < numChainable; ++i) {
          const int8 edesc = b.get8();
          const uint32 code = b.get32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc8(); //position/overlay sequence
          const uint32 description = b.get32();

          choiceFilterMatch
            .push_back(getFilterVal(controller, location,
                                    sequence, CFM_ACTIVATE));

          choices.push_back(toChoiceI(i));

          if (generateStrings) {
            std::string label = "Activate " + decodeCode(code) +
              " (" + std::to_string(code) + ") " + decodeLocation(location);
            std::string activatableCardsStr = "\"" + decodeCode(code) +
              "\" " + decodeLocation(location);

            std::string descriptionDecoded = decodeDesc(description);
            if (!descriptionDecoded.empty()) {
              label += " (" + descriptionDecoded + ")";
              activatableCardsStr += " (" + descriptionDecoded + ")";
            }

            choiceLabels.push_back(label);
            activatableCardsVec.push_back(activatableCardsStr);
          }
        }

        if (generateStrings) {
          std::string hintStr3;

          if (activatableCardsVec.size() > 2) {
            hintStr3 = "(" + std::to_string(activatableCardsVec.size())
              + " cards/effects available to activate";
          } else {
            hintStr3 = "(can activate: "
              + joinString(activatableCardsVec, ", ");
          }

          if (isForced) {
            hintStr3 += ")";
          } else {
            hintStr3 += " or pass priority)";
          }

          hintStrings.push_back(hintStr3);
        }
      } else {
        if (generateStrings) {
          hintStrings.push_back("(nothing available for player " +
                                std::to_string((int) playerId) +
                                " to activate at this time)");
        }
      }
    }
    break;
  case 18: //MSG_SELECT_PLACE
    {
      const int8 playerId = b.get8();
      const int8 count = b.get8();
      const uint32 flag = b.get32();

      if (generateStrings) {
        hintStrings.push_back("Asking player " +
                              std::to_string((int) playerId) +
                              " to select " + std::to_string((int) count) +
                              " place(s).");
      }

      const int numFlagBits = 8 * sizeof(flag);
      for (int i = 0; i < numFlagBits; ++i) {
        if (!(flag & (1 << i))) {
          const int c = (i >> 4) ^ playerId;
          const int l = (((i >> 3) % 2) + 1) << 2;
          const int s = i % 8;

          //surely don't need to check filter here...
          choiceFilterMatch.push_back(CFM_SELECT);

          Buffer fb;
          fb.w8(c);
          fb.w8(l);
          fb.w8(s);
          choices.push_back(toChoiceB(fb));

          if (generateStrings) {
            std::string label = "Select player " + std::to_string(c) +
              "'s " + decodeFieldLocation(l, s);
            choiceLabels.push_back(label);
          }
        }
      }
    }
    break;
  case 19: //MSG_SELECT_POSITION
    {
      const int8 playerId = b.get8();
      const uint32 code = b.get32();
      const int8 positions = b.get8();

      if (generateStrings) {
        hintStrings.push_back("Asking player " +
                              std::to_string((int) playerId) +
                              " to select a position for " +
                              std::to_string((int) code) +
                              " (" + decodeCode(code) + ").");
      }

      if (POS_FACEUP_ATTACK & positions) {
        choices.push_back(toChoiceI(POS_FACEUP_ATTACK));
        choiceFilterMatch.push_back(CFM_SELECT);

        if (generateStrings)
          choiceLabels.push_back("Faceup attack mode");
      }
      if (POS_FACEDOWN_ATTACK & positions) {
        choices.push_back(toChoiceI(POS_FACEDOWN_ATTACK));
        choiceFilterMatch.push_back(CFM_SELECT);

        if (generateStrings)
          choiceLabels.push_back("Facedown attack mode");
      }
      if (POS_FACEUP_DEFENSE & positions) {
        choices.push_back(toChoiceI(POS_FACEUP_DEFENSE));
        choiceFilterMatch.push_back(CFM_SELECT);

        if (generateStrings)
          choiceLabels.push_back("Faceup defense mode");
      }
      if (POS_FACEDOWN_DEFENSE & positions) {
        choices.push_back(toChoiceI(POS_FACEDOWN_DEFENSE));
        choiceFilterMatch.push_back(CFM_SELECT);

        if (generateStrings)
          choiceLabels.push_back("Facedown defense mode");
      }
    }
    break;
  case 23: //MSG_SELECT_SUM
    {
      const int8 hasLimit = b.get8();
      const int8 playerId = b.get8();
      const int8 acc = b.get32(); // & 0xffff...?
      const int8 min = b.get8();
      const int8 max = b.get8();
      const int8 mustSelectSize = b.get8(); //core.must_select_cards.size()

      extras.push_back(min);
      extras.push_back(max);
      extras.push_back(hasLimit);
      extras.push_back(acc); //not necessary...?
      extras.push_back(mustSelectSize);

      if (generateStrings) {
        std::string hintStr = "Asking player " + std::to_string(playerId)
          + " to select";
        if (!hasLimit) {
          hintStr += " (" + std::to_string((int) min);
          hintStr += "+) selections so that vals total ";
          hintStr += std::to_string(acc) + ".";

        } else if (min == max) {
          hintStr += " (" + std::to_string((int) min) + ") ";
          hintStr += (min == 1) ? "selection" : "selections";
          hintStr += " so that vals total " + std::to_string(acc) + ".";

        } else {
          hintStr += " (" + std::to_string((int) min) + " - ";
          hintStr += std::to_string((int) max) + ") selections";
          hintStr += " so that vals total " + std::to_string(acc) + ".";
        }

        hintStrings.push_back(hintStr);
      }

      std::vector<std::string> mustSelectVector;


      for (int8 i = 0; i < mustSelectSize; ++i) {
        const uint32 code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        const int32 sumParam = b.get32();

        choiceFilterMatch
          .push_back(getFilterVal(controller, location,
                                  sequence, CFM_SELECT));

        Buffer mssb;
        mssb.w8(i); //what actually goes here doesn't matter; only the offset matters...?
        choices.push_back(toChoiceB(mssb));

        if (generateStrings) {
          mustSelectVector.push_back("[val: " + std::to_string(sumParam)
                                     + "] " + decodeCode(code) + " "
                                     + decodeLocation(location));
        }
      }

      hintStrings.push_back("(Already selected: "
                            + joinString(mustSelectVector, ", ") + ")");


      const int8 selectSize = b.get8(); //as opposed to mustSelectSize
      for (int8 i = 0; i < selectSize; ++i) {
        const uint32 code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        const int32 sumParam = b.get32();

        choiceFilterMatch
          .push_back(getFilterVal(controller, location,
                                  sequence, CFM_SELECT));

        Buffer mssb;
        mssb.w8(i); //what actually goes here doesn't matter; only the offset matters...?
        choices.push_back(toChoiceB(mssb));

        if (generateStrings) {
          choiceLabels.push_back("[val: " + std::to_string(sumParam)
                                 + "] " + decodeCode(code) + " "
                                 + decodeLocation(location));
        }
      }
    }
    break;
  case 26: //MSG_SELECT_UNSELECT_CARD
    {
      const int8 playerId = b.get8();
      const int8 finishable = b.get8();
      const int8 cancelable = b.get8();
      const int8 minSelect = b.get8();
      const int8 maxSelect = b.get8();
      const int8 validSelectionsSize = b.get8();

      extras.push_back(minSelect);
      extras.push_back(maxSelect);
      extras.push_back(cancelable);
      extras.push_back(finishable);

      if (generateStrings) {
        bool pushOneAtATimeHint = false;
        std::string hintStr = "Asking player " +
          std::to_string((int) playerId)+ " to select/unselect (";

        if (minSelect != maxSelect) {
          hintStr += std::to_string((int) minSelect) + "-" +
            std::to_string((int) maxSelect) + ") cards.";
          pushOneAtATimeHint = true;
        } else {
          hintStr += std::to_string((int) minSelect) + ") ";
          if (minSelect == 1) {
            hintStr += "card";
          } else {
            hintStr += "cards";
            pushOneAtATimeHint = true;
          }
          hintStr += ".";
        }
        hintStrings.push_back(hintStr);

        if (pushOneAtATimeHint)
          hintStrings.push_back("(only enter one choice at a time)");
      }

      for (int8 i = 0; i < validSelectionsSize; ++i) {
        const uint32 code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        b.inc8(); //position/overlay sequence

        choiceFilterMatch
          .push_back(getFilterVal(controller, location,
                                  sequence, CFM_SELECT));

        Buffer vssb;
        vssb.w8(1);
        vssb.w8(i);
        choices.push_back(toChoiceB(vssb));

        if (generateStrings) {
          choiceLabels.push_back("Select " + decodeCode(code) + " " +
                                 decodeLocation(location));
        }
      }

      int8 validUnselectionsSize = b.get8();
      for (int8 i = 0; i < validUnselectionsSize; ++i) {
        const uint32 code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        b.inc8(); //position/overlay sequence

        /*
        //this is the correct logic
        choiceFilterMatch
          .push_back(getFilterVal(controller, location,
                                  sequence, CFM_SELECT));
        */

        //modified logic from above to always return the choice being
        // filtered to reduce infinite loops by constantly
        // selecting and unselecting when brute forcing states;
        //could also just introduce another flag and | it
        choiceFilterMatch.push_back(0);


        Buffer vusb;
        vusb.w8(1);
        vusb.w8(validUnselectionsSize + i);
        choices.push_back(toChoiceB(vusb));

        if (generateStrings) {
          choiceLabels.push_back("Unselect " + decodeCode(code) + " " +
                                 decodeLocation(location));
        }
      }
    }
    break;
  case 31: //MSG_CONFIRM_CARDS
    {
      if (!generateStrings)
        return tpl;

      const int8 otherPlayer = b.get8();
      const int8 numCardsToConfirm = b.get8();

      hintStrings
        .push_back("Allowing player " + std::to_string((int) otherPlayer)
                   + " to confirm "
                   + std::to_string((int) numCardsToConfirm) +
                   ((numCardsToConfirm > 1) ? " cards:" : " card:"));

      for (int8 i = 0; i < numCardsToConfirm; ++i) {
        const uint32 code = b.get32();
        const int8 controller = b.get8();
        const uint8 location = b.get8();
        const int8 sequence = b.get8();
        hintStrings.push_back("Player " + std::to_string(controller)
                              + "'s " + decodeCode(code) + " "
                              + decodeLocation(location));
      }
    }
    break;
  case 32: //MSG_SHUFFLE_DECK
    {
      if (!generateStrings)
        return tpl;

      const int8 playerId = b.get8();
      hintStrings.push_back("Shuffled player "
                            + std::to_string((int) playerId)
                            + "'s deck.");
    }
    break;
  case 33: //MSG_SHUFFLE_HAND
    {
      if (!generateStrings)
        return tpl;

      const int8 playerId = b.get8();
      hintStrings.push_back("Shuffled player "
                            + std::to_string((int) playerId)
                            + "'s hand.");
      //can also for loop to display new order of contents of hand
    }
    break;
  case 40: //MSG_NEW_TURN
    {
      if (!generateStrings)
        return tpl;

      const int8 playerId = b.get8();
      hintStrings.push_back("Start of turn for player "
                            + std::to_string((int) playerId) + ".");
    }
    break;
  case 41: //MSG_NEW_TURN
    {
      if (!generateStrings)
        return tpl;

      const int16 phaseId = b.get16();
      std::string hintStr = "Entering ";

      switch (phaseId) {
      case PHASE_DRAW:
        hintStrings.push_back(hintStr + "draw phase.");
        break;
      case PHASE_STANDBY:
        hintStrings.push_back(hintStr + "standby phase.");
        break;
      case PHASE_MAIN1:
        hintStrings.push_back(hintStr + "main phase 1.");
        break;
      case PHASE_BATTLE_START:
        hintStrings.push_back(hintStr + "start of battle phase.");
        break;
      case PHASE_BATTLE_STEP:
        hintStrings.push_back(hintStr + "battle step.");
        break;
      case PHASE_DAMAGE:
        hintStrings.push_back(hintStr + "damage step.");
        break;
      case PHASE_DAMAGE_CAL:
        hintStrings.push_back(hintStr + "damage calculation.");
        break;
      case PHASE_MAIN2:
        hintStrings.push_back(hintStr + "main phase 2.");
        break;
      case PHASE_END:
        hintStrings.push_back(hintStr + "end phase.");
        break;
      }
    }
    break;
  case 50: //MSG_MOVE
    {
      if (!generateStrings)
        return tpl;

      const uint32 code = b.get32();
      const uint32 fromLocation = b.get32();
      const uint32 toLocation = b.get32();
      b.inc32();
      hintStrings.push_back(decodeCode(code) + " moved from " +
                            decodeLocation(fromLocation, false) +
                            " to " + decodeLocation(toLocation, false)
                            + ".");
    }
    break;
  case 53: //MSG_POS_CHANGE
    {
      if (!generateStrings)
        return tpl;

      const uint32 code = b.get32();
      const uint32 location = b.get32();
      const int8 newPosition = b.get8();
      hintStrings.push_back(decodeCode(code) +
                            " " + decodeLocation(location) +
                            " changed its position!");
    }
    break;
  case 54: //MSG_SET
    {
      if (!generateStrings)
        return tpl;

      const uint32 code = b.get32();
      const uint32 location = b.get32();
      hintStrings.push_back(decodeCode(code) + " was set to " +
                            decodeLocation(location + false) + "!");
    }
    break;
  case 60: //MSG_SUMMONING
    {
      if (!generateStrings)
        return tpl;

      const uint32 code = b.get32();
      const uint32 location = b.get32();
      hintStrings.push_back("Attempting to normal summon "
                            + decodeCode(code) + " "
                            + decodeLocation(location) + ".");
    }
    break;
  case 61: //MSG_SUMMONED
    {
      if (!generateStrings)
        return tpl;

      hintStrings.push_back("Normal summon successful.");
    }
    break;
  case 62: //MSG_SPSUMMONING
    {
      if (!generateStrings)
        return tpl;

      const uint32 code = b.get32();
      const uint32 location = b.get32();
      hintStrings.push_back("Attempting to special summon "
                            + decodeCode(code) + " "
                            + decodeLocation(location) + ".");
    }
    break;
  case 63: //MSG_SPSUMMONED
    {
      if (!generateStrings)
        return tpl;

      hintStrings.push_back("Special summon successful.");
    }
    break;
  case 70: //MSG_CHAINING
    {
      if (!generateStrings)
        return tpl;

      const uint32 code = b.get32();
      const uint32 infoLocation = b.get32();
      const int8 trigController = b.get8();
      const uint8 trigLocation = b.get8();
      const int8 trigSeq = b.get8();
      const int32 description = b.get32();
      const int8 chainSize = b.get8();

      hintStrings.push_back("Activating " +
                            decodeCode(code) + " as chain link " +
                            std::to_string((int) chainSize) + ".");
    }
    break;
  case 71: //MSG_CHAINED
    {
      //basically after paying costs to activate card
      if (!generateStrings)
        return tpl;

      const int8 currentChainSize = b.get8();
      hintStrings.push_back("Added to chain.");
      hintStrings.push_back("(current chain size is now " +
                            std::to_string((int) currentChainSize) + ")");
    }
    break;
  case 72: //MSG_CHAIN_SOLVING
    {
      if (!generateStrings)
        return tpl;

      const int8 chainCount = b.get8();
      hintStrings.push_back("Resolving chain link " +
                            std::to_string((int) chainCount) + " effect.");
    }
    break;
  case 73: //MSG_CHAIN_SOLVED
    {
      if (!generateStrings)
        return tpl;

      const int8 chainCount = b.get8();
      hintStrings.push_back("Finished resolving effect at chain link " +
                            std::to_string((int) chainCount) + ".");
    }
    break;
  case 74: //MSG_CHAIN_END
    {
      if (!generateStrings)
        return tpl;

      hintStrings.push_back("Chain ended.");
    }
    break;
  case 75: //MSG_CHAIN_NEGATED
    {
      if (!generateStrings)
        return tpl;

      const int8 chainCount = b.get8();
      hintStrings.push_back("Chain link (activation?) "
                            + std::to_string((int) chainCount)
                            + " was negated.");
    }
    break;
  case 76: //MSG_CHAIN_DISABLED
    {
      if (!generateStrings)
        return tpl;

      const int8 chainCount = b.get8();
      hintStrings.push_back("Effect at chain link " +
                            std::to_string((int) chainCount) +
                            " was negated.");
    }
    break;
  case 83: //MSG_BECOME_TARGET
    {
      if (!generateStrings)
        return tpl;

      b.inc8();
      const uint32 location = b.get32();

      hintStrings.push_back("Card " +
                            decodeLocation(location) + " targeted.");
    }
    break;
  case 90: //MSG_DRAW
    {
      if (!generateStrings)
        return tpl;

      const int8 playerId = b.get8();
      const int8 cardsDrawn = b.get8();
      hintStrings.push_back("Player "+ std::to_string((int) playerId)
                            + " drew " + std::to_string((int) cardsDrawn)
                            + " card(s):");
      for (int8 i = 0; i < cardsDrawn; ++i) {
        uint32 code = b.get32();
        hintStrings.push_back("[" + std::to_string(code) +
                              "] " + decodeCode(code));
      }
    }
    break;
  case 91: //MSG_DAMAGE
    {
      if (!generateStrings)
        return tpl;

      const int8 playerId = b.get8();
      const int32 damage = b.get32();
      hintStrings.push_back("Player " + std::to_string((int) playerId)
                            + " took " + std::to_string((int) damage)
                            + " damage.");
    }
    break;
  case 92: //MSG_RECOVER
    {
      if (!generateStrings)
        return tpl;

      const int8 playerId = b.get8();
      const int32 recover = b.get32();
      hintStrings.push_back("Player " + std::to_string((int) playerId)
                            + " gained " + std::to_string((int) recover)
                            + " lifepoints.");
    }
    break;
  case 100: //MSG_PAY_LPCOST
    {
      if (!generateStrings)
        return tpl;

      const int8 playerId = b.get8();
      const int32 lpCost = b.get32();
      hintStrings.push_back("Player " + std::to_string((int) playerId)
                            + " paid " + std::to_string((int) lpCost)
                            + " lifepoints!");
    }
    break;
  case 110: //MSG_ATTACK
    {
      if (!generateStrings)
        return tpl;

      const int32 attacker = b.get32();
      const int32 attackTarget = b.get32();
      hintStrings.push_back("Attack declared!");
    }
    break;
  case 111: //MSG_BATTLE
    {
      if (!generateStrings)
        return tpl;

      const uint32 attackerLocationMaybe = b.get32();
      const int32 attackerAtkMaybe = b.get32();
      const int32 attackerDefMaybe = b.get32();
      const int8 attackerBattleDamageMaybe = b.get8();
      const uint32 attackTargetLocationMaybe = b.get32();
      const int32 defenderAtkMaybe = b.get32();
      const int32 defenderDefMaybe = b.get32();
      const int8 defenderBattleDamageMaybe = b.get8();

      hintStrings.push_back("Battling!");
    }
    break;
  case 113: //MSG_DAMAGE_STEP_START
    {
      if (!generateStrings)
        return tpl;

      const int8 p0hint = b.get8();
      const int8 p0hintEvent = b.get8();
      const int8 p0IdMaybe = b.get8(); //always 0
      const int32 p0_40 = b.get32(); //always 40

      const int8 p1hint = b.get8();
      const int8 p1hintEvent = b.get8();
      const int8 p1IdMaybe = b.get8(); //always 1
      const int32 p1_40 = b.get32(); //always 40

      hintStrings.push_back("Start of damage step!");
    }
    break;
  case 114: //MSG_DAMAGE_STEP_END
    {
      if (!generateStrings)
        return tpl;

      hintStrings.push_back("Reached end of damage step.");
    }
    break;
  case 160: //MSG_CARD_HINT
    {
      if (!generateStrings)
        return tpl;

      hintStrings.push_back("Received a card hint.");

      const uint32 location = b.get32();
      const uint8 cardHintDescription = b.get8();

      switch (cardHintDescription) {
      case 1:
        {
          hintStrings.push_back("CHINT_TURN");
        }
        break;
      case 2:
        {
          hintStrings.push_back("CHINT_CARD");
        }
        break;
      case 3:
        {
          hintStrings.push_back("CHINT_RACE");
        }
        break;
      case 4:
        {
          hintStrings.push_back("CHINT_ATTRIBUTE");
        }
        break;
      case 5:
        {
          hintStrings.push_back("CHINT_NUMBER");
        }
        break;
      case 6:
        {
          hintStrings.push_back("CHINT_DESC_ADD");
          uint32 description = b.get32();
          std::string descDecoded = decodeDesc(description);
          if (!descDecoded.empty())
            descDecoded = " (" + descDecoded + ")";

          hintStrings.push_back("description = "
                                + std::to_string(description)
                                + descDecoded);
          //still a mysterious 2 bytes left at end
        }
        break;
      case 7:
        {
          hintStrings.push_back("CHINT_DESC_REMOVE");
          uint32 description = b.get32();
          std::string descDecoded = decodeDesc(description);
          if (!descDecoded.empty())
            descDecoded = " (" + descDecoded + ")";

          hintStrings.push_back("description = "
                                + std::to_string(description)
                                + descDecoded);
          //very many mysterious bytes left at end
        }
        break;
      default:
        {
          clog("w", "Unknown chint value '",
               (int) cardHintDescription, "'.");
        }
      }
    }
    break;
  default:
    {
      //got an unknown message or did not implement case for
      // message yet
      if (!generateStrings)
        return tpl;

      hintStrings.push_back("Not sure how to handle msg '" +
                            std::to_string(msg) + "'.");
    }
  }

  return tpl;
}

//Accept a temporary filter object relative to the currently available
// choices in the duel state and returns a filter object passable to
// ddd::getChoicesFromDuelState() to filter choices, even in future
// calls to the function after processing
std::unordered_set<int> ddd::getChoiceFilter(const DuelState& ds, const std::unordered_set<int>& choicesFilter) {

  //the filter set to return
  //typically would be the card id ((card*)->cardid) to filter out
  // actions involving that card
  //for common actions (in an open gamestate), the following values
  //  are also defined for the choice filter as:
  // -1: shuffle hand
  // -2: move to battle phase
  // -3: move to main phase 2
  // -4: move to end phase
  std::unordered_set<int> filterSet;
  bool wasDsActivated = true;

  duel* pD = (duel*) ds.pDuel;
  const int len = pD->message_buffer.size();

  //check buffer 0 length (no buffer, no choices, no filter)
  if (len == 0) {
    return filterSet;
  }

  Buffer b = Buffer(pD->message_buffer);
  b.resetItr(); //just in case; maybe not necessary

  if (len == 0)
    return filterSet;

  //var to keep track of current choice (needed to match that in
  // the passed choice filter object)
  int currChoice = 0;

  const uint8 msg = b.get8();

  //unlike the switch statement in ddd::getChoicesFromDuelState(),
  // only need to deal with cases here that generated choices
  switch (msg) {
  case 10: //MSG_SELECT_BATTLECMD
    {
      const int8 activatableCount = b.get8();
      for (int8 i = 0; i < activatableCount; ++i) {
        if (choicesFilter.find(currChoice) == choicesFilter.end()) {
          b.inc8(11);
        } else {
          b.inc32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc32();

          card* c = pD->game_field->get_field_card(controller,
                                                   location, sequence);
          filterSet.insert(c->cardid);
        }
        ++currChoice;
      }
      const int8 attackableCount = b.get8();
      for (int8 i = 0; i < attackableCount; ++i) {
        if (choicesFilter.find(currChoice) == choicesFilter.end()) {
          b.inc8(8);
        } else {
          b.inc32(1);
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc8();

          card* c = pD->game_field->get_field_card(controller,
                                                   location, sequence);
          filterSet.insert(c->cardid);
        }
        ++currChoice;
      }
      const bool canGoMP2 = (b.get8() == 1);
      const bool canGoEP = (b.get8() == 1);
      if (canGoMP2) {
        if ((choicesFilter.find(currChoice) != choicesFilter.end()) ||
            (choicesFilter.find(-3) != choicesFilter.end()))
          filterSet.insert(-3);
        ++currChoice;
      }
      if (canGoEP) {
        if ((choicesFilter.find(currChoice) != choicesFilter.end()) ||
            (choicesFilter.find(-4) != choicesFilter.end()))
          filterSet.insert(-4);
        ++currChoice;
      }
    }
    break;
  case 11: //MSG_SELECT_IDLECMD
    {
      b.inc8();
      for (int8 i = 0; i < 5; ++i) {
        const int8 ableCards = b.get8();
        for (int8 j = 0; j < ableCards; ++j) {
          if (choicesFilter.find(currChoice) == choicesFilter.end()) {
            b.inc8(7);
          } else {
            b.inc32();
            const int8 controller = b.get8();
            const uint8 location = b.get8();
            const int8 sequence = b.get8();

            card* c
              = pD->game_field->get_field_card(controller,
                                               location, sequence);
            filterSet.insert(c->cardid);
          }
          ++currChoice;
        }
      }
      const int8 activatableCards = b.get8();
      for (int8 i = 0; i < activatableCards; ++i) {
        if (choicesFilter.find(currChoice) == choicesFilter.end()) {
          b.inc8(11);
        } else {
          b.inc32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc32();

          card* c = pD->game_field->get_field_card(controller,
                                                   location, sequence);
          filterSet.insert(c->cardid);
        }
        ++currChoice;
      }
      const int8 canGoBP = b.get8();
      const int8 canGoEP = b.get8();
      const int canShuffleHand = b.get8();
      if (canGoBP) {
        if ((choicesFilter.find(currChoice) != choicesFilter.end()) ||
            (choicesFilter.find(-2) != choicesFilter.end()))
          filterSet.insert(-2);
        ++currChoice;
      }
      if (canGoEP) {
        if ((choicesFilter.find(currChoice) != choicesFilter.end()) ||
            (choicesFilter.find(-4) != choicesFilter.end()))
          filterSet.insert(-4);
        ++currChoice;
      }
      if (canShuffleHand) {
        if ((choicesFilter.find(currChoice) != choicesFilter.end()) ||
            (choicesFilter.find(-1) != choicesFilter.end()))
          filterSet.insert(-1);
        ++currChoice;
      }
    }
    break;
  case 15: //MSG_SELECT_CARD
    {
      b.inc8(4);
      const int8 validSelectionsSize = b.get8();

      for (int8 i = 0; i < validSelectionsSize; ++i) {
        if (choicesFilter.find(currChoice) == choicesFilter.end()) {
          b.inc32(2);
        } else {
          b.inc32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc8();

          card* c = pD->game_field->get_field_card(controller,
                                                   location, sequence);
          filterSet.insert(c->cardid);
        }
        ++currChoice;
      }
    }
    break;
  case 16: //MSG_SELECT_CHAIN
    {
      b.inc8(1);
      const int8 numChainable = b.get8();
      b.inc8(10);
      if (numChainable > 0) {
        for (int8 i = 0; i < numChainable; ++i) {
          if (choicesFilter.find(currChoice) == choicesFilter.end()) {
            b.inc8(13);
          } else {
            b.inc8(5);
            const int8 controller = b.get8();
            const uint8 location = b.get8();
            const int8 sequence = b.get8();
            b.inc8(5);

            card* c = pD->game_field->get_field_card(controller,
                                                     location, sequence);
            filterSet.insert(c->cardid);
          }
          ++currChoice;
        }
      }
    }
    break;
  case 23: //MSG_SELECT_SUM
    {
      b.inc8(8);
      const int8 mustSelectSize = b.get8(); //core.must_select_cards.size()
      for (int8 i = 0; i < mustSelectSize; ++i) {
        if (choicesFilter.find(currChoice) == choicesFilter.end()) {
          b.inc8(11);
        } else {
          b.inc32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc32();

          card* c = pD->game_field->get_field_card(controller,
                                                   location, sequence);
          filterSet.insert(c->cardid);
        }
        ++currChoice;
      }

      const int8 selectSize = b.get8(); //as opposed to mustSelectSize
      for (int8 i = 0; i < selectSize; ++i) {
        if (choicesFilter.find(currChoice) == choicesFilter.end()) {
          b.inc8(11);
        } else {
          b.inc32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc32();

          card* c = pD->game_field->get_field_card(controller,
                                                   location, sequence);
          filterSet.insert(c->cardid);
        }
        ++currChoice;
      }
    }
    break;
  case 26: //MSG_SELECT_UNSELECT_CARD
    {
      b.inc8(5);
      const int8 validSelectionsSize = b.get8();
      for (int8 i = 0; i < validSelectionsSize; ++i) {
        if (choicesFilter.find(currChoice) == choicesFilter.end()) {
          b.inc8(8);
        } else {
          b.inc32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc8();

          card* c = pD->game_field->get_field_card(controller,
                                                   location, sequence);
          filterSet.insert(c->cardid);
        }
        ++currChoice;
      }
      const int8 validUnselectionsSize = b.get8();
      for (int8 i = 0; i < validUnselectionsSize; ++i) {
        if (choicesFilter.find(currChoice) == choicesFilter.end()) {
          b.inc8(8);
        } else {
          b.inc32();
          const int8 controller = b.get8();
          const uint8 location = b.get8();
          const int8 sequence = b.get8();
          b.inc8();

          card* c = pD->game_field->get_field_card(controller,
                                                   location, sequence);
          filterSet.insert(c->cardid);
        }
        ++currChoice;
      }
    }
    break;
  default:
    {
      clog("w", "Msg ", msg, " not supported for getting choice filters.");
    }
    break;
  }

  return filterSet;
}

//returns whether the last process succeeded for a duel pointer
// (does this by checking the first byte in the duel buffer if
//  length is at least 1, which means this function may return
//  true even though it shouldn't if the duel buffer was
//  previously cleared)
bool ddd::lastProcessSucceeded(const intptr_t pDuel) {
  duel* pD = (duel*) pDuel;
  if (pD->message_buffer.size() > 0)
    if (pD->message_buffer[0] == 1)
      return false;
  return true;
}

//attempt to perform a process iteration on a duel state's pDuel
// and if successful, update other members of the duel state and
// the shadow duel state
bool ddd::processDuelState(DuelState& ds, ShadowDuelStateResponses& sdsr) {
  int32 result = -1;
  duel* pD = (duel*) ds.pDuel;

  //prepare to actually process the duel
  pD->clear_buffer();

  if (ds.lastResponse.responseSet) {
    if (ds.lastResponse.response.iactive) {
      set_responsei(ds.pDuel, ds.lastResponse.response.u.ivalue);
    } else {
      set_responseb(ds.pDuel, ds.lastResponse.response.u.bvaluebytes);
    }
  }

  //actually process
  result = process(ds.pDuel);

  //get msg
  int8 msg = 0;
  if (pD->message_buffer.size() > 0)
    msg = pD->message_buffer[0];

  if (msg == 1) //MSG_RETRY
    return result;

  //push response to its (existing) shadow state
  sdsr.responses.push_back(ds.lastResponse);

  if (sdsr.responses.size() != sdsr.maxExpectedResponses) {
    //only a warning but if code ended up here, something
    // really messed up probably happened
    clog("w", "Expected shadow duel state ", ds.shadowDsId, " to be"
         " at its maximum response size ", sdsr.maxExpectedResponses,
         " (was ", sdsr.responses.size(), " instead).");
  }

  sdsr.maxExpectedResponses++;

  //reset last response in duel state after process succeeded
  ds.lastResponse = DuelStateResponse();

  return result;
}

//advances a duel state using responses saved in its shadow duel state
// (somewhat more for internal use as this handles duel pointers
//  rather than duel state ids)
bool ddd::advanceDuelState(const intptr_t pDuel, const ShadowDuelStateResponses& sdsr) {
  bool status = true;
  duel* pD = (duel*) pDuel;

  int processIterations = 0;
  int emptyProcessIterations = 0;

  if ((sdsr.responses.size() != (sdsr.maxExpectedResponses - 1)) &&
      (sdsr.responses.size() != sdsr.maxExpectedResponses)) {
    clog("w", "Major discrepency in shadow duel state response sizes"
         " (sdsr.responses.size: ", sdsr.responses.size(),
         "; max expected responses: ", sdsr.maxExpectedResponses, ")");
  }

  for (const auto &drp: sdsr.responses) {
    //check if response set was empty or not
    if (drp.responseSet) {
      if (drp.response.iactive) {
        set_responsei(pDuel, drp.response.u.ivalue);
      } else {
        //can't use any set_responseb() because bytes arg not const
        // so instead, set directly using memcpy
        std::memcpy(pD->game_field->returns.bvalue,
                    drp.response.u.bvaluebytes, 64);
      }
    } else {
      ++emptyProcessIterations;
    }

    pD->clear_buffer();  //clear buffer before processing
    process(pDuel);  //actually process
    ++processIterations;

    //print errors and exit if encountered
    if (!ddd::lastProcessSucceeded(pDuel)) {
      status = false;

      clog("w", "Attempt to advance duel state ", pDuel, " (",
           (duel*) pDuel, ") did not succeed.");
      if (!drp.responseSet) {
        clog("d", "(last response attempted to set (sdsr size: ",
             sdsr.responses.size(), "): empty)");
      } else if (drp.response.iactive) {
        clog("d", "(last response attempted to set (sdsr size: ",
             sdsr.responses.size(), "): ", drp.response.u.ivalue);
      } else {
        std::vector<std::string> v;
        for (int i = 0; i < 64; ++i)
          v.push_back(std::to_string((int) drp.response.u.bvaluebytes[i]));
        clog("d", "(last response attempted to set (drpv size: ",
             sdsr.responses.size(), "): \n[", joinString(v, "]["), "])");
      }
      break;
    }
  }

  return status;
}

//helper function to copy the random state from one mt19937
// object to another
// (requires compiling with the USE_PRIVATE_ACCESSOR macro defined
//  when compiling to utilize friend injection method defined in
//  dddapiutil.hpp to access private member of mt19937; if macro
//  not defined, attempts to copy it directly as if public member)
void ddd::copyRandomState(mt19937& srand, mt19937& nrand) {
#ifdef USE_PRIVATE_ACCESSOR
  nrand.*get(mt19937Accessor()) = srand.*get(mt19937Accessor());
#else
  nrand.rng = srand.rng;
#endif
}

//helper function to help generate duel state ids
//  (also used for shadow duels such that no duel state or
//   shadow duel state will share the same id)
unsigned long long ddd::generateId() {
  //start at 1; reserve 0 for invalid
  return ++getDDD_GS().dddapi.currId;
}
