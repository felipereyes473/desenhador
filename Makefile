compile: desenhador.c
	gcc -o desenhador desenhador.c -lSDL3 -Wall -Wextra -Wformat

run: compile
	./desenhador
