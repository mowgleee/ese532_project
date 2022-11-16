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

void lzw_kernel(unsigned char* input, int size, uint8_t* output_code_packed, uint64_t* output_code_size)
{
	uint32_t length = size;
	uint32_t output_code[8192];

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
			output_code[op_idx++] = search(table, code, MurmurHash2(p, p_idx - 1));
			table[code] = hash_val;
            code++;
			p_idx = 0;
			p[p_idx++] = c;
        }
    }
	output_code[op_idx++] = search(table, code, MurmurHash2(p, p_idx-1));
	// std::bitset<13> y(output_code[op_idx-1]);
	// std::cout<<"Out Byte in HW: "<<y<<"\n";



	/////////////////////////////////////////////////////////////
    uint32_t offset_pack = 0;

    uint32_t curr_code = 0;
	uint32_t write_data = 0;
	uint32_t old_byte = 0;
	uint32_t rem_bits = 0;
	uint32_t running_bits = 0;
	uint32_t write_byte_size = 0;//number of bytes to write
	uint8_t write_byte = 0;//whats written in file

    for(int idx=0; idx < op_idx; idx++)
	{
		curr_code = output_code[idx];
		write_data = curr_code<<(32 - (int)CODE_LENGTH - rem_bits);
		write_data |= old_byte;
		running_bits = rem_bits + (int)CODE_LENGTH;
		write_byte_size = running_bits/8;
		for (uint32_t i=1; i<=write_byte_size; i++)
		{
			write_byte = write_data>>(32-i*8);
            output_code_packed[offset_pack] = write_byte;
			// memcpy(&file[offset], &write_byte, sizeof(unsigned char));
			offset_pack += 1;
		}
		old_byte = write_data<<(write_byte_size*8);
		rem_bits = running_bits - write_byte_size*8;
	}
	if(rem_bits){
		write_byte = old_byte>>24;
        output_code_packed[offset_pack] = write_byte;
        offset_pack +=1;
	}

	*output_code_size = op_idx;
}
