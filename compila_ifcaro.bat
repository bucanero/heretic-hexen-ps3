@echo off

@set PS3DEV=j:/ps3dev
@set MINGW=%PS3DEV%/MinGW

@set CYGWIN= nodosfilewarning
@set PATH=%PS3DEV%/cygwin;%MINGW%/bin;%MINGW%/msys/1.0/bin;%PS3DEV%/ppu/bin;%PS3DEV%/spu/bin;%PATH%;

@set PSL1GHT=%PS3DEV%/psl1ght

make

pause
