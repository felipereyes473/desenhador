compile: desenhador.c
	gcc -o desenhador desenhador.c -lSDL3 -Wall -Wextra -Wformat -pedantic

run: compile
	./desenhador
