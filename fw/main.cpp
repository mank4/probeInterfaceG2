#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pio_spi.hpp"

#include "scpi/scpi.h"

// This program instantiates a PIO SPI with each of the four possible
// CPOL/CPHA combinations, with the serial input and output pin mapped to the
// same GPIO. Any data written into the state machine's TX FIFO should then be
// serialised, deserialised, and reappear in the state machine's RX FIFO.

#define PIN_LED 25

#define PIN_SCK 18
#define PIN_MOSI 16
#define PIN_MISO 16 // same as MOSI, so we get loopback

#define BUF_SIZE 150

#define SCPI_ERROR_QUEUE_SIZE 17
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

#define SCPI_INPUT_BUFFER_LENGTH 256
static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];

scpi_result_t DMM_MeasureVoltageDcQ(scpi_t * context) {
    SCPI_ResultDouble(context, 3.14);
    return SCPI_RES_OK;
}

scpi_command_t scpi_commands[] = {
	{ .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
	{ .pattern = "*RST", .callback = SCPI_CoreRst,},
	{ .pattern = "MEASure:VOLTage:DC?", .callback = DMM_MeasureVoltageDcQ,},
	SCPI_CMD_LIST_END
};

size_t myWrite(scpi_t * context, const char * data, size_t len) {
    (void) context;
    return fwrite(data, 1, len, stdout);
}

scpi_interface_t scpi_interface = {
    /*.error = */ NULL,
    /*.write = */ myWrite,
    /*.control = */ NULL,
    /*.flush = */ NULL,
    /*.reset = */ NULL,
};

scpi_t scpi_context;

void test(pioSpi &spi) {
    static uint8_t txbuf[BUF_SIZE];
    static uint8_t rxbuf[BUF_SIZE];
    printf("TX:");
    for (int i = 0; i < BUF_SIZE; ++i) {
        txbuf[i] = rand() >> 16;
        rxbuf[i] = 0;
        //printf(" %02x", (int) txbuf[i]);
    }
    printf("\n");
    
    sleep_ms(200);

    spi.enable(true);
    spi.rw8_blocking(txbuf, rxbuf, BUF_SIZE);
    spi.enable(false);

    sleep_ms(200);
    
    printf("RX:");
    bool mismatch = false;
    for (int i = 0; i < BUF_SIZE; ++i) {
        //printf(" %02x", (int) rxbuf[i]);
        mismatch = mismatch || rxbuf[i] != txbuf[i];
    }
    if (mismatch)
        printf("\nNope\n");
    else
        printf("\nOK\n");
}

int main() {
    stdio_init_all();
    
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 1);
    
    gpio_put(PIN_LED, 1);
    printf("RP2040 booting...\n");
    sleep_ms(1000);
    gpio_put(PIN_LED, 0);
    sleep_ms(1000);
        
    pioSpi spi0;
    spi0.set_sck_pin(PIN_SCK);
    spi0.set_miso_pin(PIN_MISO);
    spi0.set_mosi_pin(PIN_MOSI);
    spi0.set_baudrate(5);
    
    SCPI_Init(&scpi_context,
              scpi_commands,
              &scpi_interface,
              scpi_units_def,
              "LTE", "probeInterface", "G2", "dev",
              scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
              scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE
    );
    
    while (true) {
        for (int cpha = 0; cpha <= 1; ++cpha) {
            spi0.set_cpha(cpha);
            
            for (int cpol = 0; cpol <= 0; ++cpol) {
                spi0.set_cpol(cpol);
                //printf("CPHA = %d, CPOL = %d\n", cpha, cpol);
                
                //spi0.enable(true);
                //test(spi0);
                //spi0.enable(false);
                
                char in[40];
                if(fgets(in, 40, stdin) != NULL){
                    //printf("%s", in);
                    SCPI_Input(&scpi_context, in, strlen(in));
                }
                //printf("ping\n");

                sleep_ms(2000);
            }
        }
    }
}
