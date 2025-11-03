/**
 * @file icons.h
 * @brief Weather icon creation and animation functions for the weather station display
 *
 * This file contains functions to create animated weather icons using LVGL graphics library.
 * Each weather condition (sunny, cloudy, rainy, etc.) has its own animated icon.
 *
 * The icons use simple geometric shapes (circles, rectangles) with animations to create
 * visually appealing weather representations without requiring image files.
 *
 * @author Weather Station Project
 * @date 2024
 */

#ifndef ICONS_H
#define ICONS_H

#include <lvgl.h>

// ==========================================
// SIMPLE TEXT ICON HELPERS
// ==========================================

/**
 * @brief Creates a simple text-based icon (fallback for when graphics fail)
 *
 * This function creates a text label that can be used as a simple icon.
 * Useful as a fallback when graphical icons don't render properly.
 *
 * @param parent The parent LVGL object where the icon will be placed
 * @param iconText The text to display (e.g., "☀", "☁", "☔")
 * @param color The color of the text in hex format (e.g., 0xFFD700 for gold)
 * @param size The font size (not currently used, but kept for future implementation)
 * @return lv_obj_t* Pointer to the created label object
 */
lv_obj_t *createTextIcon(lv_obj_t *parent, const char *iconText, uint32_t color, int size)
{
    // Create a label object (text display element)
    lv_obj_t *label = lv_label_create(parent);

    // Set the text content
    lv_label_set_text(label, iconText);

    // Set the font size to 28 pixels (medium-large)
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);

    // Set the text color
    lv_obj_set_style_text_color(label, lv_color_hex(color), 0);

    // Center the text in its parent container
    lv_obj_center(label);

    return label;
}

// ==========================================
// ANIMATION CALLBACK FUNCTIONS
// ==========================================
// These functions are called by the animation system to update object properties

/**
 * @brief Animation callback to rotate an object
 * @param obj The object to rotate (cast from void*)
 * @param value The rotation angle in degrees (0-3600 for full rotation)
 */
static void anim_set_transform_angle_cb(void *obj, int32_t value)
{
    lv_obj_set_style_transform_angle((lv_obj_t *)obj, value, 0);
}

/**
 * @brief Animation callback to change object opacity (transparency)
 * @param obj The object to modify (cast from void*)
 * @param value The opacity value (0 = transparent, 255 = opaque)
 */
static void anim_set_opa_cb(void *obj, int32_t value)
{
    lv_obj_set_style_opa((lv_obj_t *)obj, value, 0);
}

/**
 * @brief Animation callback to move object horizontally
 * @param obj The object to move (cast from void*)
 * @param value The X position in pixels
 */
static void anim_set_x_cb(void *obj, int32_t value)
{
    lv_obj_set_x((lv_obj_t *)obj, value);
}

/**
 * @brief Animation callback to move object vertically
 * @param obj The object to move (cast from void*)
 * @param value The Y position in pixels
 */
static void anim_set_y_cb(void *obj, int32_t value)
{
    lv_obj_set_y((lv_obj_t *)obj, value);
}

// ==========================================
// ANIMATED WEATHER ICON FUNCTIONS
// ==========================================

/**
 * @brief Creates an animated sun icon for clear sky weather
 *
 * Creates a golden circle as the sun with 8 animated rays around it.
 * The rays pulse (fade in and out) to create a glowing effect.
 *
 * @param parent The parent container where the icon will be created
 */
void createClearSkyIcon(lv_obj_t *parent)
{
    // Create the main sun circle
    lv_obj_t *sun = lv_obj_create(parent);
    lv_obj_set_size(sun, 40, 40);                              // 40x40 pixel circle
    lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);         // Make it circular
    lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFD700), 0); // Gold color
    lv_obj_set_style_border_width(sun, 0, 0);                  // No border
    lv_obj_center(sun);                                        // Center in parent

    // Create 8 sun rays around the sun
    for (int i = 0; i < 8; i++)
    {
        // Create a small circle for each ray
        lv_obj_t *ray = lv_obj_create(parent);
        lv_obj_set_size(ray, 8, 8);                                // Small 8x8 pixel circle
        lv_obj_set_style_radius(ray, LV_RADIUS_CIRCLE, 0);         // Make it circular
        lv_obj_set_style_bg_color(ray, lv_color_hex(0xFFD700), 0); // Same gold color
        lv_obj_set_style_border_width(ray, 0, 0);                  // No border

        // Calculate position in a circle around the sun
        // Convert degrees to radians: angle = i * 45 degrees (360/8 = 45)
        float angle = (i * 45) * 3.14159 / 180;
        int x = 30 * cos(angle); // X position using cosine
        int y = 30 * sin(angle); // Y position using sine
        lv_obj_align(ray, LV_ALIGN_CENTER, x, y);

        // Create pulsing animation for this ray
        lv_anim_t a;
        lv_anim_init(&a);                                      // Initialize animation
        lv_anim_set_var(&a, ray);                              // Animate this ray
        lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_30);       // Fade from opaque to semi-transparent
        lv_anim_set_time(&a, 1200 + (i * 150));                // Duration with offset for each ray
        lv_anim_set_playback_time(&a, 1200);                   // Fade back duration
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE); // Loop forever
        lv_anim_set_exec_cb(&a, anim_set_opa_cb);              // Use opacity animation callback
        lv_anim_start(&a);                                     // Start the animation
    }
}

/**
 * @brief Creates an animated partly cloudy icon
 *
 * Shows a pulsing sun partially hidden behind floating clouds.
 * Used for weather symbols 2, 3, and 4.
 *
 * @param parent The parent container where the icon will be created
 */
void createPartlyCloudyIcon(lv_obj_t *parent)
{
    // Create the sun (smaller and offset since it's partially hidden)
    lv_obj_t *sun = lv_obj_create(parent);
    lv_obj_set_size(sun, 25, 25);                              // Smaller sun
    lv_obj_set_style_radius(sun, LV_RADIUS_CIRCLE, 0);         // Circular
    lv_obj_set_style_bg_color(sun, lv_color_hex(0xFFA500), 0); // Orange color
    lv_obj_set_style_border_width(sun, 0, 0);                  // No border
    lv_obj_align(sun, LV_ALIGN_CENTER, -18, -12);              // Position top-left

    // Create pulsing animation for the sun
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

    // Create 4 overlapping circles to form a cloud
    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *cloud = lv_obj_create(parent);
        lv_obj_set_size(cloud, cloud_size[i], cloud_size[i]);        // Different sizes
        lv_obj_set_style_radius(cloud, LV_RADIUS_CIRCLE, 0);         // Circular
        lv_obj_set_style_bg_color(cloud, lv_color_hex(0xF0F0F0), 0); // Light gray
        lv_obj_set_style_border_width(cloud, 0, 0);                  // No border
        lv_obj_align(cloud, LV_ALIGN_CENTER, cloud_pos[i][0], cloud_pos[i][1]);

        // Create floating animation for cloud parts
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, cloud);
        lv_anim_set_values(&a, -3, 3);          // Float up and down 3 pixels
        lv_anim_set_time(&a, 3000 + (i * 500)); // Different speed for each part
        lv_anim_set_playback_time(&a, 3000);    // Float back down
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, anim_set_y_cb); // Vertical movement
        lv_anim_start(&a);
    }
}

/**
 * @brief Creates an animated cloudy sky icon
 *
 * Shows floating gray clouds without sun.
 * Used for overcast weather conditions (symbols 5 and 6).
 *
 * @param parent The parent container where the icon will be created
 */
void createCloudyIcon(lv_obj_t *parent)
{
    // Define positions and sizes for cloud parts
    int cloud_pos[][2] = {{-10, 5}, {5, 3}, {15, 7}, {20, 10}};
    int cloud_size[] = {28, 35, 25, 22};

    // Create 4 overlapping circles to form a fluffy cloud
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

/**
 * @brief Creates an animated rain icon with falling droplets
 *
 * Shows a dark cloud with animated rain drops falling.
 * The intensity parameter controls how many drops and how fast they fall.
 *
 * @param parent The parent container where the icon will be created
 * @param intensity Rain intensity (0=light, 1=moderate, 2=heavy)
 */
void createRainIcon(lv_obj_t *parent, int intensity)
{
    // Create dark storm cloud using multiple circles
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

    // Create animated rain drops
    int dropCount = 3 + intensity; // More drops for higher intensity
    for (int i = 0; i < dropCount; i++)
    {
        // Create a raindrop (thin vertical rectangle)
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
        lv_anim_set_values(&a, -3, 30);                // Fall from top to bottom
        lv_anim_set_time(&a, 900 - (intensity * 100)); // Faster for heavier rain
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&a, i * 100); // Stagger the drops
        lv_anim_set_exec_cb(&a, anim_set_y_cb);
        lv_anim_start(&a);
    }
}

/**
 * @brief Creates an animated snow icon with falling snowflakes
 *
 * Shows a light cloud with animated snowflakes that fall and drift.
 * The intensity parameter controls the number of snowflakes.
 *
 * @param parent The parent container where the icon will be created
 * @param intensity Snow intensity (0=light, 1=moderate, 2=heavy)
 */
void createSnowIcon(lv_obj_t *parent, int intensity)
{
    // Create light gray cloud for snow
    int cloud_pos[][2] = {{-12, -12}, {0, -14}, {12, -11}, {18, -7}};
    int cloud_size[] = {28, 33, 30, 24};

    for (int i = 0; i < 4; i++)
    {
        lv_obj_t *cloudPart = lv_obj_create(parent);
        lv_obj_set_size(cloudPart, cloud_size[i], cloud_size[i]);
        lv_obj_set_style_radius(cloudPart, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(cloudPart, lv_color_hex(0xE0E0E0), 0); // Light gray
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

    // Create animated snowflakes
    int count = 4 + (intensity * 2); // More flakes for heavier snow
    for (int i = 0; i < count; i++)
    {
        // Create a snowflake (white circle)
        lv_obj_t *flake = lv_obj_create(parent);
        lv_obj_set_size(flake, 7, 7);                                    // 7x7 pixel snowflake
        lv_obj_set_style_radius(flake, LV_RADIUS_CIRCLE, 0);             // Circular
        lv_obj_set_style_bg_color(flake, lv_color_white(), 0);           // White
        lv_obj_set_style_border_width(flake, 1, 0);                      // Thin border
        lv_obj_set_style_border_color(flake, lv_color_hex(0xF0F0F0), 0); // Light border
        lv_obj_align(flake, LV_ALIGN_CENTER, -20 + (i * 5), 5);

        // Falling animation (vertical movement)
        lv_anim_t a1;
        lv_anim_init(&a1);
        lv_anim_set_var(&a1, flake);
        lv_anim_set_values(&a1, 2, 32);          // Fall distance
        lv_anim_set_time(&a1, 2000 + (i * 150)); // Slower than rain
        lv_anim_set_repeat_count(&a1, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&a1, 300 * i); // Stagger snowflakes
        lv_anim_set_exec_cb(&a1, anim_set_y_cb);
        lv_anim_start(&a1);

        // Drifting animation (horizontal movement)
        lv_anim_t a2;
        lv_anim_init(&a2);
        lv_anim_set_var(&a2, flake);
        lv_anim_set_values(&a2, -5, 5); // Drift left and right
        lv_anim_set_time(&a2, 3000 + (i * 200));
        lv_anim_set_playback_time(&a2, 3000);
        lv_anim_set_repeat_count(&a2, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a2, anim_set_x_cb);
        lv_anim_start(&a2);
    }
}

/**
 * @brief Creates an animated fog icon
 *
 * Shows layered horizontal bars that fade in and out to simulate fog.
 *
 * @param parent The parent container where the icon will be created
 */
void createFogIcon(lv_obj_t *parent)
{
    // Create 5 horizontal bars to represent fog layers
    for (int i = 0; i < 5; i++)
    {
        lv_obj_t *fogLayer = lv_obj_create(parent);
        lv_obj_set_size(fogLayer, 50, 6);                             // Wide thin bars
        lv_obj_set_style_radius(fogLayer, 3, 0);                      // Rounded ends
        lv_obj_set_style_bg_color(fogLayer, lv_color_hex(0xC0C0C0), 0); // Silver/gray
        lv_obj_set_style_border_width(fogLayer, 0, 0);
        lv_obj_align(fogLayer, LV_ALIGN_CENTER, 0, -20 + (i * 10)); // Stack vertically

        // Create pulsing/fading animation for fog effect
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, fogLayer);
        lv_anim_set_values(&a, LV_OPA_40, LV_OPA_90);       // Fade between 40% and 90% opacity
        lv_anim_set_time(&a, 2500 + (i * 200));            // Different timing per layer
        lv_anim_set_playback_time(&a, 2500 + (i * 200));
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_exec_cb(&a, anim_set_opa_cb);
        lv_anim_start(&a);
    }
}

/**
 * @brief Creates an animated sleet icon (rain and snow mix)
 *
 * Shows a cloud with both rain drops and snowflakes falling.
 *
 * @param parent The parent container where the icon will be created
 * @param intensity Sleet intensity (0=light, 1=moderate, 2=heavy)
 */
void createSleetIcon(lv_obj_t *parent, int intensity)
{
    // Create gray cloud
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

    // Create mix of rain drops and snowflakes
    int count = 3 + intensity;
    for (int i = 0; i < count; i++)
    {
        if (i % 2 == 0)
        {
            // Create raindrop
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
            // Create snowflake
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

/**
 * @brief Creates an animated thunderstorm icon with lightning
 *
 * Shows a dark storm cloud with an animated lightning bolt that flashes.
 * The lightning is made of multiple segments to look more realistic.
 *
 * @param parent The parent container where the icon will be created
 */
void createThunderIcon(lv_obj_t *parent)
{
    // Create dark storm cloud (5 circles for more volume)
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

    // Create lightning bolt using 4 segments for zigzag effect

    // Top segment
    lv_obj_t *bolt_top = lv_obj_create(parent);
    lv_obj_set_size(bolt_top, 8, 12);
    lv_obj_set_style_bg_color(bolt_top, lv_color_hex(0xFFD700), 0); // Gold color
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

    // Create flashing animation for all lightning segments
    lv_obj_t *bolts[] = {bolt_top, bolt_mid1, bolt_mid2, bolt_bottom};
    for (int i = 0; i < 4; i++)
    {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, bolts[i]);
        lv_anim_set_values(&a, LV_OPA_0, LV_OPA_COVER); // Flash from invisible to visible
        lv_anim_set_time(&a, 80);                       // Very fast flash (80ms)
        lv_anim_set_playback_time(&a, 80);              // Flash back to invisible
        lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
        lv_anim_set_repeat_delay(&a, 1500); // Wait 1.5 seconds between flashes
        lv_anim_set_exec_cb(&a, anim_set_opa_cb);
        lv_anim_start(&a);
    }
}

/**
 * @brief Main function to create weather icon based on SMHI weather symbol code
 *
 * This function maps SMHI weather symbol codes to the appropriate icon function.
 * SMHI (Swedish Meteorological and Hydrological Institute) uses specific codes
 * for different weather conditions.
 *
 * Complete symbol mapping (1-27):
 * - 1: Clear sky
 * - 2: Nearly clear sky
 * - 3: Variable cloudiness
 * - 4: Halfclear sky
 * - 5: Cloudy sky
 * - 6: Overcast
 * - 7: Fog
 * - 8: Light rain showers
 * - 9: Moderate rain showers
 * - 10: Heavy rain showers
 * - 11: Thunderstorm
 * - 12: Light sleet showers
 * - 13: Moderate sleet showers
 * - 14: Heavy sleet showers
 * - 15: Light snow showers
 * - 16: Moderate snow showers
 * - 17: Heavy snow showers
 * - 18: Light rain
 * - 19: Moderate rain
 * - 20: Heavy rain
 * - 21: Thunder
 * - 22: Light sleet
 * - 23: Moderate sleet
 * - 24: Heavy sleet
 * - 25: Light snowfall
 * - 26: Moderate snowfall
 * - 27: Heavy snowfall
 *
 * @param parent The parent container where the icon will be created
 * @param symbol The SMHI weather symbol code (1-27)
 */
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