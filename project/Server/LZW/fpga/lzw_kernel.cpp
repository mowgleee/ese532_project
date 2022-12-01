#include "lzw_kernel.h"

uint64_t MurmurHash2( const void * key, int len)
{
	uint64_t seed = 1;
  	const uint64_t m = 0xc6a4a7935bd1e995;
  	const int r = 47;

	const uint64_t * data = (const uint64_t *)key;

	uint64_t h = seed ^ (len * m);

  const uint64_t * end = data + (len/8);

  while(data != end)
  {
    uint64_t k = *data;
	data++;

    k *= m; 
    k ^= k >> r; 
    k *= m; 
    
    h ^= k;
    h *= m; 
  }

  const unsigned char * data2 = (const unsigned char*)data;

  switch(len & 7)
  {
  case 7: h ^= uint64_t(data2[6]) << 48;
  case 6: h ^= uint64_t(data2[5]) << 40;
  case 5: h ^= uint64_t(data2[4]) << 32;
  case 4: h ^= uint64_t(data2[3]) << 24;
  case 3: h ^= uint64_t(data2[2]) << 16;
  case 2: h ^= uint64_t(data2[1]) << 8;
  case 1: h ^= uint64_t(data2[0]);
          h *= m;
  };
 
  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
} 

int32_t search(uint64_t* table, uint32_t length, uint64_t hash_val)
{
	for(uint64_t i = 0; i < length; i++)
	{
		if(table[i] == hash_val)
			return i;
	}
	return -1;
}

void load(const unsigned char* input_packet,
		  uint32_t* chunk_bndry,
		  bool* is_chunk_unique,
		  uint32_t num_chunks,
		  hls::stream<unsigned char> &input)
{
	uint32_t last_boundary = 0;
	for(uint32_t i = 0; i < num_chunks; i++)
	{
		if(is_chunk_unique[i])
		{
			for(uint32_t j = 0; j < chunk_bndry[i] + 1; j++)
			{
				input.write(input_packet[j + last_boundary]);
			}
		}
		last_boundary = chunk_bndry[i] + 1;
	}
}

void lzw_encode(hls::stream<unsigned char> &input,
				uint32_t* chunk_bndry,
				bool* is_chunk_unique,
				uint32_t num_chunks,
				hls::stream<int32_t> &lzw_encode_out)
{
	uint64_t resetValue = 0;
	uint32_t output_code[MAX_NUM_OF_CODES];
	uint64_t table[MAX_NUM_OF_CODES]; // index i is the value and value stored is the hash of that substring
	// #pragma HLS ARRAY_PARTITION variable = table block factor = 1024

	// Use while loop and reset for multiple chunks
	for (int i = 0; i < MAX_NUM_OF_CODES; i++)
	{
	#pragma HLS PIPELINE II = 1
	#pragma HLS UNROLL FACTOR = 2
		table[i] = resetValue;
	}

	uint64_t hash_val;
	uint8_t ch = 0;
	uint32_t code = 0;

    for (code = 0; code <= 255; code++)
	{
	#pragma HLS PIPELINE II = 1
	#pragma HLS UNROLL FACTOR = 2
        ch = char(code);
        // ch += char(i);
		hash_val = MurmurHash2((void*)&ch, 1/*Default single character length*/);
		table[code] = hash_val;
    }

	for(uint32_t j = 0; j < num_chunks; j++)
	{
		if(is_chunk_unique[j])
		{
			// lzw_end = 0;
			code = 255;
			uint32_t length = chunk_bndry[j];

			uint8_t c;
			uint8_t p[MAX_NUM_OF_CODES] = {0};
			uint32_t p_idx = 0;

			p[p_idx] = input.read();
			p_idx++;

			// std::vector<int> output_code;

			uint32_t op_idx = 0;
			
			for (uint32_t i = 0; i < length; i++)
			{
				if (i != length - 1)
				{
					c = input.read();
					p[p_idx] = c;
					p_idx++;
				}

				// Search algorithm for the array
				hash_val = MurmurHash2((void*)p, p_idx);
				int32_t match = search(table, code, hash_val);

				if (match < 0/*table.find(p + c) != table.end()*/) {			// Change the find function
					lzw_encode_out.write(search(table, code, MurmurHash2((void*)p, p_idx - 1)));
					// op_idx++;

					// table[p + c] = code;
					table[code] = hash_val;
					
					code++;
					p_idx = 0;
					p[p_idx] = c;
					p_idx++;
				}
			}

			lzw_encode_out.write(search(table, code, MurmurHash2((void*)p, p_idx)));
			lzw_encode_out.write(-1);
		}
	}
}

void bit_pack(hls::stream<int32_t> &bit_pack_in,
			  bool* is_chunk_unique,
			  uint32_t num_chunks,
			  hls::stream<uint32_t> &packed_size,
			  hls::stream<unsigned char> &bit_pack_out)
{
	uint32_t offset = 0;
	for(uint32_t j = 0; j < num_chunks; j++)
	{
		if(is_chunk_unique[j])
		{
			uint32_t offset_pack = 0;
			uint32_t curr_code = 0;
			uint32_t write_data = 0;
			uint32_t old_byte = 0;
			uint32_t rem_bits = 0;
			uint32_t running_bits = 0;
			uint32_t write_byte_size = 0;	//number of bytes to write
			uint8_t write_byte = 0;	//whats written in file
			
			while(bit_pack_in.empty() && (curr_code >= 0))
			{
				curr_code = bit_pack_in.read();

				if(curr_code >= 0)
				{
					write_data = curr_code<<(32 - (int)CODE_LENGTH - rem_bits);
					write_data |= old_byte;
					running_bits = rem_bits + (int)CODE_LENGTH;
					write_byte_size = running_bits/8;
					for (uint32_t i=1; i<=write_byte_size; i++)
					{
						write_byte = write_data>>(32-i*8);
						// output_code_packed[offset_pack] = write_byte;
						bit_pack_out.write(write_byte);
						offset_pack += 1;
					}
					old_byte = write_data<<(write_byte_size*8);
					rem_bits = running_bits - write_byte_size*8;
				}
			}

			if(rem_bits)
			{
				write_byte = old_byte>>24;
				bit_pack_out.write(write_byte);
				offset_pack +=1;
			}
			packed_size.write(offset_pack);
		}
	}
}

void store(hls::stream<unsigned char> &output,
		   hls::stream<uint32_t> &packed_size,
		   bool* is_chunk_unique,
		   uint32_t num_chunks,
		   uint32_t* dup_chunk_head,
		   uint8_t* output_file,
		   uint32_t* output_size)
{
	uint32_t offset = 0;
	for(uint32_t i = 0; i < num_chunks; i++)
	{
		if(is_chunk_unique[i])
		{
			// Writing unique chunk header to global file pointer
			uint32_t size = packed_size.read();
			uint32_t chunk_header = size << 1;
			for(uint32_t j = 0; j < 4; j++)
			{
				output_file[offset] = chunk_header >> (8 * (3-j));
				offset++;
			}
			
			// Writing unique chunk data to global file pointer
			for(uint32_t j = 0; j < size; j++)
			{
				output_file[offset] = output.read();
				offset++;
			}
		}
		else
		{
			// Writing duplicate chunk header to global file pointer
			uint32_t size = dup_chunk_head[i];
			uint32_t chunk_header = size << 1;
			
			for(uint32_t j = 0; j < 4; j++)
			{
				output_file[offset] = chunk_header >> (8 * (3-j));
				offset++;
			}
		}
	}
	*output_size = offset + 1;
}

void lzw_kernel(const unsigned char* input_packet,
				uint32_t* chunk_bndry,
				uint32_t num_chunks,
				bool* is_chunk_unique,
				uint8_t* output_file,
				uint32_t* output_size,
				uint32_t* dup_chunk_head)
{
	#pragma HLS INTERFACE m_axi port=input_packet bundle=p0 depth=8192
	#pragma HLS INTERFACE m_axi port=output_file bundle=p1 depth=8192

	#pragma HLS DATAFLOW

	hls::stream<unsigned char> input("input_to_store");
	hls::stream<int32_t> lzw_encode_out("lzw_encode_out");
	hls::stream<uint32_t> packed_size("packed_size");
	hls::stream<unsigned char> bit_pack_out("bit_pack_out");
	hls::stream<unsigned char> output("final_output");

	load(input_packet, chunk_bndry, is_chunk_unique, num_chunks, input);
	lzw_encode(input, chunk_bndry, is_chunk_unique, num_chunks, lzw_encode_out);
	bit_pack(lzw_encode_out, is_chunk_unique, num_chunks, packed_size, bit_pack_out);
	store(bit_pack_out, packed_size, is_chunk_unique, num_chunks, dup_chunk_head, output_file, output_size);
}
