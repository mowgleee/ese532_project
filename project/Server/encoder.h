#ifndef _ENCODER_H_
#define _ENCODER_H_


#include "Common/common.h"

#include "CDC/cdc.h"
#include "SHA/sha.h"
#include "Dedup/dedup.h"
#include "LZW/cpu/lzw.h"
#include "LZW/fpga/lzw_host.h"

int offset = 0;
unsigned char* file = 0;
stopwatch bit_pack_timer;

void handle_input(int argc, char* argv[], int* blocksize);
void compress(unsigned char *buffer, packet* pptr);


#endif
