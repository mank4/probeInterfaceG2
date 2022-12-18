#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pio_spi.hpp"

#include "scpi/scpi.h"
#include "usbtmc_app.h"

#define PIN_LED 25

#define PIN_SCK 18
#define PIN_MOSI 16
#define PIN_MISO 16 // same as MOSI, so we get loopback

#define SCPI_ERROR_QUEUE_SIZE 17
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

//#define SCPI_INPUT_BUFFER_LENGTH 256
//static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];

bool scpi_doIndicatorPulse = 0;
absolute_time_t scpi_doIndicatorPulseUntil;

//###########################################
// SCPI callbacks
//###########################################

//we could add a second spi later so lets call it spi0
pioSpi spi0;

scpi_result_t DMM_MeasureVoltageDcQ(scpi_t * context) {
    SCPI_ResultDouble(context, 3.14);
    return SCPI_RES_OK;
}

static scpi_result_t pi_echo(scpi_t* context) {
    char retVal[500] = "hm";
    size_t text_len = 3;
    
    if(!SCPI_ParamCopyText(context, retVal, sizeof(retVal), &text_len, true)) {
        
    }
    
    SCPI_ResultCharacters(context, retVal, text_len);
    
    return SCPI_RES_OK;
}

static scpi_result_t pi_bootsel(scpi_t* context) {
    reset_usb_boot(PIN_LED, 0);
    
    return SCPI_RES_OK;
}

//syntax is #<num of byte digits><num of bytes><data>
//e.g. #18abcdefgh
static scpi_result_t pi_spi_transfer(scpi_t* context) {  
    const char* data_rx;
    size_t len = 0;
    
    if(SCPI_ParamArbitraryBlock(context, &data_rx, &len, true)) {
        char* data_tx = (char*)malloc(len);
        if(data_tx == NULL) {
            SCPI_ErrorPush(context, SCPI_ERROR_EXECUTION_ERROR);
        } else {
            spi0.enable(true);
            spi0.rw8_blocking((uint8_t*)data_rx, (uint8_t*)data_tx, len);
            spi0.enable(false);
            SCPI_ResultArbitraryBlock(context, data_tx, len);
            //SCPI_ResultArrayUInt8(context, (const uint8_t*)data, len, SCPI_FORMAT_LITTLEENDIAN);
            free(data_tx);
        }       
    } else {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    }
    
    return SCPI_RES_OK;
}

static scpi_result_t pi_spi_sck(scpi_t* context) {
    uint32_t value = 255;
    
    SCPI_ParamUInt32(context, &value, true);
    
    if(value < 0 || value > 28) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        spi0.set_sck_pin(value);
    }
    
    return SCPI_RES_OK;
}

static scpi_result_t pi_spi_miso(scpi_t* context) {
    uint32_t value = 255;
    
    SCPI_ParamUInt32(context, &value, true);
    
    if(value < 0 || value > 28) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        spi0.set_miso_pin(value);
    }
    
    return SCPI_RES_OK;
}

static scpi_result_t pi_spi_mosi(scpi_t* context) {
    uint32_t value = 255;
    
    SCPI_ParamUInt32(context, &value, true);
    
    if(value < 0 || value > 28) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        spi0.set_mosi_pin(value);
    }
    
    return SCPI_RES_OK;
}

//###########################################
// SCPI callback definition
//###########################################

scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    {"*CLS", SCPI_CoreCls, 0},
    {"*ESE", SCPI_CoreEse, 0},
    {"*ESE?", SCPI_CoreEseQ, 0},
    {"*ESR?", SCPI_CoreEsrQ, 0},
    {"*IDN?", SCPI_CoreIdnQ, 0},
    {"*OPC", SCPI_CoreOpc, 0},
    {"*OPC?", SCPI_CoreOpcQ, 0},
    {"*RST", SCPI_CoreRst, 0},
    {"*SRE", SCPI_CoreSre, 0},
    {"*SRE?", SCPI_CoreSreQ, 0},
    {"*STB?", SCPI_CoreStbQ, 0},
    {"*TST?", SCPI_CoreTstQ, 0},
    {"*WAI", SCPI_CoreWai, 0},
    
    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    {"SYSTem:ERRor[:NEXT]?", SCPI_SystemErrorNextQ, 0},
    {"SYSTem:ERRor:COUNt?", SCPI_SystemErrorCountQ, 0},
    {"SYSTem:VERSion?", SCPI_SystemVersionQ, 0},

    //{"STATus:OPERation?", scpi_stub_callback, 0},
    //{"STATus:OPERation:EVENt?", scpi_stub_callback, 0},
    //{"STATus:OPERation:CONDition?", scpi_stub_callback, 0},
    //{"STATus:OPERation:ENABle", scpi_stub_callback, 0},
    //{"STATus:OPERation:ENABle?", scpi_stub_callback, 0},

    {"STATus:QUEStionable[:EVENt]?", SCPI_StatusQuestionableEventQ, 0},
    //{"STATus:QUEStionable:CONDition?", scpi_stub_callback, 0},
    {"STATus:QUEStionable:ENABle", SCPI_StatusQuestionableEnable, 0},
    {"STATus:QUEStionable:ENABle?", SCPI_StatusQuestionableEnableQ, 0},

    {"STATus:PRESet", SCPI_StatusPreset, 0},
    
    /* probeInterface */
    { .pattern = "ECHO", .callback = pi_echo,},
    { .pattern = "BOOTSEL", .callback = pi_bootsel,},
	{ .pattern = "MEASure:VOLTage:DC?", .callback = DMM_MeasureVoltageDcQ,},
    
    { .pattern = "SPI:TRANSfer?", .callback = pi_spi_transfer,},
    { .pattern = "SPI[:PIN]:SCK", .callback = pi_spi_sck,},
    { .pattern = "SPI[:PIN]:MISO", .callback = pi_spi_miso,},
    { .pattern = "SPI[:PIN]:MOSI", .callback = pi_spi_mosi,},
    
	SCPI_CMD_LIST_END
};

scpi_t scpi_context;

size_t scpi_write(scpi_t * context, const char * data, size_t len) {
    (void) context;
    usbtmc_app_response(data, len, false);
    return len;
}

scpi_result_t scpi_flush(scpi_t* context) {
    SCPI_RegSetBits(context, SCPI_REG_STB, STB_MAV);
    usbtmc_app_response(NULL, 0, true);
    return SCPI_RES_OK;
}

scpi_interface_t scpi_interface = {
    /*.error = */ NULL,
    /*.write = */ scpi_write,
    /*.control = */ NULL,
    /*.flush = */ scpi_flush,
    /*.reset = */ NULL,
};

//###########################################
// USB callbacks
//###########################################

void usbtmc_app_query_cb(char* data, size_t len)
{
    //use usb internal buffer and SCPI_Parse
    //use scpi library for ieee488.2
    SCPI_Parse(&scpi_context, data, len);
    return;
}

uint8_t usbtmc_app_get_stb_cb(void)
{
    uint8_t status = SCPI_RegGet(&scpi_context, SCPI_REG_STB);
    SCPI_RegClearBits(&scpi_context, SCPI_REG_STB, STB_SRQ);
    return status;
}

void usbtmc_app_clear_stb_cb(void)
{
    SCPI_RegClearBits(&scpi_context, SCPI_REG_STB, 0xff);
}

void usbtmc_app_clear_mav_cb(void)
{
    SCPI_RegClearBits(&scpi_context, SCPI_REG_STB, STB_MAV);
}

void usbtmc_app_set_srq_cb(void)
{
    scpi_context.registers[SCPI_REG_STB] |= STB_SRQ;
    //SCPI_RegSetBits(&scpi_context, SCPI_REG_STB, STB_SRQ);
}

void usbtmc_app_clear_srq_cb(void)
{
    scpi_context.registers[SCPI_REG_STB] &= ~STB_SRQ;
    //SCPI_RegClearBits(&scpi_context, SCPI_REG_STB, STB_SRQ);
}

//indicator light is required by IEEE 488.2
void usbtmc_app_indicator_cb(void)
{
    gpio_put(PIN_LED, 1);
    scpi_doIndicatorPulse = true;
    scpi_doIndicatorPulseUntil = make_timeout_time_ms(750);
}

//###########################################
// main application
//###########################################

int main() {
    stdio_init_all();
    
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, 1);
    //printf("RP2040 booting...\n");
    sleep_ms(1000);
    gpio_put(PIN_LED, 0);
        
    //pioSpi spi0; //we need this global
    spi0.set_sck_pin(PIN_SCK);
    spi0.set_miso_pin(PIN_MISO);
    spi0.set_mosi_pin(PIN_MOSI);
    spi0.set_baudrate(5);
    spi0.set_cpha(0);
    spi0.set_cpol(0);
    
    SCPI_Init(&scpi_context,
              scpi_commands,
              &scpi_interface,
              scpi_units_def,
              "LTE", "probeInterface", "G2", "dev",
              NULL, 0, //scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
              scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE
    );
    
    usbtmc_app_init();
    
    while (true) {
        if(scpi_doIndicatorPulse && get_absolute_time() > scpi_doIndicatorPulseUntil) {
            scpi_doIndicatorPulse = false;
            gpio_put(PIN_LED, 0);
        }
        
        usbtmc_app_task_iter();
    }
}
