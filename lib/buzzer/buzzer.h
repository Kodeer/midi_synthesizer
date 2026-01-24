#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize the PWM buzzer
 * @param gpio_pin GPIO pin connected to the buzzer
 * @return true if initialization successful, false otherwise
 */
bool buzzer_init(uint8_t gpio_pin);

/**
 * @brief Play a tone at specified frequency and duration
 * @param frequency Frequency in Hz (e.g., 440 for A4, 0 to stop)
 * @param duration_ms Duration in milliseconds (0 for continuous)
 */
void buzzer_tone(uint16_t frequency, uint16_t duration_ms);

/**
 * @brief Stop the buzzer
 */
void buzzer_stop(void);

/**
 * @brief Play a short click sound
 */
void buzzer_click(void);

/**
 * @brief Play an error sound (two-tone alarm)
 */
void buzzer_error(void);

/**
 * @brief Play a boot-up melody
 */
void buzzer_boot_melody(void);

/**
 * @brief Play a success sound (ascending tones)
 */
void buzzer_success(void);

#endif // BUZZER_H
