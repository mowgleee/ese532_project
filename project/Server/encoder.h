#ifndef _ENCODER_H_
#define _ENCODER_H_


#include "Common/common.h"

#include "CDC/cdc.h"
#include "SHA/sha.h"
#include "Dedup/dedup.h"
#include "LZW/lzw.h"

int offset = 0;
unsigned char* file = 0;

void handle_input(int argc, char* argv[], int* blocksize);
void compress(unsigned char *buffer, uint32_t length);


#endif
