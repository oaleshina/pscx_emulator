# PlayStation emulator

Investigation of console's emulation, writing emulator on C++ language.
The emulator contains several modules that are needed to successfully run BIOS
or PS logo screen if disc is present.

Currently performance is poor, the goal of the project is to investigate how PS1 emulation
should work. Run the code in release mode.

## Implemented

![PlayStation Boot Screen](https://github.com/oaleshina/pscx_emulator/playstation_boot_screen.png)

* CPU
* Instruction Cache
* Interrupts
* DMA
* Basic GPU ( without textures support )
* Basic GTE support ( several instructions, not all implemented yet )
* Timers ( incomplete )
* CDROM controller ( not all instructions are implemented yet )
* Gamepad controller ( haven't tested yet )

## TODO ( Scheduled to implement )

* GPU additional instructions
* MDEC
* SPU
* Other things

## Build

Visual studio 2017 C++11/14. Run code in release mode, as C++ containers were used.

## Run

To run the emulator, you should provide the BIOS binary file. Currently emulator works with SCPH1001 one.

![PlayStation Bios Screen](https://github.com/oaleshina/pscx_emulator/playstation_bios_screen.png)

Command line:

```
pscx_emulator.exe [path to the SCPH1001 BIOS] 
```

![PlayStation Logo Screen](https://github.com/oaleshina/pscx_emulator/playstation_logo_screen.png)

To run the game you should use such command line:

```
pscx_emulator.exe [path to the SCPH1001 BIOS] -disc [path to the disc]
```

Currently the PS logo should be rendered without game launching.