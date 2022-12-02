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
stopwatch lzw_sem_timer;
stopwatch cdc_sem_timer;
stopwatch dedup_sem_timer;
stopwatch sha_sem_timer;
unsigned char* input[NUM_PACKETS] = {0};
uint32_t total_packets = INT_MAX;

stopwatch kernel_init_timer;
stopwatch kernel_timer;
stopwatch kernel_mem_timer;

void handle_input(int argc, char* argv[], int* blocksize);
// void compress(unsigned char *buffer, packet* pptr);
void compress(unsigned char *buffer, packet* pptr, lzw_request *kernel_cl_obj, semaphores* sem);


#endif
