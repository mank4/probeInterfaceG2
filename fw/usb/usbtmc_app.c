/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Nathan Conrad
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <strings.h>
#include <stdlib.h>     /* atoi */
#include "tusb.h"
#include "bsp/board.h"
#include "usbtmc_app.h"

void usbtmc_app_init(void) {
    tud_init(BOARD_TUD_RHPORT);
    return;
}

#if (CFG_TUD_USBTMC_ENABLE_488)
static usbtmc_response_capabilities_488_t const
#else
static usbtmc_response_capabilities_t const
#endif
tud_usbtmc_app_capabilities  =
{
    .USBTMC_status = USBTMC_STATUS_SUCCESS,
    .bcdUSBTMC = USBTMC_VERSION,
    .bmIntfcCapabilities =
    {
        .listenOnly = 0,
        .talkOnly = 0,
        .supportsIndicatorPulse = 1
    },
    .bmDevCapabilities = {
        .canEndBulkInOnTermChar = 0
    },

#if (CFG_TUD_USBTMC_ENABLE_488)
    .bcdUSB488 = USBTMC_488_VERSION,
    .bmIntfcCapabilities488 =
    {
        .supportsTrigger = 1,
        .supportsREN_GTL_LLO = 0,
        .is488_2 = 1
    },
    .bmDevCapabilities488 =
    {
      .SCPI = 1,
      .SR1 = 0,
      .RL1 = 0,
      .DT1 =0,
    }
#endif
};

//#define IEEE4882_STB_QUESTIONABLE (0x08u)
//#define IEEE4882_STB_MAV          (0x10u)
//#define IEEE4882_STB_SER          (0x20u)
//#define IEEE4882_STB_SRQ          (0x40u)

//static const char idn[] = "TinyUSB,ModelNumber,SerialNumber,FirmwareVer123456\r\n";
//static const char idn[] = "TinyUSB,ModelNumber,SerialNumber,FirmwareVer and a bunch of other text to make it longer than a packet, perhaps? lets make it three transfers...\n";
//static volatile uint8_t status;

static volatile uint16_t queryState = 0;
//static volatile uint32_t bulkInStarted;

static size_t buffer_len = 0;
static size_t buffer_tx = 0; // for transmitting using multiple transfers
static uint8_t buffer[8300]; // A few packets long should be enough.


static usbtmc_msg_dev_dep_msg_in_header_t rspMsg = {
    .bmTransferAttributes =
    {
      .EOM = 1,
      .UsingTermChar = 0
    }
};

void tud_usbtmc_open_cb(uint8_t interface_id)
{
  (void)interface_id;
  tud_usbtmc_start_bus_read();
}

#if (CFG_TUD_USBTMC_ENABLE_488)
usbtmc_response_capabilities_488_t const *
#else
usbtmc_response_capabilities_t const *
#endif
tud_usbtmc_get_capabilities_cb()
{
  return &tud_usbtmc_app_capabilities;
}


bool tud_usbtmc_msg_trigger_cb(usbtmc_msg_generic_t* msg) {
  (void)msg;
  // Let trigger set the SRQ
  usbtmc_app_set_srq_cb(); //status |= IEEE4882_STB_SRQ;
  return true;
}

//pc starts write to pico
bool tud_usbtmc_msgBulkOut_start_cb(usbtmc_msg_request_dev_dep_out const * msgHeader)
{
  (void)msgHeader;
  buffer_len = 0;
  if(msgHeader->TransferSize > sizeof(buffer))
  {

    return false;
  }
  return true;
}

//pc sends data packages to pico
bool tud_usbtmc_msg_data_cb(void *data, size_t len, bool transfer_complete)
{
  // If transfer isn't finished, we just ignore it (for now)

  if(len + buffer_len < sizeof(buffer))
  {
    memcpy(&(buffer[buffer_len]), data, len);
    buffer_len += len;
  }
  else
  {
    return false; // buffer overflow!
  }
  if(transfer_complete) {
    queryState = 1;
  }
    
  tud_usbtmc_start_bus_read();
  return true;
}

//send data to computer
bool tud_usbtmc_msgBulkIn_complete_cb()
{
  if(buffer_tx == buffer_len) {
    usbtmc_app_clear_mav_cb(); //status &= (uint8_t)~(IEEE4882_STB_MAV); // clear MAV
    queryState = 0;
  }
    
  tud_usbtmc_start_bus_read();

  return true;
}

//pc wants to read data
bool tud_usbtmc_msgBulkIn_request_cb(usbtmc_msg_request_dev_dep_in const * request)
{
  rspMsg.header.MsgID = request->header.MsgID,
  rspMsg.header.bTag = request->header.bTag,
  rspMsg.header.bTagInverse = request->header.bTagInverse;
  unsigned int msgReqLen = request->TransferSize; //maximum length allowed by computer

#ifdef xDEBUG
  uart_tx_str_sync("MSG_IN_DATA: Requested!\r\n");
#endif
  
  if(queryState == 0)
  {
    // > If a USBTMC interface receives a Bulk-IN request prior to receiving a USBTMC command message
    //   that expects a response, the device must NAK the request (*not stall*)
  }
  else if(queryState == 3)
  {
    size_t tx_len = tu_min32(buffer_len-buffer_tx, msgReqLen); //length of data to transmit, limited by msgReqLen
    tud_usbtmc_transmit_dev_msg_data(&buffer[buffer_tx], tx_len, buffer_tx+tx_len == buffer_len, false);
    buffer_tx+=tx_len;
  }
  // Always return true indicating not to stall the EP.
  return true;
}

void usbtmc_app_task_iter(void) {
  tud_task();
    
  if(queryState == 1)
  {
      queryState = 2;
      size_t rx_buffer_len = buffer_len;     
    
      buffer_len = 0;
      usbtmc_app_query_cb(buffer, rx_buffer_len);
  }
}

void usbtmc_app_response(const void* data, size_t len, bool endOfMessage)
{
  if(!endOfMessage) {
    size_t writeLen = tu_min32(len, sizeof(buffer)-buffer_len);
    memcpy(&buffer[buffer_len], data, writeLen);
    buffer_len += writeLen;
  } else {
    queryState = 3;
    buffer_tx = 0;
  }
}

bool tud_usbtmc_initiate_clear_cb(uint8_t *tmcResult)
{
  *tmcResult = USBTMC_STATUS_SUCCESS;
  queryState = 0;
  usbtmc_app_clear_stb_cb(); //status = 0;
  return true;
}

bool tud_usbtmc_check_clear_cb(usbtmc_get_clear_status_rsp_t *rsp)
{
  queryState = 0;
  usbtmc_app_clear_stb_cb(); //status = 0;
  buffer_len = 0u;
  rsp->USBTMC_status = USBTMC_STATUS_SUCCESS;
  rsp->bmClear.BulkInFifoBytes = 0u;
  return true;
}
bool tud_usbtmc_initiate_abort_bulk_in_cb(uint8_t *tmcResult)
{
  *tmcResult = USBTMC_STATUS_SUCCESS;
  return true;
}
bool tud_usbtmc_check_abort_bulk_in_cb(usbtmc_check_abort_bulk_rsp_t *rsp)
{
  (void)rsp;
  tud_usbtmc_start_bus_read();
  return true;
}

bool tud_usbtmc_initiate_abort_bulk_out_cb(uint8_t *tmcResult)
{
  *tmcResult = USBTMC_STATUS_SUCCESS;
  return true;

}
bool tud_usbtmc_check_abort_bulk_out_cb(usbtmc_check_abort_bulk_rsp_t *rsp)
{
  (void)rsp;
  tud_usbtmc_start_bus_read();
  return true;
}

void tud_usbtmc_bulkIn_clearFeature_cb(void)
{
}

void tud_usbtmc_bulkOut_clearFeature_cb(void)
{
  tud_usbtmc_start_bus_read();
}

// Return status byte, but put the transfer result status code in the rspResult argument.
uint8_t tud_usbtmc_get_stb_cb(uint8_t *tmcResult)
{
  //clear srq in usbtmc_app_get_status_cb after reading
  uint8_t status = usbtmc_app_get_stb_cb();
  usbtmc_app_clear_srq_cb();

  *tmcResult = USBTMC_STATUS_SUCCESS;
  // Increment status so that we see different results on each read...

  return status;
}

bool tud_usbtmc_indicator_pulse_cb(tusb_control_request_t const * msg, uint8_t *tmcResult)
{
  (void)msg;
  usbtmc_app_indicator_cb();
  *tmcResult = USBTMC_STATUS_SUCCESS;
  return true;
}

