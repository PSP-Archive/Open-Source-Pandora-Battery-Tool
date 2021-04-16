export RELVER := 0.6
RELFN := _060

all: elf eboot

release: clean
	@tar -cvjf ospbt-src-$(RELVER).tar.bz2 source makefile readme.txt
	make all
	@tar -cvjf ospbt-$(RELVER).tar.bz2 ospbt-src-$(RELVER).tar.bz2 ospbt$(RELFN) ospbt_elf$(RELFN) readme.txt
	@rm -f ospbt-src-$(RELVER).tar.bz2

clean:
	@rm -r -f ospbt_elf$(RELFN)
	@rm -r -f ospbt$(RELFN)
	make -C source -f makefile_elf clean
	make -C source -f makefile_eboot clean
	make -C source/kprx clean

elf: prx
	make -C source -f makefile_elf
	@rm -r -f ospbt_elf$(RELFN)
	@mkdir -p ospbt_elf$(RELFN)
	@cp -f source/kprx/batser.prx ospbt_elf$(RELFN)/batser.prx
	@cp -f source/ospbt_strip.elf ospbt_elf$(RELFN)/ospbt$(RELFN).elf
	make -C source -f makefile_elf clean

eboot: prx
	make -C source -f makefile_eboot
	@rm -r -f ospbt$(RELFN)
	@mkdir -p ospbt$(RELFN)
	@cp -f source/kprx/batser.prx ospbt$(RELFN)/batser.prx
	@cp -f source/EBOOT.PBP ospbt$(RELFN)/EBOOT.PBP
	make -C source -f makefile_eboot clean

prx:
	make -C source/kprx
