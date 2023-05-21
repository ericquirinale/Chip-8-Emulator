#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <raylib.h>
#include <unistd.h>
#include <sys/time.h>

#define MEMSIZE 4096
#define SWIDTH 64
#define SHEIGHT 32
#define SCALE 10
#define DISPLAYWAIT true
#define CPURATE 15


unsigned char fontset[80] = 
{ 
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F 
};

typedef struct chip8{
    uint16_t opcode; //2 bytes
    uint8_t memory[MEMSIZE];
    uint8_t reg[16];
    uint16_t indexReg;
    uint16_t pc; //program counter
    uint8_t delayTimer;
    u_int8_t soundTimer;
    uint16_t stack[16];
    uint8_t sp; //stack pointer
    bool key[16]; //keypad
    bool draw; //draw flag
    bool graphics[SWIDTH][SHEIGHT]; //current display
    //uint8_t oldGraphics[SWIDTH][SHEIGHT]; //old display
    FILE *rom;
} c8;

void initialize(char *romName);
void updateTimers();
void unknownOpcode();
void cycle(int cycleNum);
void updateScreen();
bool checkKeyDown(int key);
bool checkKeyUp(int key);
void keymap();
int getK(int key);
void updateKeyReg();
void clearKeys();
void playSound();
