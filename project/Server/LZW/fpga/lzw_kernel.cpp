#include <iostream>
#include <ctime>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include<ap_int.h>
#include<hls_stream.h>

// Creating ap_int datatype for 13 bit code
// typedef ap_uint<13> lzw_code;

#define MAX_NUM_OF_CODES 8192 // Dictionary size acc. to 13 bit code in LZW

#include "lzw.h"

unsigned int MurmurHash2(const unsigned char * key, int len/*, unsigned int seed*/) {

	// Naive implementation default seed
	unsigned int seed = 1;

	const unsigned int m = 0x5bd1e995;

	// Initialize the hash to a 'random' value

	unsigned int h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char * data = (const unsigned char *) key;

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

void lzw_kernel(unsigned char* input, int size, /*std::vector<int>,*/ uint64_t* output_code_packed, uint64_t output_code_size)
{

#pragma HLS INTERFACE m_axi port=input bundle=a
#pragma HLS INTERFACE m_axi port=cptr bundle=a
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
		hash_val = MurmurHash2(ch, 1/*Default single character length*/);
        // table[ch] = i;
		table[code] = hash_val;
    }

    // std::string p = "", c = "";
	uint8_t c = '';
	uint8_t p[MAX_NUM_OF_CODES];
	uint64_t p_idx = 0;

    p[p_idx++] = input[0];

    // std::vector<int> output_code;

	uint64_t output_code[MAX_NUM_OF_CODES];
	uint64_t op_idx = 0;

    // cout << "String\tOutput_Code\tAddition\n";
	
    for (uint32_t i = 0; i < length; i++) {
        if (i != length - 1)
            c = input[i + 1];
		
		// Search algorithm for the array
		p[p_idx++] = c;
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
        c = '';
    }

    // cout << p << "\t" << table[p] << endl;
    // output_code.push_back(table[p]);
	output_code[op_idx++] = get_val(table, MurmurHash2(p, p_idx));

	output_code_size = op_idx;

	// int output_code_size = output_code.size();
	// std::cout << "output_code size = " << output_code_size << "\n";

	/*
	bit_pack_timer.start();
	uint32_t curr_code = 0;
	uint32_t write_data = 0;
	uint32_t old_byte = 0;
	uint32_t rem_bits = 0;
	uint32_t running_bits = 0;
	uint32_t write_byte_size = 0;//number of bytes to write
	uint8_t write_byte = 0;//whats written in file
	uint32_t bytes_written = ceil(output_code_size*13.0/8.0);
	total_length_compressed += bytes_written;
	total_length_compressed += 4;
	uint32_t chunk_header = (bytes_written<<1);
	std::cout<<"\nLZW Header: "<<chunk_header<<"\n";
	memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	for(int idx=0; idx<output_code.size(); idx++)
	{
		curr_code = output_code[idx];
		write_data = curr_code<<(32 - (int)CODE_LENGTH - rem_bits);
		write_data |= old_byte;
		running_bits = rem_bits + (int)CODE_LENGTH;
		write_byte_size = running_bits/8;
		for (uint32_t i=1; i<=write_byte_size; i++)
		{
			write_byte = write_data>>(32-i*8);
			memcpy(&file[offset], &write_byte, sizeof(unsigned char));
			offset += sizeof(unsigned char);
		}
		old_byte = write_data<<(write_byte_size*8);
		rem_bits = running_bits - write_byte_size*8;
	}
	if(rem_bits){
		std::bitset<32> x(old_byte);
		std::cout<<"No of Remaining Bits:"<<rem_bits<<"\n";
		std::cout<<"Remaining Bits:"<<x<<"\n";
		write_byte = old_byte>>24;
		std::bitset<8> y(write_byte);
		std::cout<<"Remaining Byte:"<<y<<"\n";
		memcpy(&file[offset], &write_byte, sizeof(unsigned char));
		offset+= sizeof(unsigned char);
	}
	bit_pack_timer.stop();
    */

	// std::cout<<"RUNNING TOTAL BYTES(LZW): "<<total_length_compressed<<"\n";
	// std::cout<<"RUNNING TOTAL BYTES OF UNIQUE CHUNKS BEFORE COMPRESSION(LZW): "<<total_length_uncompressed<<"\n";
}