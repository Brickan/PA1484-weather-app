// Weather Icon Animations
// Animated weather icons using LVGL shapes

#ifndef ICONS_H
#define ICONS_H

#include <lvgl.h>

// Text-based icon fallback
lv_obj_t *createTextIcon(lv_obj_t *parent, const char *iconText, uint32_t color, int size)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, iconText);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);
    lv_obj_center(label);
    return label;
}

// Animation callbacks
static void anim_set_transform_angle_cb(void *obj, int32_t value)
{
    lv_obj_set_style_transform_angle((lv_obj_t *)obj, value, 0);
}

static void anim_set_opa_cb(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, value, 0);
}

static void anim_set_x_cb(void *obj, int32_t value)
{
    lv_obj_set_x((lv_obj_t *)obj, value);
}

static void anim_set_y_cb(void *obj, int32_t value)
{
    lv_obj_set_y((lv_obj_t *)obj, value);
}

// ==========================================
// ANIMATED WEATHER ICON FUNCTIONS
// ==========================================

// Clear sky - sun with pulsing rays
void createClearSkyIcon(lv_obj_t *parent)
{
    lv_obj_t *sun = lv_obj_create(parent);
    lv_obj_set_size(sun, 40, 40);
    lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_border_width(sun, 0, 0);
    lv_obj_center(sun);

    for (int i = 0; i < 8; i++)
    {
        lv_obj_t *ray = lv_obj_create(parent);
        lv_obj_set_size(ray, 8, 8);
        lv_obj_set_style_radius(ray, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(ray, lv_color_hex(0xFFD700), 0);
        lv_obj_set_style_border_width(ray, 0, 0);

        float angle = (i * 45) * 3.14159 / 180;
        int x = 30 * cos(angle);
        int y = 30 * sin(angle);
        lv_obj_align(ray, LV_ALIGN_CENTER, x, y);

        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, ray);
        lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_30);
        lv_anim_set_time(&a, 1200 + (i * 150));
        lv_anim_set_playback_time(&a, 1200);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, anim_set_opa_cb);
        lv_anim_start(&a);
    }
}

// Partly cloudy - sun behind clouds
void createPartlyCloudyIcon(lv_obj_t *parent)
{
    lv_obj_t *sun = lv_obj_create(parent);
    lv_obj_set_size(sun, 25, 25);
    lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFA500), 0);
    lv_obj_set_style_border_width(sun, 0, 0);
    lv_obj_align(sun, LV_ALIGN_CENTER, -18, -12);
    lv_anim_t a_sun;
    lv_anim_init(&a_sun);
    lv_anim_set_var(&a_sun, sun);
    lv_anim_set_values(&a_sun, LV_OPA_80, LV_OPA_COVER); // Pulse between 80% and 100% opacity
    lv_anim_set_time(&a_sun, 2000);                      // 2 second fade
    lv_anim_set_playback_time(&a_sun, 2000);             // 2 second fade back
    lv_anim_set_repeat_count(&a_sun, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a_sun, anim_set_opa_cb);
    lv_anim_start(&a_sun);

    // Define cloud positions and sizes for a fluffy cloud effect
    int cloud_pos[][2] = {{-10, 5}, {5, 3}, {15, 7}, {20, 10}};

    int cloud_size[] = {28, 35, 25, 22};

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *cloud = lv_obj_create(parent);
        lv_obj_set_size(cloud, cloud_size[i], cloud_size[i]);
        lv_obj_set_style_radius(cloud, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cloud, lv_color_hex(0xF0F0F0), 0);
        lv_obj_set_style_border_width(cloud, 0, 0);
        lv_obj_align(cloud, LV_ALIGN_CENTER, cloud_pos[i][0], cloud_pos[i][1]);
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, cloud);
        lv_anim_set_values(&a, -3, 3);
        lv_anim_set_time(&a, 3000 + (i * 500));
        lv_anim_set_playback_time(&a, 3000);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, anim_set_y_cb);
        lv_anim_start(&a);
    }
}

// Cloudy - gray clouds
void createCloudyIcon(lv_obj_t *parent)
{
    int cloud_pos[][2] = {{-10, 5}, {5, 3}, {15, 7}, {20, 10}};

    int cloud_size[] = {28, 35, 25, 22};

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *cloud = lv_obj_create(parent);
        lv_obj_set_size(cloud, cloud_size[i], cloud_size[i]);
        lv_obj_set_style_radius(cloud, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cloud, lv_color_hex(0xD0D0D0), 0); // Darker gray than partly cloudy
        lv_obj_set_style_border_width(cloud, 0, 0);
        lv_obj_align(cloud, LV_ALIGN_CENTER, cloud_pos[i][0], cloud_pos[i][1]);

        // Floating animation
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, cloud);
        lv_anim_set_values(&a, -3, 3);
        lv_anim_set_time(&a, 3000 + (i * 500));
        lv_anim_set_playback_time(&a, 3000);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, anim_set_y_cb);
        lv_anim_start(&a);
    }
}

// Rain - cloud with falling drops
void createRainIcon(lv_obj_t *parent, int intensity)
{
    int cloud_pos[][2] = {{-12, -12}, {0, -15}, {12, -12}, {20, -8}};

    int cloud_size[] = {30, 35, 32, 25};

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *cloudPart = lv_obj_create(parent);
        lv_obj_set_size(cloudPart, cloud_size[i], cloud_size[i]);
        lv_obj_set_style_radius(cloudPart, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cloudPart, lv_color_hex(0x808080), 0); // Dark gray
        lv_obj_set_style_border_width(cloudPart, 0, 0);
        lv_obj_align(cloudPart, LV_ALIGN_CENTER, cloud_pos[i][0], cloud_pos[i][1]);

        // Cloud bobbing animation
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, cloudPart);
        lv_anim_set_values(&a, -2, 2);
        lv_anim_set_time(&a, 3000 + (i * 300));
        lv_anim_set_playback_time(&a, 3000);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, anim_set_y_cb);
        lv_anim_start(&a);
    }
    int dropCount = 3 + intensity; // More drops for higher intensity
    for (int i = 0; i < dropCount; i++)
    {
        lv_obj_t *drop = lv_obj_create(parent);
        lv_obj_set_size(drop, 2, 10);                               // 2 pixels wide, 10 pixels tall
        lv_obj_set_style_radius(drop, 1, 0);                        // Slightly rounded
        lv_obj_set_style_bg_color(drop, lv_color_hex(0x4682B4), 0); // Steel blue
        lv_obj_set_style_border_width(drop, 0, 0);
        lv_obj_align(drop, LV_ALIGN_CENTER, -15 + (i * 6), 5); // Spread drops horizontally

        // Falling animation
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, drop);
        lv_anim_set_values(&a, -3, 30); // Fall from top to bottom
        lv_anim_set_time(&a, 900 - (intensity * 100));
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&a, i * 100);
        lv_anim_set_exec_cb(&a, anim_set_y_cb);
        lv_anim_start(&a);
    }
}

// Snow - cloud with snowflakes
void createSnowIcon(lv_obj_t *parent, int intensity)
{
    int cloud_pos[][2] = {{-12, -12}, {0, -14}, {12, -11}, {18, -7}};

    int cloud_size[] = {28, 33, 30, 24};

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *cloudPart = lv_obj_create(parent);
        lv_obj_set_size(cloudPart, cloud_size[i], cloud_size[i]);
        lv_obj_set_style_radius(cloudPart, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cloudPart, lv_color_hex(0xE0E0E0), 0);
        lv_obj_set_style_border_width(cloudPart, 0, 0);
        lv_obj_align(cloudPart, LV_ALIGN_CENTER, cloud_pos[i][0], cloud_pos[i][1]);

        // Cloud floating animation
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, cloudPart);
        lv_anim_set_values(&a, -3, 3);
        lv_anim_set_time(&a, 4000 + (i * 300));
        lv_anim_set_playback_time(&a, 4000);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, anim_set_y_cb);
        lv_anim_start(&a);
    }
    int count = 4 + (intensity * 2); // More flakes for heavier snow
    for (int i = 0; i < count; i++)
    {
        lv_obj_t *flake = lv_obj_create(parent);
        lv_obj_set_size(flake, 7, 7); // 7x7 pixel snowflake
        lv_obj_set_style_radius(flake, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(flake, lv_color_white(), 0); // White
        lv_obj_set_style_border_width(flake, 1, 0);
        lv_obj_set_style_border_color(flake, lv_color_hex(0xF0F0F0), 0); // Light border
        lv_obj_align(flake, LV_ALIGN_CENTER, -20 + (i * 5), 5);

        // Falling animation (vertical movement)

        lv_anim_t a1;
        lv_anim_init(&a1);
        lv_anim_set_var(&a1, flake);
        lv_anim_set_values(&a1, 2, 32);          // Fall distance
        lv_anim_set_time(&a1, 2000 + (i * 150)); // Slower than rain
        lv_anim_set_repeat_count(&a1, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&a1, 300 * i);
        lv_anim_set_exec_cb(&a1, anim_set_y_cb);
        lv_anim_start(&a1);

        // Drifting animation (horizontal movement)

        lv_anim_t a2;
        lv_anim_init(&a2);
        lv_anim_set_var(&a2, flake);
        lv_anim_set_values(&a2, -5, 5);
        lv_anim_set_time(&a2, 3000 + (i * 200));
        lv_anim_set_playback_time(&a2, 3000);
        lv_anim_set_repeat_count(&a2, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a2, anim_set_x_cb);
        lv_anim_start(&a2);
    }
}

// Fog - fading horizontal bars
void createFogIcon(lv_obj_t *parent)
{
    for (int i = 0; i < 5; i++)
    {
        lv_obj_t *fogLayer = lv_obj_create(parent);
        lv_obj_set_size(fogLayer, 50, 6);
        lv_obj_set_style_radius(fogLayer, 3, 0);
        lv_obj_set_style_bg_color(fogLayer, lv_color_hex(0xC0C0C0), 0); // Silver/gray
        lv_obj_set_style_border_width(fogLayer, 0, 0);
        lv_obj_align(fogLayer, LV_ALIGN_CENTER, 0, -20 + (i * 10)); // Stack vertically

        // Create pulsing/fading animation for fog effect
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, fogLayer);
        lv_anim_set_values(&a, LV_OPA_40, LV_OPA_90);
        lv_anim_set_time(&a, 2500 + (i * 200));
        lv_anim_set_playback_time(&a, 2500 + (i * 200));
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, anim_set_opa_cb);
        lv_anim_start(&a);
    }
}

// Sleet - rain and snow mix
void createSleetIcon(lv_obj_t *parent, int intensity)
{
    int cloud_pos[][2] = {{-12, -12}, {0, -14}, {12, -11}, {18, -8}};

    int cloud_size[] = {28, 33, 30, 24};

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *cloudPart = lv_obj_create(parent);
        lv_obj_set_size(cloudPart, cloud_size[i], cloud_size[i]);
        lv_obj_set_style_radius(cloudPart, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cloudPart, lv_color_hex(0xA0A0A0), 0); // Medium gray
        lv_obj_set_style_border_width(cloudPart, 0, 0);
        lv_obj_align(cloudPart, LV_ALIGN_CENTER, cloud_pos[i][0], cloud_pos[i][1]);
    }
    int count = 3 + intensity;

    for (int i = 0; i < count; i++)
    {
        if (i % 2 == 0)
        {
            lv_obj_t *drop = lv_obj_create(parent);
            lv_obj_set_size(drop, 2, 8);
            lv_obj_set_style_radius(drop, 1, 0);
            lv_obj_set_style_bg_color(drop, lv_color_hex(0x4682B4), 0); // Steel blue
            lv_obj_set_style_border_width(drop, 0, 0);
            lv_obj_align(drop, LV_ALIGN_CENTER, -18 + (i * 7), 5);

            // Falling animation
            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, drop);
            lv_anim_set_values(&a, -2, 28);
            lv_anim_set_time(&a, 1000);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_repeat_delay(&a, i * 120);
            lv_anim_set_exec_cb(&a, anim_set_y_cb);
            lv_anim_start(&a);
        }
        else
        {
            lv_obj_t *flake = lv_obj_create(parent);
            lv_obj_set_size(flake, 6, 6);
            lv_obj_set_style_radius(flake, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(flake, lv_color_white(), 0);
            lv_obj_set_style_border_width(flake, 1, 0);
            lv_obj_set_style_border_color(flake, lv_color_hex(0xE0E0E0), 0);
            lv_obj_align(flake, LV_ALIGN_CENTER, -18 + (i * 7), 5);

            // Falling animation (slower than rain)

            lv_anim_t a;
            lv_anim_init(&a);
            lv_anim_set_var(&a, flake);
            lv_anim_set_values(&a, 0, 30);
            lv_anim_set_time(&a, 1800);
            lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
            lv_anim_set_repeat_delay(&a, i * 150);
            lv_anim_set_exec_cb(&a, anim_set_y_cb);
            lv_anim_start(&a);
        }
    }
}

// Thunder - storm cloud with lightning
void createThunderIcon(lv_obj_t *parent)
{
    int cloud_pos[][2] = {{-15, -15}, {-2, -18}, {12, -15}, {22, -11}, {-8, -8}};

    int cloud_size[] = {32, 38, 35, 28, 30};

    for (int i = 0; i < 5; i++)
    {
        lv_obj_t *cloudPart = lv_obj_create(parent);
        lv_obj_set_size(cloudPart, cloud_size[i], cloud_size[i]);
        lv_obj_set_style_radius(cloudPart, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cloudPart, lv_color_hex(0x666666), 0); // Very dark gray
        lv_obj_set_style_border_width(cloudPart, 0, 0);
        lv_obj_align(cloudPart, LV_ALIGN_CENTER, cloud_pos[i][0], cloud_pos[i][1]);
    }
    // Top segment
    lv_obj_t *bolt_top = lv_obj_create(parent);
    lv_obj_set_size(bolt_top, 8, 12);
    lv_obj_set_style_bg_color(bolt_top, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_border_width(bolt_top, 0, 0);
    lv_obj_align(bolt_top, LV_ALIGN_CENTER, -2, 2);

    // Middle segment 1 (angled)
    lv_obj_t *bolt_mid1 = lv_obj_create(parent);
    lv_obj_set_size(bolt_mid1, 12, 8);
    lv_obj_set_style_bg_color(bolt_mid1, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_border_width(bolt_mid1, 0, 0);
    lv_obj_set_style_transform_angle(bolt_mid1, 150, 0); // Rotate 15 degrees
    lv_obj_align(bolt_mid1, LV_ALIGN_CENTER, 3, 10);

    // Middle segment 2 (angled opposite)
    lv_obj_t *bolt_mid2 = lv_obj_create(parent);
    lv_obj_set_size(bolt_mid2, 6, 10);
    lv_obj_set_style_bg_color(bolt_mid2, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_border_width(bolt_mid2, 0, 0);
    lv_obj_set_style_transform_angle(bolt_mid2, -100, 0); // Rotate -10 degrees
    lv_obj_align(bolt_mid2, LV_ALIGN_CENTER, -1, 16);

    // Bottom segment (arrow tip)
    lv_obj_t *bolt_bottom = lv_obj_create(parent);
    lv_obj_set_size(bolt_bottom, 8, 10);
    lv_obj_set_style_bg_color(bolt_bottom, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_border_width(bolt_bottom, 0, 0);
    lv_obj_set_style_transform_angle(bolt_bottom, 100, 0); // Rotate 10 degrees
    lv_obj_align(bolt_bottom, LV_ALIGN_CENTER, 2, 23);
    lv_obj_t *bolts[] = {bolt_top, bolt_mid1, bolt_mid2, bolt_bottom};

    for (int i = 0; i < 4; i++)
    {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, bolts[i]);
        lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER);
        lv_anim_set_time(&a, 80); // Very fast flash (80ms)

        lv_anim_set_playback_time(&a, 80);
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&a, 1500);
        lv_anim_set_exec_cb(&a, anim_set_opa_cb);
        lv_anim_start(&a);
    }
}

// Map SMHI symbol codes to icons
void createWeatherIconBySymbol(lv_obj_t *parent, int symbol)
{
    switch (symbol)
    {
    case 1: // Clear sky
        createClearSkyIcon(parent);
        break;

    case 2: // Nearly clear sky
    case 3: // Variable cloudiness
    case 4: // Halfclear sky
        createPartlyCloudyIcon(parent);
        break;

    case 5: // Cloudy sky
    case 6: // Overcast
        createCloudyIcon(parent);
        break;

    case 7: // Fog
        createFogIcon(parent);
        break;

    case 8:                                 // Light rain showers
    case 9:                                 // Moderate rain showers
    case 10:                                // Heavy rain showers
        createRainIcon(parent, symbol - 8); // intensity = 0, 1, or 2
        break;

    case 11: // Thunderstorm
    case 21: // Thunder
        createThunderIcon(parent);
        break;

    case 12:                                  // Light sleet showers
    case 13:                                  // Moderate sleet showers
    case 14:                                  // Heavy sleet showers
        createSleetIcon(parent, symbol - 12); // intensity = 0, 1, or 2
        break;

    case 15:                                 // Light snow showers
    case 16:                                 // Moderate snow showers
    case 17:                                 // Heavy snow showers
        createSnowIcon(parent, symbol - 15); // intensity = 0, 1, or 2
        break;

    case 18:                                 // Light rain
    case 19:                                 // Moderate rain
    case 20:                                 // Heavy rain
        createRainIcon(parent, symbol - 18); // intensity = 0, 1, or 2
        break;

    case 22:                                  // Light sleet
    case 23:                                  // Moderate sleet
    case 24:                                  // Heavy sleet
        createSleetIcon(parent, symbol - 22); // intensity = 0, 1, or 2
        break;

    case 25:                                 // Light snowfall
    case 26:                                 // Moderate snowfall
    case 27:                                 // Heavy snowfall
        createSnowIcon(parent, symbol - 25); // intensity = 0, 1, or 2
        break;

    default: // Unrecognized weather symbol code
        // Display a question mark for unknown weather conditions
        createTextIcon(parent, "?", 0x808080, 28);
    }
}

#endif // ICONS_H