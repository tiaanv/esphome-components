#include "rmt_pulse.h"

#include <driver/rmt.h>
#include <esp_intr_alloc.h>
#include <esp_attr.h>

// Required for IDF 5.x (Arduino 3.2.1)
#include <hal/rmt_ll.h>
#include <soc/rmt_struct.h>
#include <soc/rmt_reg.h>

static intr_handle_t gRMT_intr_handle = NULL;

// RMT channel configuration
static rmt_config_t row_rmt_config;

// Track active pulse
volatile bool rmt_tx_done = true;

/**
 * RMT interrupt handler: signals when transmission is done.
 */
static void IRAM_ATTR rmt_interrupt_handler(void *arg)
{
    rmt_tx_done = true;
    RMT.int_clr.val = RMT.int_st.val;
}

void rmt_pulse_init(gpio_num_t pin)
{
    row_rmt_config.rmt_mode = RMT_MODE_TX;

    // Use channel 1 (was channel 0 in older versions)
    row_rmt_config.channel = RMT_CHANNEL_1;
    row_rmt_config.gpio_num = pin;
    row_rmt_config.mem_block_num = 2;

    // Divide 80MHz clock by 8 = 0.1 µs ticks
    row_rmt_config.clk_div = 8;

    row_rmt_config.tx_config.loop_en = false;
    row_rmt_config.tx_config.carrier_en = false;
    row_rmt_config.tx_config.carrier_level = RMT_CARRIER_LEVEL_LOW;
    row_rmt_config.tx_config.idle_level = RMT_IDLE_LEVEL_LOW;
    row_rmt_config.tx_config.idle_output_en = true;

    // Register interrupt handler
    rmt_isr_register(rmt_interrupt_handler, 0,
                     ESP_INTR_FLAG_LEVEL3, &gRMT_intr_handle);

    // Apply RMT peripheral config
    rmt_config(&row_rmt_config);

    // Enable TX-end interrupt (modern IDF)
    rmt_ll_enable_tx_end_interrupt(&RMT, row_rmt_config.channel, true);
}

void IRAM_ATTR pulse_ckv_ticks(uint16_t high_ticks,
                               uint16_t low_ticks,
                               bool wait)
{
    while (!rmt_tx_done) { }

    volatile rmt_item32_t *rmt_mem_ptr =
        &(RMTMEM.chan[row_rmt_config.channel].data32[0]);

    if (high_ticks > 0) {
        rmt_mem_ptr->level0    = 1;
        rmt_mem_ptr->duration0 = high_ticks;
        rmt_mem_ptr->level1    = 0;
        rmt_mem_ptr->duration1 = low_ticks;
    } else {
        rmt_mem_ptr->level0    = 1;
        rmt_mem_ptr->duration0 = low_ticks;
        rmt_mem_ptr->level1    = 0;
        rmt_mem_ptr->duration1 = 0;
    }

    // Next item = terminator
    RMTMEM.chan[row_rmt_config.channel].data32[1].val = 0;

    rmt_tx_done = false;

    RMT.conf_ch[row_rmt_config.channel].conf1.mem_rd_rst = 1;
    RMT.conf_ch[row_rmt_config.channel].conf1.mem_owner  = RMT_MEM_OWNER_TX;
    RMT.conf_ch[row_rmt_config.channel].conf1.tx_start   = 1;

    while (wait && !rmt_tx_done) { }
}

void IRAM_ATTR pulse_ckv_us(uint16_t high_us,
                            uint16_t low_us,
                            bool wait)
{
    pulse_ckv_ticks(high_us * 10, low_us * 10, wait);
}

bool IRAM_ATTR rmt_busy()
{
    return !rmt_tx_done;
}
