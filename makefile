

nsh: main.c parse.c parse.h
	gcc -o nsh main.c parse.h parse.c

debug: main.c parse.c parse.h
	gcc -o nsh main.c parse.h parse.c -ggdb

clean: nsh
	rm nsh
