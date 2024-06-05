#ifndef CHIP_8
#define CHIP_8

#include <stdbool.h>

typedef struct 
{
    unsigned char memory[4096];
    unsigned char display[64 * 32]; 
    unsigned char V[16];
    unsigned char keys[16];
    unsigned short stack[16];

    bool pause;

    unsigned char delay;
    unsigned char sound_delay;
    unsigned char sp;

    unsigned short I;
    unsigned short pc;
}chip_state;

typedef struct
{
    int x, y;
}draw_pos;

chip_state ChipInit(void);
void LoadChipRom(char *file, chip_state *state);

void ChipExecute(chip_state *state);

void Chip8DrawDisplay(chip_state *state, int *posx, int *posy);


#endif