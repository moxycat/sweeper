all:
	windres res.rc -O coff -o res.res
	gcc -static -Wall -O3 -static-libgcc res.res main.c board.c -lmingw32 -lSDL2main -lSDL2 -lm -ldinput8 -ldxguid -ldxerr8 -luser32 -lgdi32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lversion -luuid -lsetupapi
	strip -s a.exe