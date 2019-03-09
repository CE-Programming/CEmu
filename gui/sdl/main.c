#include "../../core/cemu.h"
#include "keymap.h"

#include <SDL2/SDL.h>
#include <getopt.h>
#include <stdio.h>

typedef struct {
    SDL_Window *window;
    SDL_Surface *surface;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
} sdl_t;

typedef struct {
    char *rom;
    char *image;
    int spi;
    int speed;
    int fullscreen;
    sdl_t sdl;
} cemu_sdl_t;

static const cemu_sdl_key_t *keymap = cemu_keymap;

void gui_console_clear() {}
void gui_console_printf(const char *format, ...) { (void)format; }
void gui_console_err_printf(const char *format, ...) { (void)format; }

void sdl_update_lcd(void *data) {
    cemu_sdl_t *cemu = (cemu_sdl_t*)data;
    sdl_t *sdl = &cemu->sdl;
    void *pixels;
    int pitch;

    if (!SDL_LockTexture(sdl->texture, NULL, &pixels, &pitch)) {
        emu_lcd_drawframe(pixels);
        SDL_UnlockTexture(sdl->texture);
    }
}

void sdl_event_loop(cemu_sdl_t *cemu) {
    SDL_Event event;
    bool done = false;
    uint32_t last_ticks, speed_ticks;
    unsigned speed_count = 0;
    float speed = 0.0f;
    uint32_t pixfmt = SDL_PIXELFORMAT_RGBA32;
    sdl_t *sdl = &cemu->sdl;
    char buf[20];
    unsigned max_frame_skip = 5;
    int fmt = cemu->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        fprintf(stderr, "could not initialize sdl2: %s\n", SDL_GetError());
        return;
    }
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
    sdl->window = SDL_CreateWindow("CEmu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, LCD_WIDTH, LCD_HEIGHT, fmt | SDL_WINDOW_SHOWN);
    if (sdl->window == NULL) {
        fprintf(stderr, "could not create window: %s\n", SDL_GetError());
        return;
    }
    sdl->renderer = SDL_CreateRenderer(sdl->window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
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
    emu_set_lcd_callback(sdl_update_lcd, cemu);
    emu_set_lcd_spi(cemu->spi);

    last_ticks = SDL_GetTicks();
    speed_ticks = last_ticks + 1000;
    while (done == false) {
        SDL_DisplayMode mode;
        uint32_t max_ticks, ticks, expected_ticks, actual_ticks;
        int status;

        SDL_GetWindowDisplayMode(sdl->window, &mode);
        max_ticks = 1000 * max_frame_skip / mode.refresh_rate;

        ticks = SDL_GetTicks();
        expected_ticks = ticks - last_ticks;
        last_ticks = ticks;
        actual_ticks = expected_ticks < max_ticks ? expected_ticks : max_ticks;
        speed += expected_ticks ? 100.0f * actual_ticks / expected_ticks : 100.0f;
        speed_count++;
        emu_run(actual_ticks);

        if (control.ports[5] & 1 << 4) {
            uint8_t brightness = backlight.factor < 1 ? backlight.factor * 255 : 255;
            SDL_SetTextureColorMod(sdl->texture, brightness, brightness, brightness);
            SDL_RenderCopy(sdl->renderer, sdl->texture, NULL, NULL);
        } else {
            SDL_SetRenderDrawColor(sdl->renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(sdl->renderer);
        }
        SDL_RenderPresent(sdl->renderer);

        if (SDL_TICKS_PASSED(ticks, speed_ticks)) {
            snprintf(buf, sizeof buf - 1, "CEmu | %.1f%%", speed / speed_count);
            SDL_SetWindowTitle(sdl->window, buf);
            speed_ticks += 1000;
            speed_count = 0;
            speed = 0.0f;
        }

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    for (const cemu_sdl_key_t *key = keymap; key && (key->row | key->col) >= 0; key++) {
                        if (keysyms_match(&key->keysym, &event.key.keysym, event.type == SDL_KEYUP)) {
                            emu_keypad_event(key->row, key->col, event.type == SDL_KEYDOWN);
                        }
                    }
                    break;

                case SDL_DROPFILE:
                    status = emu_send_variable(event.drop.file, LINK_FILE);
                    SDL_ShowSimpleMessageBox(
                        SDL_MESSAGEBOX_INFORMATION,
                        status == LINK_GOOD ? "Transfer Success" : "Transfer Failure",
                        event.drop.file,
                        sdl->window
                    );
                    SDL_free(event.drop.file);
                    break;

                case SDL_QUIT:
                    done = true;
                    break;

                default:
                    break;
            }
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
    cemu.spi = 0;

    for (;;) {
        int c;
        int option_index = 0;

        static const struct option long_options[] = {
            {"fullscreen", no_argument,       0,  'f' },
            {"rom",        required_argument, 0,  'r' },
            {"image",      required_argument, 0,  'i' },
            {"spi",        no_argument,       0,  's' },
            {"keymap",     required_argument, 0,  'k' },
            {"yum",        no_argument,       0,  'y' },
        };

        c = getopt_long(argc, argv, "fr:i:", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
            case 'r':
                fprintf(stdout, "rom: %s\n", optarg);
                cemu.rom = optarg;
                break;

            case 's':
                fprintf(stdout, "spi: yes\n");
                cemu.spi = 1;
                break;

            case 'i':
                fprintf(stdout, "image: %s\n", optarg);
                cemu.image = optarg;
                break;

            case 'f':
                fprintf(stdout, "fullscreen: yes\n");
                cemu.fullscreen = 1;
                break;

            case 'k':
                if (!strcmp(optarg, "cemu")) {
                    keymap = cemu_keymap;
                }
                if (!strcmp(optarg, "smartpad")) {
                    keymap = smartpad_keymap;
                }
                break;

            default:
                break;
        }
    }

    sdl_event_loop(&cemu);

    return 0;
}
