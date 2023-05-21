buildmac:
	gcc -g -Wall -Werror -Wno-unused-function -Wno-unused-variable -lraylib chip8.c -o chip8
	
clean:
	rm -f chip8