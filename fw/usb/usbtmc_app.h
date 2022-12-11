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

#ifdef	__cplusplus
}
#endif

#endif
