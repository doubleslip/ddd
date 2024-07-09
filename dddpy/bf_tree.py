# bf_tree.py

# Possible implementations of brute forcing for a duel state
# (brute_force_state_choices_bfs() and brute_force_state_choices_dfs())


import time
from .utils import *

# Node for a tree (implemented as a dict)
#  (generally only for internal use in this module)
class BfNode:
    parent = -1
    children = []
    depth = 0
    choice = ''


# Brute force choices into their own duel states and return them
#  (limited by the depth specified and if any choices match
#   the filter passed in here)
def brute_force_state_choices(dddapi, dsid, last_valid_choices, depth_limit, max_depth_results_limit, show_limit, in_filter=[], bf_type='dfs', show_messages=False, ignore_warnings=False):

    if bf_type == 'bfs':
        return brute_force_state_choices_bfs(
            dddapi, dsid, last_valid_choices, depth_limit,
            max_depth_results_limit, show_limit,
            in_filter, show_messages, ignore_warnings)
    elif bf_type == 'dfs':
        return brute_force_state_choices_dfs(
            dddapi, dsid, last_valid_choices, depth_limit,
            max_depth_results_limit, show_limit,
            in_filter, show_messages, ignore_warnings)
    else:
        print('Invalid type "' + bf_type + '".')
        return dict()


# Brute force duel states with using a breadth first search (bfs)
def brute_force_state_choices_bfs(dddapi, dsid, last_valid_choices, depth_limit, max_depth_results_limit, show_limit, in_filter, show_messages, ignore_warnings):
    if not ignore_warnings:
        if not confirm_yn(dddapi, 'Really brute force all states'
                          ' with current choices?'):
            print('Not doing it.')
            return dict()
    
    # Track whether duel state was active and activate it regardless
    dsid_was_active = dddapi.py_is_duel_state_active(dsid)
    dddapi.py_reactivate_duel_state(dsid)
    
    dsid_results = set()
    choices_tree = dict()
    depth_map = dict()
    parents_to_bf = []
    end_msgs = set([5, 40, 41])

    # Get actual filter used by the lib
    choice_filter = dddapi.py_get_choice_filter(dsid, in_filter)
    print('(in_)filter:', in_filter, '  choice filter:', choice_filter)
    
    # Populate first node/entry to trees/dicts (with dsid)
    parents_to_bf.append(dsid)
    node = BfNode()
    node.choice = '(start)'
    choices_tree[dsid] = node
    depth_map[0] = [dsid]
    
    # Start timer
    start_time = time.time()
    max_depth_reached = 0
    end_search_early = False
    
    # Loop until max depth reached (or found no more duel states
    #  with choices)
    for curr_depth in range(1, depth_limit + 1, 1):
        print('Current depth:', curr_depth, '\tDuel states found'
              ' so far:', (len(choices_tree) - 1))

        # Clear/initialize list to track children nodes with choices
        children_to_bf = []
        # Initialize current depth in depth map (with empty list)
        depth_map[curr_depth] = []

        # Iterate through nodes with choices to brute force
        for p in parents_to_bf:
            dddapi.py_reactivate_duel_state(p)
            p_choices = dddapi.py_get_choices_from_duel_state(
                p, True, choice_filter)

            # Brute force duel state's choices and get results
            bf_states = brute_force_state_choices_at_state(
                dddapi, p, p_choices, choices_tree,
                end_msgs, False) # , show_messages)
            
            dddapi.py_deactivate_duel_state(p)

            # Iterate through results and save to tree
            for bf_state in bf_states:
                
                # Check if results limit reached
                if max_depth_results_limit > 0:
                    if max_depth_reached == max_depth_results_limit:
                        if (len(depth_map[max_depth_reached])
                            >= max_depth_results_limit):
                            end_search_early = True
                            break
                
                # Create node from result
                node = BfNode()
                node.parent = p
                node.depth = curr_depth + 1
                node.choice = bf_state[1]

                # Add node to tree
                choices_tree[bf_state[0]] = node
                # Add node's dsid to parent's list of children
                choices_tree[p].children.append(bf_state[0])
                # Add entry to depth map
                depth_map[curr_depth].append(bf_state[0])
                
                # Only add result to list of children nodes with
                #  choices if message of result's duel state is not
                #  one of a list of messages to indicate not brute
                #  forcing beyond
                if bf_state[2] not in end_msgs:
                    children_to_bf.append(bf_state[0])
                
                # Add result to found duel states regardless
                dsid_results.add(bf_state[0])
                
                dddapi.py_deactivate_duel_state(bf_state[0])

            # break if exiting early
            if end_search_early:
                break
            
            # Periodically output progress to user
            if ((int((len(dsid_results) - 1) / 100)) >
                ((int(len(dsid_results) - len(bf_states) - 1) / 100))):
                print('\t\tDuel states found so far:',
                      (len(dsid_results) - 1))

        # break if exiting early
        if end_search_early:
            break
        
        # Check if next depth has any duel states to brute force
        if len(children_to_bf) < 1:
            # No more duel states to brute force; end early
            #  (by breaking out of loop as max depth not reached yet)
            break
        else:
            # Copy list of duel states to brute force to actual
            #  variable that will brute force them next loop iteration
            parents_to_bf = children_to_bf[:]
            children_to_bf.clear()
            max_depth_reached = curr_depth
    
    end_time = time.time()
    taken_time = end_time - start_time

    # Erase only the original duel state id (to adjust tallys)
    #  (might have to remove from other vars as well)
    if dsid in dsid_results:
        dsid_results.remove(dsid)
    
    # Since brute forcing loop deactivates the duel state,
    #  deactivate if was originally deactive
    if not dsid_was_active:
        dddapi.py_deactivate_duel_state(dsid)
    
    # Get duel states at deepest depth reached
    deepest_duel_states_list = []
    for ds in depth_map[max_depth_reached]:
        if ((show_limit == 0) or
            (len(deepest_duel_states_list) <= show_limit)):
            # Output the chain of choices used to reach duel state
            c = get_choices_from_tree(choices_tree, ds)
            deepest_duel_states_list.append(
                "[" + str(ds) + "]:  \t" +
                " => ".join(list(reversed(c))))
    
    # Print results
    for s in deepest_duel_states_list:
        print("    \t" + s)
    
    if ((show_limit > 0) and
        (show_limit < len(depth_map[max_depth_reached]))):
        print("    \t ......")
        print("    \t(omitted " +
              str(len(depth_map[max_depth_reached]) - show_limit) +
              " more results (limited to showing " +
              str(show_limit) + " results))")
    
    # Print deepest depth reached for user and the duel states
    #  that reached that depth
    print("Found total of " + str(len(dsid_results)) +
          " duel states with " + str(len(depth_map[max_depth_reached]))
          + " duel states at deepest depth of "
          + str(max_depth_reached) + ".")
    if taken_time == 0:
        taken_time = 1
    print("    \t(" + str(round(len(choices_tree) / taken_time, 2)) +
          " duel states per second)")
    
    if end_search_early:
        print("    \t(search ended early because result limit of " +
              str(max_depth_results_limit) + "for the deepest depth"
              " was reached)")

        # Deactivate any possibly active duel states
        for dm_list in depth_map:
            for pa_dsid in dm_list:
                dddapi.py_deactivate_duel_state(pa_dsid)
    
    return dsid_results


# Brute force all immediate choices available for a given duel state
#  and returns a list of duel states that did not result in an error
#  (does this by duplicating a state for each possible choice (or
#   combination of choices), processing the choice and then advancing
#   the duel state until an end message reached or error occurred)
#  (generally for internal use only)
def brute_force_state_choices_at_state(dddapi, p, p_choices, choices_tree, end_msgs, show_messages):
    p_msg = p_choices[1][0]
    state_results = []  # tuple format: (dsid, choice_used, msg)
    
    if show_messages:
        print('Brute forcing dsid ' + str(p) + '.')
    
    # Get possible choices to brute force
    possible_choices = brute_force_possible_choices(p_choices)
    
    # Iterate through possible choices
    for pc in possible_choices:
        # Duplicate duel state
        new_dsid = dddapi.py_duplicate_duel_state(p)
        
        if new_dsid == 0:
            print('Error duplicating duel state', p, 'to brute force.')
            return state_results
        
        # If var true, there's a very real possibility processing may
        #  not succeed so if it doesn't (but only on the first process
        #  iteration), don't abort from receving an error because not
        #  succeeding was an expected possibility
        check_first_process = False  # Maybe not necessary?
        
        # Only set response based on choice(s) selected
        if len(pc) > 0:
            # At least 1 choice
            set_response(dddapi, new_dsid, p_choices, pc)
        else:
            # No choices selected
            check_first_process = True
        
        if p_msg == 23:  # MSG_SELECT_SUM
            check_first_process = True
        
        # Process duel state (after setting response if necessary)
        #  and keep processing until end message reached or otherwise
        #  encountered an error
        apl_tpl = auto_process_until_choices_available(
            dddapi, new_dsid, show_messages,
            check_first_process, end_msgs, p_choices[4])
        
        # Check if result was valid
        #  (either got error message (MSG_RETRY) after 1st process
        #   iteration (while checking for first process or if not,
        #   even on the 1st process iteration) or got some other
        #   kind of error)
        if apl_tpl[1] == False:
            print('Advancing duel state', dsid, 'resulted in an'
                  ' unexpected invalid msg (' + str(apl_tpl[0]) + ').')
            dddapi.py_remove_duel_state(new_dsid)
            continue
        
        # Check if got error message (MSG_RETRY)
        if apl_tpl[0] == 1:
            # Got MSG_RETRY but on first process
            dddapi.py_remove_duel_state(new_dsid)
            continue
        
        # Get choice string for choice selected
        choices_str_list = []
        choices_str = ''
        if len(pc) > 0:
            # Loop because possible to have multiple choices selected
            for c in pc:
                choices_str_list.append(p_choices[3][c])
                choices_str = ' + '.join(choices_str_list)
        else:
            # No choice selected
            choices_str = '(continue processing)'
        
        # Generate and add tuple to results
        state_results.append((new_dsid, choices_str, apl_tpl[0]))
        
        # don't deactivate duel state here; let calling func do it
        
    return state_results


# Get the list of choices used to get to duel state dsid from root
#  node of tree
#  (returning them in reverse order)
def get_choices_from_tree(choices_tree, dsid):
    curr_dsid = dsid
    choices_str_list = []
    
    # Iterate until reached root node which should be the only
    #  node whose parent is -1 (a nonexistent duel state id)
    while curr_dsid != -1:
        choices_str_list.append(choices_tree[curr_dsid].choice)
        curr_dsid = choices_tree[curr_dsid].parent
    
    return choices_str_list


# Brute force duel states with using a depth first search (dfs)
def brute_force_state_choices_dfs(dddapi, dsid, last_valid_choices, depth_limit, max_depth_results_limit, show_limit, in_filter, show_messages, ignore_warnings):
    if not ignore_warnings:
        if not confirm_yn(dddapi, 'Really brute force all states'
                          ' with current choices?'):
            print('Not doing it.')
            return set()
    
    # Track whether duel state was active and activate it regardless
    dsid_was_active = dddapi.py_is_duel_state_active(dsid)
    dddapi.py_reactivate_duel_state(dsid)
    
    dsid_results = dict()  # Unlike bfs, not just a set
    choices_tree = dict()
    depth_map = dict()
    dfs_stack = []  # Of tuple (duel state id, choices selected)
    dsid_reserve = dict()
    dsid_reusable = []
    end_msgs = set([5, 40, 41])

    choice_filter = dddapi.py_get_choice_filter(dsid, in_filter)

    # Populate stack with initial values
    #  (no need to push sentinel value to deactivate because
    #   keeping original dsid active is desired anyway)
    dsid_results[dsid] = dddapi.py_get_choices_from_duel_state(
        dsid, True, choice_filter)

    possible_choices = brute_force_possible_choices(dsid_results[dsid])
    for pc in possible_choices:
        dfs_stack.append((dsid, pc, False))

    # Populate first node/entry to trees/dicts (with dsid)
    node = BfNode()
    node.choice = '(start)'
    choices_tree[dsid] = node
    depth_map[0] = [dsid]

    # Start timer
    start_time = time.time()
    max_depth_reached = 0
    end_search_early = False
    
    # Iterate until stack cleared
    while len(dfs_stack) > 0:

        # Check if results limit reached
        if max_depth_results_limit > 0:
            if max_depth_reached == max_depth_results_limit:
                if (len(depth_map[max_depth_reached])
                    >= max_depth_results_limit):
                    end_search_early = True
                    break
        
        # Get parent duel state id and its choice combination
        #  then pop it off the stack
        p_dsid = dfs_stack[-1][0]
        p_dsid_cc = dfs_stack[-1][1]  # choice combination
        is_last = dfs_stack[-1][2]
        dfs_stack.pop()

        # Check max_depth not exceeded
        curr_depth = choices_tree[p_dsid].depth + 1
        if curr_depth > depth_limit:
            if is_last:
                if dddapi.py_is_duel_state_pduel_duplicatable(p_dsid):
                    dsid_reusable.append(p_dsid)
                else:
                    dddapi.py_deactivate_duel_state(p_dsid)
            
            continue

        # Get new duel state to duplicate
        new_dsid = 0
        if is_last:
            # Parent duel state no longer needed; assume it
            new_dsid = dddapi.py_assume_duel_state(p_dsid)
            
            # Check dsid_reserve for any entries
            if p_dsid in dsid_reserve:
                # Iterate all entries, destroy them and then remove key

                # Iterate all entries and destroy them
                for ds in dsid_reserve[p_dsid]:
                    dddapi.py_remove_duel_state(ds)

                del dsid_reserve[p_dsid]
            
        else:
            # Check if duel state available in reserve (can use
            if ((p_dsid in dsid_reserve) and
                (len(dsid_reserve[p_dsid]) > 0)):
                # Available duel state in reserve; use that duel state
                #  and also remove it from reserve
                new_dsid = dsid_reserve[p_dsid][-1]
                dsid_reserve[p_dsid].pop()

            elif len(dsid_reusable) > 0:
                # Available duel state to reuse; use that duel state
                #  and also remove from reuse
                new_dsid = dddapi.py_duplicate_duel_state_reusing(
                    p_dsid, dsid_reusable[-1])
                dsid_reusable.pop()
                
            else:
                # No available duel state in reserve or to reuse;
                #  create new duplicate
                new_dsid = dddapi.py_duplicate_duel_state(p_dsid)

        if new_dsid == 0:
            print('Error duplicating duel state', p_dsid,
                  'to brute force.')
            dsid_results_set = set()
            for dsid_result in dsid_results:
                dsid_results_set.add(dsid_result)
            return dsid_results_set
        
        
        # If var true, there's a very real possibility processing may
        #  not succeed so if it doesn't (but only on the first process
        #  iteration), don't abort from receving an error because not
        #  succeeding was an expected possibility
        check_first_process = False  # Maybe not necessary?
        
        # Only set response based on choice(s) selected
        if len(p_dsid_cc) > 0:
            # At least 1 choice
            set_response(dddapi, new_dsid,
                         dsid_results[p_dsid], p_dsid_cc)
        else:
            # No choices selected
            check_first_process = True
        
        # Check message (if 23)
        if dsid_results[p_dsid][1][0] == 23:  # MSG_SELECT_SUM
            check_first_process = True
        
        
        # Process duel state (after setting response if necessary)
        #  and keep processing until end message reached or otherwise
        #  encountered an error
        apl_tpl = auto_process_until_choices_available(
            dddapi, new_dsid, show_messages,
            check_first_process, end_msgs, choice_filter)

        
        # Check if result was valid
        #  (either got error message (MSG_RETRY) after 1st process
        #   iteration (while checking for first process or if not,
        #   even on the 1st process iteration) or got some other
        #   kind of error)
        if apl_tpl[1] == False:
            print('Advancing duel state', dsid, 'resulted in an'
                  ' unexpected invalid msg (' + str(apl_tpl[0]) + ').')
            dddapi.py_remove_duel_state(new_dsid)
            continue
        
        # Check if got error message (MSG_RETRY)
        if apl_tpl[0] == 1:
            # Got MSG_RETRY but on first process when possibility
            # of it was expected
            
            # dddapi.py_remove_duel_state(new_dsid)
            if (p_dsid not in dsid_reserve):
                dsid_reserve[p_dsid] = []
            
            dsid_reserve[p_dsid].append(new_dsid)
            continue
        
        # Get choice string for choice selected
        choices_str_list = []
        choices_str = ''
        if len(p_dsid_cc) > 0:
            # Loop because possible to have multiple choices selected
            for c in p_dsid_cc:
                choices_str_list.append(dsid_results[p_dsid][3][c])
                choices_str = ' + '.join(choices_str_list)
        else:
            # No choice selected
            choices_str = '(continue processing)'
        
        # Update results
        dsid_results[new_dsid] = apl_tpl[2]
        node = BfNode()
        node.parent = p_dsid
        node.choice = choices_str
        node.depth = curr_depth
        choices_tree[new_dsid] = node
        choices_tree[p_dsid].children.append(new_dsid)
        if curr_depth in depth_map:
            depth_map[curr_depth].append(new_dsid)
        else:
            depth_map[curr_depth] = [new_dsid]
        max_depth_reached = curr_depth
        
        # Print # of duel states found so far for user
        if ((len(dsid_results) > 1) and
            (((len(dsid_results) - 1) % 100) == 0)):
            print('\tDuel states found so far:', len(dsid_results) - 1)
        
        
        # Check if depth limit reached
        if (curr_depth >= depth_limit):
            # Depth limit reached (therefore a leaf node)
            if dddapi.py_is_duel_state_pduel_duplicatable(new_dsid):
                dsid_reusable.append(new_dsid)
            else:
                dddapi.py_deactivate_duel_state(new_dsid)
            
            continue

        
        # Depth limit not yet reached
        # Get potential children
        choice_combinations = brute_force_possible_choices(apl_tpl[2])

        # Check if children found
        if (len(choice_combinations) == 0):
            # No further children for this node found
            if dddapi.py_is_duel_state_pduel_duplicatable(new_dsid):
                dsid_reusable.append(new_dsid)
            else:
                dddapi.py_deactivate_duel_state(new_dsid)
            
            continue

        
        # Push children to stack
        first_child = True  # technically last since popped last
        for cc in choice_combinations:
            dfs_stack.append((new_dsid, cc, first_child))
            if first_child:
                first_child = False

    # # # end while loop
    
    end_time = time.time()
    taken_time = end_time - start_time
    
    # Erase only the original duel state id (to adjust tallys)
    #  (might have to remove from other vars as well)
    del dsid_results[dsid]
    
    # Since brute forcing loop deactivates the duel state,
    #  deactivate if was originally deactive
    if not dsid_was_active:
        dddapi.py_deactivate_duel_state(dsid)
    
    # Get duel states at deepest depth reached
    deepest_duel_states_list = []
    for ds in depth_map[max_depth_reached]:
        if ((show_limit == 0) or
            (len(deepest_duel_states_list) <= show_limit)):
            # Output the chain of choices used to reach duel state
            c = get_choices_from_tree(choices_tree, ds)
            deepest_duel_states_list.append(
                "[" + str(ds) + "]:  \t" +
                " => ".join(list(reversed(c))))
    
    # Print results
    for s in deepest_duel_states_list:
        print("    \t" + s)
    
    if ((show_limit > 0) and
        (show_limit < len(depth_map[max_depth_reached]))):
        print("    \t ......")
        print("    \t(omitted " +
              str(len(depth_map[max_depth_reached]) - show_limit) +
              " more results (limited to showing " +
              str(show_limit) + " results))")
    
    # Print deepest depth reached for user and the duel states
    #  that reached that depth
    print("Found total of " + str(len(dsid_results)) +
          " duel states with " + str(len(depth_map[max_depth_reached]))
          + " duel states at deepest depth of "
          + str(max_depth_reached) + ".")
    if taken_time == 0:
        taken_time = 1
    print("    \t(" + str(round(len(choices_tree) / taken_time, 2)) +
          " duel states per second)")
    
    if end_search_early:
        print("    \t(search ended early because result limit of " +
              str(max_depth_results_limit) + "for the deepest depth"
              " was reached)")
        
        # Deactivate any possibly active duel states
        for dm_list in depth_map:
            for pa_dsid in dm_list:
                dddapi.py_deactivate_duel_state(pa_dsid)
    
    dsid_results_set = set()
    for dsid_result in dsid_results:
        dsid_results_set.add(dsid_result)
        
    return dsid_results_set
    
