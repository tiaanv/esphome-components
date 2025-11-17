#include "rmt_pulse.h"

#include <driver/rmt.h>
#include <driver/gpio.h>
#include <esp_attr.h>

static rmt_channel_t s_rmt_channel = RMT_CHANNEL_1;
static gpio_num_t s_rmt_pin;

// We keep the symbol for API compatibility, but don't need ISR any more
static intr_handle_t gRMT_intr_handle = NULL;

// Not actually used in this simplified implementation, but kept for signature compatibility
volatile bool rmt_tx_done = true;

void rmt_pulse_init(gpio_num_t pin)
{
    s_rmt_pin = pin;
    s_rmt_channel = RMT_CHANNEL_1;

    rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = s_rmt_channel,
        .gpio_num = s_rmt_pin,
        .mem_block_num = 1,
        .clk_div = 8,  // 80 MHz / 8 = 0.1 µs per tick
        .tx_config = {
            .loop_en = false,
            .carrier_en = false,
            .carrier_level = RMT_CARRIER_LEVEL_LOW,
            .idle_level = RMT_IDLE_LEVEL_LOW,
            .idle_output_en = true,
        },
    };

    rmt_config(&config);
    // No RX buffer, no event queue
    rmt_driver_install(s_rmt_channel, 0, 0);
}

/**
 * Send a single pulse defined in ticks.
 * This is blocking; when it returns, the pulse has completed.
 */
static void IRAM_ATTR send_pulse_ticks(uint16_t high_ticks, uint16_t low_ticks)
{
    rmt_item32_t item = {0};

    if (high_ticks > 0) {
        item.level0    = 1;
        item.duration0 = high_ticks;
        item.level1    = 0;
        item.duration1 = low_ticks;
    } else {
        item.level0    = 1;
        item.duration0 = low_ticks;
        item.level1    = 0;
        item.duration1 = 0;
    }

    // Blocking write: wait_tx_done = true
    rmt_write_items(s_rmt_channel, &item, 1, true);
}

void IRAM_ATTR pulse_ckv_ticks(uint16_t high_time_ticks,
                               uint16_t low_time_ticks,
                               bool wait)
{
    // We always block until the pulse is done, so "wait" is effectively ignored.
    (void) wait;
    send_pulse_ticks(high_time_ticks, low_time_ticks);
}

void IRAM_ATTR pulse_ckv_us(uint16_t high_time_us,
                            uint16_t low_time_us,
                            bool wait)
{
    (void) wait;
    // 1 tick = 0.1 µs at clk_div=8 → 10 ticks per µs
    send_pulse_ticks(high_time_us * 10, low_time_us * 10);
}

bool IRAM_ATTR rmt_busy()
{
    // Because we use blocking rmt_write_items(), by the time any caller checks,
    // the hardware is idle. So we can safely report "not busy".
    return false;
}
