all:  gshell.c
	gcc -g -Wall -I . -o gsh gshell.c -I/usr/include/tcl8.4 -ltcl8.4

clean:
	rm gsh
