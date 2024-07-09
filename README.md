D/D/D - Declan's Devious Device
=====

Declan's/Devious/Device (ddd for short) is state management system using [ygopro-core](https://github.com/Fluorohydride/ygopro-core), which simulates the gamestate. State management includes the duplication of states, allowing for possibilities such as the exploration or traversal of the game space.

## Dependencies
ddd requires the following to build:
- the ygopro-core [source files](https://github.com/Fluorohydride/ygopro-core/archive/b16497d321ef83e6e2a808205647951d22c2afce.zip)
- g++ (with at least C++17)
- GNU make
- Python 3
- Sqlite 3
- Lua 5.4
- [nholmann json](https://github.com/nlohmann/json)

While not required to build, ddd also requires the following to function properly:
- [ygopro scripts](https://github.com/Fluorohydride/ygopro-scripts/archive/4e0fbaf3c8e51473317a8c69d766e31d719af3d3.zip)
- [cards.db file](https://github.com/mycard/ygopro-database/archive/b4dd6c09e48479d50648ca6d54e0598a96f1a40f.zip)
- At least one `.ydk` deck file (2 sample ones are provided in the samples directory)


## Building and Setup
#### To build ddd:
1. Gather/install the [dependencies](#dependencies) to build.
2. In the makefile, set the `YC_PATH` variable to the directory containing the ygopro-core source files.
3. Configure the paths in the makefile for Python, Sqlite and Lua dependencies if necessary.
    - If those dependencies are not in your compiler's search path, you will to specify them in the makefile. Each of those dependencies has its own `*_INCLUDE_PATH`, `*_LIB` and `*_LIB_PATH` variable that can be set.
    - `*_INCLUDE_PATH` is the path for the directory where the header files for the dependency are located.
    - `*_LIB` is the name of the dependency's shared libary file. On windows, this is the name of the library without the `.dll` extension. On Linux, this is the name of the library without the `lib` prefix or any extensions.
    - `*_LIB_PATH` is the path where the corresponding `*_LIB` file is located. The variable can be set to just `.` if a copy of the shared library file is in the same directory as the program.

4. If building for windows, set the `OPT_COMPILE_MINGW` variable in the makefile to `1`; otherwise, set it to `0`. 
5. Ensure the [nholmann json](https://github.com/nlohmann/json) `json.hpp` file is also in the same directory as the makefile.
6. Run `make all`.

If successful, should result in a shared library (dddapi) and an executable (ddd). The shared library is used by the executable (unless statically compiled) and the Python implementation.

If the compiler was unable to find one of the dependencies, you may need to modify the `*_INCLUDE_PATH` variable for the missing dependency in the makefile. If the compiler was unable to link to one of the dependencies, you may need to modify either the `*_LIB` or `*_LIB_PATH` variables for the dependency in the makefile.

> If you specifically encountered *undefined reference* linking errors involving lua functions and are certain your paths are correct, you can try the following steps:
> 1. Use the provided modified lua headers in the `samples` directory by setting the `LUA_INCLUDE_PATH` makefile variable to `./samples/lua_54_modified_include`.
> 2. Run `make clean`.
> 3. Run `make all`.
>
> Or if you prefer to modify them yourself:
> 1. Make a copy of your lua 5.4's `include` directory somewhere.
> 2. In the copied directory, for each of the following 3 files `lua.h`, `lualib.h` and `lauxlib.h`:
>     1. Add `extern "C" {` at the very beginning of the file, on its own line.
>     2. Add a single `}` at the very end of the file, on its own line.
> 3. In the makefile, set the `LUA_INCLUDE_PATH` variable to the path of the copied directory.
> 4. Run `make clean`.
> 5. Run `make all`.

#### To set up (after building):

1. Ensure the conf file `ddd_conf.json` file exists in the same directory as the `dddapi` executable or Python script (whichever you plan to use).
2. Extract your [ygopro scripts](https://github.com/Fluorohydride/ygopro-scripts/archive/4e0fbaf3c8e51473317a8c69d766e31d719af3d3.zip) to a directory in the same directory as the `dddapi` library and name that directory `script`.
3. Extract your [cards.db file](https://github.com/mycard/ygopro-database/archive/b4dd6c09e48479d50648ca6d54e0598a96f1a40f.zip) with the language of your choice.
4. In the conf file, add/set the `cardsCdbPath` member to the location of your cards.db file (relative or absolute).
5. In the conf file, add/set the `player0DeckPath` and `player1DeckPath` to the location of your `.ydk` deck files (2 sample ydk files are provided in the samples directory).
6. Run the executable `ddd` or Python script `python ddd.py` to verify it works (if it does not exit, no errors are shown and you are able to enter commands, congratulations! You've successfully built and have set it up.).
    - If the program exits without displaying an error, you may be missing a dependency. To resolve this, you can try making a copy of the shared library for Python, Sqlite or Lua in the same directory as the program and `dddapi` library.


## Using
ddd provides an api for C++ and a Python library that uses the api via ctypes, as well as an interactive C++ commandline program and a Python script that makes use of the library. A program can use the library by being linked like any other shared library/dll. A Python script can use the Python library by importing it with `import dddpy as ddd` when both the shared library/dll and the `dddpy` directory are in (or copied to) the same directory as the Python script. The interactive mode program/script to manage the states also comes with various other systems, including additional functionality to automate commands via [dcs scripts](#dcs-scripts). The C++ program has some extra functionality that is beyond the scope of the library whereas the Python script only makes use of the dddapi functions. Otherwise, both are used in more or less the same way.

#### Conf file
Conf files are json files and are necessary to initialize the dddapi library before it can be used. At minimum, the conf_file must define the `cardsCdbPath`, `player0DeckPath` and `player1DeckPath` members. When using the [dddapi library](#api), it must first be initialized with calling the `(py_)set_conf_and_init` with the path to the conf file. A sample conf file (`ddd_conf.json`) is included in the project root. The interactive commandline program and Python script requires checks its current directory for `ddd_conf.json` and uses it. A different conf file can be used instead when run with the `--conf-file` flag, followed by the path to the conf file.

#### Commands
Commands are used to interact with the interactive mode in the executable and Python script. Commands available to the implementation can be found using the `help` command. The interactive mode an also be automated via a [dcs script](#dcs-scripts) which are simple scripts that contain only commands. Some commands can also accept arguments when they are called.

The syntax for commands with arguments is: `command arg1 = value1, arg2 = value2, ...`, such that commands, values and arguments are cAsE sEnSiTiVe. Depending on the arg, its corresponding value can be an integer, a boolean or string. Values for strings should be wrapped in quotes.

#### Dcs Scripts
Dcs scripts are simply text files that contain [commands](#commands) (one per line) and also support C-style comments (both `//single line` and `/*multiline*/`). Commands are identical to those entered in the interactive mode and are executed first to last. The `dcs` command in the interactive mode can also be used to load a dcs script. Sample dcs scripts to run are available in the `samples/dcs_scripts` directory.

#### Brute force
Both implementations have a sample depth-first search algorithm that can be called with the `bf` command that can be used to traverse the current game space and print all possible paths up to a certain depth with other constraints. This command supports the following arguments:
> `depth|maxDepth = [int]`
> - Specifies the maximum depth the algorithm should search to. The algorithm will not attempt to traverse duel states if they are beyond the specified depth. Defaults to `4` if this argument is not specified.

> `filter = [string]`
> - Specifies certain cards or actions to avoid using when brute forcing the duel state. This can be useful in significantly pruning the game space and increasing traversal time. Cards are specified based on the current choices available to be selected for the current duel state. The value for filter is a string containing a comma-separated list of choices to filter out. Defaults to `""` (no filter) if this argument is not specified. The filter is also applied at all states, not just the current one.
>
> For example:
>> Given current choices available for current duel state are:
>> 1. Normal summon `card A`
>> 2. Normal summon `card B`
>> 3. Normal summon `card C`
>> 4. Activate `card B`
>> 5. Activate `card D`
>>
>> - `bf` or `bf filter = ""` will explore all (5) choices.
>> - `bf filter = "1"` will explore all choices that do not involve `card A` because that was the card used in choice `1`, which was specified in the filter. In this example, only choice 1 involves `card A` so it will be the only choice that won't be explored.
>> - `bf filter = "2"` will explore all choices that do not involve `card B` because that was the card used in choice `2`, which was specified in the filter. In this example, both choices `2` and `4` involve `card B` so both those choices won't be explored, even though choice `4` was not specified in the filter.
>> - `bf filter = "2, 3"` will explore all choices that do not involve `card B` or `card C`.

> `maxDepthResultsLimit|resultsLimit = [int]`
> - Specifies the maximum number of results such that if enough results were found at the maximum specified depth (specified with `depth|maxDepth`), the search will terminate (even if there are potentially more) and will show the results currently found. If set to `0`, no limit is set. Defaults to `0` if this argument is not specified.

> `showResultsLimit|showLimit = [int]`
> - Specify the maximum number of results to show when the algorithm finishes. If set to `0`, all results will be shown. Defaults to `0` if this argument is not specified.



## Api
ddd exposes certain functions to help manage and manipulate game states with many functions in both C++ and Python. The C++ bindings are available if a program was linked to the dddapi library when compiled. The Python bindings are available using `import dddpy as ddd` when the dddpy directory and dddapi library are in the same directory as the script. Python bindings are prefixed with `py_` and usually have the same arguments (except where noted). Here are the common and more useful functions:

#### Common Functions

> `bool set_conf_and_init(const char* conf_path, const bool show_non_errors)` \
> `py_set_conf_and_init(conf_path, show_non_errors)`
>
> - Initializes the library via `conf_path` which is a path to the [conf file](#conf-file). The `show_non_errors` variable can be enabled for extra verbose output while initializing. Returns true on success; false otherwise.
> - **This function should be called before using any other function in the library.**

> `uint64 create_duel_state(const bool show_non_errors)` \
> `py_create_duel_state(show_non_errors)`
>
> - Create a new duel state using settings specified in the conf file (using a random seed if not specified in the conf file). Returns the id to the newly created duel state; 0 otherwise.

> `uint64 create_duel_state_from_seed(const bool show_non_errors, const uint32 seed)` \
> `py_create_duel_state_from_seed(show_non_errors, seed)`
>
> - Create a new duel state using settings specified in the conf file but with a specified seed that overrides any seed specified in the conf file. Returns the id to the newly created duel state; 0 otherwise.

> `uint64 duplicate_duel_state(const uint64 source_dsid)` \
> `py_duplicate_duel_state(source_dsid)`
>
> - Creates a duplicate of the duel state based on the duel state id passed. If successful, returns the id of the newly created duel state; 0 otherwise.

> `uint64 remove_duel_state(const uint64 dsid)` \
> `py_remove_duel_state(dsid)`
>
> - Destroys the specified duel state and invalidates the duel state id from being used. Returns true on success; false otherwise.
>> To save memory but not completely lose access to the state, use the `(py_)deactivate_duel_state` function instead.

> `int32 get_duel_state_message(const uint64 dsid, byte* buffer)`
>
> - (**C++**): Copies the contents of the specified duel state's duel buffer to the buffer argument and returns the size of the buffer.
>
> `py_get_duel_state_message(dsid)`
>
> - (**Python**): Returns a bytearray containing the contents of the specified duel state's duel buffer.

> `void set_duel_state_responsei(const uint64 dsid, const int32 ivalue)` \
> `py_set_duel_state_responsei(dsid, ivalue)`
>
> - Set latest response in the specified duel state as an int32.

> `void set_duel_state_responseb(const uint64 dsid, const byte* bvalue)` \
> `py_set_duel_state_responseb(dsid, bvalue)`
>
> - Set latest response in the specified duel state as a byte array.

> `int32 process_duel_state(const uint64 dsid)` \
> `py_process_duel_state(dsid)`
>
> - Perform a process iteration on the specified duel state.

> `std::tuple<std::vector<DDDReturn>, std::vector<int>, std::vector<std::string>, std::vector<std::string>, std::vector<int>> get_choices_from_duel_state(const uint64 dsid, const bool generate_strings, const std::unordered_set<int>& filter)` \
> `py_get_choices_from_duel_state(dsid, generate_strings, filter)`
>
> - Returns the choices available to be selected in the current duel state, as well as whether the choice match the filter (optionally passed). If generate_strings is true, also returns the choice and message as human-readable text (if any, and over multiple lines).
> - The returned object is a tuple containing:
>   - [index 0]: An array of either an int32 or bytearray that can be used in `(py_)set_duel_state_responsei` or `(py_)set_duel_state_responseb` respectively.
>   - [index 1]: An array of int with a length of at least 1, such that the 0th index contains the msg and all other indicies may contain extra data specific to the msg.
>   - [index 2]: An array of strings containing human-readable text to describe the last action that just happened and any action the user can currently select.
>   - [index 3]: An array of strings containing human-readable text for the choices themselves. The indicies here correspond to those of index 0 (e.g. the string at index 2 of this array, corresponds with the int32/bytearray at index 2 of the index 0 array).
>   - [index 4]: An array of int containing matches for the filter that was passed to the function. The indicies here correspond to those of index 0 (e.g. if the value at index 4 of this array is 0, the int32/bytearray at index 4 of the index 0 array matches that of the filter passed to the function).

> `std::tuple<std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>, std::vector<std::string>> get_field_visual_gamestate(uint64 dsid)` \
> `py_get_field_visual_gamestate(dsid)`
>
> - Returns a text-based representation (over multiple lines) of the common elements of the present game state.
> - The returned object is a tuple containing:
>   - [index 0]: An array of strings that visually represents the field
>   - [index 1]: An array of strings that prints info for player 0's field
>   - [index 2]: An array of strings that prints info for player 1's field
>   - [index 3]: An array of strings that prints info for player 0
>   - [index 4]: An array of strings that prints info for player 1
>   - [index 5]: An array of strings that prints info for any chains that are active

#### Other functions

> `uint64 duplicate_duel_state_reusing(const uint64 source_dsid, const uint64 reuse_dsid)` \
> `py_duplicate_duel_state_reusing(source_dsid, reuse_dsid)`
>
> - Same as the (py_)duplicate_duel_state function but also passes an additional duel state id that is no longer needed and is used to create the new duel state. If successful, returns the id of the newly created duel state and deactivates the duel state to reuse; 0 otherwise.
> - Faster than (py_)duplicate_duel_state if duel state was successfully reused. If unable to reuse, functionally equivalent to `(py_)duplicate_duel_state` on the source duel state and then `(py_)deactivate_duel_state` on the duel state to reuse.
>> To reuse the source duel state with the same state it contains, use the `(py_)assume_duel_state` function instead.

> `uint64 assume_duel_state(const uint64 dsid)` \
> `py_assume_duel_state(dsid)`
>
> - Creates a new duel state whose state is the same as the specified duel state and deactivates the specified duel state. Returns the id to the newly created duel state; 0 otherwise.
> - Faster than and functionally equivalent to `(py_)duplicate_duel_state` and then `(py_)deactivate_duel_state` on the source duel state.

> `std::pair<std::string, int> get_dcs_command()` \
> `py_get_dcs_command()`
>
> - Returns the next dcs command in the singleton its line in the script, or an empty string if no more commands.

> `void load_dcs_commands(const char* src)` \
> `py_load_dcs_commands(src)`
>
> - Loads dcs commands from a dcs script file into the singleton and discards any previous dcs commands.

> `void clear_dcs_commands()` \
> `py_clear_dcs_commands()`
>
> - Discards any dcs commands in the singleton.

> `bool is_duel_state_active(const uint64 dsid)` \
> `py_is_duel_state_active(dsid)`
>
> - Returns if the specified duel state is active; false otherwise. Duel states that are active can be used as normal. Duel states that are deactivated cannot be used for any functions until they are reactivated. Deactivated duel states are freed of memory but still retain the means to recreate the duel state they represent when reactivated at a later time.

> `bool reactivate_duel_state(const uint64 dsid)` \
> `py_reactivate_duel_state(dsid)`
>
> - Attempts to reactivate a duel state. Returns true if successful or if duel state was already activated; false otherwise.

> `bool deactivate_duel_state(const uint64 dsid)` \
> `py_deactivate_duel_state(dsid)`
>
> - Attempts to deactivate a duel state. Returns true if successful; false otherwise.

> `std::unordered_set<int> get_choice_filter(const uint64 dsid, const std::unordered_set<int>& filter)` \
> `py_get_choice_filter(dsid, filter);`
>
> - Returns a filter object that can be passed to the `(py_)get_choices_from_duel_state` function. This will make the 5th index of the tuple returned from the `(py_)get_choices_from_duel_state` function match the filter that was returned from this function. The returned object is valid even in future calls of the `(py_)get_choices_from_duel_state` function for the same duel state. See the filter argument in the [brute force](#brute-force) section for more detail.

