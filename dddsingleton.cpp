/*
 * dddsingleton.cpp
 */

#include "dddsingleton.hpp"

DDD_GS::DDD_GS() {
  if (!Py_IsInitialized())
    Py_Initialize();
}
