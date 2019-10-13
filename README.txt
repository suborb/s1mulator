s1mulator v0.5.0 Copyright (c) A. J. Buxton <a.j.buxton@gmail.com>. 
This software is licensed under the GPL.

Z80 core emulation Copyright (c) 2006, BlueChip of Cyborg Systems
See z80.c for more information.

This is an emulator for the S1MP3 media player.

It can currently emulate a Z80 core with an attached LCD display.

Requirements
------------

wxWidgets development packages with wxGLCanvas support.

z88dk - to compile the example asm.

Compiling
---------

Linux users should just do a 'make'

If you are cross-compiling using a Linux host, look at cross-mingw.sh.

If you want to build a source tarball and windows zip packages from a
SVN working copy (and you have all the dependancies) then just do 
'make dist'.

Compiling natively under Windows is not yet supported.

Usage
-----

s1mulator [-h] [-D <columns>,<pages>,<pport>,<ppin>,<pinv>,<mport>,<mpin>,<minv>] \
               [-B <rport>,<rpin>,<rinv>,<rport>,<rpin>,<rinv>,<rport>,<rpin>,<rinv>] \
               [-b <filename>[,<offset>]] [-e <str>] [-t <str>]

  -h, --help            Display usage info

  -D, --display=<...>   Parameters for connected display. All must be provided.
                        
        <columns> - Columns of the display. Equal to the number of horizontal pixels.
        <pages>   - Pages of the display. Vertical pixels / 8 for mono displays.
        <pport>   - Port which the power control line is mapped to.
        <ppin>    - Pin of <pport> which is connected to the display.
        <pinv>    - Whether the power control line is inverted.
        <mport>
        <mpin>
        <minv>    - As above, for the command/data mode control line.

  -B, --backlight=<...> Parameters for connected backlight. All must be provided.

        <rport>   - Port which the red backlight is mapped to.
        <rpin>    - Pin of <rport> which is connected to red backlight.
        <rinv>    - Whether the red backlight line is inverted.
        <gport>   - 
        <gpin>    - 
        <ginv>    - 
        <bport>   - 
        <bpin>    - 
        <binv>    - As above, for the green and blue backlights.

  -b, --bin=<...>       Load <filename> into memory at <offset>, or 0 if 
                        no offset given. If 0x8000, load into brom memory.

  -e, --execute=<str>   Initial execution offset.

  -t, --trap=<str>      Trap address. Execution will stop when reached.

Individual params should be separated by "," and there must be no spaces. 

 <*port> should be a number between 0 and 0xff.
 <*pin> should be one of 0x01, 0x02, 0x04, 0x08, ..., 0xf0.
 <*inv> should be 0 if false or 1 if true.

All numerical arguments may be specified in any format accepted by a C compiler.

Example: (These are also the default display and backlight settings.)

./s1mulator -D 140,6,0xf4,0x01,0,0xf4,0x04,0 \
            -B 0xee,0x01,1,0xee,0x02,0,0xee,0x04,0 \
            -b test.bin,0x3400 -e 0x3400

You should see a counting pattern on the simulated LCD, and the backlight 
should cycle colours.

You can use the "View" menu to show various information about the current 
machine state.
