Some basic command-line utilities.

# roll / rollv / rollh
Rolls dice; takes 1 or more argument strings.

### Example Usage
1. `roll 3d4+1       # rolls 3 d4's with +1 modifier (added at the end)`
2. `roll 1d20+9^1    # rolls a single d20 with advantage, modifier +9`
3. `roll d20^-2-5    # rolls a single d20 with double disadvantage, modifer -5`

Run `rollv` to print more information or `rollh` to get a histogram. Alternatively, you can run `roll` with flags `-v` and/or `-h` respectively.

```
$ rollh 1000000d20^1
rolled 1000000d20 with advantage:
    data = [18,18,9,8,18,19,16,16,20,18,3,4,14,18,18,19,20,19,9,16,...]
    sum = 13821685
    avg = 13.822
    nat1  = 2553
    nat20 = 97274
    hist = [
         2553x  1: #
         7473x  2: ###
        12716x  3: ######
        17635x  4: #########
        22535x  5: ###########
        27295x  6: ##############
        32577x  7: ################
        37274x  8: ###################
        42638x  9: #####################
        47705x 10: ########################
        51988x 11: ##########################
        57559x 12: #############################
        62485x 13: ################################
        67772x 14: ##################################
        72471x 15: #####################################
        77476x 16: #######################################
        82729x 17: ##########################################
        87268x 18: ############################################
        92577x 19: ###############################################
        97274x 20: ##################################################
    ]
total = 13821685
```

TODO: Figure out how to actually parse command-line arguments.
