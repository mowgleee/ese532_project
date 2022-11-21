// #include "../Server/encoder.h"
#ifndef _CDC_H_
#define _CDC_H_

#include "../Common/common.h"

uint64_t hash_func(unsigned char *input, uint32_t pos);
void cdc_eff(unsigned char *buff, packet* pptr);

#endif