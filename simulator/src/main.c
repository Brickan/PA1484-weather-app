#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#include <unistd.h>
#include <pthread.h>

#include "lvgl/lvgl.h"
#include "lvgl/examples/lv_examples.h"
#include "lvgl/demos/lv_demos.h"
#include <SDL.h>

#include "hal/hal.h"
#include "hardware_mock.h"

static void hal_init(void);

// Arduino sketch functions (provided by project_wrapper.cpp)
extern void setup(void);
extern void loop(void);

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    lv_init();
    hal_init();

    initHardware();
    initWiFi();

    printf("====================================\n");
    printf("  T4-S3 Weather\n");
    printf("  Display: %dx%d\n", TFT_WIDTH, TFT_HEIGHT);
    printf("====================================\n\n");

    setup();

    while(1) {
        lv_timer_handler();
        loop();
        // Don't add extra delay - let the Arduino loop control timing
        // The .ino file already has delay(10) at the end of loop()
    }

    return 0;
}

static void hal_init(void)
{
    #include "lvgl/src/drivers/sdl/lv_sdl_window.h"
    #include "lvgl/src/drivers/sdl/lv_sdl_mouse.h"

    lv_display_t * disp = lv_sdl_window_create(TFT_WIDTH, TFT_HEIGHT);
    lv_indev_t * mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
    lv_display_set_default(disp);

    lv_tick_set_cb(SDL_GetTicks);
}
