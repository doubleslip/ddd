# utils.py

# General functions, including those that use the dddapi shared library

import sys
import os.path
import ctypes
import json


# Load the dddapi lib/dll at the path provided, sets the ctypes arg
#  types and return types and returns the object
def get_dddapi_from_lib(lib_path):
    dddapi = None
    ops = os.path.split(lib_path)
    
    # check possible names of the dddapi lib based on platform
    if sys.platform.startswith('linux'):
        path_candidates = [
            os.path.abspath(
                os.path.join(ops[0], 'lib' + ops[1] + '.so')),
            os.path.abspath(os.path.join(ops[0], ops[1] + '.so')),
            os.path.abspath(os.path.join(ops[0], ops[1]))
        ]
        for pc in path_candidates:
            if os.path.isfile(pc):
                dddapi = ctypes.CDLL(pc)
                print('Loaded "' + lib_path + '" at "' + pc + '".')
                break
        
    elif (sys.platform.startswith('win32') or
          sys.platform.startswith('cygwin')):
        path_candidates = [
            os.path.abspath(os.path.join(ops[0], ops[1] + '.dll')),
            os.path.abspath(os.path.join(ops[0], ops[1]))
        ]
        for pc in path_candidates:
            if os.path.isfile(pc):
                # need to specify winmode due to a change in Python 3.8
                dddapi = ctypes.WinDLL(pc, winmode = 0)
                print('Loaded "' + lib_path + '" at "' + pc + '".')
                break

    # Set ctypes args and return type
    dddapi.py_set_conf_and_init.argtypes = [ctypes.py_object, ctypes.py_object]
    dddapi.py_set_conf_and_init.restype = ctypes.py_object
    dddapi.py_create_duel_state.argtypes = [ctypes.py_object]
    dddapi.py_create_duel_state.restype = ctypes.py_object
    dddapi.py_create_duel_state_from_seed.argtypes = [ctypes.py_object, ctypes.py_object]
    dddapi.py_create_duel_state_from_seed.restype = ctypes.py_object
    dddapi.py_get_duel_states.argtypes = None
    dddapi.py_get_duel_states.restype = ctypes.py_object
    dddapi.py_duplicate_duel_state.argtypes = [ctypes.py_object]
    dddapi.py_duplicate_duel_state.restype = ctypes.py_object
    dddapi.py_duplicate_duel_state_reusing.argtypes = [ctypes.py_object, ctypes.py_object]
    dddapi.py_duplicate_duel_state_reusing.restype = ctypes.py_object
    dddapi.py_is_duel_state_pduel_duplicatable.argtypes = [ctypes.py_object]
    dddapi.py_is_duel_state_pduel_duplicatable.restype = ctypes.py_object
    dddapi.py_assume_duel_state.argtypes = [ctypes.py_object]
    dddapi.py_assume_duel_state.restype = ctypes.py_object
    dddapi.py_remove_duel_state.argtypes = [ctypes.py_object]
    dddapi.py_remove_duel_state.restype = ctypes.py_object
    dddapi.py_remove_duel_states.argtypes = [ctypes.py_object]
    dddapi.py_remove_duel_states.restype = ctypes.py_object
    dddapi.py_process_duel_state.argtypes = [ctypes.py_object]
    dddapi.py_process_duel_state.restype = ctypes.py_object
    dddapi.py_get_choices_from_duel_state.argtypes = [ctypes.py_object, ctypes.py_object, ctypes.py_object]
    dddapi.py_get_choices_from_duel_state.restype = ctypes.py_object
    dddapi.py_get_logs_from_cache.argtypes = None
    dddapi.py_get_logs_from_cache.restype = ctypes.py_object
    dddapi.py_set_duel_state_responsei.argtypes = [ctypes.py_object, ctypes.py_object]
    dddapi.py_set_duel_state_responsei.restype = None
    dddapi.py_set_duel_state_responseb.argtypes = [ctypes.py_object, ctypes.py_object]
    dddapi.py_set_duel_state_responseb.restype = None
    dddapi.py_get_field_visual_gamestate.argtypes = [ctypes.py_object]
    dddapi.py_get_field_visual_gamestate.restype = ctypes.py_object
    dddapi.py_is_duel_state_active.argtypes = [ctypes.py_object]
    dddapi.py_is_duel_state_active.restype = ctypes.py_object
    dddapi.py_reactivate_duel_state.argtypes = [ctypes.py_object]
    dddapi.py_reactivate_duel_state.restype = ctypes.py_object
    dddapi.py_deactivate_duel_state.argtypes = [ctypes.py_object]
    dddapi.py_deactivate_duel_state.restype = ctypes.py_object
    dddapi.py_get_dcs_command.argtypes = None
    dddapi.py_get_dcs_command.restype = ctypes.py_object
    dddapi.py_load_dcs_commands.argtypes = [ctypes.py_object]
    dddapi.py_load_dcs_commands.restype = None
    dddapi.py_clear_dcs_commands.argtypes = None
    dddapi.py_clear_dcs_commands.restype = None
    dddapi.py_get_choice_filter.argtypes = [ctypes.py_object, ctypes.py_object]
    dddapi.py_get_choice_filter.restype = ctypes.py_object
    
    return dddapi


# Legacy alias for get_dddapi_from_lib()
def get_dddapi_from_dll(dll_path):
    return get_dddapi_from_lib(dll_path)


# Load and parse conf json file, then return an object containing
#  values set and set some default values if not specified
#  (does not warn of missing mandatory values as the ddddapi
#   py_set_conf_and_init function should be called beforehand)
#  (does however warns of options that aren't available in the script)
def load_conf_file(conf_path):
    with open(conf_path, 'r') as conf_json:
        dddgs = json.load(conf_json)

        # Set defaults
        if not 'autoProcess' in dddgs:
            dddgs['autoProcess'] = False

        if not 'echoDcsCommand' in dddgs:
            dddgs['echoDcsCommand'] = False
        
        # Warns of unavailable options
        if 'logToFile' in dddgs:
            if (dddgs['logToFile']):
                print('  (note: option logToFile enabled in conf file'
                      ' but option not available in script; ignoring)')

        if 'logToFile' in dddgs and 'logFile' in dddgs:
            if (dddgs['logFile']):
                print('  (note: option logFile enabled in conf file'
                      ' but option not available in script; ignoring)')

        if 'useFixedSeed' in dddgs:
            if (dddgs['useFixedSeed']):
                print('  (note: option useFixedSeed enabled in conf'
                      ' file but option not available in script;'
                      ' ignoring)')
        
        return dddgs

# Get a y/n response from user (returning True for y and False for n).
def confirm_yn(dddapi, q):
    while True:
        p = dddapi.py_get_dcs_command()
        try:
            if p[0] != '':
                tpl = parse_dcs_cmd(p[0])
                res = tpl[0]
            else:
                res = input(' ->   ' + q + ' [y/n]: ')
                res = res.strip().lower()
                
            if (res == 'y'):
                return True
            elif (res == 'n'):
                return False
            else:
                print('Invalid response "' + res + '".')

        except Exception as e:
            print('Invalid response "' + res + '".')
            print('  (' + str(e) + ')')


# String + len to the more 'recognizable'(?) 'dddapi test' format.
def to_bytearray(ba, ba_len):
    return list(bytes(ba)[:ba_len])


# Parse string to bool as per custom rules
def parse_bool(b):
    if b.lower() in ['true', 't']:
        return True
    if b.lower() in ['false', 'f']:
        return False
    return bool(int(b))


# Remove immediate surrounding quotes from str (if they match)
def dequote_str(s):
    ss = s.strip()
    if len(ss) < 2:
        return s
    if ss[0] == '\'' and ss[len(ss) - 1] == '\'':
        return ss[1:-1]
    if ss[0] == '"' and ss[len(ss) - 1] == '"':
        return ss[1:-1]
    return s


# Prints a list of available choices.
def print_available_choices(choices):
    print('Available choices to select:')
    for i in range(0, len(choices), 1):
        print('  [Choice ' + str(i) + '] ' + choices[i])


# Set response values based on message and selected choices
def set_response(dddapi, dsid, choices, selected_choices):
    msg = choices[1][0]
    if msg == 15:
        _set_response_15(dddapi, dsid, selected_choices)
    elif msg == 23:
        _set_response_23(dddapi, dsid, choices, selected_choices)
    else:
        _set_response_gen(dddapi, dsid, choices, selected_choices)


# Allow user to input selected choices and then sets values based
#  on the selected choices
def set_response_by_input(dddapi, dsid, choices, choice_str):
    min_choices = 1
    max_choices = 1
    must_select_size = 0
    input_str = ''
    msg = choices[1][0]

    # Handle variables in specific messages
    if msg == 15:  # MSG_SELECT_CARD
        if len(choices[1]) < 4:
            print('Malformed extras received for msg 15:', choices[1])
            return False

        # Set values from extras
        min_choices = choices[1][1]
        max_choices = choices[1][2]
    
    elif msg == 23:  # MSG_SELECT_SUM
        if len(choices[1]) < 6:
            print('Malformed extras received for msg 23:', choices[1])
            return False
        
        # Set values from extras
        min_choices = choices[1][1]
        max_choices = choices[1][2]
        must_select_size = choices[1][5]

        # Check if no limit and if not, set to total choices
        if choices[1][3] == 0:
            # maybe also check if max_val == 99?
            max_val = len(choices[0])

    # Generate hint string to help user when inputting choice(s)
    if min_choices > 1 or min_choices != max_choices:
        input_str = 'Select '
        if (msg == 23) and (choices[1][3] == 0):
            input_str += '(' + str(min_choices) + '+)'
        elif min_choices != max_choices:
            input_str += ('(' + str(min_choices) + ' - '
                          + str(max_choices) + ')')
        else:
            input_str += '(' + str(min_choices) + ')'
            
        input_str += ' choices (separate multiple choices by commas)'
        
    else:
        input_str = 'Select a choice'
    
    # Print hint string
    print('\n  '.join(choices[2]))
    
    # Print available choices
    if choice_str == '':
        print_available_choices(choices[3])
        
        # Get choices from user
        try:
            p = dddapi.py_get_dcs_command()
            if p[0] == '':
                choice_str = input(' ->  ' + input_str + ': ')
            else:
                choice_str = p[0]
                
        except Exception as e:
            print('Invalid input "' + choice_str + '".')
            print('  (' + str(e) + ')')
            return False

    # Check if cancelled
    if (choice_str.strip().lower() == 'c' or
            choice_str.strip().lower() == 'cancel'):
        print('Cancelled by user.')
        return False

    # Format choices
    try:
        if choice_str.strip() == '':
            selected_choices = []
        else:
            # Convert string to list of unique int
            selected_choices = choice_str.split(',')
            selected_choices = [int(i) for i in selected_choices]
            selected_choices = {s for s in selected_choices}
            selected_choices = list(selected_choices)
        
    except Exception as e:
        print('Invalid input "' + choice_str + '".')
        print('  (' + str(e) + ')')
        return False
    
    # Validate choices
    try:
        if (len(selected_choices) + must_select_size) < min_choices:
            print('Not enough choices selected (' +
                  str(len(selected_choices)) + ' given; need at least '
                  + str(min_choices) + ').')
            return False
        
        if (len(selected_choices) + must_select_size) > max_choices:
            print('Too many choices selected (' +
                  str(len(selected_choices)) + ' given; cannot select'
                  ' more than ' + str(max_choices) + ').')
            return False

        for s in selected_choices:
            if (((s) < 0) or
                    (s >= (len(choices[0]) - must_select_size))):
                print('Invalid choice "' + str(s) + '" selected.')
                return False
        
        # Handle input based on msg with a bit more possible validation
        set_response(dddapi, dsid, choices, selected_choices)
        
        # Print selected choice(s) upon success
        if len(selected_choices) == 1:
            print('Selected:', choices[3][selected_choices[0]])
        else:
            print('Selected:')
            for s in selected_choices:
                print('  ' + choices[3][selected_choices[s]])
        
        return True

    except (ValueError, KeyboardInterrupt):
        print('Vals were not set.')
        return False


# Set response for message 15 for set_response functions
#  (internal use only)
def _set_response_15(dddapi, dsid, selected_choices):
    ba = bytearray(64)
    ba[0] = len(selected_choices)

    for i in range(1, 64, 1):
        if (i - 1) < len(selected_choices):
            ba[i] = selected_choices[i - 1]
        else:
            break
    
    dddapi.py_set_duel_state_responseb(dsid, ba)


# Set response for message 23 for set_response functions
#  (internal use only)
def _set_response_23(dddapi, dsid, choices, selected_choices):
    ba = bytearray(64)
    must_select_size = choices[1][5]
    ba[0] = len(selected_choices) + must_select_size
    
    for i in range(1, 64, 1):
        if (i - 1) < must_select_size:
            ba[i] = (i - 1)
        elif (i - must_select_size - 1) < len(selected_choices):
            ba[i] = selected_choices[i - must_select_size - 1]
        else:
            break
    
    dddapi.py_set_duel_state_responseb(dsid, ba)


# Generic set response for set_response functions
#  (internal use only)
def _set_response_gen(dddapi, dsid, choices, selected_choices):
    is_ivalue = type(choices[0][0]) == int
    if is_ivalue:
        dddapi.py_set_duel_state_responsei(
            dsid, choices[0][selected_choices[0]])
    else:
        dddapi.py_set_duel_state_responseb(
            dsid, choices[0][selected_choices[0]])

# Currently no checks on which player's choices are available
#  when returned.
def auto_process_until_choices_available(dddapi, dsid, print_hint_strings=False, check_first_process=False, end_msgs=set(), fil=[]):
    first_process_check = check_first_process
    while True:
        dddapi.py_process_duel_state(dsid)
        
        choices_temp = dddapi.py_get_choices_from_duel_state(
            dsid, True, fil)
        msg_temp = choices_temp[1][0]
        
        if print_hint_strings and (len(choices_temp[2]) > 0):
            print('\n  '.join(choices_temp[2]))
        
        # Exit loop by returning last msg and choices as tuple
        #  (either choices were found or got specific msg)
        if (len(choices_temp[0]) > 0):
            # Exit because choices available to be selected
            return (msg_temp, True, choices_temp)
        
        elif msg_temp == 1:
            # Exit because got error msg (MSG_RETRY)
            if check_first_process:
                # Only return true if enabled checking for error msg
                #  from first process iteration
                return (msg_temp, first_process_check, None)
            else:
                # Return false from error regardless
                return (msg_temp, False, None)
            
        elif msg_temp in end_msgs:
            # Exit because reached an end msg
            return (msg_temp, True, choices_temp)
        
        if check_first_process:
            first_process_check = False


# Parses a string containing a dcs command into a tuple containing
#  its command and any arguments or flags
def parse_dcs_cmd(raw_str):
    tokens = []
    found_last = False
    in_squote = False
    in_dquote = False
    temp_str = raw_str

    # Continually parse temp_str for tokens
    while not found_last:
        # Inside single quote; looking for matching single quote
        if in_squote and not found_last:
            squote_mark = temp_str.find('\'')
            if squote_mark != -1:
                # Found matching single quote; extract text before
                #  single quote and slice temp_str after quote
                tokens.append('"' + temp_str[:squote_mark] + '"')
                temp_str = temp_str[squote_mark + 1:]
                in_squote = False
                continue
            else:
                # No matching single quote; append single quote to
                #  end and assume rest of temp_str is wrapped in it
                tokens.append(temp_str)
                found_last = True
                break
        
        # Inside double quote; looking for matching single quote
        if in_dquote and not found_last:
            dquote_mark = temp_str.find('"')
            if dquote_mark != -1:
                # Found matching double quote; extract text before
                #  double quote and slice temp_str after quote
                tokens.append('"' + temp_str[:dquote_mark] + '"')
                temp_str = temp_str[dquote_mark + 1:]
                in_dquote = False
                continue
            else:
                # No matching double quote; append single quote to
                #  end and assume rest of temp_str is wrapped in it
                tokens.append(temp_str)
                found_last = True
                break

        # Regular parsing (not in any quotes)
        if not found_last:
            for i in range(0, len(temp_str) + 1, 1):
                if i == len(temp_str):
                    tokens.append(temp_str)
                    found_last = True
                    break
                
                if temp_str[i] == ' ':
                    # Extract contents before space
                    tokens.append(temp_str[:i])
                    temp_str = temp_str[i + 1:]
                    break
                
                elif temp_str[i] == '=' or temp_str[i] == ',':
                    # Extract contents before '=' or ',' and also
                    #  extract the '=' or ',' itself
                    tokens.append(temp_str[:i])
                    tokens.append(temp_str[i])
                    temp_str = temp_str[i + 1:]
                    break
                
                elif temp_str[i] == '\'':
                    # Extract contents before single quote then
                    #  set flag indicating inside single quote
                    tokens.append(temp_str[:i])
                    temp_str = temp_str[i + 1:]
                    in_squote = True
                    break
                
                elif temp_str[i] == '"':
                    # Extract contents before double quote then
                    #  set flag indicating inside double quote
                    tokens.append(temp_str[:i])
                    temp_str = temp_str[i + 1:]
                    in_dquote = True
                    break

            # Can immediate exit if temp_str empty
            if temp_str == '':
                found_last = True
    
    args_tokens = []
    cmd = ''
    cmd_buffer = ''
    args = dict()
    flags = set()

    # Find cmd+flags and non empty args tokens
    for t in tokens:
        tst = t.strip()
        if tst != '':
            if cmd_buffer == '':
                cmd_buffer = tst
            else:
                args_tokens.append(tst)

    # Separate cmd and flags
    flags_list = set(['!'])
    found_flag = False
    for c in cmd_buffer:
        # Treat everything past first valid flag as a flag
        if c in flags_list:
            found_flag = True
        if found_flag:
            flags.add(c)
        else:
            cmd += c

    # Parse args tokens into args map
    args_tokens_buffer = []
    for i in range(0, len(args_tokens) + 1, 1):
        # Set current token in args buffer
        if i < len(args_tokens):
            t = args_tokens[i]
        else:
            # 1 past last token in args buffer; set token as ','
            #  (delimiter) to parse the last token
            t = ','

        # Handle tokens (other than ',' (delimiter))
        if t != ',':
            # Only add current token to token buffer
            args_tokens_buffer.append(t)
            
        elif len(args_tokens_buffer) > 0:
            # If token buffer not empty, parse current token(s)
            #  in token buffer to args into a key and value pair
            k = ''
            v = ''

            # Initial parsing of token buffer to key and value
            expects_val = False
            for d in args_tokens_buffer:
                if d == '=':
                    expects_val = True
                elif not expects_val:
                    k = d
                else:
                    v = d

            # If value not found, interpret it based on previous
            #  tokens (in tokens buffer)
            if v == '':
                if expects_val:
                    # Got key and '=' token but no value
                    #  Assume blank/False
                    v = '0'
                else:
                    # Only got key token; assume as key set to 1 (True)
                    v = '1'
                
            args_tokens_buffer = []  # Clear token buffer
            args[k] = v  # Actually add key and value to args dict
        
    return (cmd, args, flags)


# Output all possible combination of currently selectable choices
#  (accounting for message type)
def brute_force_possible_choices(choices):
    possible_choices = []
    min_val = 1
    max_val = 1
    total_choices = len(choices[0])
    fil = choices[4][:]
    msg = 0
    
    if len(choices[1]) > 0:
        msg = choices[1][0]

    # Adjust some values for some specific messages
    if msg == 15:  # MSG_SELECT_CARD
        if len(choices[1]) < 4:
            print('Malformed extras received for msg 15:')
            print(choices[1])
            return
        min_val = choices[1][1]
        max_val = choices[1][2]
        
    elif msg == 23:  # MSG_SELECT_SUM
        if len(choices[1]) < 6:
            print('Malformed extras received for msg 23:')
            print(choices[1])
            return
        min_val = choices[1][1]
        max_val = choices[1][2]
        must_select_size = choices[1][5]
        
        total_choices -= must_select_size
        fil = fil[must_select_size:]
        
        # Check if limit actually specified or not
        if choices[1][3] == 0: # ...maybe also check if max_val == 99?
            # No limit specified; cap max at total_choices
            max_val = total_choices
        
    # Iterate min and max values inclusively (ignoring 0)
    for curr_num_choices in range(min_val, (max_val + 1), 1):
        possible_choices = brute_force_possible_choices_inner(
            total_choices, curr_num_choices, possible_choices, [], fil)
        
    # Push back selecting 0 choices only in specific cases
    if (msg == 15 or  # MSG_SELECT_CARD
        msg == 26 or  # MSG_SELECT_UNSELECT_CARD
        msg == 16 or  # MSG_SELECT_CHAIN
        msg == 20):   # MSG_SELECT_TRIBUTE
        possible_choices.append([])

    # ...maybe also push selecting -1 as a choice in specific cases
    #      (would also require changes to the set_response functions
    #       to handle this)
    
    return possible_choices


# For internal use only for brute_force_possible_choices
def brute_force_possible_choices_inner(total_choices, curr_num_choices, results=[], buf=[], fil=[]):
    depth = len(buf)
    # buf at maximum length, return it as a possible choice
    if depth == curr_num_choices:
        if depth > 0: # ...but don't return 0 length (for now)
            results.append(buf[:])  # gotta add a different copy of list
        return results
    
    # Get current depth maximum value it can be incremented to
    if depth > 0:
        start_range = max(depth, buf[depth - 1] + 1)
    else:
        start_range = depth
    
    end_range = total_choices - curr_num_choices + depth
    
    # Iterate between values recursively
    for i in range(start_range, (end_range + 1), 1):
        # Ignore if current choice is in filter
        if fil[i] == 0:
            continue
        
        # Push current variable to buffer and recursively call
        #  function, getting the updating results
        buf.append(i)
        results = brute_force_possible_choices_inner(
            total_choices, curr_num_choices, results, buf, fil)
        buf.pop()
    
    return results
    
