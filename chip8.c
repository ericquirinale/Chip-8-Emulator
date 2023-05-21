#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip8.h"
#include <raylib.h>
#include <unistd.h>
#include <sys/time.h>

c8 ch8;
void unknownOpcode(){
    printf("Opcode nnot found: %X/n", ch8.opcode);
    exit(42);
}

void initialize(char *romName){
    
    ch8.rom= fopen(romName, "rb");
    if (!ch8.rom)
    {
        printf("couldn't find rom\n");
        exit(1);
    }

    fread(ch8.memory+0x200, 1, MEMSIZE-0x200, ch8.rom); //load the rom into memorys

    for (size_t i = 0; i < 80; i++) //load fontset into memory
    {
        ch8.memory[i] = fontset[i];
    }

    ch8.pc=0x200; //pc starts at 0x200
    ch8.sp=0;
    ch8.indexReg=0;
    ch8.opcode=0;
    ch8.delayTimer=0;
    ch8.soundTimer=0;
    ch8.draw=false;
    for (size_t i = 0; i < 16; i++)
    {
        ch8.stack[i]=0;
        ch8.reg[i]=0;
        ch8.key[i] = false;
    }
}

void cycle(int cycleNum){ //executes the opcode

    uint8_t x, y, n, nn;
    uint16_t nnn, pixel;

    ch8.opcode = ch8.memory[ch8.pc] << 8 | ch8.memory[ch8.pc+1];
    x = (ch8.opcode >> 8) & 0x000f; //4 bit register identifier second (example 8xy1)
    y = (ch8.opcode >> 4) & 0x000f; //second 4 bit register identifier 
    n = ch8.opcode & 0x000f; //last 4 bits
    nn = ch8.opcode & 0x00ff; //last 8 bits
    nnn = ch8.opcode & 0x0fff; //last 12 bits

    int k = 0; //int for key
    bool carry;
    switch((ch8.opcode & 0xf000)){ //decode the next instruction by looking at the first "nibble"
        case 0x0000:
            switch (nn)
            {
                case 0x00e0:
                    //Clears display
                    for (size_t x = 0; x < SWIDTH; x++)
                    {
                        for (size_t y = 0; y < SHEIGHT; y++)
                        {
                            ch8.graphics[x][y] = 0;
                        }
                        
                    }
                    ch8.draw = true;
                    ch8.pc+=2;
                    break;
            
                case 0x00ee:
                    //returns from a subroutine
                    ch8.pc = ch8.stack[(--ch8.sp)];
                    break;
                default:unknownOpcode();
            }
        break;
        case 0x1000:
            //jumps to address
            ch8.pc = nnn;
            break;
        case 0x2000:
            //calls subroutne
            ch8.stack[ch8.sp] = ch8.pc+2;
            ch8.sp++;
            ch8.pc = nnn;
            break;
        case 0x3000:
            //Skips the next instruction if VX equals NN
            if (ch8.reg[x] == (nn))
            {
                ch8.pc+=4;
            }
            else{
                ch8.pc+=2;
            }
            break;
        case 0x4000:
            //Skips the next instruction if VX does not equal NN
            if (ch8.reg[x] != (ch8.opcode & 0x00ff))
            {
                ch8.pc+=4;
            }
            else{
                ch8.pc+=2;
            }
            break;
        case 0x5000:
            //Skips the next instruction if VX equals VY
            if (ch8.reg[x] == ch8.reg[y])
            {
                ch8.pc+=4;
            }
            else{
                ch8.pc+=2;
            }
            break;
        case 0x6000:
            //Sets VX to NN.
            ch8.reg[x] = nn;
            ch8.pc+=2;
            break;
        case 0x7000:
            //Adds NN to VX
            ch8.reg[x]+=nn;
            ch8.pc+=2;
            break;
        case 0x8000:
            switch (n)
            {
            case 0x0000:
                //set vx to vy
                ch8.reg[x]=ch8.reg[y];
                break;
            case 0x0001:
                //set vx to vx or vy
                ch8.reg[x]|=ch8.reg[y];
                ch8.reg[0xf] = 0;
                break;
            case 0x0002:
                //set vx to vx and vy
                ch8.reg[x]&=ch8.reg[y];
                ch8.reg[0xf] = 0;
                break;
            case 0x0003:
                //set vx to vx xor vy
                ch8.reg[x]^=ch8.reg[y];
                ch8.reg[0xf] = 0;
                break;
            case 0x0004:
                //Adds VY to VX. VF is set to 1 when there's a carry, and to 0 when there is not.
                carry = ((int)ch8.reg[x]+(int)ch8.reg[y]) > 255 ? 0x01 : 0x00;
                ch8.reg[x]+=ch8.reg[y];
                ch8.reg[0xf] = carry; //set carry flag if the addition is greater than 255
                break;
            case 0x0005:
                //VY is subtracted from VX. VF is set to 0 when there's a borrow, and 1 when there is not.
                carry = ((int)ch8.reg[x] > (int)ch8.reg[y]) ? 0x01 : 0x00; 
                ch8.reg[x]-=ch8.reg[y];
                ch8.reg[0xf] = carry; //set carry flag if the addition is greater than 255
                break;
            case 0x0006:
                //Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
                carry = ch8.reg[x] & 0x1;
                ch8.reg[x] = ch8.reg[y];
                ch8.reg[x] = (ch8.reg[x] >> 1);   
                ch8.reg[0xf] = carry;
                break;
            case 0x0007:
                //Sets VX to VY minus VX. VF is set to 0 when there's a borrow, and 1 when there is not.
                ch8.reg[x] = ch8.reg[y] - ch8.reg[x];
                ch8.reg[0xf] = ((int)ch8.reg[y] > (int)ch8.reg[x]) ? 0x01 : 0x00; //set carry flag if the addition is greater than 255
                break;
            case 0x000e:
                //Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
                carry = (ch8.reg[x]>>7) & 0x1;
                ch8.reg[x] = ch8.reg[y];
                ch8.reg[x] = (ch8.reg[x] << 1);   
                ch8.reg[0xf] = carry; 
                break;
            default: unknownOpcode();
            }
            ch8.pc+=2;
            break;
        case 0x9000:
            //Skips the next instruction if VX does not equal VY.
            if (ch8.reg[x]!=ch8.reg[y])
            {
                ch8.pc+=4;
            }
            else{
                ch8.pc+=2;
            }
            break;
        case 0xA000:
            //Sets I to the address NNN
            ch8.indexReg = nnn;
            ch8.pc+=2;
            break;
        case 0xB000:
            //Jumps to the address NNN plus V0
            ch8.pc=nnn+ch8.reg[0];
            break;
        case 0xC000:
            //Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
            ch8.reg[x]= (rand()%256) & nn;
            ch8.pc+=2;
            break;
        case 0xD000:
            //Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. Each row of 8 pixels is read as bit-coded starting from memory location I; I value does not change after the execution of this instruction. As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn, and to 0 if that does not happen.
            if(DISPLAYWAIT){
                if (cycleNum)
                {
                    break;
                }
            }
            ch8.reg[0xf] = 0; //reset collision flag
            
            uint8_t xcoord = ch8.reg[x] % SWIDTH; //get coordinates with wrap around values
            uint8_t ycoord = ch8.reg[y] % SHEIGHT;

            for (size_t y = 0; y < n; y++) //length (n) (how many rows)
            {
                uint8_t line =  ch8.memory[ch8.indexReg+y]; //horizontal sprite line
                uint8_t ty = (ycoord+y);
                for (size_t x = 0; x < 8; x++) //horizontal line (always 8 bits)
                {
                    uint8_t tx = (xcoord+x); 
                    
                    if ((line & (0x80 >> x)) != 0) //current bit in sprite is on
                    {
                        if (tx>=SWIDTH)
                        {
                            break;
                        }
                        
                        if(ch8.graphics[tx][ty]==1){ //if display is one
                            ch8.reg[0xf] = 1;
                        }
                        ch8.graphics[tx][ty]^=1;
                    }
                    else{
                        ch8.graphics[tx][ty]^=0;
                    }  
                }
                if (ty>=SHEIGHT)
                {
                    break;
                }
            }
            ch8.draw = true;
            ch8.pc+=2;
            break;

        case 0xE000:
            switch (n)
            {
            case 0x000e:
                //Skips the next instruction if the key stored in VX is pressed
                //ch8.pc += (checkKeys(ch8.reg[x])) ? 4 : 2;
                ch8.pc += (ch8.key[ch8.reg[x]]) ? 4 : 2;
                break;
            case 0x0001:
                //Skips the next instruction if the key stored in VX is not pressed
                //ch8.pc += (checkKeys(ch8.reg[x])) ? 2 : 4;
                ch8.pc += (ch8.key[ch8.reg[x]]) ? 2 : 4;
                break;
            default: unknownOpcode();
                break;
            }
            break;
        case 0xF000:
            switch (n)
            {
            case 0x0007:
                //Sets VX to the value of the delay timer.
                ch8.reg[x] = ch8.delayTimer;
                ch8.pc+=2;
                break;
            case 0x000a:
                //A key press is awaited, and then stored in VX (blocking operation, all instruction halted until next key event).
                //todo
                clearKeys(); //clear keys to get new input
                updateKeyReg();
                for (size_t i = 0; i < 16; i++)
                {
                    if (ch8.key[i]==true)
                    {
                        while (!checkKeyUp(ch8.key[i])) //while key isn't release yet wait
                        {
                            
                        }
                        ch8.reg[x] = ch8.key[i];
                        ch8.pc+=2;
                        break;
                    }           
                }  
                break;
            case 0x0005:
                switch (nn)
                {
                case 0x0015:
                    //Sets the delay timer to VX.
                    ch8.delayTimer=ch8.reg[x];
                    ch8.pc+=2;
                    break;
                case 0x0055:
                    //Stores from V0 to VX (including VX) in memory, starting at address I. The offset from I is increased by 1 for each value written, but I itself is left unmodified
                    for (size_t i = 0; i <= x; i++)
                    {
                        ch8.memory[ch8.indexReg+i] = ch8.reg[i];
                    }
                    ch8.indexReg++;
                    ch8.pc+=2;
                    break;
                case 0x0065:
                    //Fills from V0 to VX (including VX) with values from memory, starting at address I. The offset from I is increased by 1 for each value read, but I itself is left unmodified.
                    for (size_t i = 0; i <= x; i++)
                    {
                        ch8.reg[i] = ch8.memory[ch8.indexReg+i];
                    }
                    ch8.indexReg++;
                    ch8.pc+=2;
                    break;
                
                default: unknownOpcode();
                    break;
                }
                break;
            case 0x0008:
                //Sets the sound timer to VX.
                ch8.soundTimer=ch8.reg[x];
                ch8.pc+=2;
                break;
            case 0x000e:
                //Adds VX to I. VF is not affected.
                ch8.indexReg+=ch8.reg[x];
                ch8.pc+=2;
                break;
            case 0x0009:
                //Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented by a 4x5 font.
                ch8.indexReg = ch8.reg[x]*5;
                ch8.pc+=2;
                break;
            case 0x0003:
                //Stores the binary-coded decimal representation of VX, with the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
                ch8.memory[ch8.indexReg] = (ch8.reg[x]%1000) / 100;
                ch8.memory[ch8.indexReg+1] = (ch8.reg[x]%100) / 10;
                ch8.memory[ch8.indexReg+2] = (ch8.reg[x]%10);
                ch8.pc+=2;
                break;

            default: unknownOpcode();
            }
            break;
        default: unknownOpcode();
    }
}

void updateScreen(){
    BeginDrawing();
        ClearBackground(BLACK); 
        for (int x=0; x<SWIDTH; x++) { //loops through graphics array which stores the screen
            for (int y=0; y<SHEIGHT; y++) {
                BeginDrawing();
                if (ch8.graphics[x][y]) //if pixel
                {
                    DrawRectangle(x*SCALE, y*SCALE, SCALE, SCALE, RAYWHITE);  
                }
            }
        }
    EndDrawing();
}

void updateTimers(){
    if (ch8.delayTimer>0){
        ch8.delayTimer--;
    }
     if (ch8.soundTimer>0){
        playSound();
        ch8.soundTimer--;
    }
}

void playSound(){
    //to do
}

bool checkKeyDown(int key){
    PollInputEvents();
    switch (key)
    {
        case 0x1: 
            if(IsKeyDown(KEY_ONE)){
                return true;
            }
            return false;
        case 0x2: 
            if(IsKeyDown(KEY_TWO)){
                return true;
            }
            return false;
        case 0x3: 
            if(IsKeyDown(KEY_THREE)){
                return true;
            }
            return false;
        case 0xc: 
            if(IsKeyDown(KEY_FOUR)){
                return true;
            }
            return false;
        case 0x4: 
            if(IsKeyDown(KEY_Q)){
                return true;
            }
            return false;
        case 0x5: 
            if(IsKeyDown(KEY_W)){
                return true;
            }
            return false;
        case 0x6: 
            if(IsKeyDown(KEY_E)){
                return true;
            }
            return false;
        case 0xd: 
            if(IsKeyDown(KEY_R)){
                return true;
            }
            return false;
        case 0x7: 
            if(IsKeyDown(KEY_A)){
                return true;
            }
            return false;
        case 0x8: 
            if(IsKeyDown(KEY_S)){
                return true;
            }
            return false;
        case 0x9: 
            if(IsKeyDown(KEY_D)){
                return true;
            }
            return false;
        case 0xe: 
            if(IsKeyDown(KEY_F)){
                return true;
            }
            return false;
        case 0xa: 
            if(IsKeyDown(KEY_Z)){
                return true;
            }
            return false;
        case 0x0: 
            if(IsKeyDown(KEY_X)){
                return true;
            }
            return false;
        case 0xb: 
            if(IsKeyDown(KEY_C)){
                return true;
            }
            return false;
        case 0xf: 
            if(IsKeyDown(KEY_V)){
                return true;
            }
            return false;
        default:  return false;
    }
}

bool checkKeyUp(int key){
    PollInputEvents();
    switch (key)
    {
        case 0x1: 
            if(IsKeyUp(KEY_ONE)){
                return true;
            }
            return false;
        case 0x2: 
            if(IsKeyUp(KEY_TWO)){
                return true;
            }
            return false;
        case 0x3: 
            if(IsKeyUp(KEY_THREE)){
                return true;
            }
            return false;
        case 0xc: 
            if(IsKeyUp(KEY_FOUR)){
                return true;
            }
            return false;
        case 0x4: 
            if(IsKeyUp(KEY_Q)){
                return true;
            }
            return false;
        case 0x5: 
            if(IsKeyUp(KEY_W)){
                return true;
            }
            return false;
        case 0x6: 
            if(IsKeyUp(KEY_E)){
                return true;
            }
            return false;
        case 0xd: 
            if(IsKeyUp(KEY_R)){
                return true;
            }
            return false;
        case 0x7: 
            if(IsKeyUp(KEY_A)){
                return true;
            }
            return false;
        case 0x8: 
            if(IsKeyUp(KEY_S)){
                return true;
            }
            return false;
        case 0x9: 
            if(IsKeyUp(KEY_D)){
                return true;
            }
            return false;
        case 0xe: 
            if(IsKeyUp(KEY_F)){
                return true;
            }
            return false;
        case 0xa: 
            if(IsKeyUp(KEY_Z)){
                return true;
            }
            return false;
        case 0x0: 
            if(IsKeyUp(KEY_X)){
                return true;
            }
            return false;
        case 0xb: 
            if(IsKeyUp(KEY_C)){
                return true;
            }
            return false;
        case 0xf: 
            if(IsKeyUp(KEY_V)){
                return true;
            }
            return false;
        default:  return false;
    }
}

int getK(int key){
    switch (key)
    {
        case 49: 
            return 0x1;
        case 50: 
            return 0x2;
        case 51: 
            return 0x3;
        case 52: 
            return 0xc;
        case 81: 
            return 0x4;
        case 87: 
            return 0x5;
        case 69: 
           return 0x6;
        case 82: 
            return 0xd;
        case 65: 
            return 0x7;
        case 83: 
            return 0x8;
        case 68: 
            return 0x9;
        case 70: 
            return 0xe;
        case 90: 
            return 0xa;
        case 88: 
            return 0x0;
        case 67: 
            return 0xb;
        case 86: 
            return 0xf;
        default:  return -1;
    }
}

void keyMap(int key){
    switch (key)
    {
        case 49: 
            ch8.key[0x1]=true;
            break;
        case 50: 
            ch8.key[0x2]=true;
            break;
        case 51: 
            ch8.key[0x3]=true;
            break;
        case 52: 
            ch8.key[0xc]=true;
            break;
        case 81: 
            ch8.key[0x4]=true;
            break;
        case 87: 
            ch8.key[0x5]=true;
            break;
        case 69: 
            ch8.key[0x6]=true;
            break;
        case 82: 
            ch8.key[0xd]=true;
            break;
        case 65: 
            ch8.key[0x7]=true;
            break;
        case 83: 
            ch8.key[0x8]=true;
            break;
        case 68: 
            ch8.key[0x9]=true;
            break;
        case 70: 
            ch8.key[0xe]=true;
            break;
        case 90: 
            ch8.key[0xa]=true;
            break;
        case 88: 
            ch8.key[0x0]=true;
            break;
        case 67: 
            ch8.key[0xb]=true;
            break;
        case 86: 
            ch8.key[0xf]=true;
            break;

        default:  return;
    }
}

void clearKeys(){
    for (size_t i = 0; i < 16; i++)
    {
        ch8.key[i] = false;
    }
}

void updateKeyReg(){
    //PollInputEvents();
    //keyMap(GetKeyPressed());
    
    for (size_t i = 0; i < 16; i++)
    {
        if (checkKeyDown(i))
        {
            ch8.key[i] = true;
        }
        else{
            ch8.key[i] = false;
        }        
    }   
}

int calculateRemainingTime(struct timeval start_time, struct timeval end_time, int interval) {
    long execution_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
                          (end_time.tv_usec - start_time.tv_usec);

    int remaining_time = interval - execution_time;

    return remaining_time > 0 ? remaining_time : 0;
}

int main(int argc, char const *argv[])
{
    initialize("./roms/breakout.ch8");
    InitWindow(SWIDTH*SCALE, SHEIGHT*SCALE, "First Emulator!");
    SetTargetFPS(60);
    //struct timeval startT, endT;
    int interval = 1000000 / 60; //16.66 ms
    BeginDrawing();
            ClearBackground(BLACK);
    EndDrawing();
      while (!WindowShouldClose()) {
        updateKeyReg();
        //gettimeofday(&startT, NULL);
        for (int i = 0; i < CPURATE; i++) //instructions per frame
        {
            cycle(i);
        }

        //gettimeofday(&endT, NULL);
        //int delayTime = calculateRemainingTime(startT, endT, interval); //calculate delay so programs run at about 60hz
        //usleep(interval > delayTime ? interval-delayTime : 0); //delay to get aprox 60fps

        if (ch8.draw) //todo: only draw on screen if pixel changes not everytime draw is called.
        {
            updateScreen();
            ch8.draw = false;
        }

        updateTimers();
        
    }
    
    CloseWindow();
    return 0;
}



