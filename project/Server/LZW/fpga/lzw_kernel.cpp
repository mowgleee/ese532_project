// #include <iostream>
// #include <ctime>
// #include <utility>
// #include <cstdio>
// #include <cstdlib>
// #include <cstring>

// #include<ap_int.h>
// #include<hls_stream.h>

// // Creating ap_int datatype for 13 bit code
// // typedef ap_uint<13> lzw_code;

// #define MAX_NUM_OF_CODES 8192 // Dictionary size acc. to 13 bit code in LZW

#include "lzw_kernel.h"

unsigned int MurmurHash2(const unsigned char * key, int len/*, unsigned int seed*/) {

	// Naive implementation default seed
	unsigned int seed = 1;

	const unsigned int m = 0x5bd1e995;

	// Initialize the hash to a 'random' value

	unsigned int h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char * data = (const unsigned char *) key;
	
	 while (len >= 4)
	 {
	 	size_t k = data[len-1];
	 	k *= m;
	 	k ^= k >> 24;
	 	k *= m;
	 	h *= m;
	 	h ^= k;
	 	data += 4;
	 	len -= 4;
	 }

	switch (len) {
	case 3:
		h ^= data[2] << 16;
	case 2:
		h ^= data[1] << 8;
	case 1:
		h ^= data[0];
		h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

int64_t search(uint64_t* table, uint64_t length, uint64_t hash_val)
{
	for(uint64_t i = 0; i < length; i++)
	{
		if(table[i] == hash_val)
			return i;
	}
	return -1;
}

void lzw_kernel(unsigned char* input, int size, uint64_t* output_code, uint64_t* output_code_size)
{

#pragma HLS INTERFACE m_axi port=input bundle=a
#pragma HLS INTERFACE m_axi port=size bundle=a
#pragma HLS INTERFACE m_axi port=output_code_packed bundle=a
#pragma HLS INTERFACE m_axi port=output_code_size bundle=a

// #pragma HLS INTERFACE s_axilite port=output_code bundle=a

//     lzw_encoding(input, cptr);
// }

// void lzw_encoding(unsigned char* input, chunk* cptr)
// {

	uint32_t length = size;

	/*
	static uint32_t total_length_compressed = 0;
	static uint32_t total_length_uncompressed = 0;

	total_length_uncompressed += length;
	
	std::cout<<"length of chunk: "<<length<<"\n";
	*/

    // std::cout << "Encoding\n";

	// Convert to memory map
	// std::unordered_map<std::string, int> table;
    
	// lzw_code
	uint64_t table[MAX_NUM_OF_CODES]; // index i is the value and value stored is the hash of that substring
// #pragma HLS ARRAY_PARTITION variable = table block factor = 1024 //idk

	// lzw_code resetValue = 0;
	uint64_t resetValue = 0;

	// Use while loop and reset for multiple chunks
	// Reset dictionary
	for (int i = 0; i < MAX_NUM_OF_CODES; i++) {
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
		table[i] = resetValue;
	}

	uint64_t hash_val;
	uint8_t ch = 0;
	uint64_t code = 0;

    for (code = 0; code <= 255; code++) {
#pragma HLS PIPELINE II = 1
#pragma HLS UNROLL FACTOR = 2
        ch = char(code);
        // ch += char(i);
		hash_val = MurmurHash2(&ch, 1/*Default single character length*/);
        // table[ch] = i;
		table[code] = hash_val;
    }

    // std::string p = "", c = "";
	uint8_t c;
	uint8_t p[MAX_NUM_OF_CODES] = {0};
	uint64_t p_idx = 0;

    p[p_idx++] = input[0];

    // std::vector<int> output_code;

	// uint64_t output_code[MAX_NUM_OF_CODES];
	uint64_t op_idx = 0;
	
    for (uint32_t i = 0; i < length; i++) {
        if (i != length - 1)
		{
            c = input[i + 1];
            p[p_idx++] = c;
		}

		// Search algorithm for the array

		hash_val = MurmurHash2(p, p_idx);
		int64_t match = search(table, code, hash_val);

        if (match < 0/*table.find(p + c) != table.end()*/) {			// Change the find function
            // p = p + c;
			//p_idx--;//c is here
            // cout << p << "\t" << table[p] << "\t\t"
            //      << p + c << "\t" << code << endl;
            // output_code.push_back(table[p]);
			output_code[op_idx++] = search(table, code, MurmurHash2(p, p_idx - 1));

            // table[p + c] = code;
			table[code] = hash_val;
			
            code++;
            // p = c;
			p_idx = 0;
			p[p_idx++] = c;
        }
//        c = '\0';
    }

    // cout << p << "\t" << table[p] << endl;
    // output_code.push_back(table[p]);
	output_code[op_idx++] = search(table, code, MurmurHash2(p, p_idx-1));

	// output_code_packed = output_code;

	*output_code_size = op_idx;
}
