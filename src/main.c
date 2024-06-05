#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <string.h>

#include <SDL2/SDL.h>

#include "chip8.h"

//TODO 
/*
    Be able to open file exploere with button press
    be able to retreave selectide file
*/


#define SGN(_x) ((_x) < 0 ? -1 : ((_x) > 0 ? 1 : 0))


typedef uint64_t u64;
typedef uint32_t u32;
typedef int32_t  i32;
typedef uint8_t  u8;

static int window_width = 0;
static int window_height = 0;
static int byteperpixel = 4;

static struct
{
    SDL_Window      *window;
    SDL_Renderer    *renderer;
    SDL_Texture     *texture;
    u32             *pixels;
}eg_state;

static void CreateTexture(SDL_Renderer *renderer, int width, int height)
{
    if(eg_state.pixels) free(eg_state.pixels);
    if(eg_state.texture) SDL_DestroyTexture(eg_state.texture);

    eg_state.texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
    window_height = height;
    window_width = width;
    eg_state.pixels = malloc(width * height * byteperpixel);
}


static void RenderUpdate(SDL_Window *window, SDL_Renderer *renderer)
{
    SDL_UpdateTexture(eg_state.texture, 0, eg_state.pixels, window_width * byteperpixel);
    SDL_RenderCopy(renderer, eg_state.texture, 0, 0);
    SDL_RenderPresent(renderer);
}

static void WindowEventHandler(SDL_Event *e)
{
    switch(e->type)
    {
        case SDL_WINDOWEVENT:
        {
            switch(e->window.event)

            {
                case SDL_WINDOWEVENT_EXPOSED:
                {
                    SDL_Window *window = SDL_GetWindowFromID(e->window.windowID);
                    SDL_Renderer *renderer = SDL_GetRenderer(window);
                    RenderUpdate(window, renderer);
                }
            }
        }
    }
}

static void InitSDL(void)
{
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {printf("failed to initlize SDL :["); exit(1); }

    eg_state.window = SDL_CreateWindow("Think of cool name later", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 512, 256, 0);

    if(eg_state.window)
    {
        eg_state.renderer = SDL_CreateRenderer(eg_state.window, -1, 0);

        if(eg_state.renderer)
        {
            CreateTexture(eg_state.renderer, 512, 256);
        }
    }
}

void DrawPixel(int x, int y, u32 colour)
{
    if(x >= 0 && x < window_width && y >= 0 && y < window_height){eg_state.pixels[y * window_width + x] = colour; }
}

void DrawLine(int x0, int y0, int x1, int y1, u32 colour)
{
    int dx, dy;
    int sx, sy;
    int accum;

    dx = x1 - x0;
    dy = y1 - y0;

    sx = SGN(dx);
    sy = SGN(dy);

    dx = abs(dx);
    dy = abs(dy);

    x1 += sx;
    y1 += sy;

    if(dx > dy)
    {
        accum = dx >> 1;
        do
        {
            DrawPixel(x0, y0, colour);
            accum -= dy;

            if(accum < 0)
            {
                accum += dx;
                y0 += sy;
            }
            x0 += sx;
        }while(x0 != x1);
    }else
    {
        accum = dy >> 1;

        do
        {
            DrawPixel(x0, y0, colour);
            accum -= dx;
            if(accum < 0)
            {
                accum += dy;
                x0 += sx;
            }
            y0 += sy;
        }while(y0 != y1);
    }
}

void DrawFilledRect(int posx, int posy, int width, int height, u32 colour)
{  
    int xmin = posx, ymin = posy;
    int xmax = posx + width, ymax = posy + height;

    for(int hy=ymin;hy<=ymax;hy++)
    {
        for(int hx=xmin;hx<=xmax;hx++)
        {
            if(hx >= xmin && hx <= xmax && hy >= ymin && hy <= ymax)
            {
                DrawPixel(hx,hy,colour);
            }
        }
    }
}


int main(int argc, char **argv)
{
    InitSDL();

    chip_state c8_state = ChipInit();
    LoadChipRom(argv[1], &c8_state);


    while(1)
    {
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if(e.type == SDL_QUIT) exit(0);
            WindowEventHandler(&e);


        }

        SDL_RenderClear(eg_state.renderer);

        memset(eg_state.pixels, 0, window_width * window_height * byteperpixel);



        u64 start_frame_timer = SDL_GetPerformanceCounter();

        for(u32 i = 0; i < 500 /60; i++)
        {
            ChipExecute(&c8_state);

            for (int i = 0; i < 64 * 32; i++)
            {
                int x = i % 64;
                int y = i / 64;

                if(c8_state.display[i])
                {
                    DrawFilledRect(x * 8, y * 8, 8, 8, 0x008000);
                }
            }


        
        }
    
        u64 end_frame_time = SDL_GetPerformanceCounter();

        const double time_elapsed = (double)((end_frame_time - start_frame_timer) / 1000) / SDL_GetPerformanceFrequency();

        SDL_Delay(16.67f > time_elapsed ? 16.67f - time_elapsed : 0);



        RenderUpdate(eg_state.window, eg_state.renderer);
    }

    SDL_Quit();

    return 0;
}