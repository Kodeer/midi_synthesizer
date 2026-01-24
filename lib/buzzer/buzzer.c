#include "buzzer.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

static uint8_t buzzer_gpio = 0;
static uint pwm_slice = 0;
static uint pwm_channel = 0;
static bool initialized = false;

/**
 * @brief Initialize the PWM buzzer
 */
bool buzzer_init(uint8_t gpio_pin) {
    buzzer_gpio = gpio_pin;
    
    // Set up GPIO for PWM
    gpio_set_function(buzzer_gpio, GPIO_FUNC_PWM);
    
    // Get PWM slice and channel for this GPIO
    pwm_slice = pwm_gpio_to_slice_num(buzzer_gpio);
    pwm_channel = pwm_gpio_to_channel(buzzer_gpio);
    
    // Configure PWM
    pwm_config config = pwm_get_default_config();
    pwm_init(pwm_slice, &config, false);
    
    // Start with buzzer off
    pwm_set_gpio_level(buzzer_gpio, 0);
    
    initialized = true;
    return true;
}

/**
 * @brief Play a tone at specified frequency
 */
void buzzer_tone(uint16_t frequency, uint16_t duration_ms) {
    if (!initialized || frequency == 0) {
        buzzer_stop();
        return;
    }
    
    // Calculate PWM parameters
    // System clock is 125MHz
    uint32_t clock_freq = 125000000;
    uint32_t divider = clock_freq / (frequency * 4096); // 4096 = wrap value for good resolution
    
    if (divider < 1) divider = 1;
    if (divider > 255) divider = 255;
    
    uint32_t wrap = clock_freq / (frequency * divider);
    
    // Set PWM frequency
    pwm_set_clkdiv(pwm_slice, (float)divider);
    pwm_set_wrap(pwm_slice, wrap);
    
    // 50% duty cycle for square wave
    pwm_set_chan_level(pwm_slice, pwm_channel, wrap / 2);
    
    // Enable PWM
    pwm_set_enabled(pwm_slice, true);
    
    // If duration specified, wait and then stop
    if (duration_ms > 0) {
        sleep_ms(duration_ms);
        buzzer_stop();
    }
}

/**
 * @brief Stop the buzzer
 */
void buzzer_stop(void) {
    if (!initialized) return;
    
    pwm_set_gpio_level(buzzer_gpio, 0);
    pwm_set_enabled(pwm_slice, false);
}

/**
 * @brief Play a short click sound
 */
void buzzer_click(void) {
    buzzer_tone(2000, 30); // 2kHz for 30ms
}

/**
 * @brief Play an error sound (two-tone alarm)
 */
void buzzer_error(void) {
    buzzer_tone(500, 150);   // Low tone
    sleep_ms(50);
    buzzer_tone(1000, 150);  // High tone
    sleep_ms(50);
    buzzer_tone(500, 150);   // Low tone again
}

/**
 * @brief Play a boot-up melody (ascending notes)
 */
void buzzer_boot_melody(void) {
    // Simple startup melody: C5, E5, G5, C6
    buzzer_tone(523, 100);  // C5
    sleep_ms(20);
    buzzer_tone(659, 100);  // E5
    sleep_ms(20);
    buzzer_tone(784, 100);  // G5
    sleep_ms(20);
    buzzer_tone(1047, 150); // C6
}

/**
 * @brief Play a success sound (ascending tones)
 */
void buzzer_success(void) {
    buzzer_tone(800, 80);   // First tone
    sleep_ms(20);
    buzzer_tone(1200, 120); // Higher tone, longer
}
