#include <stdlib.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "pio_spi.hpp"

#include "scpi/scpi.h"
#include "usbtmc_app.h"

//red led
#define PIN_LED1 28
//green led
#define PIN_LED2 27

#define PIN_REG_CLK 26
#define PIN_REG_DATA 24
#define PIN_REG_LATCH 25

#define PIN_SCK 1
#define PIN_MOSI 2
#define PIN_MISO 2 // same as MOSI, so we get loopback
#define PIN_CS 3

//waste index 0 to avoid -1 at every access
const uint8_t PIN_PP[] = {42, 21, 20, 19, 18, 13, 12, 11, 10, 5, 4, 3, 2};
const uint8_t PIN_PP_DIR[] = {42, 23, 22, 17, 16, 15, 14, 9, 8, 7, 6, 1, 0};
const uint8_t PIN_PP_SW[] = {42, 8, 9, 7, 10, 6, 11, 0, 5, 1, 4, 2, 3};

//value of register that is shifted out to the 74hc595s
static uint32_t shiftregValue = 0;

//we could add a second spi later so lets call it spi0
pioSpi spi0;


// --- shift register functions (dac and relays) ---

void shiftreg_write(uint32_t shiftData) {
    for (int i = 0; i < 24; i++) {      //loop that runs for 24 bits
        gpio_put(PIN_REG_CLK, 0);
        gpio_put(PIN_REG_DATA, shiftData & 0x1);
        shiftData >>= 1;
        sleep_us(5);
        gpio_put(PIN_REG_CLK, 1);
        sleep_us(5);
    }
    gpio_put(PIN_REG_CLK, 0);

    gpio_put(PIN_REG_LATCH, 1);
    sleep_us(5);
    gpio_put(PIN_REG_LATCH, 0);
}

void dac_set(float dacVoltage) {
    uint16_t dacValue = dacVoltage * 4095.0f / 3.3f;
    if(dacValue & 0xf000)
        dacValue = 0x0fff;
    shiftregValue = (dacValue << 12) | (shiftregValue & 0xfff);
    shiftreg_write(shiftregValue);
}

void relay_set(uint8_t ppPin, bool close) {
    shiftregValue = shiftregValue & ~(1 << PIN_PP_SW[ppPin]);
    if(close)
        shiftregValue = shiftregValue | (1 << PIN_PP_SW[ppPin]);
    shiftreg_write(shiftregValue);
    sleep_us(999);
}

// --- pp gpio functions ---

void pp_gpio_dirShifter(uint8_t ppPin, bool out) {
    gpio_init(PIN_PP_DIR[ppPin]);
    gpio_set_dir(PIN_PP_DIR[ppPin], GPIO_OUT);
    gpio_put(PIN_PP_DIR[ppPin], out);
}

void pp_gpio_dirOut(uint8_t ppPin) {
    gpio_init(PIN_PP[ppPin]);
    pp_gpio_dirShifter(ppPin, true);
    gpio_set_dir(PIN_PP[ppPin], GPIO_OUT);
    gpio_put(PIN_PP[ppPin], false);
}

void pp_gpio_dirIn(uint8_t ppPin) {
    gpio_init(PIN_PP[ppPin]);
    gpio_set_dir(PIN_PP[ppPin], GPIO_IN);
    pp_gpio_dirShifter(PIN_PP[ppPin], false);
}

void pp_gpio_write(uint8_t ppPin, bool high) {
    gpio_put(PIN_PP[ppPin], high);
}

bool pp_gpio_read(uint8_t ppPin) {
    return gpio_get(PIN_PP[ppPin]);
}

void pp_gpio_reset(void) {
    spi0.set_sck_pin(0);
    spi0.set_miso_pin(0);
    spi0.set_mosi_pin(0);
    spi0.set_cs_pin(0);
    spi0.set_baudrate(1);
    spi0.set_cpha(0);
    spi0.set_cpol(0);

    for(uint8_t i = 0; i <= 12; i++) {
        pp_gpio_dirIn(i);
        relay_set(i, 0);
    }
}

//###########################################
// SCPI
//###########################################

#define SCPI_ERROR_QUEUE_SIZE 17
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

//#define SCPI_INPUT_BUFFER_LENGTH 256
//static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];

bool scpi_doIndicatorPulse = 0;
absolute_time_t scpi_doIndicatorPulseUntil;

//###########################################
// SCPI callbacks
//###########################################

static scpi_result_t pi_echo(scpi_t* context) {
    char retVal[500] = "hm";
    size_t text_len = 3;
    
    if(!SCPI_ParamCopyText(context, retVal, sizeof(retVal), &text_len, true)) {
        
    }
    
    SCPI_ResultCharacters(context, retVal, text_len);
    
    return SCPI_RES_OK;
}

static scpi_result_t pi_bootsel(scpi_t* context) {
    reset_usb_boot(PIN_LED1, 0);
    
    return SCPI_RES_OK;
}

// --- SPI callbacks ---

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

static scpi_result_t pi_spi_set_cpha(scpi_t* context) {
    bool value = false;
    
    if(!SCPI_ParamBool(context, &value, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        spi0.set_cpha(value);
    }
    
    return SCPI_RES_OK;
}

static scpi_result_t pi_spi_set_cpol(scpi_t* context) {
    bool value = false;
    
    if(!SCPI_ParamBool(context, &value, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        spi0.set_cpol(value);
    }
    
    return SCPI_RES_OK;
}

static scpi_result_t pi_spi_set_baud(scpi_t* context) {
    float value = 1.0f;
    
    if(!SCPI_ParamFloat(context, &value, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        if(!spi0.set_baudrate(value)) {
            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        }
    }
    
    return SCPI_RES_OK;
}

static scpi_result_t pi_io_level(scpi_t* context) {
    float value = 1.0f;

    if(!SCPI_ParamFloat(context, &value, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        if(value < 0.0f || value > 3.3f) {
            SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
        } else {
            dac_set(value);
        }
    }

    return SCPI_RES_OK;
}

static scpi_result_t pi_spi_get_baud(scpi_t* context) {
    SCPI_ResultFloat(context, spi0.get_baudrate());
    return SCPI_RES_OK;
}

scpi_choice_def_t pi_io_funcdef[] = {
    {"IN", 1},
    {"OUT", 2},
    {"CS", 3},
    {"SCK", 4},
    {"MISO", 5},
    {"MOSI", 6},
    SCPI_CHOICE_LIST_END
};

static scpi_result_t pi_io_func(scpi_t* context) {
    int32_t ppPin = 0;
    SCPI_CommandNumbers(context, &ppPin, 1, 0);

    int32_t choice = 0;
    if(ppPin < 1 || ppPin > 12 || !SCPI_ParamChoice(context, pi_io_funcdef, &choice, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        switch(choice) {
            case 1:
                pp_gpio_dirIn(ppPin);
                break;
            case 2:
                pp_gpio_dirOut(ppPin);
                break;
            case 3:
                pp_gpio_dirShifter(ppPin, true);
                spi0.set_cs_pin(PIN_PP[ppPin]);
                break;
            case 4:
                pp_gpio_dirShifter(ppPin, true);
                spi0.set_sck_pin(PIN_PP[ppPin]);
                break;
            case 5:
                pp_gpio_dirShifter(ppPin, false);
                spi0.set_miso_pin(PIN_PP[ppPin]);
                break;
            case 6:
                pp_gpio_dirShifter(ppPin, true);
                spi0.set_mosi_pin(PIN_PP[ppPin]);
                break;
        }
    }

    return SCPI_RES_OK;
}

static scpi_result_t pi_io_enable(scpi_t* context) {
    int32_t ppPin = 0;
    SCPI_CommandNumbers(context, &ppPin, 1, 0);

    bool on = false;
    if(ppPin < 1 || ppPin > 12 || !SCPI_ParamBool(context, &on, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        relay_set(ppPin, on);
    }

    return SCPI_RES_OK;
}

static scpi_result_t pi_io_write(scpi_t* context) {
    int32_t ppPin = 0;
    SCPI_CommandNumbers(context, &ppPin, 1, 0);

    bool on = false;
    if(ppPin < 1 || ppPin > 12 || !SCPI_ParamBool(context, &on, true)) {
        SCPI_ErrorPush(context, SCPI_ERROR_ILLEGAL_PARAMETER_VALUE);
    } else {
        pp_gpio_write(ppPin, on);
    }

    return SCPI_RES_OK;
}

static scpi_result_t pi_io_read(scpi_t* context) {
    return SCPI_RES_OK;
}

scpi_result_t pi_reset(scpi_t * context) {
    pp_gpio_reset();
    return SCPI_CoreRst(context);
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
    //{"*RST", SCPI_CoreRst, 0},
    {"*RST", .callback = pi_reset,},
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

    { .pattern = "IO:LEVel", .callback = pi_io_level,},

    { .pattern = "IO#:FUnc", .callback = pi_io_func,},
    { .pattern = "IO#:ENable", .callback = pi_io_enable,},
    { .pattern = "IO#:WRite", .callback = pi_io_write,},
    { .pattern = "IO#:REad", .callback = pi_io_read,},

    { .pattern = "SPI:TRANSfer?", .callback = pi_spi_transfer,},
    { .pattern = "SPI:CPHA", .callback = pi_spi_set_cpha,},
    { .pattern = "SPI:CPOL", .callback = pi_spi_set_cpol,},
    { .pattern = "SPI:BAUDrate", .callback = pi_spi_set_baud,},
    { .pattern = "SPI:BAUDrate?", .callback = pi_spi_get_baud,},
    
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
    gpio_put(PIN_LED2, 1);
    scpi_doIndicatorPulse = true;
    scpi_doIndicatorPulseUntil = make_timeout_time_ms(750);
}

//###########################################
// main application
//###########################################

int main() {
    stdio_init_all();
    
    //init pins for LEDs
    gpio_init(PIN_LED1);
    gpio_init(PIN_LED2);
    gpio_set_dir(PIN_LED1, GPIO_OUT);
    gpio_set_dir(PIN_LED2, GPIO_OUT);
    gpio_put(PIN_LED1, 1);
    //printf("RP2040 booting...\n");
    sleep_ms(1000);
    gpio_put(PIN_LED1, 0);

    //init pins for shift register
    gpio_init(PIN_REG_CLK);
    gpio_init(PIN_REG_DATA);
    gpio_init(PIN_REG_LATCH);
    gpio_set_dir(PIN_REG_CLK, GPIO_OUT);
    gpio_set_dir(PIN_REG_DATA, GPIO_OUT);
    gpio_set_dir(PIN_REG_LATCH, GPIO_OUT);
        
    //pioSpi spi0; //we need this global

    pp_gpio_reset();
    
    SCPI_Init(&scpi_context,
              scpi_commands,
              &scpi_interface,
              scpi_units_def,
              "LTE", "probeInterface", "G2", "dev",
              NULL, 0, //scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
              scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE
    );
    
    usbtmc_app_init();

    //test relays - turn on one and another
    //uint8_t i = 1;
    //while(i <= 12) {
    //    relay_set(i, 1);
    //    i += 1;
    //    sleep_ms(5000);
    //}

    //dac test - generate triangle
    //float dacVolt = 0;
    //while(true){
    //    dacVolt = dacVolt + 0.01f;
    //    if(dacVolt > 1.65)
    //        gpio_put(PIN_LED1, 0);
    //    if(dacVolt > 3.3) {
    //        dacVolt = 0.0f;
    //        gpio_put(PIN_LED1, 1);
    //    }
    //    dac_set(dacVolt);
    //    sleep_ms(10);
    //}

    while(true) {
        if(scpi_doIndicatorPulse && get_absolute_time() > scpi_doIndicatorPulseUntil) {
            scpi_doIndicatorPulse = false;
            gpio_put(PIN_LED2, 0);
        }
        
        usbtmc_app_task_iter();
    }
}
