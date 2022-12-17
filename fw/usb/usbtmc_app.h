#ifndef USBTMC_APP_H
#define USBTMC_APP_H

#ifdef	__cplusplus
extern "C" {
#endif

void usbtmc_app_init(void);
void usbtmc_app_task_iter(void);

//callback for running the SCPI command
//ioData is the pointer to the usbtmc buffer and bufferLen its length
//ioDataLen is the length of received data
//as return the length of the new data is expected, the data is to be stored in ioData
void usbtmc_app_query_cb(char* data, size_t len);

void usbtmc_app_response(const void* data, size_t len, bool endOfMessage);

uint8_t usbtmc_app_get_stb_cb(void);
void usbtmc_app_clear_stb_cb(void);
void usbtmc_app_clear_mav_cb(void);
void usbtmc_app_set_srq_cb(void);
void usbtmc_app_clear_srq_cb(void);

void usbtmc_app_indicator_cb(void);

#ifdef	__cplusplus
}
#endif

#endif
