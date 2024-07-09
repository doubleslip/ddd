/*
 * dddapiutil.hpp
 */

#ifndef __DDDAPIUTIL_HPP
#define __DDDAPIUTIL_HPP

#include <vector>
#include <string>
#include <random>
#include <mutex>

#include "dddsingleton.hpp"
#include "dddapi.hpp"
#include "mtrandom.h"


namespace ddd {

#ifdef USE_PRIVATE_ACCESSOR
  class mt19937Accessor {
  public:
    typedef std::mt19937 mt19937::*type;
    friend type get(mt19937Accessor);
  };
  template<typename T, typename T::type M>
  class Accessor {
    friend typename T::type get(T) {
      return M;
    }
  };
#endif

  std::tuple<std::vector<DDDReturn>, std::vector<int>, std::vector<std::string>, std::vector<std::string>, std::vector<int>> getChoicesFromDuelState(const DuelState&, const bool);
  std::tuple<std::vector<DDDReturn>, std::vector<int>, std::vector<std::string>, std::vector<std::string>, std::vector<int>> getChoicesFromDuelState(const DuelState&, const bool, const std::unordered_set<int>&);
  std::unordered_set<int> getChoiceFilter(const DuelState&, const std::unordered_set<int>&);

  bool lastProcessSucceeded(const intptr_t);
  bool processDuelState(DuelState&, ShadowDuelStateResponses&);
  bool advanceDuelState(const intptr_t, const ShadowDuelStateResponses&);
#ifdef USE_PRIVATE_ACCESSOR
  template class Accessor<mt19937Accessor, &mt19937::rng>;
#endif
  void copyRandomState(mt19937&, mt19937&);
  unsigned long long generateId();
}


#endif

