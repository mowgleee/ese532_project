#ifndef _LZWKERNEL_H_
#define _LZWKERNEL_H_
#include <iostream>
#include <ctime>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>
#include "hls_stream.h"


// #include <bits/stdc++.h>
#include<ap_int.h>
// #include<hls_stream.h>

// Creating ap_int datatype for 13 bit code
// typedef ap_uint<13> lzw_code;
// typedef ap_uint<104> long_buffer;

#define MAX_NUM_OF_CODES 8192 // Dictionary size acc. to 13 bit code in LZW

#define CODE_LENGTH 13

#define HASHMAP_CAPACITY 8192
#define BUCKET_SIZE	3

typedef struct hashmap_entry {
  uint32_t key;
  int32_t code;
} hashmap_entry_t;

uint64_t MurmurHash64( const unsigned char * data, int len);
int32_t search(uint64_t* table, uint32_t length, uint64_t hash_val);
// void lzw_kernel(unsigned char* input, uint32_t size, /*std::vector<int>,*/ uint8_t* output_code_packed, uint32_t* output_code_size);
uint8_t associative_put(ap_uint<72> keys[][4], uint32_t* value, uint8_t counter, uint32_t hash, uint32_t code);
uint8_t log_2(ap_uint<72> bit_to_get);
int32_t associative_get(ap_uint<72> keys[][4], uint32_t* value, uint32_t hash);

void lzw_kernel(unsigned char* input_packet,
				uint32_t* chunk_bndry,
				uint32_t num_chunks,
				uint8_t* is_chunk_unique,
				uint8_t* output_file,
				uint32_t* output_size,
				uint32_t* dup_chunk_head);


#endif
