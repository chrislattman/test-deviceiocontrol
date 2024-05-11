all:
	gcc -Wall -Wextra -Werror -pedantic -std=c99 -o user user.c

clean:
	rm -f user *.exe *.obj *.sys
