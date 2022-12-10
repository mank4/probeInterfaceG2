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
size_t usbtmc_app_cmd_cb(uint8_t* ioData, size_t ioDataLen, size_t bufferLen);

#ifdef	__cplusplus
}
#endif

#endif
