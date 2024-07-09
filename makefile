# # #
# ddd makefile
# # #


CC:=g++
LD:=ld
FLAGS:=-std=c++17 -O3 -fPIC -fopenmp
MAKEFLAGS:=--no-print-directory


# # # Various settable options (mostly directory related)

#ygopro core source directory
YC_PATH=ygopro-core-master

#default dir to check for certain files
SANDBOX_DIR=samples

#.o file dir
ODIR=odir

#lua related dirs and files
#LUA_INCLUDE_PATH=/usr/include/lua5.4
#LUA_LIB=lua5.4-c++
#LUA_LIB_PATH=/usr/lib/x86_64-linux-gnu
LUA_INCLUDE_PATH=./samples/lua_54_modified_include
LUA_LIB=lua54
LUA_LIB_PATH=.


#python related dirs and files
#PYTHON_INCLUDE_PATH=/usr/include/python3.11
#PYTHON_LIB=python3.11
#PYTHON_LIB_PATH=/usr/lib/x86_64-linux-gnu
PYTHON_INCLUDE_PATH=C:/msys64/ucrt64/include/python3.11
PYTHON_LIB=python3
PYTHON_LIB_PATH=.


#sqlite related dirs and files
#SQLITE3_INCLUDE_PATH=/usr/include
#SQLITE3_LIB=sqlite3
#SQLITE3_LIB_PATH=/usr/lib/x86_64-linux-gnu
SQLITE3_INCLUDE_PATH=C:/msys64/ucrt64/include
SQLITE3_LIB=sqlite3
SQLITE3_LIB_PATH=.




# # # Various toggleable options (0 or 1)

#prefer linking the dddapi library into ddd$(EXE_EXT) statically
OPT_PREFER_STATIC_LINK := 1
#compile with mingw
OPT_COMPILE_MINGW := 1
#compile with debugging symbols
OPT_DEBUG := 1
#print outputs to clog() if called
OPT_CLOG_ENABLE := 1
#force clog() to use barebones output (using printf()) and disable extra logging features (even if conf file specifies otherwise)
OPT_CLOG_FORCE_PRINTF := 0
#compile state monitoring code into ddd executable
OPT_MONITOR_GAMESTATE := 1
#compile with templates used to access private variables via templates
OPT_USE_PRIVATE_ACCESSOR := 1

# # # End of options



ifeq ($(OPT_COMPILE_MINGW),1)
#'-D_hypot=hypot' macro to address mingw bug involving cmath
#'-DHAVE_SNPRINTF' macro to address a potential issue with python
FLAGS:=$(FLAGS) -D_hypot=hypot -DHAVE_SNPRINTF -DBUILD_WIN
EXE_EXT:=.exe
LIB_EXT:=.dll
else
FLAGS:=$(FLAGS) -DBUILD_NIX
EXE_EXT:=.out
LIB_PRF:=lib
LIB_EXT:=.so
endif
ifeq ($(OPT_DEBUG),1)
FLAGS:=$(subst  -O3,-Og -g,$(FLAGS))
SUF:=_debug
endif
ifeq ($(OPT_CLOG_ENABLE),1)
FLAGS:=$(FLAGS) -DCLOG_ENABLE
endif
ifeq ($(OPT_CLOG_FORCE_PRINTF),1)
FLAGS:=$(FLAGS) -DCLOG_FORCE_PRINTF
endif
ifeq ($(OPT_MONITOR_GAMESTATE),1)
FLAGS:=$(FLAGS) -DMONITOR_GAMESTATE
STATEMONITOR_DEP:=$(ODIR)/dddstatemonitor$(SUF).o
endif
ifeq ($(OPT_USE_PRIVATE_ACCESSOR),1)
FLAGS:=$(FLAGS) -DUSE_PRIVATE_ACCESSOR
endif



.PHONY: default
default:
	$(error Please specify a target (e.g. make all))

.PHONY: clean
clean:
	rm ./$(ODIR)/*.o

.PHONY: cleanddd
cleanddd: clean_ddd
.PHONY: dddclean
dddclean: clean_ddd
.PHONY: clean_ddd
clean_ddd:
	rm ./$(ODIR)/ddd*.o

.PHONY: all
all: ddd_api ddd_static

.PHONY: ddd
ddd: ddd_static

.PHONY: dddstatic
dddstatic: ddd_static
.PHONY: ddd_static
ddd_static:
	@$(MAKE) $(MAKEFLAGS) ddd_static$(EXE_EXT)

.PHONY: ddddynamic
ddddynamic: ddd_dynamic
.PHONY: ddd_dynamic
ddd_dynamic:
	@$(MAKE) $(MAKEFLAGS) $(LIB_PRF)dddapi$(LIB_EXT)
	@$(MAKE) $(MAKEFLAGS) ddd_dynamic$(EXE_EXT)

.PHONY: dddapi
dddapi: ddd_api
.PHONY: ddd_api
ddd_api:
	@$(MAKE) $(MAKEFLAGS) $(LIB_PRF)dddapi$(LIB_EXT)




$(ODIR)/card$(SUF).o: $(YC_PATH)/card.cpp
	$(CC) $(FLAGS) $(YC_PATH)/card.cpp -c -o $(ODIR)/card$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/duel$(SUF).o: $(YC_PATH)/duel.cpp
	$(CC) $(FLAGS) $(YC_PATH)/duel.cpp -c -o $(ODIR)/duel$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/effect$(SUF).o: $(YC_PATH)/effect.cpp
	$(CC) $(FLAGS) $(YC_PATH)/effect.cpp -c -o $(ODIR)/effect$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/field$(SUF).o: $(YC_PATH)/field.cpp
	$(CC) $(FLAGS) $(YC_PATH)/field.cpp -c -o $(ODIR)/field$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/group$(SUF).o: $(YC_PATH)/group.cpp
	$(CC) $(FLAGS) $(YC_PATH)/group.cpp -c -o $(ODIR)/group$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/interpreter$(SUF).o: $(YC_PATH)/interpreter.cpp
	$(CC) $(FLAGS) $(YC_PATH)/interpreter.cpp -c -o $(ODIR)/interpreter$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/libcard$(SUF).o: $(YC_PATH)/libcard.cpp
	$(CC) $(FLAGS) $(YC_PATH)/libcard.cpp -c -o $(ODIR)/libcard$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/libdebug$(SUF).o: $(YC_PATH)/libdebug.cpp
	$(CC) $(FLAGS) $(YC_PATH)/libdebug.cpp -c -o $(ODIR)/libdebug$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/libduel$(SUF).o: $(YC_PATH)/libduel.cpp
	$(CC) $(FLAGS) $(YC_PATH)/libduel.cpp -c -o $(ODIR)/libduel$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/libeffect$(SUF).o: $(YC_PATH)/libeffect.cpp
	$(CC) $(FLAGS) $(YC_PATH)/libeffect.cpp -c -o $(ODIR)/libeffect$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/libgroup$(SUF).o: $(YC_PATH)/libgroup.cpp
	$(CC) $(FLAGS) $(YC_PATH)/libgroup.cpp -c -o $(ODIR)/libgroup$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/operations$(SUF).o: $(YC_PATH)/operations.cpp
	$(CC) $(FLAGS) $(YC_PATH)/operations.cpp -c -o $(ODIR)/operations$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/playerop$(SUF).o: $(YC_PATH)/playerop.cpp
	$(CC) $(FLAGS) $(YC_PATH)/playerop.cpp -c -o $(ODIR)/playerop$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/processor$(SUF).o: $(YC_PATH)/processor.cpp
	$(CC) $(FLAGS) $(YC_PATH)/processor.cpp -c -o $(ODIR)/processor$(SUF).o -I$(LUA_INCLUDE_PATH)

$(ODIR)/scriptlib$(SUF).o: $(YC_PATH)/scriptlib.cpp
	$(CC) $(FLAGS) $(YC_PATH)/scriptlib.cpp -c -o $(ODIR)/scriptlib$(SUF).o -I$(LUA_INCLUDE_PATH)




$(ODIR)/ddd$(SUF).o: test.cpp dddutil.hpp dddapi.hpp dddstatemonitor.hpp
	$(CC) $(FLAGS) test.cpp -c -o $(ODIR)/ddd$(SUF).o -I./$(YC_PATH) -I$(LUA_INCLUDE_PATH) -I$(PYTHON_INCLUDE_PATH)

$(ODIR)/dddutil$(SUF).o: dddutil.cpp dddutil.hpp json.hpp
	$(CC) $(FLAGS) dddutil.cpp -c -o $(ODIR)/dddutil$(SUF).o -I./$(YC_PATH) -I$(LUA_INCLUDE_PATH) -I$(PYTHON_INCLUDE_PATH)

$(ODIR)/dddsingleton$(SUF).o: dddsingleton.cpp dddsingleton.hpp
	$(CC) $(FLAGS) dddsingleton.cpp -c -o $(ODIR)/dddsingleton$(SUF).o -I./$(YC_PATH) -I$(LUA_INCLUDE_PATH) -I$(PYTHON_INCLUDE_PATH)

$(ODIR)/ddddebug$(SUF).o: ddddebug.cpp ddddebug.hpp
	$(CC) $(FLAGS) ddddebug.cpp -c -o $(ODIR)/ddddebug$(SUF).o -I./$(YC_PATH) -I$(LUA_INCLUDE_PATH) -I$(PYTHON_INCLUDE_PATH)

$(ODIR)/dddstatemonitor$(SUF).o: dddstatemonitor.cpp dddstatemonitor.hpp
	$(CC) $(FLAGS) dddstatemonitor.cpp -c -o $(ODIR)/dddstatemonitor$(SUF).o -I./$(YC_PATH) -I$(LUA_INCLUDE_PATH) -I$(PYTHON_INCLUDE_PATH)

$(ODIR)/dddapi$(SUF).o: dddapi.cpp dddapi.hpp $(YC_PATH)/ocgapi.cpp $(YC_PATH)/ocgapi.h
	$(CC) $(FLAGS) dddapi.cpp -c -o $(ODIR)/dddapi$(SUF).o -I./$(YC_PATH) -I$(LUA_INCLUDE_PATH) -I$(PYTHON_INCLUDE_PATH)

$(ODIR)/dddapiutil$(SUF).o: dddapiutil.cpp dddapiutil.hpp
	$(CC) $(FLAGS) dddapiutil.cpp -c -o $(ODIR)/dddapiutil$(SUF).o -I./$(YC_PATH) -I$(LUA_INCLUDE_PATH) -I$(PYTHON_INCLUDE_PATH)

$(ODIR)/dddapistate$(SUF).o: dddapistate.cpp dddapistate.hpp
	$(CC) $(FLAGS) dddapistate.cpp -c -o $(ODIR)/dddapistate$(SUF).o -I./$(YC_PATH) -I$(LUA_INCLUDE_PATH) -I$(PYTHON_INCLUDE_PATH)






# statically compile dddapi library into executable
#  (other dependencies however are dynamically linked)
ddd_static$(EXE_EXT): $(ODIR)/card$(SUF).o $(ODIR)/duel$(SUF).o $(ODIR)/effect$(SUF).o $(ODIR)/field$(SUF).o $(ODIR)/group$(SUF).o $(ODIR)/interpreter$(SUF).o $(ODIR)/libcard$(SUF).o $(ODIR)/libdebug$(SUF).o $(ODIR)/libduel$(SUF).o $(ODIR)/libeffect$(SUF).o $(ODIR)/libgroup$(SUF).o $(ODIR)/operations$(SUF).o $(ODIR)/playerop$(SUF).o $(ODIR)/processor$(SUF).o $(ODIR)/scriptlib$(SUF).o $(ODIR)/ddd$(SUF).o $(ODIR)/dddutil$(SUF).o $(ODIR)/dddsingleton$(SUF).o $(ODIR)/ddddebug$(SUF).o $(STATEMONITOR_DEP) $(ODIR)/dddapi$(SUF).o $(ODIR)/dddapiutil$(SUF).o $(ODIR)/dddapistate$(SUF).o
	$(CC) $(FLAGS) $(ODIR)/card$(SUF).o $(ODIR)/duel$(SUF).o $(ODIR)/effect$(SUF).o $(ODIR)/field$(SUF).o $(ODIR)/group$(SUF).o $(ODIR)/interpreter$(SUF).o $(ODIR)/libcard$(SUF).o $(ODIR)/libdebug$(SUF).o $(ODIR)/libduel$(SUF).o $(ODIR)/libeffect$(SUF).o $(ODIR)/libgroup$(SUF).o $(ODIR)/operations$(SUF).o $(ODIR)/playerop$(SUF).o $(ODIR)/processor$(SUF).o $(ODIR)/scriptlib$(SUF).o $(ODIR)/ddd$(SUF).o $(ODIR)/dddutil$(SUF).o $(ODIR)/dddsingleton$(SUF).o $(ODIR)/ddddebug$(SUF).o $(STATEMONITOR_DEP) $(ODIR)/dddapi$(SUF).o $(ODIR)/dddapiutil$(SUF).o $(ODIR)/dddapistate$(SUF).o -o ddd$(EXE_EXT) -L$(LUA_LIB_PATH) -l$(LUA_LIB) -L$(PYTHON_LIB_PATH) -l$(PYTHON_LIB) -L$(SQLITE3_LIB_PATH) -l$(SQLITE3_LIB)


#compile ddd executable and dynamically link to dddapi library
ddd_dynamic$(EXE_EXT): $(ODIR)/card$(SUF).o $(ODIR)/duel$(SUF).o $(ODIR)/effect$(SUF).o $(ODIR)/field$(SUF).o $(ODIR)/group$(SUF).o $(ODIR)/interpreter$(SUF).o $(ODIR)/libcard$(SUF).o $(ODIR)/libdebug$(SUF).o $(ODIR)/libduel$(SUF).o $(ODIR)/libeffect$(SUF).o $(ODIR)/libgroup$(SUF).o $(ODIR)/operations$(SUF).o $(ODIR)/playerop$(SUF).o $(ODIR)/processor$(SUF).o $(ODIR)/scriptlib$(SUF).o $(ODIR)/ddd$(SUF).o $(ODIR)/dddutil$(SUF).o $(ODIR)/dddsingleton$(SUF).o $(ODIR)/ddddebug$(SUF).o $(STATEMONITOR_DEP)
	$(CC) $(FLAGS) -Wl,-R,. $(ODIR)/card$(SUF).o $(ODIR)/duel$(SUF).o $(ODIR)/effect$(SUF).o $(ODIR)/field$(SUF).o $(ODIR)/group$(SUF).o $(ODIR)/interpreter$(SUF).o $(ODIR)/libcard$(SUF).o $(ODIR)/libdebug$(SUF).o $(ODIR)/libduel$(SUF).o $(ODIR)/libeffect$(SUF).o $(ODIR)/libgroup$(SUF).o $(ODIR)/operations$(SUF).o $(ODIR)/playerop$(SUF).o $(ODIR)/processor$(SUF).o $(ODIR)/scriptlib$(SUF).o $(ODIR)/ddd$(SUF).o $(ODIR)/dddutil$(SUF).o $(ODIR)/dddsingleton$(SUF).o $(ODIR)/ddddebug$(SUF).o $(STATEMONITOR_DEP) -o ddd$(EXE_EXT) -L$(LUA_LIB_PATH) -l$(LUA_LIB) -L$(PYTHON_LIB_PATH) -l$(PYTHON_LIB) -L$(SQLITE3_LIB_PATH) -l$(SQLITE3_LIB) -ldddapi


#compile the dddapi library
$(LIB_PRF)dddapi$(LIB_EXT): $(ODIR)/card$(SUF).o $(ODIR)/duel$(SUF).o $(ODIR)/effect$(SUF).o $(ODIR)/field$(SUF).o $(ODIR)/group$(SUF).o $(ODIR)/interpreter$(SUF).o $(ODIR)/libcard$(SUF).o $(ODIR)/libdebug$(SUF).o $(ODIR)/libduel$(SUF).o $(ODIR)/libeffect$(SUF).o $(ODIR)/libgroup$(SUF).o $(ODIR)/operations$(SUF).o $(ODIR)/playerop$(SUF).o $(ODIR)/processor$(SUF).o $(ODIR)/scriptlib$(SUF).o $(ODIR)/dddapi$(SUF).o $(ODIR)/dddapiutil$(SUF).o $(ODIR)/dddapistate$(SUF).o $(ODIR)/dddutil$(SUF).o $(ODIR)/dddsingleton$(SUF).o $(ODIR)/ddddebug$(SUF).o
	$(CC) $(FLAGS) $(ODIR)/card$(SUF).o $(ODIR)/duel$(SUF).o $(ODIR)/effect$(SUF).o $(ODIR)/field$(SUF).o $(ODIR)/group$(SUF).o $(ODIR)/interpreter$(SUF).o $(ODIR)/libcard$(SUF).o $(ODIR)/libdebug$(SUF).o $(ODIR)/libduel$(SUF).o $(ODIR)/libeffect$(SUF).o $(ODIR)/libgroup$(SUF).o $(ODIR)/operations$(SUF).o $(ODIR)/playerop$(SUF).o $(ODIR)/processor$(SUF).o $(ODIR)/scriptlib$(SUF).o $(ODIR)/dddapi$(SUF).o $(ODIR)/dddapiutil$(SUF).o $(ODIR)/dddapistate$(SUF).o $(ODIR)/dddutil$(SUF).o $(ODIR)/dddsingleton$(SUF).o $(ODIR)/ddddebug$(SUF).o -shared -o $(LIB_PRF)dddapi$(LIB_EXT) -L$(LUA_LIB_PATH) -l$(LUA_LIB) -L$(PYTHON_LIB_PATH) -l$(PYTHON_LIB) -L$(SQLITE3_LIB_PATH) -l$(SQLITE3_LIB)
