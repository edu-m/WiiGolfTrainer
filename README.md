# WiiGolfTrainer

I started working on this mod back in November 2025. I've been working on and off these months, with a big gap in development due to my university commitments, chipping away at each task, slowly gathering info and now it's finally ready(-ish) for prime time.

This mod has been developed using [Kokeshi](https://github.com/PackProject/Kokeshi) and thanks from the help of the Wii Sports decomp community, whose help has been invaluable.

## Building

You will need to clone the [Kokeshi repo](https://github.com/PackProject/Kokeshi), follow the instructions to set it up. Then, in the `src/` folder, paste the contents of this repository. Then, compile with `make`, you should find the rom in the `romfs/` folder.

## Usage

Simply head to any golf game (can be both a course or a training game), you should immediately see both a 3d line on the world and a projection on the map. Both will take into account wind speed and direction, as well as the rolling of the ball itself as it hits the ground. Use the (-) button to cycle through the power levels (100% down to 10%).

In the code inside `main.cpp` there is a line `#define PREVIEW 1`, leave it to 1 if you want a text overlay on the screen that indicates current power level and distance (if the ball is estimated to go out of bounds it will report "hazard"), otherwise set it to 0.
