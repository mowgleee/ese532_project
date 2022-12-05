#ifndef _COMMON_H_
#define _COMMON_H_


// max number of elements we can get from ethernet
#define NUM_ELEMENTS 16384
#define HEADER 2

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <bits/stdc++.h>
#include <bitset>
#include <math.h>

#include "../stopwatch.h"
#include "../server.h"

#include <thread>
#include <semaphore.h>

#define NUM_PACKETS 50
#define pipe_depth 4
#define DONE_BIT_L (1 << 7)
#define DONE_BIT_H (1 << 15)

#define WIN_SIZE 16
#define MIN_CHUNK_SIZE 16
#define PRIME 3
#define MODULUS 256
#define TARGET 0


#define MAX_CHUNK_SIZE 8*1024
#define MAX_NUM_CHUNKS (BLOCKSIZE/MIN_CHUNK_SIZE)
#define CODE_LENGTH 13//log2(MAX_CHUNK_SIZE)


extern int offset;
extern unsigned char* file;
extern stopwatch bit_pack_timer;
extern stopwatch lzw_sem_timer;
extern stopwatch cdc_sem_timer;
extern stopwatch dedup_sem_timer;
extern stopwatch sha_sem_timer;

extern unsigned char* input[NUM_PACKETS];
extern  uint32_t total_packets;
// stopwatch kernel_init_timer;
// stopwatch kernel_timer;
// stopwatch kernel_mem_timer;
// extern stopwatch kernel_timer;

#define VERB_LOW 0
#define VERB_MED 1
#define VERB_HIGH 2
#define VERB_DEBUG VERB_HIGH

inline void makelog(uint8_t verb, const char *fmt, ...)
{
	va_list args;

	if(verb==VERB_HIGH){
    	va_start(args, fmt);
    	vprintf(fmt, args);
    	va_end(args);

	}
   
}

// Structure to store all information about a chunk
typedef struct chunk
{
	uint32_t lower_bound = 0;
	uint32_t upper_bound = 0;
	uint32_t size = 0;
	std::string sha;
	bool is_unique;
	uint32_t num;
	uint32_t header = 0;
}chunk;

typedef struct packet
{
	uint32_t num = 0;
	uint32_t size = 0;
	uint32_t num_of_chunks = 0;
	chunk curr_chunk[MAX_NUM_CHUNKS];
}packet;

typedef struct semaphores
{
	sem_t sem_getpacket;
	sem_t sem_cdc;
	sem_t sem_sha;
	sem_t sem_dedup;
	sem_t sem_lzw;
}semaphores;

#endif