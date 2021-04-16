   .set noreorder

#include "pspstub.s"

   STUB_START   "sceSyscon_driver",0x00010000,0x00020005
	STUB_FUNC	0x40CBBD46,sceSyscon_writebat371
	STUB_FUNC	0xB9C1B0AC,sceSyscon_readbat371
   STUB_END

