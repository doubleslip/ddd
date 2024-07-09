/*
 * dddapi.hpp
 */


#ifndef __DDDAPI_HPP
#define __DDDAPI_HPP

#include <optional>

#include "ocgapi.h"

#include "dddsingleton.hpp"
#include "dddutil.hpp"
#include "dddapiutil.hpp"
#include "dddapistate.hpp"


extern "C" DECL_DLLEXPORT DDD_GS& getDDD_GS();
extern "C" DECL_DLLEXPORT bool set_conf_and_init(const char*, const bool);
extern "C" DECL_DLLEXPORT PyObject* py_set_conf_and_init(PyObject*, PyObject*);
extern "C" DECL_DLLEXPORT std::unordered_set<unsigned long long> get_duel_states();
extern "C" DECL_DLLEXPORT PyObject* py_get_duel_states();
extern "C" DECL_DLLEXPORT unsigned long long create_duel_state(const bool);
extern "C" DECL_DLLEXPORT PyObject* py_create_duel_state(PyObject*);
extern "C" DECL_DLLEXPORT unsigned long long create_duel_state_from_seed(const bool, const unsigned int);
extern "C" DECL_DLLEXPORT PyObject* py_create_duel_state_from_seed(PyObject*, PyObject*);
extern "C" DECL_DLLEXPORT unsigned long long duplicate_duel_state(const unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_duplicate_duel_state(PyObject*);
extern "C" DECL_DLLEXPORT unsigned long long duplicate_duel_state_reusing(const unsigned long long, const unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_duplicate_duel_state_reusing(PyObject*, PyObject*);
extern "C" DECL_DLLEXPORT bool is_duel_state_pduel_duplicatable(unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_is_duel_state_pduel_duplicatable(PyObject*);
extern "C" DECL_DLLEXPORT unsigned long long assume_duel_state(const unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_assume_duel_state(PyObject*);
extern "C" DECL_DLLEXPORT bool remove_duel_state(const unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_remove_duel_state(PyObject*);
extern "C" DECL_DLLEXPORT PyObject* py_remove_duel_states(PyObject*);
extern "C" DECL_DLLEXPORT int32 get_duel_state_message(const unsigned long long, byte*);
extern "C" DECL_DLLEXPORT PyObject* py_get_duel_state_message(PyObject*);
extern "C" DECL_DLLEXPORT void set_duel_state_responsei(const unsigned long long, const int32);
extern "C" DECL_DLLEXPORT void py_set_duel_state_responsei(PyObject*, PyObject*);
extern "C" DECL_DLLEXPORT void set_duel_state_responseb(const unsigned long long, const byte*);
extern "C" DECL_DLLEXPORT void py_set_duel_state_responseb(PyObject*, PyObject*);
extern "C" DECL_DLLEXPORT int32 process_duel_state(const unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_process_duel_state(PyObject*);
extern "C" DECL_DLLEXPORT PyObject* py_get_logs_from_cache();
extern "C" DECL_DLLEXPORT void py_clear_log_cache();
extern "C" DECL_DLLEXPORT std::tuple<std::vector<DDDReturn>, std::vector<int>, std::vector<std::string>, std::vector<std::string>, std::vector<int>> get_choices_from_duel_state(const unsigned long long, const bool, const std::unordered_set<int>&);
extern "C" DECL_DLLEXPORT PyObject* py_get_choices_from_duel_state(PyObject*, PyObject*, PyObject*);
extern "C" DECL_DLLEXPORT std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> get_field_visual_gamestate(unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_get_field_visual_gamestate(PyObject*);
extern "C" DECL_DLLEXPORT std::pair<std::string, int> get_dcs_command();
extern "C" DECL_DLLEXPORT PyObject* py_get_dcs_command();
extern "C" DECL_DLLEXPORT void load_dcs_commands(const char*);
extern "C" DECL_DLLEXPORT void py_load_dcs_commands(PyObject*);
extern "C" DECL_DLLEXPORT void clear_dcs_commands();
extern "C" DECL_DLLEXPORT void py_clear_dcs_commands();
extern "C" DECL_DLLEXPORT bool is_duel_state_active(const unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_is_duel_state_active(PyObject*);
extern "C" DECL_DLLEXPORT bool reactivate_duel_state(const unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_reactivate_duel_state(PyObject*);
extern "C" DECL_DLLEXPORT bool deactivate_duel_state(const unsigned long long);
extern "C" DECL_DLLEXPORT PyObject* py_deactivate_duel_state(PyObject*);
extern "C" DECL_DLLEXPORT std::unordered_set<int> get_choice_filter(const unsigned long long, const std::unordered_set<int>&);
extern "C" DECL_DLLEXPORT PyObject* py_get_choice_filter(PyObject*, PyObject*);

#endif
