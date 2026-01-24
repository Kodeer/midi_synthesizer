#ifndef LISSAJOUS_SCREENSAVER_H
#define LISSAJOUS_SCREENSAVER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the Lissajous curve screensaver
 * 
 * Sets up initial parameters for drawing animated Lissajous curves
 * on the OLED display. Parameters are randomized for variety.
 */
void lissajous_screensaver_init(void);

/**
 * @brief Update and display the Lissajous curve animation
 * 
 * Call this repeatedly (e.g., every frame) to animate the screensaver.
 * The function draws a Lissajous curve based on parametric equations:
 * x = A * sin(a * t + delta)
 * y = B * sin(b * t)
 * 
 * Parameters change occasionally for variety.
 */
void lissajous_screensaver_update(void);

/**
 * @brief Get current screensaver parameter info (for debugging)
 * 
 * @param a_freq Pointer to store frequency A
 * @param b_freq Pointer to store frequency B
 * @param phase Pointer to store phase delta
 */
void lissajous_get_params(float* a_freq, float* b_freq, float* phase);

#endif // LISSAJOUS_SCREENSAVER_H
