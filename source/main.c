/*
	Open Source Pandora Battery Tool - program to modify the EEPROM contents of a PSP battery
*/

#include <pspsdk.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspiofilemgr.h>
#include <pspdebug.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define printf pspDebugScreenPrintf
#define setc pspDebugScreenSetXY

PSP_MODULE_INFO("PromBat", 0, 1, 6);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);

// batser.prx
void RetVer(u32* version);
int ReadSerial(u16* pdata);
int WriteSerial(u16* pdata);
int ReadProm(int address, u16* pdata);
int WriteProm(int address, u16* pdata);
char appname[45] ={"Open Source Pandora's Battery Tool"};

#ifndef RELVER
	#define RELVER "0.xx"
#endif

// some constants
#define ROW_OFFSET 7 // the number of lines down that the menu begins on
#define NORMAL_MODE 0 // battery serial is neither service or autoboot
#define SERVICE_MODE 1 // battery is already service mode
#define AUTOBT_MODE 2 // battery is already autoboot mode

// some globals
u16 row;
u16 rowprev;
u16 serialdata[2]; // place to stow the current serial
u16 fileserial[2]; // place to stow the current file serial
u16 buffer[0x80]; // buffer for a dump of the prom
int mode;
int dumpExist; // if the dump is in root and the proper size this is set to 1
int maxrow;

// I hate writing controller code, this is from Chilly Willy's app as it works well for this
void wait_release(unsigned int buttons)
{
	SceCtrlData pad;

	sceCtrlReadBufferPositive(&pad, 1);
	while (pad.Buttons & buttons)
	{
		sceKernelDelayThread(10000);
		sceCtrlReadBufferPositive(&pad, 1);
	}
}

unsigned int wait_press(unsigned int buttons)
{
	SceCtrlData pad;

	sceCtrlReadBufferPositive(&pad, 1);
	while (1)
	{
		if (pad.Buttons & buttons)
			return pad.Buttons & buttons;
		sceKernelDelayThread(10000);
		sceCtrlReadBufferPositive(&pad, 1);
	}
	return 0;   /* never reaches here, again, just to suppress warning */
}

int confirm(void)
{
	SceCtrlData pad;

	while (1)
	{
		sceKernelDelayThread(10000);
		sceCtrlReadBufferPositive(&pad, 1);
		if(pad.Buttons & PSP_CTRL_CROSS)
		{
			wait_release(PSP_CTRL_CROSS);
			return 1;
		}
		if(pad.Buttons & PSP_CTRL_CIRCLE)
		{
			wait_release(PSP_CTRL_CIRCLE);
			return 0;
		}
	}
	return 0;   /* never reaches here, this suppresses a warning */
}
// end excerpt from Chilly Willy's app

void exitApp(void)
{
	sceKernelDelayThread(3*1000*1000);
	sceKernelExitGame();
}

int fileExist(const char* fname)
{
	int fp, size;
	memset((char*)buffer, 0xFF, 256);
	fp = sceIoOpen(fname, PSP_O_RDONLY, 0777);
    if (fp <= 0)
			return 0;
	size = sceIoRead(fp, &buffer, 256); // read in the file and get the size
	sceIoClose(fp);
	if (size != 256)
		return 0; // invalid eeprom dump size
	fileserial[0] = buffer[7]; // copy the serial from the file data to the file serial buffer
	fileserial[1] = buffer[9];
	memset((char*)buffer, 0xFF, 256);
	return 1;
}

void showError (int err)
{
	pspDebugScreenSetTextColor(0x000000ff);
	if (err == 0x802500b8) // battery not in place or not ready 0x802500b8
		printf("Error: battery not ready! (0x%08x)\n  Exiting now...\n", err);
	else
		printf("Error: (0x%08x)\n  Exiting now...\n", err);
}

void getSerial(void)
{
	int err;
	// get and show the serial data
	err = ReadSerial(serialdata);
	if (err < 0)
	{
		showError(err);
		exitApp();
	}
	return;
}	

void showSerial(void)
{
	getSerial();
	printf("Current serial: ");
	pspDebugScreenSetTextColor(0x000000ff);
	printf("0x%04x%04x", serialdata[0], serialdata[1]);
	pspDebugScreenSetTextColor(0x0000ff00);
	
	if ((serialdata[0] == 0xFFFF) && (serialdata[1] == 0xFFFF))
	{
		printf(" - service mode battery\n\n");
		mode = SERVICE_MODE; // sets 1 if it is already a service mode battery
	}
	else if ((serialdata[0] == 0x0000) && (serialdata[1] == 0x0000))
	{
		printf(" - autoboot battery\n\n");
		mode = AUTOBT_MODE; // sets 2 if it is already a autoboot battery
	}
	else
	{
		printf(" - unknown or normal serial\n\n");
		mode = NORMAL_MODE; // sets 0 if it has another serial
	}
	pspDebugScreenSetTextColor(0x00ffffff);
	return;
}

void showWarn(void)
{
	u32 ver[3];
	RetVer(ver);
	pspDebugScreenClear();
	pspDebugScreenSetTextColor(0x00ff0000);
	printf("%s %s\n\n", appname, RELVER);
	pspDebugScreenSetTextColor(0x00ffffff);
	printf("Devkit : 0x%08lx\n", ver[0]);
	printf("Tachyon: 0x%08lx\nBaryon : 0x%08lx\n", ver[1], ver[2]);
	if((ver[1] == 0x00500000) && (ver[2] > 0x0022B200)) // newer slims will not support PROM manipulation using known commands
//	if((ver[1] == 0x140000) && (ver[2] > 0x30500)) // test to see this works on my own unit which isn't a newer slim
	{
		pspDebugScreenSetTextColor(0x000000ff);
		printf("\n\n\n**WARNING**\n\n");
		printf("PSP VERSION NOT SUPPORTED!!\n\n");
		pspDebugScreenSetTextColor(0x00ffffff);
		printf("This version of PSP does not support the currently known\n");
		printf("commands used by this application for PROM read/write\n\n\n\n");
		printf("Press X or O to quit.");
		confirm();
		printf("\n\n\nExiting...\n");
		exitApp();
	}
	else
	{
		pspDebugScreenSetTextColor(0x000000ff);
		printf("\n\n\n**WARNING**\n\n");
		printf("USE AT YOUR OWN RISK!!\n\n");
		pspDebugScreenSetTextColor(0x00ffffff);
		printf("Press X to continue or O to quit.");
		if (!confirm())
		{
			printf("\n\n\nExiting...\n");
			exitApp();
		}
	}
}

void refreshArrow(void)
{
	setc(1, rowprev + ROW_OFFSET);
	printf("   ");
	setc(1, row + ROW_OFFSET);
	printf("-->");
}

void showMenu(void)
{
	pspDebugScreenClear();
	maxrow = 3;
//	printf("mode = %d\n", mode); //debug
	pspDebugScreenSetTextColor(0x00ff0000);
	printf("%s %s\n", appname, RELVER);
	pspDebugScreenSetTextColor(0x00ffffff);
	printf("This program is an adaptation of Chilly Willy and nem's code.\nThanks to both and everyone in Prometheus!\n\n\n");
	showSerial();
	printf("     - Quit now\n");
	if(mode != 0) printf("     - Convert to normal mode (0x12345678)             \n");
	if(mode != 1) printf("     - Convert to service mode (aka: Pandora's battery)\n");
	if(mode != 2) printf("     - Convert to autoboot mode (0x00000000)           \n");
	printf("     - Backup battery eeprom to ms0:/eeprom.bin\n");
	if(fileExist("ms0:/eeprom.bin"))
	{
		printf("     - Flash only serial from ms0:/eeprom.bin (0x%04x%04x)\n", fileserial[0], fileserial[1]);
		printf("     - Flash full EEPROM from ms0:/eeprom.bin\n");
		maxrow = 5;
	}
	else
	{
		pspDebugScreenSetTextColor(0x00aaaaaa);
		printf("     - Flash only serial from ms0:/eeprom.bin (no dump)\n");
		printf("     - Flash full EEPROM from ms0:/eeprom.bin (no dump)\n");
		pspDebugScreenSetTextColor(0x00ffffff);
	}
	rowprev = 0; row = 0;
	refreshArrow();
}

void backupBattery(void)
{
	int fp, address, err, ttlerr = 0;
	setc(0,15);
	printf("reading battery data...\n\n");
	pspDebugScreenSetTextColor(0x000000ff);
	for (address = 0; address < 0x80; address++)
	{
		err = ReadProm(address, (u16*) buffer+address);
		if (err < 0) // error occurred
		{
			printf("error: addr 0x%02x (0x%08x)", address, err);
			ttlerr++;
		}
	}
	if (ttlerr > 0)
		printf("\n%i read errors occurred!\n\n", ttlerr);
	pspDebugScreenSetTextColor(0x00ff0000);
	printf("Opening file ms0:/eeprom.bin\n\n");
	fp = sceIoOpen("ms0:/eeprom.bin", PSP_O_WRONLY | PSP_O_CREAT, 0777);
	if (fp <= 0)
	{
		pspDebugScreenSetTextColor(0x000000ff);
		printf("I/O error, unable to create file!\nExiting...\n");
		exitApp();
	}
	printf("Writing dump to file\n\n");
	sceIoWrite(fp, (u8*)buffer, 256);
	sceIoClose(fp);
	pspDebugScreenSetTextColor(0x00ffffff);
	printf("Done! Press X to continue.");
	while(!confirm());
	showMenu();
}

void restoreBattery(void)
{
	int fp, address, err, ttlerr = 0;
	setc(0,15);
	memset((char*)buffer, 0xFF, 256);
	printf("opening file ms0:/eeprom.bin\n");
	fp = sceIoOpen("ms0:/eeprom.bin", PSP_O_RDONLY, 0777);
	pspDebugScreenSetTextColor(0x000000ff);
	if (fp > 0)
	{
		pspDebugScreenSetTextColor(0x00ff0000);
		printf("reading file...\n");
		sceIoRead(fp, &buffer, 256);
		sceIoClose(fp);
		printf("writing to battery...\n\n");
		pspDebugScreenSetTextColor(0x000000ff);
		for (address = 0; address < 0x80; address++)
		{
			err = WriteProm(address, (u16*) buffer+address);
			if (err) // error occurred
			{
				printf("0x%02x ", address);
				ttlerr++;
			}
		}
		if (ttlerr > 0)
			printf("\nwrite errors occurred!\n\n");
	}
	else
		printf("file open error!\n\n");
	pspDebugScreenSetTextColor(0x00ffffff);
	printf("Done! Press X to continue.");
	while(!confirm());
	showMenu();
}

void restoreSerial(void)
{
	setc(0,15);
	printf("checking file ms0:/eeprom.bin\n");
	pspDebugScreenSetTextColor(0x000000ff);
	if (fileExist("ms0:/eeprom.bin"))
	{
		pspDebugScreenSetTextColor(0x00ff0000);
		printf("writing 0x%04x%04x\n\n", fileserial[0], fileserial[1]);
		WriteSerial(fileserial);
		printf("verify: ");
		getSerial();
		printf("0x%04x%04x ", serialdata[0], serialdata[1]);
		if ((serialdata[0] == fileserial [0]) && (serialdata[1] == fileserial [1]))
		{
			pspDebugScreenSetTextColor(0x00ff0000);
			printf("Success!\n\n");
		}
		else
		{
			pspDebugScreenSetTextColor(0x000000ff);
			printf("Failed!\n\n");
		}
	}
	else
		printf("file open error!\n\n");
	pspDebugScreenSetTextColor(0x00ffffff);
	printf("Done! Press X to continue.");
	while(!confirm());
	showMenu();
}

void changeSerial(int option)
{
	setc(0,15);
	switch(option)
	{
		case SERVICE_MODE:
			printf("Converting to service mode (0xFFFFFFFF)...\n\n");
			buffer[0] = 0xffff; buffer [1] = 0xffff;
			break;
		case AUTOBT_MODE:
			printf("Converting to autoboot mode (0x00000000)...\n\n");
			buffer[0] = 0x0000; buffer [1] = 0x0000;
			break;
		default:
			printf("Converting to normal mode (0x12345678)...\n\n");
			buffer[0] = 0x1234; buffer [1] = 0x5678;
			break;
	}
	pspDebugScreenSetTextColor(0x00ff0000);
	printf("writing 0x%04x%04x\n\n", buffer[0], buffer[1]);
	WriteSerial(buffer);
	printf("verify: ");
	getSerial();
	printf("0x%04x%04x ", serialdata[0], serialdata[1]);
	if ((serialdata[0] == buffer [0]) && (serialdata[1] == buffer [1]))
	{
		pspDebugScreenSetTextColor(0x00ff0000);
		printf("Success!\n\n");
	}
	else
	{
		pspDebugScreenSetTextColor(0x000000ff);
		printf("Failed!\n\n");
	}
	pspDebugScreenSetTextColor(0x00ffffff);
	printf("Done! Press X to continue\n");
	while(!confirm());
	showMenu();
}

// handle keypresses
void doMenu(void)
{
	while(1)
	{
		unsigned int b;
		b = wait_press(PSP_CTRL_UP|PSP_CTRL_DOWN|PSP_CTRL_CROSS);
		wait_release(PSP_CTRL_UP|PSP_CTRL_DOWN|PSP_CTRL_CROSS);
		if (b & PSP_CTRL_UP)
		{
			rowprev = row;
			if(row != 0) row--;
			else row = maxrow;
			refreshArrow();
		}
		else if (b & PSP_CTRL_DOWN)
		{
			rowprev = row;
			if(row != maxrow) row++;
			else row = 0;
			refreshArrow();
		}
		else if (b & PSP_CTRL_CROSS) // figure out which menu option was selected and use it
		{
			if (row == 0) // 0 is always quit
			{
				setc(0,15);
				printf("Exiting...");
				return;
			}
			else if (row == 1) // change serial 1
			{
				if (mode == NORMAL_MODE) // convert to service mode
					changeSerial(SERVICE_MODE);
				else // convert to normal mode 0x12345678
					changeSerial(NORMAL_MODE);
			}
			else if (row == 2) // change serial 2
			{
				if (mode == AUTOBT_MODE) // convert to service mode
					changeSerial(SERVICE_MODE);
				else // convert to autoboot mode 0x00000000
					changeSerial(AUTOBT_MODE);
			}
			else if (row == 3) // backup EEPROM
			{
					backupBattery();
			}
			else if (row == 4) // restore serial from EEPROM dump
			{
					restoreSerial();
			}
			else if (row == 5) // restore full EEPROM from dump
			{
					restoreBattery();
			}
		}
	}
	return;
}

int main(void)
{
	pspDebugScreenInit();
	pspDebugScreenSetBackColor(0x00000000);
	pspDebugScreenSetTextColor(0x00ffffff);
	pspDebugScreenClear();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_DIGITAL);
	row = 0;

	// load the kernel module up
//	SceUID mod = pspSdkLoadStartModule("ms0:/elf/batser.prx", PSP_MEMORY_PARTITION_KERNEL); // temporal fix until jas0n releases a fixed elf menu
	SceUID mod = pspSdkLoadStartModule("batser.prx", PSP_MEMORY_PARTITION_KERNEL); // for relative path
	if (mod < 0)
	{
		printf("  Error 0x%08X loading/starting batser.prx.\n", mod);
		exitApp();
	}
	showWarn();
	showMenu();
	doMenu();

	exitApp();	
	return 0;   // never reaches here, again, just to suppress warning 
}
