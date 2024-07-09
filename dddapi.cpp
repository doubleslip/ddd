/*
 * dddapi.cpp
 */

#include "dddapi.hpp"

//include ocgapi.cpp body to access static globals:
// static script_reader sreader = default_script_reader;
// static card_reader creader = default_card_reader;
// static message_handler mhandler = default_message_handler;
// static byte buffer[0x20000];
// static std::set<duel*> duel_set;
#include "ygopro-core-master/ocgapi.cpp"


//Helper function to get true singleton the dll uses which would be
// different to dynamically linked applications on windows that would
// create their own instance of the singleton
extern "C" DECL_DLLEXPORT DDD_GS& getDDD_GS() {
  return DDD_GS::get();
}

//Wrapper function for dddGsInit() exposed to shared library
extern "C" DECL_DLLEXPORT bool set_conf_and_init(const char* confPath, const bool showNonErrors) {
  return dddGsInit(confPath, showNonErrors);
}
//Python wrapper for set_conf_and_init()
extern "C" DECL_DLLEXPORT PyObject* py_set_conf_and_init(PyObject* poConfPath, PyObject* poShowNonErrors) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject* poUnicode = PyUnicode_AsUTF8String(poConfPath);
  const char* confPath = PyBytes_AsString(poUnicode);
  bool result = set_conf_and_init(confPath,
                                  PyObject_IsTrue(poShowNonErrors));
  PyObject* poResult = PyBool_FromLong(result);

  Py_DecRef(poUnicode);
  PyGILState_Release(gstate);
  return poResult;
}

//Return all duel states from the singleton
extern "C" DECL_DLLEXPORT std::unordered_set<unsigned long long> get_duel_states() {
  std::unordered_set<unsigned long long> ds;

  for (const auto &d: getDDD_GS().dddapi.duelStates)
    ds.insert(d.first);

  return ds;
}
//Python wrapper for get_duel_states()
extern "C" DECL_DLLEXPORT PyObject* py_get_duel_states() {
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject* set = PySet_New(NULL);

  for (const auto &d: getDDD_GS().dddapi.duelStates)
    PySet_Add(set, PyLong_FromLongLong(d.first));

  PyGILState_Release(gstate);
  return set;
}

//Wrapper function for ddd::createDuelState() exposed to shared library
// but using a random seed to create duel state
//Creates a new duel state with a random seed (unless otherwise
// specified by conf file in which case, will use seed in conf file)
// and adds any needed references to and from its shadow state
extern "C" DECL_DLLEXPORT unsigned long long create_duel_state(const bool showNonErrors) {
  return ddd::createDuelState(showNonErrors, false, 0);
}
//Python wrapper for create_duel_state()
extern "C" DECL_DLLEXPORT PyObject* py_create_duel_state(PyObject* poShowNonErrors) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  unsigned long long dsId
    = create_duel_state(PyObject_IsTrue(poShowNonErrors));
  PyObject* poDsId = PyLong_FromLongLong(dsId);

  PyGILState_Release(gstate);
  return poDsId;
}

//Wrapper function for ddd::createDuelState() exposed to shared library
// but using a specified seed to create duel state with
//Creates a duel state with specified seed (that overrides conf file
// specified seed and also whether seed is random) and adds any
// references to and from its shadow state
extern "C" DECL_DLLEXPORT unsigned long long create_duel_state_from_seed(const bool showNonErrors, const unsigned int seed) {
  return ddd::createDuelState(showNonErrors, true, seed);
}
//Python wrapper for create_duel_state_from_seed()
extern "C" DECL_DLLEXPORT PyObject* py_create_duel_state_from_seed(PyObject* poShowNonErrors, PyObject* poSeed) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  unsigned long long dsId
    = create_duel_state_from_seed(PyObject_IsTrue(poShowNonErrors),
                                  PyLong_AsLongLong(poSeed));
  PyObject* poDsId = PyLong_FromLongLong(dsId);

  PyGILState_Release(gstate);
  return poDsId;
}


//Creates a copy of a duel state so that any responses set or process
// iterations performed on one duel state do not affect the other
extern "C" DECL_DLLEXPORT unsigned long long duplicate_duel_state(const unsigned long long sourceDsId) {
  return duplicate_duel_state_reusing(sourceDsId, 0);
}

//Python wrapper for duplicate_duel_state()
extern "C" DECL_DLLEXPORT PyObject* py_duplicate_duel_state(PyObject* poSourceDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  unsigned long long dsId
    = duplicate_duel_state_reusing(PyLong_AsLongLong(poSourceDsId), 0);
  PyObject* poDsId = PyLong_FromLongLong(dsId);

  PyGILState_Release(gstate);
  return poDsId;
}

//Creates a copy of a duel state so that any responses set or process
// iterations performed on one duel state do not affect the other
// (same as duplicate_duel_state() but accepts a duel state that is
//  not longer needed as its second argument to have its pDuel reused,
//  avoiding the need to construct a new pDuel and saving on memory;
//  if the pDuel was successfully reused, the duel state that was
//  reused will be deactivated)
// (if the duel state to reuse is not active or if its pDuel is in a
//  state where it cannot be duplicated, a new pDuel will be
//  constructed anyway and the duel state to reuse will not be
//  deactivated)
extern "C" DECL_DLLEXPORT unsigned long long duplicate_duel_state_reusing(const unsigned long long sourceDsId, const unsigned long long reuseDsId) {
  unsigned long long newDsId = 0;
  intptr_t pSourceDuel = 0;
  intptr_t pDuelReuse = 0;

  std::optional<DuelState> ods; //local copy for thread safety...?
  std::optional<DuelState> oReuseDs;
  std::optional<std::reference_wrapper<ShadowDuelState>> osds;
  std::optional<std::reference_wrapper<ShadowDuelStateResponses>> osdsr;
  {
    std::scoped_lock<std::mutex, std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex,
       getDDD_GS().dddapi.shadowDuelStatesMutex);

    if (getDDD_GS().dddapi.duelStates.find(sourceDsId) ==
        getDDD_GS().dddapi.duelStates.end()) {
      clog("e", "Unable to duplicate duel state ", sourceDsId,
           "; duel state does not exist or was not found.");
      return 0;
    }

    ods = getDDD_GS().dddapi.duelStates.at(sourceDsId);

    if (getDDD_GS().dddapi.shadowDuelStates.find(ods->shadowDsId) ==
        getDDD_GS().dddapi.shadowDuelStates.end()) {
      clog("e", "Unable to duplicate duel state ", sourceDsId,
           "; shadow duel state does not exist or was not found.");
      return 0;
    }

    osds = getDDD_GS().dddapi.shadowDuelStates.at(ods->shadowDsId);

    if (osds->get().responsesMap.find(sourceDsId) ==
        osds->get().responsesMap.end()) {
      clog("e", "Unable to duplicate duel state ", sourceDsId,
           "; unable to find its responses in its shadow duel state.");
      return 0;
    }

    osdsr = osds->get().responsesMap.at(sourceDsId);

    if (reuseDsId != 0) {
      if (getDDD_GS().dddapi.duelStates.find(reuseDsId) ==
          getDDD_GS().dddapi.duelStates.end()) {
        clog("e", "Unable to duplicate duel state ", sourceDsId,
             " while reusing duel state id ", reuseDsId,
             "; duel state to reuse does not exist or was not found.");
        return 0;
      }
      oReuseDs = getDDD_GS().dddapi.duelStates.at(reuseDsId);
      pDuelReuse = oReuseDs->pDuel;
    }
  }

  //would normally check if source duel state active but no need
  // since current implementation only duplicates shadow state
  // (which should always be active anyway)

  auto tpl = ddd::duplicateDuelStateFromShadow
    (osds->get().pDuel, *ods, *osdsr, pDuelReuse);

  if (std::get<0>(tpl) == 0) {
    clog("e", "Unable to duplicate duel state ", sourceDsId, ".");
    return 0;
  }

  {
    std::scoped_lock<std::mutex, std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex,
       getDDD_GS().dddapi.shadowDuelStatesMutex);

    //check if pDuel to reuse was actually reused and if so,
    // deactivate the duel state that originally held it
    if (oReuseDs.has_value()) {
      if (oReuseDs->pDuel == std::get<1>(tpl).pDuel) {
        auto& rds = getDDD_GS().dddapi.duelStates.at(reuseDsId);
        //don't call ddd::deactivate_duel_state() here because
        // it would destroy the pDuel which the duplicated
        // duel state now uses
        rds.pDuel = 0;
        rds.active = false;
      }
    }

    //update singleton
    getDDD_GS().dddapi.duelStates
      .emplace(std::get<0>(tpl), std::move(std::get<1>(tpl)));
    osds->get().responsesMap
      .emplace(std::get<0>(tpl), *osdsr);
  }

  return std::get<0>(tpl);
}

//Python wrapper for ddd::duplicate_duel_state_reusing()
extern "C" DECL_DLLEXPORT PyObject* py_duplicate_duel_state_reusing(PyObject* poSourceDsId, PyObject* poReuseDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  unsigned long long dsId
    = duplicate_duel_state_reusing(PyLong_AsLongLong(poSourceDsId),
                                   PyLong_AsLongLong(poReuseDsId));
  PyObject* poDsId = PyLong_FromLongLong(dsId);

  PyGILState_Release(gstate);
  return poDsId;
}

//Wrapper function for ddd::isDuelDuplicatable() exposed to shared library
//Checks if a duel state's pDuel can be duplicated without using
// the duel state's shadow duel
extern "C" DECL_DLLEXPORT bool is_duel_state_pduel_duplicatable(unsigned long long dsId) {
  return ddd::isDuelDuplicatable
    (getDDD_GS().dddapi.duelStates.at(dsId).pDuel);
}

//Python wrapper for is_duel_state_pduel_duplicatable()
extern "C" DECL_DLLEXPORT PyObject* py_is_duel_state_pduel_duplicatable(PyObject* poDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  unsigned long long dsId = PyLong_AsLongLong(poDsId);
  const auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);

  bool status = false;

  if (ds.pDuel) {
    status = ddd::isDuelDuplicatable(ds.pDuel);
  }

  PyObject* poResult = PyBool_FromLong(status);

  PyGILState_Release(gstate);
  return poResult;
}

//Wrapper function for ddd::assumeDuelState() exposed to shared library
//Functionally equivalent to duplicate_duel_state(dsId) followed by
// deactivate_duel_state(dsId) and then returning the newly
// duplicated duel state id
// (but more efficient in that no new duel state is actually
//  created but merely reassigned (or "assumed"))
extern "C" DECL_DLLEXPORT unsigned long long assume_duel_state(const unsigned long long dsId) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    return 0;
  }
  auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);

  if (getDDD_GS().dddapi.shadowDuelStates.find(ds.shadowDsId) ==
      getDDD_GS().dddapi.shadowDuelStates.end()) {
    clog("e", "Attempted to assume duel state ", dsId, " but unable",
         " to find its responses in shadow duel state.");
    return 0;
  }

  unsigned long long newDsId = 0;
  {
    std::scoped_lock<std::mutex, std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex,
       getDDD_GS().dddapi.shadowDuelStatesMutex);

    //assume the duel state
    const auto newDsP = ddd::assumeDuelState(ds);
    newDsId = std::get<0>(newDsP);

    //update singletons
    getDDD_GS().dddapi.duelStates.emplace(newDsId,
                                          std::get<1>(newDsP));

    auto& sds = getDDD_GS().dddapi.shadowDuelStates.at(ds.shadowDsId);
    const auto& sdsr = sds.responsesMap.at(dsId);
    sds.responsesMap.emplace(newDsId, sdsr);
  }

  return newDsId;
}

//Python wrapper for assume_duel_state()
extern "C" DECL_DLLEXPORT PyObject* py_assume_duel_state(PyObject* poSourceDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  unsigned long long dsId
    = assume_duel_state(PyLong_AsLongLong(poSourceDsId));
  PyObject* poDsId = PyLong_FromLongLong(dsId);

  PyGILState_Release(gstate);
  return poDsId;
}

//Wrapper function for ddd::removeDuelState() exposed to shared library
//Remove a duel state by destroying the duel and removing any
// references to it from any shadow state
extern "C" DECL_DLLEXPORT bool remove_duel_state(const unsigned long long dsId) {
  bool status = ddd::removeDuelState(dsId);

  if (!status) {
    clog("e", "Unable to remove duel state ", dsId, ".");
  }
  return status;
}
//Python wrapper for remove_duel_state()
extern "C" DECL_DLLEXPORT PyObject* py_remove_duel_state(PyObject* poDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  bool result = remove_duel_state(PyLong_AsLongLong(poDsId));
  PyObject* poResult = PyBool_FromLong(result);

  PyGILState_Release(gstate);
  return poResult;
}
//Python function to remove multiple duel states
extern "C" DECL_DLLEXPORT PyObject* py_remove_duel_states(PyObject* poDsIds) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  //check if Python object iterable
  PyObject* poItr = PyObject_GetIter(poDsIds);
  std::unordered_set<unsigned long long> dsIdSet;
  if (poItr == NULL) {
    PyErr_SetString(PyExc_TypeError, "Filter argument not iterable.");
    Py_DecRef(poItr);

    PyGILState_Release(gstate);
    return NULL;
  }

  //iterate and build set
  for (PyObject* poItem = PyIter_Next(poItr);
       poItem != NULL; poItem = PyIter_Next(poItr)) {
    if (PyLong_Check(poItem)) {
      dsIdSet.insert(PyLong_AsLong(poItem));
    }
    Py_DecRef(poItem);
  }
  Py_DecRef(poItr);

  bool status = true;

  //iterate set and remove duel state one by one
  for (const auto &dsId: dsIdSet)
    if (!ddd::removeDuelState(dsId)) {
      status = false;
      clog("e", "Unable to remove duel state ", dsId, ".");
    }

  PyObject* poResult = PyBool_FromLong(status);

  PyGILState_Release(gstate);
  return poResult;
}

//Copy duel's buffer to second argument and return the length of buffer
// (unlike the ocgapi get_message() function which this function was
//  intended to replace, this function doesn't clear the duel buffer)
extern "C" DECL_DLLEXPORT int32 get_duel_state_message(const unsigned long long dsId, byte* b) {
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("e", "Unable to get message for duel state ", dsId,
         "; was not found.");
    return 0;
  }
  //activate duel state if not active
  if (!getDDD_GS().dddapi.duelStates.at(dsId).active) {
    if (!reactivate_duel_state(dsId)) {
      clog("e", "Unable to process duel state ", dsId, "; reactivation"
           " of duel state which was deactivated, did not succeed.");
      return 0;
    }
  }

  intptr_t pDuel = getDDD_GS().dddapi.duelStates.at(dsId).pDuel;
  int32 len = ((duel*) pDuel)->read_buffer(b);

  return len;
}
//Python function to get duel's buffer
// (Python equivalent of get_duel_state_message() function but returns
//  the buffer as a bytearray rather than returning the length)
// (could consider deprecating this function as much more common to now
//  use py_get_choices() and not have to parse the message)
extern "C" DECL_DLLEXPORT PyObject* py_get_duel_state_message(PyObject* poDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  byte b[MAX_DUEL_BUFFER_SIZE];
  int32 len = get_duel_state_message(PyLong_AsLongLong(poDsId), b);
  PyObject* poB
    = PyByteArray_FromStringAndSize(reinterpret_cast<char*>(b), len);

  PyGILState_Release(gstate);
  return poB;
}

//Accepts a response as a int32 and saves the response in duel state
// (unlike ocgapi's set_responsei, function does not actually set
//  the response into the duel's returns.ivalue, instead saving it
//  so that it will actually be set later when doing a process
//  iteration)
extern "C" DECL_DLLEXPORT void set_duel_state_responsei(const unsigned long long dsId, const int32 i) {
  std::optional<DuelState> ods;

  {
    std::scoped_lock<std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex);

    if (getDDD_GS().dddapi.duelStates.find(dsId) ==
        getDDD_GS().dddapi.duelStates.end()) {
      clog("e", "Unable to set response for duel state ", dsId,
           "; was not found.");
      return;
    }

    ods = getDDD_GS().dddapi.duelStates.at(dsId);
  }

  ddd::setDuelStateResponseI(*ods, i);

  {
    std::scoped_lock<std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex);
    getDDD_GS().dddapi.duelStates.at(dsId) = std::move(*ods);
  }
}
//Python wrapper for set_duel_state_responsei()
extern "C" DECL_DLLEXPORT void py_set_duel_state_responsei(PyObject* poDsId, PyObject* poI) {
  PyGILState_STATE gstate = PyGILState_Ensure();
  set_duel_state_responsei(PyLong_AsLongLong(poDsId),
                           PyLong_AsLong(poI));
  PyGILState_Release(gstate);
}
//Accepts a response as an array of bytes and saves the response in
//  duel state
// (unlike ocgapi's set_responseb, function does not actually set
//  the response into the duel's returns.bvalue, instead saving it
//  so that it will actually be set later when doing a process
//  iteration)
extern "C" DECL_DLLEXPORT void set_duel_state_responseb(const unsigned long long dsId, const byte* b) {
  std::optional<DuelState> ods;

  {
    std::scoped_lock<std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex);

    if (getDDD_GS().dddapi.duelStates.find(dsId) ==
        getDDD_GS().dddapi.duelStates.end()) {
      clog("e", "Unable to set response for duel state ", dsId,
           "; was not found.");
      return;
    }

    ods = getDDD_GS().dddapi.duelStates.at(dsId);
  }

  ddd::setDuelStateResponseB(*ods, b);

  {
    std::scoped_lock<std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex);
    getDDD_GS().dddapi.duelStates.at(dsId) = std::move(*ods);
  }
}
//Python wrapper for set_duel_state_responseb()
extern "C" DECL_DLLEXPORT void py_set_duel_state_responseb(PyObject* poDsId, PyObject* poB) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  byte* b = reinterpret_cast<byte*>(PyByteArray_AsString(poB));
  set_duel_state_responseb(PyLong_AsLongLong(poDsId), b);

  PyGILState_Release(gstate);
}


//Perform a process iteration on the specified duel state
extern "C" DECL_DLLEXPORT int32 process_duel_state(const unsigned long long dsId) {
  int32 result = -1;
  intptr_t pDuel = 0;

  //activate duel state if not active
  if (!is_duel_state_active(dsId)) {
    if (!reactivate_duel_state(dsId)) {
      clog("e", "Unable to process duel state ", dsId, "; reactivation"
           " of duel state, which was deactivated, did not succeed.");
      return result;
    }
  }

  std::optional<DuelState> ods;
  std::optional<ShadowDuelStateResponses> osdsr;

  //make copy of previous duel state and shadow duel state
  {
    std::scoped_lock<std::mutex, std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex,
       getDDD_GS().dddapi.shadowDuelStatesMutex);

    ods = getDDD_GS().dddapi.duelStates.at(dsId);
    osdsr = getDDD_GS().dddapi.shadowDuelStates
      .at(ods->shadowDsId).responsesMap.at(dsId);
  }

  //do a process iteration and update copy of duel state and shadow
  // duel state
  result = ddd::processDuelState(*ods, *osdsr);

  //move copies of duel state and shadow duel state back to singleton
  // (whether process succeeded or not)
  {
    std::scoped_lock<std::mutex, std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex,
       getDDD_GS().dddapi.shadowDuelStatesMutex);
    getDDD_GS().dddapi.duelStates.at(dsId) = std::move(*ods);
    getDDD_GS().dddapi.shadowDuelStates.at(ods->shadowDsId)
      .responsesMap.at(dsId) = std::move(*osdsr);
  }

  return result;
}

//Python wrapper for process_duel_state()
extern "C" DECL_DLLEXPORT PyObject* py_process_duel_state(PyObject* poDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  int32 result = process_duel_state(PyLong_AsLongLong(poDsId));
  PyObject* poResult = PyLong_FromLong(result);

  PyGILState_Release(gstate);
  return poResult;
}

//Python function to get cached logs from singleton
// (only if enabled in the conf file)
//Returns an empty string otherwise
extern "C" DECL_DLLEXPORT PyObject* py_get_logs_from_cache() {
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject* poLogs = PyList_New(0);

  const auto& logsFromCache = getDDD_GS().conf.log.logCache;
  getDDD_GS().conf.log.logCache.clear();

  for (const auto &s: logsFromCache)
    PyList_Append(poLogs, PyUnicode_FromString(s.c_str()));

  PyGILState_Release(gstate);
  return poLogs;
}

//Python function to clear cached logs from singleton
extern "C" DECL_DLLEXPORT void py_clear_log_cache() {
  getDDD_GS().conf.log.logCache.clear();
}

//Wrapper function for ddd::getChoicesFromDuelState() exposed
// to shared library
//Given a duel state id, return choices available to it as well as
// some extra data such as the message type and whether the choices
// match the filter (optionally passed in)
std::tuple<std::vector<DDDReturn>, std::vector<int>, std::vector<std::string>, std::vector<std::string>, std::vector<int>> get_choices_from_duel_state(const unsigned long long dsId, const bool generateStrings, const std::unordered_set<int>& filterSet) {

  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("e", "Unable to get choices for duel state ", dsId,
         "; was not found.");

    return std::tuple<std::vector<DDDReturn>, std::vector<int>,
		      std::vector<std::string>,
		      std::vector<std::string>, std::vector<int>>();
  }

  const auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
  return ddd::getChoicesFromDuelState(ds, generateStrings, filterSet);
}

//Python wrapper for get_choices_from_duel_state()
extern "C" DECL_DLLEXPORT PyObject* py_get_choices_from_duel_state(PyObject* poDsId, PyObject* poGenerateStrings, PyObject* poFilter) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  unsigned long long dsId = PyLong_AsLongLong(poDsId);
  bool generateStrings = PyObject_IsTrue(poGenerateStrings);

  //declare but no need to initialize Python objects yet
  PyObject* poRetVals;
  PyObject* poExtras;
  PyObject* poHintStrings;
  PyObject* poRetLabels;
  PyObject* poFilterMatches;

  PyObject* poTpl = PyTuple_New(5);

  //check duel state exists
  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    clog("e", "Unable to get choices for duel state ", dsId,
         "; was not found.");

    PyTuple_SetItem(poTpl, 0, PyList_New(0));
    PyTuple_SetItem(poTpl, 1, PyList_New(0)); //maybe push 0 to [0]?
    PyTuple_SetItem(poTpl, 2, PyList_New(0));
    PyTuple_SetItem(poTpl, 3, PyList_New(0));
    PyTuple_SetItem(poTpl, 4, PyList_New(0));

    PyGILState_Release(gstate);
    return poTpl;
  }

  //check if duel state was active
  bool dsWasActive = is_duel_state_active(dsId);
  if (!reactivate_duel_state(dsId)) {
    PyErr_SetString(PyExc_RuntimeError, "Unable to activate duel"
                    " state id (or invalid duel state id?).");

    PyGILState_Release(gstate);
    return NULL;
  }

  //check filter object iterable
  PyObject* poItr = PyObject_GetIter(poFilter);
  if (poItr == NULL) {
    PyErr_SetString(PyExc_TypeError, "Filter argument not iterable?");
    Py_DecRef(poItr);

    PyGILState_Release(gstate);
    return NULL;
  }

  //iterate filter object and build filter set
  std::unordered_set<int> filter;
  for (PyObject* poItem = PyIter_Next(poItr);
       poItem != NULL; poItem = PyIter_Next(poItr)) {
    if (PyLong_Check(poItem)) {
      filter.insert(PyLong_AsLong(poItem));
    }
    Py_DecRef(poItem);
  }
  Py_DecRef(poItr);

  //actually get choices
  const auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
  auto choices
    = ddd::getChoicesFromDuelState(ds, generateStrings, filter);

  //resize arrays
  poRetVals = PyList_New(std::get<0>(choices).size());
  poExtras = PyList_New(std::get<1>(choices).size());
  poHintStrings = PyList_New(std::get<2>(choices).size());
  poRetLabels = PyList_New(std::get<3>(choices).size());
  poFilterMatches = PyList_New(std::get<4>(choices).size());

  //choices
  for (int i = 0; i < std::get<0>(choices).size(); ++i) {
    const auto &r = std::get<0>(choices)[i];
    PyObject* retVal;

    if (r.iactive)
      retVal = PyLong_FromLong(r.u.ivalue);
    else
      retVal
        = PyByteArray_FromStringAndSize(reinterpret_cast<const char*>
                                        (r.u.bvaluebytes), 64);
    PyList_SetItem(poRetVals, i, retVal);
  }

  //extras
  for (int i = 0; i < std::get<1>(choices).size(); ++i) {
    const auto &e = std::get<1>(choices)[i];
    PyList_SetItem(poExtras, i, PyLong_FromLong(e));
  }

  //hint strings
  for (int i = 0; i < std::get<2>(choices).size(); ++i) {
    const auto &hs = std::get<2>(choices)[i];
    PyList_SetItem(poHintStrings, i, PyUnicode_FromString(hs.c_str()));
  }

  //choice labels
  for (int i = 0; i < std::get<3>(choices).size(); ++i) {
    const auto &cl = std::get<3>(choices)[i];
    PyList_SetItem(poRetLabels, i, PyUnicode_FromString(cl.c_str()));
  }

  //filter matches
  for (int i = 0; i < std::get<4>(choices).size(); ++i) {
    const auto &fm = std::get<4>(choices)[i];
    PyList_SetItem(poFilterMatches, i, PyLong_FromLong(fm));
  }

  //set lists into tuple
  PyTuple_SetItem(poTpl, 0, poRetVals);
  PyTuple_SetItem(poTpl, 1, poExtras);
  PyTuple_SetItem(poTpl, 2, poHintStrings);
  PyTuple_SetItem(poTpl, 3, poRetLabels);
  PyTuple_SetItem(poTpl, 4, poFilterMatches);

  PyGILState_Release(gstate);

  if (!dsWasActive)
    deactivate_duel_state(dsId);

  return poTpl;
}

//Wrapper function for ddd::getFieldVisualGamestate() exposed
// to shared library
extern "C" DECL_DLLEXPORT std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> get_field_visual_gamestate(unsigned long long dsId) {
  return ddd::getFieldVisualGamestate(dsId);
}

//Python wrapper for get_field_visual_gamestate() function
extern "C" DECL_DLLEXPORT PyObject* py_get_field_visual_gamestate(PyObject* poDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  //get visual representation of game state (of field)
  unsigned long long dsId = PyLong_AsLongLong(poDsId);
  auto fvgTuple = ddd::getFieldVisualGamestate(dsId);

  PyObject* poTpl = PyTuple_New(6);
  PyObject* po0 = PyList_New(std::get<0>(fvgTuple).size());
  PyObject* po1 = PyList_New(std::get<1>(fvgTuple).size());
  PyObject* po2 = PyList_New(std::get<2>(fvgTuple).size());
  PyObject* po3 = PyList_New(std::get<3>(fvgTuple).size());
  PyObject* po4 = PyList_New(std::get<4>(fvgTuple).size());
  PyObject* po5 = PyList_New(std::get<5>(fvgTuple).size());

  //field (visual)
  for (int i = 0; i < std::get<0>(fvgTuple).size(); ++i) {
    const auto &s = std::get<0>(fvgTuple)[i];
    PyList_SetItem(po0, i, PyUnicode_FromString(s.c_str()));
  }
  PyTuple_SetItem(poTpl, 0, po0);

  //specific player 0 field info
  for (int i = 0; i < std::get<1>(fvgTuple).size(); ++i) {
    const auto &s = std::get<1>(fvgTuple)[i];
    PyList_SetItem(po1, i, PyUnicode_FromString(s.c_str()));
  }
  PyTuple_SetItem(poTpl, 1, po1);

  //specific player 1 field info
  for (int i = 0; i < std::get<2>(fvgTuple).size(); ++i) {
    const auto &s = std::get<2>(fvgTuple)[i];
    PyList_SetItem(po2, i, PyUnicode_FromString(s.c_str()));
  }
  PyTuple_SetItem(poTpl, 2, po2);

  //player 0 info
  for (int i = 0; i < std::get<3>(fvgTuple).size(); ++i) {
    const auto &s = std::get<3>(fvgTuple)[i];
    PyList_SetItem(po3, i, PyUnicode_FromString(s.c_str()));
  }
  PyTuple_SetItem(poTpl, 3, po3);

  //player 1 info
  for (int i = 0; i < std::get<4>(fvgTuple).size(); ++i) {
    const auto &s = std::get<4>(fvgTuple)[i];
    PyList_SetItem(po4, i, PyUnicode_FromString(s.c_str()));
  }
  PyTuple_SetItem(poTpl, 4, po4);

  //chain information
  for (int i = 0; i < std::get<5>(fvgTuple).size(); ++i) {
    const auto &s = std::get<5>(fvgTuple)[i];
    PyList_SetItem(po5, i, PyUnicode_FromString(s.c_str()));
  }
  PyTuple_SetItem(poTpl, 5, po5);

  PyGILState_Release(gstate);
  return poTpl;
}

//Wrapper function for getDcsCommand() exposed to shared library
extern "C" DECL_DLLEXPORT std::pair<std::string, int> get_dcs_command() {
  return getDcsCommand();
}
//Python wrapper for get_dcs_command()
extern "C" DECL_DLLEXPORT PyObject* py_get_dcs_command() {
  PyGILState_STATE gstate = PyGILState_Ensure();

  auto cmd = getDcsCommand();

  PyObject* poCmdStr = PyUnicode_FromString(cmd.first.c_str());
  PyObject* poLine = PyLong_FromLong(cmd.second);

  PyObject* poTpl = PyTuple_New(2);
  PyTuple_SetItem(poTpl, 0, poCmdStr);
  PyTuple_SetItem(poTpl, 1, poLine);

  PyGILState_Release(gstate);
  return poTpl;
}

//Wrapper function for loadDcsCommandsFromScript() exposed
// to shared library
extern "C" DECL_DLLEXPORT void load_dcs_commands(const char* src) {
  loadDcsCommandsFromScript(dequoteString(src));
}
//Python wrapper for load_dcs_commands()
extern "C" DECL_DLLEXPORT void py_load_dcs_commands(PyObject* poSrc) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  PyObject* poUnicode = PyUnicode_AsUTF8String(poSrc);
  const char* src = PyBytes_AsString(poUnicode);
  //check if src == NULL?

  loadDcsCommandsFromScript(dequoteString(src));

  Py_DecRef(poUnicode);
  PyGILState_Release(gstate);
}

//Clear any remaining dcs commands from singleton
extern "C" DECL_DLLEXPORT void clear_dcs_commands() {
  int sz = getDDD_GS().conf.dcsCommandsCache.size();
  clog("w", "Clearing dcs command cache.");
  if (sz > 0)
    clog("w", "  (discarding ", sz, " remaining dcs command(s)).");
  getDDD_GS().conf.dcsCommandsCache.clear();
}
//Python wrapper for clear_dcs_commands()
extern "C" DECL_DLLEXPORT void py_clear_dcs_commands() {
  clear_dcs_commands();
}

//Wrapper function for ddd::isDuelStateActive() exposed
// to shared library
//Returns whether a duel state is active or not
extern "C" DECL_DLLEXPORT bool is_duel_state_active(const unsigned long long dsId) {
  std::scoped_lock<std::mutex> dddgs
    (getDDD_GS().dddapi.duelStatesMutex);

  if (getDDD_GS().dddapi.duelStates.find(dsId) ==
      getDDD_GS().dddapi.duelStates.end()) {
    return false;
  }

  const auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
  return ddd::isDuelStateActive(ds);
}
//Python wrapper for is_duel_state_active()
extern "C" DECL_DLLEXPORT PyObject* py_is_duel_state_active(PyObject* poDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  bool result = is_duel_state_active(PyLong_AsLongLong(poDsId));
  PyObject* poResult = PyBool_FromLong(result);

  PyGILState_Release(gstate);
  return poResult;
}

//Wrapper function for ddd::reactivateDuelState() exposed
// to shared library
//Activates a duel state and returns whether it was successsful or not
// (if already active, function does nothing and returns true)
extern "C" DECL_DLLEXPORT bool reactivate_duel_state(const unsigned long long dsId) {
  bool status = true;
  std::optional<DuelState> ods;
  intptr_t pShadowDuel = 0;
  std::optional<ShadowDuelStateResponses> osdsr;

  {
    std::scoped_lock<std::mutex, std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex,
       getDDD_GS().dddapi.shadowDuelStatesMutex);

    if (getDDD_GS().dddapi.shadowDuelStates.find(dsId) !=
        getDDD_GS().dddapi.shadowDuelStates.end()) {
      clog("w", "Attempted to reactivate ", dsId, " which is a"
           " shadow duel state.");
      return true; //...or false...?
    }
    if (getDDD_GS().dddapi.duelStates.find(dsId) ==
        getDDD_GS().dddapi.duelStates.end()) {
      clog("e", "Unable to reactivate duel state ", dsId,
           "; does not exist or was not found.");
      return false;
    }

    ods = getDDD_GS().dddapi.duelStates.at(dsId);

    if (ddd::isDuelStateActive(*ods)) {
      //was already activated
      return true;
    }

    if (getDDD_GS().dddapi.shadowDuelStates.find(ods->shadowDsId) ==
        getDDD_GS().dddapi.shadowDuelStates.end()) {
      clog("e", "Unable to reactivate duel state ", dsId, "; shadow"
           " duel state (", ods->shadowDsId, ") was invalid (does not"
           " exist or was not found).");
      return false;
    }

    const auto& sds =
      getDDD_GS().dddapi.shadowDuelStates.at(ods->shadowDsId);

    pShadowDuel = sds.pDuel;

    if (sds.responsesMap.find(dsId) == sds.responsesMap.end()) {
      clog("e", "Unable to reactivate duel state ", dsId,
         "; shadow duel state ", ods->shadowDsId, " does not have"
         " duel state ", dsId, " in its response map.");
      return false;
    }

    osdsr = sds.responsesMap.at(dsId);
  }

  status = ddd::reactivateDuelState(dsId, *ods, pShadowDuel, *osdsr);

  if (status) {
    std::scoped_lock<std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex);

    getDDD_GS().dddapi.duelStates.at(dsId) = std::move(*ods);
  }

  return status;
}
//Python wrapper for reactivate_duel_state()
extern "C" DECL_DLLEXPORT PyObject* py_reactivate_duel_state(PyObject* poDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  bool result = reactivate_duel_state(PyLong_AsLongLong(poDsId));
  PyObject* poResult = PyBool_FromLong(result);

  PyGILState_Release(gstate);
  return poResult;
}

//Wrapper function for ddd::deactivateDuelState() exposed
// to shared library
//Dectivates a duel state and returns whether it was successsful or not
// (if already deactivated, function does nothing and returns true)
// (deactivated duel states destroy the pDuel containing the duel
//  state's state, saving memory but cannot be used unless
//  reactivated first)
extern "C" DECL_DLLEXPORT bool deactivate_duel_state(const unsigned long long dsId) {
  bool status = true;
  std::optional<DuelState> ods;

  {
    std::scoped_lock<std::mutex, std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex,
       getDDD_GS().dddapi.shadowDuelStatesMutex);

    if (getDDD_GS().dddapi.shadowDuelStates.find(dsId) !=
        getDDD_GS().dddapi.shadowDuelStates.end()) {
      clog("e", "Unable to deactivate duel state ", dsId,
           "; (might be a shadow duel state, which cannot"
           " be deactivated?");
      return false;
    }
    if (getDDD_GS().dddapi.duelStates.find(dsId) ==
        getDDD_GS().dddapi.duelStates.end()) {
      clog("e", "Unable to deactivate duel state ", dsId,
           "; does not exist or was not found.");
      return false;
    }

    ods = getDDD_GS().dddapi.duelStates.at(dsId);
  }

  if (!ddd::isDuelStateActive(*ods)) {
    //was already deactivated
    return true;
  }

  status = ddd::deactivateDuelState(*ods);

  if (status) {
    std::scoped_lock<std::mutex> dddgs
      (getDDD_GS().dddapi.duelStatesMutex);

    getDDD_GS().dddapi.duelStates.at(dsId) = std::move(*ods);
  }

  return status;
}
//Python wrapper for deactivate_duel_state()
extern "C" DECL_DLLEXPORT PyObject* py_deactivate_duel_state(PyObject* poDsId) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  bool result = deactivate_duel_state(PyLong_AsLongLong(poDsId));
  PyObject* poResult = PyBool_FromLong(result);

  PyGILState_Release(gstate);
  return poResult;
}

//Wrapper function for ddd::getChoiceFilter() exposed
// to shared library
extern "C" DECL_DLLEXPORT std::unordered_set<int> get_choice_filter(const unsigned long long dsId, const std::unordered_set<int>& choicesFilter) {

  const auto& ds = getDDD_GS().dddapi.duelStates.at(dsId);
  return ddd::getChoiceFilter(ds, choicesFilter);
}
//Python wrapper for get_choice_filter()
extern "C" DECL_DLLEXPORT PyObject* py_get_choice_filter(PyObject* poDsId, PyObject* poInFilter) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  unsigned long long dsId = PyLong_AsLongLong(poDsId);
  PyObject* poItr = PyObject_GetIter(poInFilter);

  //check filter object iterable
  if (poItr == NULL) {
    PyErr_SetString(PyExc_TypeError, "Filter argument not iterable?");
    Py_DecRef(poItr);
    PyGILState_Release(gstate);
    return NULL;
  }

  //iterate filter object and build filter set
  std::unordered_set<int> choicesFilter;
  for (PyObject* poItem = PyIter_Next(poItr);
       poItem != NULL; poItem = PyIter_Next(poItr)) {
    if (PyLong_Check(poItem)) {
      choicesFilter.insert(PyLong_AsLong(poItem));

    } else {
      PyErr_SetString(PyExc_TypeError, "Unable to convert item in"
                      " passed filter to an int.");

      Py_DecRef(poItem);
      Py_DecRef(poItr);
      PyGILState_Release(gstate);
      return NULL;
    }
    Py_DecRef(poItem);
  }
  Py_DecRef(poItr);

  //get the actual filter
  const auto &ds = getDDD_GS().dddapi.duelStates.at(dsId);
  auto filterTpl = ddd::getChoiceFilter(ds, choicesFilter);

  //convert actual filter to Python object
  PyObject* poFilter = PySet_New(0);
  for (const auto &f: filterTpl) {
    PyObject* poLong = PyLong_FromLongLong(f);
    PySet_Add(poFilter, poLong);
    Py_DecRef(poLong);
  }

  PyGILState_Release(gstate);
  return poFilter;
}
