
include $(PSL1GHT)/Makefile.base

.PHONY: all clean

all:
		$(MAKE) pkg -C hheretic
		$(MAKE) pkg -C hhexen

clean:
	
		$(MAKE) clean -C hheretic
		$(MAKE) clean -C hhexen
		
	
