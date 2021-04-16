Open Source Pandora Battery Tool 0.60 <- for SE/OE/M33/Des.Cem homebrew enabled kernels
-------------------------------------
Note: after the initial confirmation, there are _NO_ OTHERS!
see bottom for changelog

Allows the same options as the original Pandora tool:
service mode	writes a serial 0xFFFFFFFF to the battery eeprom (inserting the battery will power on and wait for a proper MS)
auto boot mode	writes a serial 0x00000000 to the battery eeprom (inserting the battery will power on and boot the firmware)
dummy normal	writes a serial 0x12345678 to the battery eeprom (the battery will work normally)
backup eeprom	reads the battery eeprom to a file "eeprom.bin" in the root of a MS
restore eeprom	if a file "eeprom.bin" is present, it will attempt to write it to the batteries eeprom

additional option:
Flash only serial from ms0:/eeprom.bin (0x########)
	######## is the serial from the dump
	will flash only the serial number found in the dump. Best option instead of restoring the entire dump.
		With the first byte of a dump being [0], and the serial being 0x12345678 (for hex editing)
		[14] = 0x34
		[15] = 0x12
		[18] = 0x78
		[19] = 0x56

May not work with all batteries.

Installation:
-------------
Use whatever method you'd normally use to run homebrew under 3.xx or 2.xx OE/SE/M33 kernels on your PSP.

example: 
-put the folder "ospbt_xx" (with eboot and prx) into /PSP/GAME371
-run it from the PSP MS menu.

example2:
-put the contents of the elf folder (.elf and .prx) into the ms0:/elf
-run with jas0nuk's elf menu (possibly outdated version can be found here: http://forums.maxconsole.net/showthread.php?t=83119)

known issues:
-------------
-when writing a full battery eeprom.bin file to a battery, errors may occur. Redumps may show differences from the dump being written.
-when no eeprom.bin file is present on the MS, the option to write the file to a battery will intentionally be grayed out.
-only relevant options are listed and available with any given battery.
-after the intial confirmation, there are NO OTHERS!

No warranties expressed or implied. GPL is included with the source, source modifications should be published with rebuilds.

credits:
---------
- Chilly Willy for releasing source to his apps (3.xx examples), and for the key waiting routines used in this app
- nem for sharing the battery syscon prototypes on ps2dev (BIG thanks!)
- Everyone involved with Prometheus for creating and releasing the Pandora project (including but not limited to Noobz)
- M33 for enabling homebrew on the kernel I tested this on
- Dark_Alex for proving CFW was possible, then going 1000 steps further
- Everyone behind the forums and toolchains at PS2dev.org <- without them this stuff just... wouldn't be.
- SilverSpring for your never ending patience with me, for the reversed NVM routines, and the true NVM NID name.
- whomever it was who scared everyone into not testing the battery creators with slim batteries. You were wrong. It works fine
	if you only write the serial.
- Icon and pic provided by Tracksol of maxconsole, thanks!
	original artwork can be found and prints thereof purchased from http://blackeri.deviantart.com/gallery/
	Thanks blackeri, your images are very impressive :)
- my father for always encouraging me to go on, rest in peace.
- anyone else I may have forgotten.......

version history:
-----------------
0.6 - updated build/makefile for auto build/packing to bzip format
	- added icon/backpic (thanks Tracksol)
	- added check and warning for new version slims
0.52- fixed misc. syscon error handling (previously only handled battery not present error)
0.5 - updated to work with all current custom firmwares (at this point 3.80m33 is the most recent), elf also tested under des.cem V3
0.4 - by reqest (Mandingo@maxconsole) when eeprom file is present it will now display the serial from the dump on the option line
0.3 - tested as working on a slim battery
	- added option to restore only the serial from a dump instead of the full dump
	- added an elf folder for use with jas0nuk's elf menu (put both the elf and prx into ms:/elf/ folder)
0.2 - updated to support 3.71 syscon NIDs, should work under every SE/OE/M33 version to date.
0.1 - initial release, confirmed working in 3.52m33 and 3.60m33(slim) with PSP-1000 type batteries

If anyone feels I have abused the privelidges of their source releases, let me know and I will try to rectify it.

cory1492

cory1492 @ gmail . com
