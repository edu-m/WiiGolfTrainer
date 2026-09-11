# WiiGolfTrainer

I started working on this mod back in November 2025. I've been working on and off these months, with a big gap in development due to my university commitments, chipping away at each task, slowly gathering info and now it's finally ready(-ish) for prime time.

This mod has been developed using [Kokeshi](https://github.com/PackProject/Kokeshi) and thanks from the help of the Wii Sports decomp community, whose help has been invaluable.

## Building

You will need to clone the [Kokeshi repo](https://github.com/PackProject/Kokeshi), follow the instructions to set it up. Then, in the `src/` folder, paste the contents of this repository. Then, compile with `make`, you should find the rom in the `romfs/` folder.

## Usage

Simply head to any golf game (can be both a course or a training game), you should immediately see both a 3d line on the world and a projection on the map. Both will take into account wind speed and direction, as well as the rolling of the ball itself as it hits the ground. Use the (-) button to cycle through the power levels (100% down to 10%).

In the code inside `main.cpp` there is a line `#define PREVIEW 1`, leave it to 1 if you want a text overlay on the screen that indicates current power level and distance (if the ball is estimated to go out of bounds it will report "hazard"), otherwise set it to 0.

## Preview performance

### Holeability

The HUD now shows whether the system has found a shot that puts the ball in the hole,
for the current club, ball position, lie and wind:

- `holeable: searching`: simulation still running
- `holeable: yes`: a shot is found to hole the ball
- `holeable: not found`: no successful shot was found within the search budget;
- `holeable: too far!`: you are phisically too distant to have a chance to hole, most maps don't allow hole in one;
  this is **not** a proof that the position is impossible to hole.

Note that the visible prediction takes priority. After it completes, the search borrows
the simulation ball for up to 15 physics steps per update. It tries 12 aim
directions around the full circle and 5 powers, then refines the four best
candidates over 4 levels.
it stops immediately on success. Moving the aim will temporarily pause the simulation and 
restart as soon as the aim is settled.

Success and exhausted-search results are cached across aim/power changes.
Changing player, ball position, club, lie, field, goal or wind invalidates them.
Both calculations stop doing physics work once the preview and search finish.
A successful visible preview can also establish `yes`, even after search exhaustion.
`trajectory.Holeability().Power()` and `.Angle()` expose the found shot (absolute
power in [0,1], aim in radians) when `.State() == HoleSearch::Found`.
No shot is taken automatically. Cup detection is checked each physics substep;
passing over the hole's horizontal coordinates does not count.

### Curve rendering

Select the world-curve coloring at compile time with `GOLF_CURVE_MODE` in
`main.cpp`: `0` = solid, `1` = hue phase along the curve, `2` = speed (default).
It can also be overridden with the compiler flag `-DGOLF_CURVE_MODE=1`.
`StripShiftPhase` and `StripSpeed` are separate drawing implementations.
The overhead map retains its solid outlined path.

Speed coloring gives the trajectory a red-to-black gradient as to intuitively indicate the velocity.
This is the recommended mode and the default option.
`Trajectory::StepSpeeds()` records the launch speed and every physics substep's
velocity magnitude. `StepSpeedCount()` gives the valid history length.
`Speeds()[i]` corresponds to `Points()[i]`, including the final endpoint, so
drawing needs no speed estimation. Values are in world units per substep;
multiply by 24 for m/s. Hazard speeds are captured before forced-stop cleanup.
The two speed arrays add about 15 KiB per trajectory; reset/restart invalidate
old entries through their counts, and completed idle predictions add no work.


## Note

This mod is still considered in beta, it might contain bugs. While most graphical glitches have been ironed out, there can be some issues still.
