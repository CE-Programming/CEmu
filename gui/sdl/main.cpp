#include <SDL2/SDL.h>
#include "../../core/cemu.h"

int main(int argc, char **argv) {
    SDL_Window *window;
    SDL_Surface *surface;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
        return 1;
    }
    window = SDL_CreateWindow("CEmu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LCD_WIDTH, LCD_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (window == NULL) {
        fprintf(stderr, "error: could not create window: %s\n", SDL_GetError());
        return 1;
    }

    surface = SDL_GetWindowSurface(window);
    SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0, 255, 0));
    SDL_UpdateWindowSurface(window);
    SDL_Delay(1000);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
