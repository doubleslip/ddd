# dddtest.py
# Python implementation of test.cpp with subset of functionality

import sys
import time
import getopt
import dddpy as ddd


# Var declarations
dsid = 0  # current/active duel state
dsid_original = dsid  # main/original duel state for this script

stay_in_main_loop = True
lib_path = 'dddapi'
conf_path = './ddd_conf.json'
auto_dcs_script = ''  # to be initialized later
auto_start = True
auto_process = False
choices_check = False
interact_check = False

last_valid_msg = 0
last_valid_choices = ([], [0], [], [], [])
duel_and_states_dict = dict()

dddgs = None  # conf file settings



# # # Start of script # # #
print('Initializing...')


# Parse args passed to main function
short_options = 'dca'
long_options = ['dcs=', 'load-dcs-script=',
                'conf=', 'conf-file=',
                'auto-start=']
optlist, args = getopt.gnu_getopt(
    sys.argv[1:], short_options, long_options)

# Handle args (1st pass; only handle conf path)
for o, a in optlist:
    if o in ('-c', '--conf', '--conf-file'):
        conf_path = a
    else:
        # No error handling needed; message already printed
        #  and exception raised (to exit)
        pass


# Load conf file and also get dddapi obj
dddapi = ddd.get_dddapi_from_dll(lib_path)
if not dddapi.py_set_conf_and_init(conf_path, True):
    print('Error loading or using conf file at path "' + conf_path +
          '"; exiting.')
    exit()
dddgs = ddd.load_conf_file(conf_path)
# auto_process = dddgs['autoProcess']


# Handle args (2nd pass; handle everything else)
#  (vals set here override that of conf file)
for o, a in optlist:
    if o in ('-d', '--dcs', '--load-dcs-script'):
        auto_dcs_script = a
    elif o in ('-a', '--auto-script'):
        auto_start = ddd.parse_bool(a)


# # # Main loop # # #
while stay_in_main_loop:
    # Clear previous iteration of command
    cmd = ''
    cmd_args = dict()
    cmd_flags = dict()
    
    # Potentially display hint string or hints for possible user actions
    if interact_check:
        if len(last_valid_choices[2]) > 0:
            print('Vals set; process to use set vals and continue.')
    else:
        if len(last_valid_choices[2]) > 0:
            print('\n  '.join(last_valid_choices[2]))
            if choices_check:
                print('Choices are available to be selected.')
        else:
            interact_check = True
    
    # Automatically set command under some circumstances
    if auto_dcs_script:
        # If enabled (only as arg passed to main function),
        #  automatically set first command to load a dcs script
        #  at the val that was passed with the arg
        
        tpl = ddd.parse_dcs_cmd(
            'dcs src="' + auto_dcs_script + '"')
        
        cmd = tpl[0]
        cmd_args = tpl[1]
        cmd_flags = tpl[2]
        
        auto_dcs_script = False
        auto_start = False  # implies this?
    
    elif auto_start:
        # If enabled, automatically set first command when entering
        #  the main loop to the 'start' command to automatically
        #  create a duel state
        
        auto_start = False
        cmd = 'start'
        
    elif auto_process and not choices_check:
        # If auto processing enabled and no choices are available
        #  to be selected (for last valid choices), automatically
        #  use the 'process' commmand (without setting any response)
        #  (should technically use the global var equivalent for
        #   auto_process instead here if I ever implement that...)
        
        cmd = 'process'
    
    # Check command automatically set; if not, either get command
    #  from dcs or from user input
    if not cmd:
        auto_process = False
        
        p = dddapi.py_get_dcs_command()
        if p[0] != '':
            tpl = ddd.parse_dcs_cmd(p[0])
        else:
            cmd_str = input(' ->   Enter a command: ')
            tpl = ddd.parse_dcs_cmd(cmd_str.strip())
        
        cmd = tpl[0]
        cmd_args = tpl[1]
        cmd_flags = tpl[2]

        if dddgs['echoDcsCommand']:
            print(' -> [executing dcs cmd (line ' + str(p[1]) + ')]: '
                  + p[0])
        
    # Handle essential commands (that do not need a duel in progress)
    if (cmd == 'quit' or cmd == 'exit' or cmd == 'q'):
        # Break out of main loop and exit
        break  

    elif (cmd == 'dcs'):
        # Load and run a dcs script
        
        ignore_warnings = '!' in cmd_flags
        arg_status = True
        src = ''
        
        # Parse cmd args if any
        for arg in cmd_args:
            try:
                if arg == 'src':
                    src = ddd.dequote_str(cmd_args[arg])
                elif arg == 'clear' or arg == 'stop':
                    if ddd.parse_bool(cmd_args[arg]):
                        if ddd.confirm_yn(dddapi, 'Clear dcs'
                                          ' command cache?'):
                            print('Will no clear remaining dcs commands')
                        else:
                            print('Clearing dcs command cache')
                            p = dddapi.py_get_dcs_command()
                            dddapi.py_clear_dcs_commands()
                            if p[0] != '':
                                print('Last commmand (on line '
                                      + str(p[1]) + '):', p[0])
                else:
                    if (len(cmd_args) == 1 and
                        ddd.parse_bool(cmd_args[arg])):
                        # if only 1 arg specified, can also treat it
                        #  as val for src
                        src = ddd.dequote_str(arg)
                    else:
                        print('Unknown arg "' + arg + '" in command "'
                              + cmd + '".')
                        arg_status = False
                continue
            except Exception as e:
                print('Invalid val', cmd_args[arg], 'for arg "' +
                      arg + '".')
                print('  (' + str(e) + ')')
                arg_status = False
                
        if not arg_status:
            continue
        
        dddapi.py_load_dcs_commands(src)
        continue
        
    elif (cmd == 'set' or cmd == 'setval' or cmd == 'sv'):
        # Set certain values
        
        ignore_warnings = '!' in cmd_flags
        
        # Parse cmd args if any
        for arg in cmd_args:
            try:
                if (arg == 'autoProcess' or
                    arg == 'auto_process'):
                    dddgs['autoProcess'] = ddd.parse_bool(
                        cmd_args[arg])
                    print('Set val ' + arg + ' to: ' +
                          str(ddd.parse_bool(cmd_args[arg])))
                    
                elif (arg == 'echoDcsCommand' or
                      arg == 'echo_dcs_command'):
                    dddgs['echoDcsCommand'] = ddd.parse_bool(
                        cmd_args[arg])
                    print('Set val ' + arg + ' to: ' +
                          str(ddd.parse_bool(cmd_args[arg])))
                    
                elif (arg == 'dsId' or
                      arg == 'dsid'):
                    if not ignore_warnings:
                        print('Attempting to potentially unsafely set'
                              ' duel state (consider using the '
                              '\'swap\' command to safely change duel'
                              ' states instead).')
                        if not ddd.confirm_yn(
                                dddapi, 'Really set ' + cmd_args[arg] +
                                ' as the current/active duel state'
                                ' (this might segfault something)?'):
                            print('Will not set dsId to new var.')
                            continue

                    dsid = cmd_args[arg]
                    # does not change original_dsid
                    print('Set val ' + arg + ' to: ' +
                          str(ddd.parse_bool(cmd_args[arg])))
                    
                elif (arg == 'logToFile' or
                      arg == 'log_to_file'):
                    print('  Arg "' + arg + '" not available for "' +
                          cmd + '" command in Python script;'
                          ' will skip.')
                    
                elif (arg == 'logFile' or
                      arg == 'log_file'):
                    print('  Arg "' + arg + '" not available for "' +
                          cmd + '" command in Python script;'
                          ' will skip.')
                    
                elif (arg == 'useFixedSeed' or
                      arg == 'use_fixed_seed'):
                    print('  Arg "' + arg + '" not available for "' +
                          cmd + '" command in Python script;'
                          ' will skip.')
                    
                else:
                    print('Unknown arg "' + arg + '" in command "'
                          + cmd + '".')
                
            except Exception as e:
                print('Invalid val', cmd_args[arg], 'for arg "' +
                      arg + '".')
                print('  (' + str(e) + ')')
                arg_status = False
        continue
    
    elif (cmd == 'echo'):
        for arg in cmd_args:
            if arg == 'flag':
                pass # ignore
            elif arg == 'msg' or arg == 'message':
                print(cmd_args[arg])
            else:
                print('Unknown arg "' + arg + '" in command "'
                      + cmd + '".')
        continue
    
    elif (cmd == 'h' or cmd == 'help'):
        # Print quick description of commands
        print('Commands:')
        print('  (note: some commands may accept args and some only'
              'work when used with args')
        print('')
        print('Common commands:')
        print('h / help\tPrint this help.')
        print('rs / restart\tStart a new duel.')
        print('q / quit / exit\tQuit script.')
        print('gs / gamestate\tPrint (simplified) gamestate.')
        print('i / interact\tShow available choices and set vals that affect'
              ' how the duel is processed.')
        print('p / process\tDo a game tick based on vals set.')
        print('c / cancel\tSet vals to attempt to cancel or finish'
              ' (early) current interaction before processing.')
        print('')
        print('Other commands:')
        print('d / duplicate\tAttempt to duplicate current duel'
              ' state.')
        print('s / swap\tSwitch current duel to another duel state.')
        print('bf / bruteforce\tBrute force current duel state.')
        print('dcs\tLoad a dcs script.')
        print('sv / setval\tSet certain (non-duel) program values.')
        print('')
        print('Script start args:')
        print('-d / --dcs / --load-dcs-script\tLoad and run a specified dcs'
              'script.')
        print('-c / --conf / --conf-file\tLoad and use a specified conf file.')
        print('-a / --auto-start\tImmediately start a duel on program start'
              ' unless set to false.')
        print('')
        continue
    
    elif cmd == 'start' or cmd == 'st' or cmd == 'restart' or cmd == 'rs':
        # Start duel if dsid not set (0) or if already set, prompt
        #  user to end duel and start another
        
        ignore_warnings = '!' in cmd_flags
        arg_status = True
        seed = 0
        use_specified_seed = False
        
        # Parse cmd args if any
        for arg in cmd_args:
            try:
                if arg == 'seed':
                    seed = int(cmd_args[arg])
                    use_specified_seed = True
                else:
                    print('Unknown arg "' + arg + '" in command "'
                          + cmd + '".')
                    arg_status = False
                    
            except Exception as e:
                print('Invalid val', cmd_args[arg], 'for arg "' +
                      arg + '".')
                print('  (' + str(e) + ')')
                arg_status = False
        
        if not arg_status:
            continue

        # Check if current duel state already started
        if dsid != 0:
            # Already started
            print('Attempting to start another duel...')
            if ignore_warnings:
                resp = True
            else:
                resp = ddd.confirm_yn(dddapi, 'Destroy current duel'
                                      ' and start another (duplicated'
                                      ' states will also be lost)?')
            if not resp:
                print('Keeping current duel.')
                
            else:
                print('Destroying duel...')

                # Destroy all duel states (not just current one)
                gd_result = dddapi.py_get_duel_states()
                dddapi.py_remove_duel_states(gd_result)
                
                # Reset/clear variables
                dsid = 0
                duel_and_states_dict.clear()
                last_valid_msg = 0
                last_valid_choices = ([], [0], [], [], [])
                # auto_process = dddgs['autoProcess']
                choices_check = False
                interact_check = True

                # Start new duel and set associated variables
                print('Starting another...')
                if use_specified_seed:
                    dsid = dddapi.py_create_duel_state_from_seed(
                        True, seed)
                else:
                    dsid = dddapi.py_create_duel_state(True)
                
                dsid_original = dsid
                duel_and_states_dict[dsid] = (
                    last_valid_msg, last_valid_choices)
                
        else:
            # Not started yet

            # Start new duel and set associated variables
            if use_specified_seed:
                dsid = dddapi.py_create_duel_state_from_seed(
                    True, seed)
            else:
                dsid = dddapi.py_create_duel_state(True)
            
            last_valid_msg = 0
            last_valid_choices = ([], [0], [], [], [])
            duel_and_states_dict[dsid] = (
                last_valid_msg, last_valid_choices)
            dsid_original = dsid
        
        auto_process = dddgs['autoProcess']
        continue
    
    elif dsid == 0:
        # Don't handle commands below this block if did not
        #  start duel state yet
        print('Duel not created yet. Start one first.')
        auto_process = False
        continue

    # Handle rest of commands
    if cmd == 'interact' or cmd == 'i':
        # Allow user to set response based on current msg
        #  (without processing)
        if last_valid_msg < 2:
            print('No last valid msg or need to interact with/set values.')
            continue
        
        if len(last_valid_choices[0]) < 1:
            print('No choices available to set.')
            continue

        ignore_warnings = '!' in cmd_flags
        arg_status = True
        choice_str = ''
        
        # Parse cmd args if any
        for arg in cmd_args:
            try:
                if arg == 'choice' or arg == 'c':
                    choice_str = cmd_args[arg]

                else:
                    if (len(cmd_args) == 1 and
                        ddd.parse_bool(cmd_args[arg])):
                        # if only 1 arg specified, can also treat it
                        #  as val for src
                        choice_str = arg
                    else:
                        print('Unknown arg "' + arg + '" in command "'
                              + cmd + '".')
                    arg_status = False
                    
            except Exception as e:
                print('Invalid val', cmd_args[arg], 'for arg "' +
                      arg + '".')
                print('  (' + str(e) + ')')
                arg_status = False
        
        if not arg_status:
            continue

        # Interact by using func to set values based on msg
        interact_check = ddd.set_response_by_input(
            dddapi, dsid, last_valid_choices, choice_str)
        
        # Special hint for msg 26 (MSG_SELECT_UNSELECT) if vals set
        if interact_check and last_valid_msg == 26:
            print('  (process again before selecting the next card(s))')
        
        if not interact_check:
            print('')  # Print empty line for formatting...

    elif cmd == 'process' or cmd == 'p':
        # Check if attempting to process with choices available
        #  but none of them were selected (can be valid if
        #  attempting to skip certain interactions)
        
        ignore_warnings = '!' in cmd_flags
        arg_status = True
        choice_str = ''
        
        # Parse cmd args if any
        for arg in cmd_args:
            try:
                if arg == 'choice' or arg == 'c':
                    choice_str = cmd_args[arg]

                else:
                    if (len(cmd_args) == 1 and
                        ddd.parse_bool(cmd_args[arg])):
                        # if only 1 arg specified, can also treat it
                        #  as val for src
                        choice_str = arg
                    else:
                        print('Unknown arg "' + arg + '" in command "'
                              + cmd + '".')
                    arg_status = False
                    
            except Exception as e:
                print('Invalid val', cmd_args[arg], 'for arg "' +
                      arg + '".')
                print('  (' + str(e) + ')')
                arg_status = False
        
        if not arg_status:
            continue

        # Set response here if choice arg was passed and is valid
        #  (instead of needing it through the interact command)
        if choice_str != '':
            interact_check = ddd.set_response_by_input(
                dddapi, dsid, last_valid_choices, choice_str)
            
            if not interact_check:
                print('')  # Print empty line for formatting...
                continue
        
        # Warn user if attempting to process without selecting
        #  any choices beforehand
        should_process = True
        if not auto_process:
            if not interact_check and not ignore_warnings:
                print('Attempted to process where choices are'
                      ' available but may not have been selected.')
                should_process = ddd.confirm_yn(
                    dddapi, 'Really skip interacting and continue'
                    ' with processing anyway?')
                if should_process:
                    print('Will continue processing...')
                else:
                    print('Process command cancelled.')

        # Abort processing if cancelled by user
        if not should_process:
            continue
        
        # Process and get results
        dddapi.py_process_duel_state(dsid)  # Process (do a game tick)
        
        # Get choices and their labels
        choices_temp = dddapi.py_get_choices_from_duel_state(
            dsid, True, [])
        msg_temp = choices_temp[1][0]
        
        # Handle results
        if msg_temp == 0:
            # A msg of 0 could actually mean something else went
            #  wrong but if not, then merely means buffer is empty
            # Enable auto_process to True and return to start
            #  of main loop
            print('Skipping empty buffer.')
            auto_process = dddgs['autoProcess']
            continue
        
        # Save last valid choices if valid
        if msg_temp > 1:
            # Update last valid choices and dict
            last_valid_msg = msg_temp
            last_valid_choices = choices_temp
            choices_check = (len(choices_temp[0]) > 0)
            duel_and_states_dict[dsid] = (last_valid_msg, last_valid_choices)
            
            auto_process = dddgs['autoProcess']
            
        else:
            # Print error hint strings
            if msg_temp == 1:
                print('\n  '.join(choices_temp[2]))
                print('')  # Print empty line for formatting...
            
            auto_process = False
        
        interact_check = False
        
    elif cmd == 'cancel' or cmd == 'c':
        # Set response var to -1 (without processing)
        # Usually this will not do anything as most commands cannot
        #  be cancelled once processsed
        # Some commands may have this value mean to finish early
        #  (e.g. able to select more but opting not to and want to
        #   process with what was selected)
        # Some commands may also have this value mean to be the same
        #  as no choices selected
        
        dddapi.py_set_duel_state_responsei(dsid, -1)
        interact_check = True
        print('Vals set to attempt to cancel current command; process'
              ' to see if action can be cancelled.')

    elif cmd == 'swap' or cmd == 'switch' or cmd == 's':
        # Allow user to set the current duel state
        # 
        print('Attempting to swap current duel state...')

        # Print available duel states to switch to
        #  (or the first and last 100 if excessive)
        available_duel_states = []
        if len(duel_and_states_dict) > 200:
            sorted = list(duel_and_states_dict)
            sorted.sort()
            available_duel_states = list(
                map(str, sorted[:100] + ['...... '] + sorted[-100:]))
            
        else:
            available_duel_states = list(
                map(str, duel_and_states_dict.keys()))
        
        print('Available duel states to swap to (' +
              str(len(available_duel_states)) + '):')
        print('  ', ", ".join(available_duel_states))

        # Warn user if current duel state not in dict
        if dsid not in duel_and_states_dict:
            print('   (current duel state (' + str(dsid) +
                  ') does not appear to be in duel_and_states_dict)')

        # Allow user to input duel state
        choice_str = input(' ->   Enter a duel state id: ')

        # Parsing and verification of user input
        try:
            choice = int(choice_str, 10)
        except Exception as e:
            print('Invalid choice "' + choice_str + '".')
            print('  (' + str(e) + ')')
            continue

        if choice not in duel_and_states_dict:
            print('Duel state ' + choice_str + ' not found;'
                  ' duel state not swapped.')
            continue
        
        if choice == dsid:
            print('Already using duel state ' + str(dsid) + '.')
            continue
        
        # Save/update current dsid and its last valid msg and choices
        duel_and_states_dict[dsid] = (last_valid_msg, last_valid_choices)
        
        # Reset certain values
        choices_check = False
        interact_check = False
        
        # Set swapped state's last valid msg and choices
        #  (also deactivate old duel state and reactivate new one)
        dddapi.py_deactivate_duel_state(dsid)
        dsid = choice
        dddapi.py_reactivate_duel_state(dsid)
        
        # Check if dict value for new dsid was only temporary
        #  (e.g. was added via the brute force function)
        if duel_and_states_dict[dsid] is not None:
            # Set last valid variables directly from dict
            last_valid_choices = duel_and_states_dict[dsid][1]
            last_valid_msg = last_valid_choices[1][0]
            
        else:
            # Get last valid variables and then update dict
            last_valid_choices = dddapi.py_get_choices_from_duel_state(
                dsid, True, [])
            last_valid_msg = last_valid_choices[1][0]
            duel_and_states_dict[dsid] = (
                last_valid_msg, last_valid_choices)
        
        print('Current duel state is now ', str(dsid), '.')
    
    elif cmd == 'duplicate' or cmd == 'dupe' or cmd == 'd':
        # Allows user to create a duplicate of the currently active
        #  duel state and its last valid choices where the duel
        #  states can be switched to later without selected choices
        #  and process iterations affecting the other
        
        if ddd.confirm_yn(dddapi, 'Duplicate current state?'):
            dsid_new = dddapi.py_duplicate_duel_state(dsid)

            if dsid_new == 0:
                print('Unable to duplicate current state.')
            else:
                print('Successfully duplicated current duel state ' +
                      str(dsid) + ' to state ' + str(dsid_new) + '.')
                duel_and_states_dict[dsid] = (last_valid_msg, last_valid_choices)
                duel_and_states_dict[dsid_new] = (last_valid_msg, last_valid_choices)
                
                if ddd.confirm_yn(dddapi, 'Switch to'
                                  ' duplicate_duel_state?'):
                    print('Switched active duel to new duel state.')
                    dsid = dsid_new
                else:
                    print('Continuing to use current duel as active duel.')
                
        else:
            print('Duel state not duplicated.')

    elif (cmd == 'gamestate' or cmd == 'game state' or
          cmd == 'gs' or cmd == 'g'):
        # Get current (text) visual on game state and print it

        # dddapi.py_reactivate_duel_state(dsid)
        fvg = dddapi.py_get_field_visual_gamestate(dsid)
        
        print('')
        print('\n  '.join(fvg[4]))  # Player 1 info + cards in hand
        print('\n'.join(fvg[0]))  # Field info
        print('\n  '.join(fvg[3]))  # Player 0 info + cards in hand
        
        if len(fvg[1]) > 1:  # Cards on player 0's field
            print('\n  '.join(fvg[1]))
        
        if len(fvg[2]) > 1:  # Cards on player 1's field
            print('\n  '.join(fvg[2]))
        
        print('\n  '.join(fvg[5]))  # Chain info
        print('')
        
    elif cmd == 'bruteforce' or cmd == 'bf':
        # Brute forces all possible choices (if not filtered) to a
        #  specified depth and updates dict with the resulting choices
        #  (default depth 3 if not specified; default filter empty
        #   if not specified)
        
        ignore_warnings = '!' in cmd_flags
        arg_status = True
        depth = 3
        show_messages = False
        choices_filter = []
        bf_type = 'dfs'
        max_depth_results_limit = 0  # 0 for no limit
        show_limit = 0  # 0 for no limit
        
        # Parse cmd args if any
        for arg in cmd_args:
            try:
                if (arg == 'depth' or arg == 'maxDepth'):
                    depth = int(cmd_args[arg])
                    
                elif arg == 'filter':
                    choices_filter_str = ddd.dequote_str(
                        cmd_args[arg]).strip()
                    if choices_filter_str != '':
                        choices_filter = list(
                            map(int, choices_filter_str.split(',')))

                elif arg == 'type':
                    bf_type = ddd.dequote_str(cmd_args[arg]).strip()

                elif (arg == 'showMsg' or arg == 'show_messages' or
                      arg == 'showMessages' or arg == 'show_messages'):
                    show_messages = ddd.parse_bool(cmd_args[arg])

                elif (arg == 'resultsLimit' or
                      arg == 'results_limit' or
                      arg == 'maxDepthResultsLimit' or
                      arg == 'max_depth_results_limit'):
                    max_depth_results_limit = int(cmd_args[arg])
                    
                elif (arg == 'showResultsLimit' or
                      arg == 'show_results_limit' or
                      arg == 'showLimit' or
                      arg == 'show_limit'):
                    show_limit = int(cmd_args[arg])
                    
                else:
                    print('Unknown arg "' + arg + '" in command "'
                          + cmd + '".')
                    arg_status = False
                    
            except Exception as e:
                print('Invalid val', cmd_args[arg], 'for arg "' +
                      arg + '".')
                print('  (' + str(e) + ')')
                arg_status = False
            
        if not arg_status:
            continue
        
        # Special handling of filter for msg 23 where selected choices
        #  (and therefore also the filter) need to be offset by the
        #  must_select amount
        if last_valid_msg == 23:
            offset = last_valid_choices[1][5]
            temp_filter_set = set()
            for cf in choices_filter:
                if cf >= 0:
                    temp_filter_set.add(cf + offset)
                else:
                    temp_filter_set.add(cf)
            
            choices_filter = temp_filter_set
        
        # Brute force choices and get results
        dddapi.py_reactivate_duel_state(dsid)
        result = ddd.brute_force_state_choices(
            dddapi, dsid, last_valid_choices, depth,
            max_depth_results_limit, show_limit, choices_filter,
            bf_type, show_messages, ignore_warnings)
        
        # Update dict for any results returned
        for r in result:
            if r not in duel_and_states_dict:
                # No need to get actual choices tuple; only need
                #  to set key (tuple can be generated later)
                duel_and_states_dict[r] = None
        
    elif cmd == 'temp':
        # Custom function #2
        pass
    else:
        print('Not sure what "' + cmd + '" is; use the "help" command'
              ' for a list of commands.')


# Exit block (when exiting loop)
if dsid != 0:
    print('Destroying duel...')

    gd_result = dddapi.py_get_duel_states()
    ds_count = len(gd_result)
    
    start_time = time.time()
    
    # End all duels (current duel as well as any states)
    dddapi.py_remove_duel_states(gd_result)
    
    end_time = time.time()
    taken_time = end_time - start_time

    print('Removed', ds_count, 'duel states in', int(taken_time),
          'seconds.')
    if taken_time == 0:
        taken_time = 1
    print('\t(' + str(round(ds_count / taken_time, 2)) +
          ' duel states per second)')
    
print('Exiting...\n')
