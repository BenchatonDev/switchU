# Conway Game of life SDL2 Wii U
An implementation of Conway's Game of Life in C/C++ and SDL2.

![Alt Text](media/showcase_01.gif)
Create and destroy cells using A and B, respectively. Watch the beauty unfold.

![Alt Text](media/showcase_02.gif)
Cells record their age. Red cells are volatile and changing; the older stationary cells emerge as green. Hovering the mouse over groups of cells will illuminate them.

## Controls
- `HOME`:       quit
- `+`:          pause/play
- `R`:          toggle wireframe
- `L`:          toggle colorization
- `ZL`:         reduce pointer speed
- `ZR`:         increase pointer speed
- `L Stick`:    move the pointer
- `Dpad Up`:    slow down
- `Dpad Down`:  speed up
- `A`:          spawn cell
- `B`:          delete cell
- `X`:          grow spawn perimeter
- `Y`:          shrink spawn perimeter

## Building
### Dependencies
- [wut](https://github.com/devkitPro/wut)
- SDL2 Wii U

Install Devkitpro following [the official guide for your OS](https://devkitpro.org/wiki/Getting_Started)

Install wut:
```
(sudo) (dkp-)pacman -Syu --needed wiiu-dev
```

Install SDL2 Wii U:
```
(sudo) (dkp-)pacman -S wiiu-sdl2
```

### Compiling
Simply run `make` at the root of the repo
```
make (path to Makefile)
```

# Credits
- [MatthewDiMarco](https://github.com/MatthewDiMarco) Made the original program
- [Ashquarky](https://github.com/ashquarky) For porting SDL2 to Wii U
- [Dimok](https://github.com/dimok789) For making VPADInput.h and WPADInput.h
- [GaryOderNichts](https://github.com/GaryOderNichts) For making Wii U ident which I used as 'base'
