#include "../../core/cemu.h"

#include <SDL2/SDL.h>
#include <getopt.h>

typedef struct {
    SDL_Window *window;
    SDL_Surface *surface;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} sdl_t;

typedef struct {
    char *rom;
    char *image;
    int speed;
    int fullscreen;
    uint8_t lcd[LCD_WIDTH * LCD_HEIGHT * 4];
    sdl_t sdl;
} cemu_sdl_t;

void gui_console_clear() {}
void gui_console_printf(const char *format, ...) { (void)format; }
void gui_console_err_printf(const char *format, ...) { (void)format; }

void sdl_update_lcd(void *data) {
    cemu_sdl_t *cemu = (cemu_sdl_t*)data;
    sdl_t *sdl = &cemu->sdl;
    void *pixels;
    int pitch;

    if (!SDL_LockTexture(sdl->texture, NULL, &pixels, &pitch)) {
        lcd_drawframe(pixels, lcd.control & 1 << 11 ? lcd.data : NULL, lcd.data_end, lcd.control, LCD_SIZE);
        SDL_UnlockTexture(sdl->texture);
    }

    /* SDL_RenderClear(sdl->renderer); */
    SDL_RenderCopy(sdl->renderer, sdl->texture, NULL, NULL);
    SDL_RenderPresent(sdl->renderer);
}

void sdl_event_loop(cemu_sdl_t *cemu) {
    SDL_Event event;
    bool done = false;
    uint64_t last = 0;
    uint32_t pixfmt = SDL_PIXELFORMAT_BGRA32;
    sdl_t *sdl = &cemu->sdl;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
        return;
    }
    sdl->window = SDL_CreateWindow("CEmu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LCD_WIDTH, LCD_HEIGHT, /*SDL_WINDOW_FULLSCREEN_DESKTOP | */SDL_WINDOW_SHOWN);
    if (sdl->window == NULL) {
        fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return;
    }
    sdl->renderer = SDL_CreateRenderer(sdl->window, -1, 0);
    if (sdl->renderer == NULL) {
        fprintf(stderr, "could not create renderer: %s\n", SDL_GetError());
        return;
    }
    sdl->texture = SDL_CreateTexture(sdl->renderer, pixfmt, SDL_TEXTUREACCESS_STREAMING, LCD_WIDTH, LCD_HEIGHT);
    if (sdl->texture == NULL) {
        fprintf(stderr, "could not create texture: %s\n", SDL_GetError());
        return;
    }

    if (EMU_STATE_VALID != emu_load(EMU_DATA_ROM, cemu->rom)) {
        fprintf(stderr, "could not start cemu.\n");
        return;
    }

    emu_set_run_rate(1000);
    lcd_set_gui_event(sdl_update_lcd, cemu);

    while (done == false) {

        emu_run(SDL_GetTicks() - last);
        last = SDL_GetTicks();

        SDL_PollEvent(&event);

        switch (event.type) {
            case SDL_KEYDOWN:
                break;

            case SDL_MOUSEBUTTONDOWN:
                break;

            case SDL_QUIT:
                done = true;
                break;

            default:
                break;
        }
    }

    emu_save(EMU_DATA_IMAGE, cemu->image);

    SDL_DestroyWindow(sdl->window);
    SDL_Quit();
}

int main(int argc, char **argv) {
    static cemu_sdl_t cemu;

    cemu.speed = 100;
    cemu.fullscreen = 0;
    cemu.image = NULL;
    cemu.rom = NULL;

    for (;;) {
        int tmp;
        int c;
        int option_index = 0;

        static const struct option long_options[] = {
            {"rom",        required_argument, 0,  'r' },
            {"image",      required_argument, 0,  'i' },
            {"speed",      required_argument, 0,  's' },
            {"fullscreen", required_argument, 0,  'f' },
        };

        c = getopt_long(argc, argv, "r:i:s:f:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'r':
                fprintf(stdout, "rom: %s\n", optarg);
                cemu.rom = optarg;
                break;

            case 'i':
                fprintf(stdout, "image: %s\n", optarg);
                cemu.image = optarg;
                break;

            case 's':
                tmp = strtol(optarg, NULL, 10);
                fprintf(stdout, "speed: %d\n", tmp);
                cemu.speed = tmp;
                break;

            case 'f':
                tmp = strtol(optarg, NULL, 10);
                fprintf(stdout, "fullscreen: %s\n", tmp ? "yes" : "no");
                cemu.fullscreen = tmp;
                break;

            case '?':
                break;
            default:
                break;
        }
    }

    sdl_event_loop(&cemu);

    return 0;
}
