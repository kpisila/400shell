shell: shell.c
	gcc -Wall -g -I../include shell.c -o shell
clean:
	rm -rf shell shell.dSYM
