#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL2/SDL.h>

#include "chip8.h"


unsigned char font[80] =
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

static const SDL_Scancode keymap[16] =
{
    SDL_SCANCODE_0,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_4,
    SDL_SCANCODE_5,
    SDL_SCANCODE_6,
    SDL_SCANCODE_7,
    SDL_SCANCODE_8,
    SDL_SCANCODE_9,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_F,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_X,
    SDL_SCANCODE_C,
};

chip_state ChipInit(void)
{
    chip_state state = {0};
    state.pc = 0x200;
    state.pause = false;

    memcpy(state.memory, font, sizeof(font));

    return state;
}

void LoadChipRom(char *file, chip_state *state)
{
    printf("loading: %s\n", file);

    FILE *f = fopen(file, "rb");
    if(f == NULL){printf("Failed to open %s\n", file); exit(1); }

    struct stat st;
    stat(file, &st);
    size_t fsize = st.st_size;

    size_t bytes_read = fread(state->memory + 0x200, 1, sizeof(state->memory) - 0x200, f);

    fclose(f);

    if(bytes_read != fsize)
    {
        printf("failed to load file into memory \n");
        exit(1);
    }else{printf("file loaded into memory \n");}

    if(bytes_read > sizeof(state->memory))
    {
        printf("file too large daddy :3 ...");
        exit(1);
    }
}

static void UpdateTimers(chip_state *state)
{
    if(state->delay > 0) state->delay -= 1;
    if(state->sound_delay > 0) state->sound_delay -= 1;
}

static int PlotPixels(int x, int y, chip_state *state)
{
    int loc_pixel = x + (y * 64);
    state->display[loc_pixel] ^= 1;

    return !state->display[loc_pixel];
}


void Chip8DrawDisplay(chip_state *state, int *posx, int *posy)
{
    for (int i = 0; i < 64 * 32; i++)
    {
        int x = i % 64;
        int y = i / 64;

        if(state->display[i])
        {
            *posx = x;
            *posy = y;
        }
    }
}


void ChipExecute(chip_state *state)
{
    /*  Getting opcodes

        All instructions are 16 bits (2 byte) but our memory is 8 bits (1 byte) so we have to combine 2 pieces of memory to get the full opcode

        pc & pc + 1 are grabbing both halves of the opcode

        to get the full opdode we need to shit the first piece of memory 8 bits to the left (<< 8) to make it 2 bytes long, and then merge (|) it with the second piece

        eg. in action
        each is 1 byte (8 bits) in size
        pc = 0x10
        pc + 1 = 0xF0

        shifting pc by 8 bits to the left makes it 2 bytes (16 bits) and it becomes (pc = 0x1000)
        we can then use bitwise OR (|) (just call it a merge tbh) to combine the second part of pc like so (pc | pc + 1) which then becomes this  (0x10F0)

        this is how we retrive opcodes :)
    */

    unsigned short op = state->memory[state->pc] << 8 | state->memory[state->pc + 1];

    /*  Execute instructions

        all instruction are 2 bytes long (16 bits) so after every execution of an instruction we most increase pc by 2 so the cpu knows were the next instrction is 


        instruction variables

        nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
        n or nibble - A 4-bit value, the lowest 4 bits of the instruction
        x - A 4-bit value, the lower 4 bits of the high byte of the instruction
        y - A 4-bit value, the upper 4 bits of the low byte of the instruction
        kk or byte - An 8-bit value, the lowest 8 bits of the instruction


        x variable is located in the lower 4 bits of the high byte.
        y is located in the upper 4 bits of the low byte

        eg.
        if given 0x5460

        the high byte would be 0x54 and the low byte would be 0x60
        the 4 bits (nibble) of the high byte would be 0x4, and the upper 4 bits of the low byte would be 0x6

        which leaves us with x = 0x4 adn y = 0x6
    */

    
   
    int x = (op & 0x0F00) >> 8;  //shift to the right by 8 bits to remove everything after the second nibble
    int y = (op & 0x00F0) >> 4; //shift to the right by 4 to remove everything after the third nibble

    switch(op & 0xF000)
    {
    
        case 0x0000:
        {
            switch(op & 0x0FFF)
            {
                case 0x00E0: {printf("[OK] 0x%X 00EE\n", op); memset(state->display, 0, sizeof(state->display)); state->pc += 2; } break;
                case 0x00EE: {printf("[OK] 0x%X 00EE\n", op); state->sp -= 2; state->pc = state->stack[state->sp]; state->pc += 2; } break;   
            }
        }break;

        case 0x1000:
        {
            printf("[OK] 0x%X 1NNN\n", op);
            state->pc = (op & 0x0FFF);
        }break;

        case 0x2000:
        {
            printf("[OK] 0x%X 2NNN\n", op);
            state->stack[state->sp] = state->pc;
            state->sp += 2;
            state->pc = op & 0x0FFF;
        }break;

        case 0x3000:
        {
            printf("[OK] 0x%X 3XNN\n", op);
            if(state->V[x] == (op & 0x00FF)){state->pc += 2;}
            state->pc += 2;
        }break;

        case 0x4000:
        {
            printf("[OK] 0x%X 4XNN\n", op);
            if(state->V[x] != (op & 0x00FF)){state->pc += 2;}
            state->pc += 2;
        }break;
        
        case 0x5000:
        {
            printf("[OK] 0x%X 5XNN\n", op);
            if(state->V[x] == state->V[y]){state->pc += 2;}
            state->pc += 2;
        }break;

        case 0x6000:
        {
            printf("[OK] 0x%X 6XNN \n", op);
            state->V[x] = (op & 0x00FF);
            state->pc += 2;
        }break;

        case 0x7000:
        {
            printf("[OK] 0x%X 7XNN\n", op);
            state->V[x] += (op & 0x00FF);
            state->pc += 2;
        }break;

        case 0x8000:
        {
            switch(op & 0x000F)
            {
                case 0x0:
                {
                    printf("[OK] 0x%X 8XY0\n", op);
                    state->V[x] = state->V[y];
                    state->V[0xF] = 0;
                    state->pc += 2;
                }break;

                case 0x1:
                {
                    printf("[OK] 0x%X 8XY1\n", op);
                    state->V[x] = state->V[x] | state->V[y];
                    state->V[0xF] = 0;
                    state->pc += 2;
                }break;

                case 0x2:
                {
                    printf("[OK] 0x%X 8XY2\n", op);
                    state->V[x] = (state->V[x] & state->V[y]);
                    state->V[0xF] = 0;
                    state->pc += 2;
                }break;

                case 0x3:
                {
                    printf("[OK] 0x%X 8XY3\n", op);
                    state->V[x] =  state->V[x] ^ state->V[y];
                    state->V[0xF] = 0;
                    state->pc += 2;
                }break;

                case 0x4:
                {
                    printf("[OK] 0x%X 8XY4\n", op);
                    unsigned short sum = (state->V[x] + state->V[y]);
                    int tmp_0xf = (sum > 0xFF);

                    state->V[x] = sum;
                    state->V[0xF] = tmp_0xf;
                    state->pc += 2;
                }break;

                case 0x5:
                {
                    printf("[OK] 0x%X 8XY5\n", op);
                    int tmp_0xf = (state->V[x] < state->V[y]);

                    state->V[x] -= state->V[y];
                    state->V[0xF] = !tmp_0xf;
                    state->pc += 2;
                }break;

                case 0x6:
                {
                    printf("[OK] 0x%X 8XY6\n", op);
                    int tmp_0xf = state->V[x] & 0x1;
                    state->V[x] = state->V[x] >> 1;
                    state->V[0xF] = tmp_0xf;

                    state->pc += 2;
                }break;

                case 0x7:
                {
                    printf("[OK] 0x%X 8XY7\n", op);
                    int tmp_0xf = (state->V[y] < state->V[x]);
                    state->V[x] = state->V[y] - state->V[x];
                    state->V[0xF] = !tmp_0xf;
                    state->pc += 2;
                }break;

                case 0xE:
                {
                    printf("[OK] 0x%X 8XYE\n", op);

                    int tmp_0xf = (state->V[x] >> 7);

                    state->V[x] <<= 1;
                    state->V[0xF] = tmp_0xf;
                    state->pc += 2;
                }break;
            }
        }break;

        case 0x9000:
        {
            printf("[OK] 0x%X 9xy0 \n", op);
            if(state->V[x] != state->V[y]){state->pc += 2;}
            state->pc += 2;
        }break;

        case 0xA000:
        {
            printf("[OK] 0x%X Annn  \n", op);
            state->I = (op & 0x0FFF);
            state->pc += 2;
        }break;

        case 0xB000:
        {
            printf("[OK] 0x%X Bnnn  \n", op);
            state->pc = (op & 0x0FFF) + state->V[0];
            state->pc += 2;
        }break;

        case 0xC000:
        {
            printf("[OK] 0x%X Cxkk\n", op);
            unsigned char n_rand = rand() % 256;
            state->V[x] = n_rand & (op & 0x00FF);
            state->pc += 2;
        }break;

        case 0xD000:
        {
            printf("[OK] 0x%X DXYN\n", op);
            int width = 8;
            int height = (op & 0x000F);
            state->V[0xF] = 0;

            for(int row = 0; row < height; row++)
            {
                if((row + state->V[y]) >= 32) break;
                unsigned char sprite = state->memory[state->I + row];

                for(int col = 0; col < width; col++)
                {
                    if((col + state->V[x]) >= 64) break;

                    if((sprite & 0x80) > 0)
                    {
                        if(PlotPixels(state->V[x]+col, state->V[y]+row, state))
                        {
                            state->V[0xF] = 1;
                        }
                    }
                    sprite <<= 1;
                }
            }
            state->pc += 2;
        }break;


        case 0xE000:
        {
            switch(op & 0x00FF)
            {
                case 0x9E:
                {
                    printf("[OK] 0x%X Ex9E\n", op);
                    if(SDL_GetKeyboardState(NULL)[keymap[state->V[x]]]) state->pc += 2;
                    state->pc += 2;
                }break;

                case 0xA1:
                {
                    printf("[OK] 0x%X SKNP Vx\n", op);
                    if(!SDL_GetKeyboardState(NULL)[keymap[state->V[x]]]) state->pc += 2;
                    state->pc += 2;
                }break;
            }
        }break;


        case 0xF000:
        {
            switch(op & 0x00FF)
            {
                case 0x0007:
                {
                    printf("[OK] 0x%X FX07\n", op);
                    state->V[x] = state->delay;
                    state->pc += 2;
                }break;

                case 0x000A:
                {
                    printf("[OK] 0x%X FX0A\n", op);
                    state->pc -= 2;

                    for(int i =0; i < 16; i++)
                    {
                        if(SDL_GetKeyboardState(NULL)[keymap[i]])
                        {
                            state->V[x] = i;
                            state->pc += 2;
                            break;
                        }
                    }

                    state->pc += 2;
                }break;

                case 0x0015:
                {
                    printf("[OK] 0x%X FX15\n", op);
                    state->delay =  state->V[x];
                    state->pc += 2;
                }break;

                case 0x0018:
                {
                    printf("[OK] 0x%X FX18\n", op);
                    state->sound_delay = state->V[x];
                    state->pc += 2;
                }break;

                case 0x001E:
                {
                    printf("[OK] 0x%X FX1E\n", op);
                    state->I = state->I + state->V[x];
                    state->pc += 2;
                }break;

                case 0x0029:
                {
                   printf("[OK] 0x%X FX1E\n", op);
                   state->I = state->V[x] * 5;
                   state->pc += 2; 
                }break;


                case 0x0033:
                {
                    printf("[OK] 0x%X Fx33 \n", op);
                    int val = state->V[x];
                    int ones = val % 10;
                    val /= 10;
                    int tens = val % 10;
                    val /= 10;
                    int hundred = val % 10;
                    state->memory[state->I] = hundred;
                    state->memory[state->I + 1] = tens;
                    state->memory[state->I + 2] = ones;

                    state->pc += 2;
                }break;

                case 0x0055:
                {
                    for(int i = 0; i <=  x; i++)
                    {
                        state->memory[state->I + i] = state->V[i];
                    }
                    state->pc += 2;
                }break;

                case 0x0065:
                {
                    for(int i = 0; i <= x; i++)
                    {
                        state->V[i] = state->memory[state->I + i];
                    }
                    state->pc += 2;
                }break;
            }
        }break;
    }

    if(!state->pause)
    {
        UpdateTimers(state);
    }
}