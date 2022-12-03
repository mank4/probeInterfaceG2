/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pio_spi.hpp"
#include "pio_spi.pio.h"

pioSpi::pioSpi(PIO pio, uint sm) : pio(pio), sm(sm) {
    cpha0_prog_offs = pio_add_program(pio, &spi_cpha0_program);
    cpha1_prog_offs = pio_add_program(pio, &spi_cpha1_program);
}

pioSpi::~pioSpi() {
    pio_remove_program(pio, &spi_cpha0_program, cpha0_prog_offs);
    pio_remove_program(pio, &spi_cpha1_program, cpha1_prog_offs);
}

bool pioSpi::reset() {
    if(isEnabled)
        enable(false);
        
    sck_pin = 0;
    mosi_pin = 0;
    miso_pin = 0;
    cs_pin = 0;
    
    cpha = false;
    cpol = false;
    
    clkdiv = 31.25f; // 1 MHz @ 125 clk_sys
    
    return true;
}

bool pioSpi::set_sck_pin(uint sck) {
    if(isEnabled)
        return false;
    
    sck_pin = sck;
    return true;
}

bool pioSpi::set_mosi_pin(uint mosi) {
    if(isEnabled)
        return false;
    
    mosi_pin = mosi;
    return true;
}

bool pioSpi::set_miso_pin(uint miso) {
    if(isEnabled)
        return false;
    
    miso_pin = miso;
    return true;
}

bool pioSpi::set_cs_pin(uint cs) {
    if(isEnabled)
        return false;
    
    cs_pin = cs;
    return true;
}

bool pioSpi::set_cpha(bool _cpha) {
    if(isEnabled)
        return false;
    
    cpha = _cpha;
    return true;
}

bool pioSpi::set_cpol(bool _cpol) {
    if(isEnabled)
        return false;
    
    cpol = _cpol;
    return true;
}

bool pioSpi::set_baudrate(float baudMHz) {
    if(isEnabled)
        return false;
    
    if(baudMHz > 31.25)
        return false;
    
    if(baudMHz < 5e-4)
        return false;
        
    clkdiv = 31.25f/baudMHz;
    return true;
}

bool pioSpi::enable(bool on) {
    if((on && isEnabled) || (!on && !isEnabled))
        return true;
    
    if(on) {
        pio_spi_init(pio, sm, cpha ? cpha1_prog_offs : cpha0_prog_offs,
            8,       // 8 bits per SPI frame
            clkdiv, cpha, cpol, sck_pin, mosi_pin, miso_pin);
    } else {
        pio_sm_set_enabled(pio, sm, false);
        pio_sm_set_pindirs_with_mask(pio, sm, 0, (1u << sck_pin) | (1u << mosi_pin) | (1u << miso_pin));
    }
    isEnabled = on;
    return true;
}

// Just 8 bit functions provided here. The PIO program supports any frame size
// 1...32, but the software to do the necessary FIFO shuffling is left as an
// exercise for the reader :)
//
// Likewise we only provide MSB-first here. To do LSB-first, you need to
// - Do shifts when reading from the FIFO, for general case n != 8, 16, 32
// - Do a narrow read at a one halfword or 3 byte offset for n == 16, 8
// in order to get the read data correctly justified. 
void __time_critical_func(pioSpi::rw8_blocking)(uint8_t *src, uint8_t *dst, size_t len) {
    size_t tx_remain = len, rx_remain = len;
    // Do 8 bit accesses on FIFO, so that write data is byte-replicated. This
    // gets us the left-justification for free (for MSB-first shift-out)
    io_rw_8 *txfifo = (io_rw_8 *) &pio->txf[sm];
    io_rw_8 *rxfifo = (io_rw_8 *) &pio->rxf[sm];
    while (tx_remain || rx_remain) { //this should be an ||
        if (tx_remain && !pio_sm_is_tx_fifo_full(pio, sm)) {
            *txfifo = *src++;
            --tx_remain;
        }
        if (rx_remain && !pio_sm_is_rx_fifo_empty(pio, sm)) {
            *dst++ = *rxfifo;
            --rx_remain;
        }
    }
}

