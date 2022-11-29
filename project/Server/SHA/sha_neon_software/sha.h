// Including common requirements here as commpiled separately
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <math.h>

#include <arm_neon.h>
#include <semaphore.h>
// #include <wolfssl/options.h>
// #include <wolfssl/wolfcrypt/sha3.h>

#define BLOCKSIZE 8192
#define WIN_SIZE 16
#define MIN_CHUNK_SIZE 16

// Structure to store all information about a chunk
typedef struct chunk
{
	uint32_t lower_bound = 0;
	uint32_t upper_bound = 0;
	uint32_t size = 0;
	std::string sha;
	bool is_unique;
	int num;
    uint32_t header = 0;
}chunk;

typedef struct packet
{
	uint32_t num = 0;
	uint32_t size = 0;
	uint32_t num_of_chunks = 0;
	chunk curr_chunk[BLOCKSIZE/MIN_CHUNK_SIZE];
}packet;

typedef struct semaphores
{
	sem_t sem_cdc;
	sem_t sem_sha;
	sem_t sem_dedup;
	sem_t sem_lzw;
}semaphores;

static const uint32_t K[] =
{
    0x428A2F98, 0x71374491, 0xB5C0FBCF, 0xE9B5DBA5,
    0x3956C25B, 0x59F111F1, 0x923F82A4, 0xAB1C5ED5,
    0xD807AA98, 0x12835B01, 0x243185BE, 0x550C7DC3,
    0x72BE5D74, 0x80DEB1FE, 0x9BDC06A7, 0xC19BF174,
    0xE49B69C1, 0xEFBE4786, 0x0FC19DC6, 0x240CA1CC,
    0x2DE92C6F, 0x4A7484AA, 0x5CB0A9DC, 0x76F988DA,
    0x983E5152, 0xA831C66D, 0xB00327C8, 0xBF597FC7,
    0xC6E00BF3, 0xD5A79147, 0x06CA6351, 0x14292967,
    0x27B70A85, 0x2E1B2138, 0x4D2C6DFC, 0x53380D13,
    0x650A7354, 0x766A0ABB, 0x81C2C92E, 0x92722C85,
    0xA2BFE8A1, 0xA81A664B, 0xC24B8B70, 0xC76C51A3,
    0xD192E819, 0xD6990624, 0xF40E3585, 0x106AA070,
    0x19A4C116, 0x1E376C08, 0x2748774C, 0x34B0BCB5,
    0x391C0CB3, 0x4ED8AA4A, 0x5B9CCA4F, 0x682E6FF3,
    0x748F82EE, 0x78A5636F, 0x84C87814, 0x8CC70208,
    0x90BEFFFA, 0xA4506CEB, 0xBEF9A3F7, 0xC67178F2,
};

void sha(unsigned char* buff, packet *pptr, semaphores* sems);
void sha256_process_arm(uint32_t state[8], const uint8_t data[], uint32_t length);