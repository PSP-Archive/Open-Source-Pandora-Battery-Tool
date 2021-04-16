// thanks to nem for sharing the battery stuff on ps2dev
// thanks to Chilly Willy for the source this is based on
//
// overwrite serial number to 0xffffffff to make service mode battery
// overwrite serial number to 0x00000000 to make autoboot mode battery
// serial number is stored at address 0x07 and address 0x09

#include <pspsdk.h>
#include <pspkernel.h>
#include <string.h>
#include <pspsysmem.h>

#define sceSysregGetTachyonVersion sceSysreg_driver_E2A5D1EE
#define sceSysconGetBaryonVersion sceSyscon_driver_7EC5A957

PSP_MODULE_INFO("BatSer", 0x1006, 1, 3);
PSP_MAIN_THREAD_ATTR(0);

// syscon functions for 3.71
u32 sceSyscon_writebat371 (u8 addr, u16 data);
u32 sceSyscon_readbat371 (u8 addr);

// syscon function for all versions used by silversprings' code
u32 sceSysconCmdExec(void* param, int unk);

int sceSysregGetTachyonVersion(void);
int sceSysconGetBaryonVersion(u32* val);

// function pointers
u32 (* writeBat) (u8 addr, u16 data);
u32 (* readBat) (u8 addr);

// thanks silverspring for the reversed functions
u32 write_eeprom(u8 addr, u16 data)
{
	u32 k1;
	k1 = pspSdkSetK1(0);
	int res;
	u8 param[0x60];

	if (addr > 0x7F)
		return(0x80000102);

	param[0x0C] = 0x73; // write battery eeprom command
	param[0x0D] = 5;	// tx packet length

	// tx data
	param[0x0E] = addr;
	param[0x0F] = data;
	param[0x10] = data>>8;
	
	res = sceSysconCmdExec(param, 0);
	
	if (res < 0)
		return(res);

	pspSdkSetK1(k1);
	return 0;
}

u32 read_eeprom(u8 addr)
{
	u32 k1;
	int res;
 	k1 = pspSdkSetK1(0);
	u8 param[0x60];

	if (addr > 0x7F)
		return(0x80000102);
		
	param[0x0C] = 0x74; // read battery eeprom command
	param[0x0D] = 3;	// tx packet length

	// tx data
	param[0x0E] = addr;
		
	res = sceSysconCmdExec(param, 0);

	if (res < 0)
		return(res);

	// rx data
	return((param[0x21]<<8) | param[0x20]);
}
// end functions from silverspring

void RetVer(u32* ver) // requires an array of 3 u32s
{
	u32 k1, tmp;
	k1 = pspSdkSetK1(0);
	ver[0] = sceKernelDevkitVersion();
	ver[1] = sceSysregGetTachyonVersion();
	sceSysconGetBaryonVersion(&tmp);
	ver[2] = tmp;
	pspSdkSetK1(k1);
	return;
}	

// 0x8025000x are module errors
// anything above 0x80250080 are hw errors
// 0x802500b8 - read error on a fake battery
int errCheck(u32 data)
{
	if ((data & 0x80250000) == 0x80250000) // old way (data & 0x80000000) <- checking only for -1 wrather than specifically a syscon error
		return -1;
	else if (data & 0xffff0000)
		return ((data & 0xffff0000)>>16);
	return 0;
}

// read the serial data locations
int ReadSerial(u16* pdata)
{
	int err = 0;
	u32 k1, data;

	k1 = pspSdkSetK1(0);

	// serial number is stored at address 0x07 and address 0x09
	data = readBat(0x07); // lower 16bit
	err = errCheck(data);
	if (!(err<0))
	{
		pdata[0] = (data &0xffff);
		data = readBat(0x09); // upper 16bit
		err = errCheck(data);
		if (!(err<0))
			pdata[1] =  (data &0xffff);
		else
			err = data;
	}
	else
		err = data;

	pspSdkSetK1(k1);
	return err;
}

// write the serial data locations
int WriteSerial(u16* pdata)
{
	int err = 0;
	u32 k1;

	k1 = pspSdkSetK1(0);

	err = writeBat(0x07, pdata[0]); // lower 16bit
	if (!err)
		err = writeBat(0x09, pdata[1]); // lower 16bit

	pspSdkSetK1(k1);
	return err;
}

// read anywhere in the entire prom
int ReadProm(int address, u16* pdata)
{
	int err = 0;
	u32 k1, data;

	k1 = pspSdkSetK1(0);

	// read the data at address
	// serial number is stored at address 0x07 and address 0x09
	data = readBat(address); // lower 16bit
	err = errCheck(data);
	if (!(err<0))
	{
		pdata[0] = (data &0xffff);
	}
	else
		err = data;

	pspSdkSetK1(k1);
	return err;
}

// write anywhere in the entire prom
int WriteProm(int address, u16* pdata)
{
	int err = 0;
	u32 k1;

	k1 = pspSdkSetK1(0);

	err = writeBat(address, pdata[0]);

	pspSdkSetK1(k1);
	return err;
}

int module_start(SceSize args, void *argp)
{
	u32 k1;
	int vers, ret = 0;
	// setup the functions
	k1 = pspSdkSetK1(0);
	vers = sceKernelDevkitVersion();
	if (vers == 0x03070110) // if FW is 3.71, keep compatibility
	{
		writeBat = sceSyscon_writebat371;
		readBat = sceSyscon_readbat371;
	}
	else // less than 3.71 ~ 3.70 was never set as homebrew-enabled afaik, 3.80 has a niftly NID translator that deals with the gorry details for us
	{
		writeBat = write_eeprom;
		readBat = read_eeprom;
	}
	pspSdkSetK1(k1);
	return ret;
}

int module_stop()
{
	return 0;
}
