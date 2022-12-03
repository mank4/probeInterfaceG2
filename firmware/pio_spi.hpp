/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _PIO_SPI_H
#define _PIO_SPI_H

#include <stdio.h>

#include "hardware/pio.h"

class pioSpi {
private:
    const PIO pio = pio0;
    const uint sm = 0; //state machine
    
    bool isEnabled = false;
    
    uint sck_pin = 0;
    uint mosi_pin = 0;
    uint miso_pin = 0;
    uint cs_pin = 0;
    
    bool cpha = false;
    bool cpol = false;
    
    float clkdiv = 31.25f; // 1 MHz @ 125 clk_sys
    
    uint cpha0_prog_offs = 0;
    uint cpha1_prog_offs = 0;
public:
    pioSpi(PIO pio = pio0, uint sm = 0);
    ~pioSpi();
    
    bool set_sck_pin(uint sck);
    bool set_mosi_pin(uint mosi);
    bool set_miso_pin(uint miso);
    bool set_cs_pin(uint cs);
    
    bool set_cpha(bool _cpha);
    bool set_cpol(bool _cpol);
    
    bool set_baudrate(float baudMHz);
    
    bool reset();
    bool enable(bool on);
    void rw8_blocking(uint8_t *src, uint8_t *dst, size_t len);
};

#endif
