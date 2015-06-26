all:  gshell.c
	gcc -g -Wall -I . -o gsh gshell.c -ltcl

clean:
	rm gsh
