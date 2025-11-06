Some basic command-line utilities.

# roll / rollv / rollh
Rolls dice; takes 1 or more argument strings.

### Example Usage
1. `roll 3d4+1       # rolls 3 d4's with +1 modifier (added at the end)`
2. `roll 1d20+9^1    # rolls a single d20 with advantage, modifier +9`
3. `roll d20^-2-5    # rolls a single d20 with double disadvantage, modifer -5`

Run `rollv` to print more information or `rollh` to get a histogram.

TODO: Figure out how to actually parse command-line arguments.
