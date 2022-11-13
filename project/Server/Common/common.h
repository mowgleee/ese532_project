#ifndef _COMMON_H_
#define _COMMON_H_


// max number of elements we can get from ethernet
#define NUM_ELEMENTS 16384
#define HEADER 2

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
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

#include <arm_neon.h>
#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/sha3.h>

#include "../stopwatch.h"
#include "../server.h"


#define NUM_PACKETS 8
#define pipe_depth 4
#define DONE_BIT_L (1 << 7)
#define DONE_BIT_H (1 << 15)

#define WIN_SIZE 16
#define PRIME 3
#define MODULUS 256
#define TARGET 0


#define MAX_CHUNK_SIZE 8*1024
#define CODE_LENGTH 13//log2(MAX_CHUNK_SIZE)

extern int offset;
extern unsigned char* file;

// Structure to store all information about a chunk
typedef struct chunk
{
	uint32_t lower_bound = 0;
	uint32_t upper_bound = 0;
	uint32_t size = 0;
	std::string sha;
	bool is_unique;
	int num;
}chunk;

#endif