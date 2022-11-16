#ifndef _LZWKERNEL_H_
#define _LZWKERNEL_H_
#include <iostream>
#include <ctime>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <math.h>

// #include <bits/stdc++.h>
// #include<ap_int.h>
// #include<hls_stream.h>

// Creating ap_int datatype for 13 bit code
// typedef ap_uint<13> lzw_code;
// typedef ap_uint<104> long_buffer;

#define MAX_NUM_OF_CODES 8192 // Dictionary size acc. to 13 bit code in LZW

#define CODE_LENGTH 13

unsigned int MurmurHash2(const unsigned char * key, int len/*, unsigned int seed*/);
int64_t search(uint64_t* table, uint64_t length, uint64_t hash_val);
void lzw_kernel(unsigned char* input, int size, uint8_t* output_code_packed, uint64_t* output_code_size);


#endif