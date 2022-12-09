#include "lzw_kernel.h"

typedef ap_uint<72> associative_mem;
typedef ap_uint<9> associative_key;

void hashmap_create(hashmap_entry_t hash_entries[][BUCKET_SIZE]) {
  for (uint32_t i = 0; i < HASHMAP_CAPACITY; i++) {
	// #pragma HLS unroll
	for (uint32_t j=0; j<BUCKET_SIZE; j++)
	{
		// #pragma HLS unroll
    	hash_entries[i][j].key = 0;
	}
  }
	memset(hash_entries, 0, sizeof(hashmap_entry_t)*BUCKET_SIZE*HASHMAP_CAPACITY);
  	return;
}

bool hashmap_put(hashmap_entry_t hash_entries[][BUCKET_SIZE], uint32_t key, int32_t code) 
{
	#pragma HLS inline
	uint32_t index = key % HASHMAP_CAPACITY;

	for( uint32_t j=0; j<BUCKET_SIZE; j++)
	{
		// #pragma HLS unroll
		if (hash_entries[index][j].key == 0) 
		{
			// #pragma HLS unroll
			hash_entries[index][j].key = key;
			hash_entries[index][j].code = code;
			// std::cout<<"putting value: "<<code<<" with key: "<<key<<" at: "<<index<<"\n";
			return true;
		}
	}
	// index = (index + 1) % HASHMAP_CAPACITY;
	// hash_entries[index].key = key;
	// hash_entries[index].code = code;
	// std::cout<<"couldn't put value: "<<code<<" with key: "<<key<<" at: "<<index<<"\n";
	return false;
//   map->size++;
}

int32_t hashmap_get(hashmap_entry_t hash_entries[][BUCKET_SIZE], uint32_t key)
{
	#pragma HLS inline
	uint32_t index = key % HASHMAP_CAPACITY;
	for( uint32_t j=0; j<BUCKET_SIZE; j++)
	{
		if (hash_entries[index][j].key == key) {
			// std::cout<<"found key: "<<key<<" at "<<index<<"\n";
			return hash_entries[index][j].code;
		}
	}
	// index = (index + 1) % HASHMAP_CAPACITY;
	// std::cout<<"collision for index: "<<index<<"\n";
	return -1;
}



uint32_t djb2hash(const void *key, int len) {
  uint32_t hash = 5381;
  const uint64_t * data = (const uint64_t *)key;

  for (size_t i = 0; i < len; i++)
    hash = ((hash << 5) + hash) + data[i]; /* hash * 33 + c */

  return hash;
}

uint32_t FNVHash(const void * key, unsigned int length) {
	const unsigned int fnv_prime = 0x811C9DC5;
	unsigned int hash = 0;
	const char * str = (const char *)key;
	unsigned int i = 0;

	for (i = 0; i < length; str++, i++)
	{
		hash *= fnv_prime;
		hash ^= (*str);
	}

	return hash;
}

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

uint8_t log_2(associative_mem bit_to_get)
{
	uint8_t addr = 0;
	while ((bit_to_get >> 1)) {	// unroll for more speed.
  		addr++;
		bit_to_get = bit_to_get>>1;
	}
	return addr;
}

uint8_t associative_insert(associative_mem associative_map[][512], int32_t* associative_value_list, uint8_t counter, uint32_t hash, uint32_t code)
{
	associative_mem bit_to_set = 0;
	associative_key index[4];
	associative_value_list[counter] = code; //put code in value bram

	for (uint8_t i = 0; i<4; i++) {
		index[i] = (hash>>(i*9)) & 0x1FF;
	}
	bit_to_set = 1 << counter;

	for (uint8_t i = 0; i<4; i++) {
		associative_map[i][index[i]] |= bit_to_set;
	}

	return ++counter;	
}

int32_t associative_lookup(associative_mem associative_map[4][512], int32_t* associative_value_list, uint32_t hash)
{
	associative_key index[4];
	// std::cout<<"trying to lookup hash: "<<hash<<"\n";

	for (uint8_t i = 0; i<4; i++) {
		index[i] = (hash >> (i * 9)) & 0x1FF;	// Create 9-bit keys from the 32-bit hash
	}

	associative_mem bit_to_get = associative_map[0][index[0]] & associative_map[1][index[1]];
	bit_to_get &= associative_map[2][index[2]];
	bit_to_get &= associative_map[3][index[3]];

	if(bit_to_get == 0)	{
		return -1;
	}

	int32_t code = associative_value_list[log_2(bit_to_get)];
	if(code == 0) {
		return -1;
	}

	return code;
}

void load(unsigned char* input_packet,
		  uint32_t num_chunks,
		  uint32_t* chunk_bndry,
		  uint8_t* is_chunk_unique,
		  uint32_t* dup_chunk_head,
		  hls::stream<unsigned char> &input,
		  hls::stream<uint32_t> &boundaries_1,
		  hls::stream<uint8_t> &uniques_1,
		  hls::stream<uint32_t> &head_1)
{
	uint32_t last_boundary = 0;

	uint32_t l_chunk_boundary = 0;
	uint8_t unique = false;
	uint32_t l_head = 0;

	for(uint32_t i = 0; i < num_chunks; i++)
	{
		l_chunk_boundary = chunk_bndry[i];
		unique = is_chunk_unique[i];
		l_head = dup_chunk_head[i];

		// std::cout << "\nChunk \"" << i << "\" in HW ----- Start Boundary: " << last_boundary << " End Boundary: " << l_chunk_boundary << "\n";
		// std::cout << "CHunk unique: " << unique << "\n";
		// std::cout << "CHunk header: " << l_head << "\n";
		
		boundaries_1.write(l_chunk_boundary);
		uniques_1.write(unique);
		head_1.write(l_head);
		// std::cout<<"writing bounds, uniques and heads in load\n";

		if(unique)
		{
			for(uint32_t j = last_boundary; j < l_chunk_boundary + 1; j++)
			{
				// std::cout<<"reading data in load: ";
				uint8_t l_data = input_packet[j+2];
				// std::cout << l_data << "\n";
				// std::cout<<"writing data in load\n";
				input.write(l_data);
				
			}
		}

		last_boundary = l_chunk_boundary + 1;
	}
}

void lzw_encode(hls::stream<unsigned char> &input,
				hls::stream<uint32_t> &boundaries_1,
				hls::stream<uint8_t> &uniques_1,
				hls::stream<uint32_t> &head_1,
				uint32_t num_chunks,
				hls::stream<uint32_t> &lzw_encode_out,
				
				// hls::stream<uint32_t> &boundaries_2,
				hls::stream<uint8_t> &uniques_2,
				hls::stream<uint32_t> &head_2,
				hls::stream<uint8_t> &lzw_encode_out_flag)
{
	uint32_t resetValue = 0;
	// uint32_t output_code[MAX_NUM_OF_CODES];
	// uint64_t table[MAX_NUM_OF_CODES];

	uint32_t hash_val = 0;
	uint8_t ch = 0;
	uint32_t code = 0;

	uint32_t l_chunk_boundary = 0;
	uint8_t unique = false;
	uint32_t l_head = 0;

	uint32_t last_boundary = 0;
	

	for(uint32_t j = 0; j < num_chunks; j++)
	{
		l_chunk_boundary = boundaries_1.read();
		unique = uniques_1.read();
		l_head = head_1.read();

		// boundaries_2.write(l_chunk_boundary);
		uniques_2.write(unique);
		head_2.write(l_head);
		// std::cout<<"chunk: "<<j<<"\n";
		bool already_sent = false;

		if(unique)
		{
			associative_mem associative_map[4][512];//10 brams 72 values
			// associative_mem key_high[512][4];
			int values[72];
			uint8_t counter = 0;
			int32_t match = 0;
			// uint32_t ins_counter = 0;

			// for(uint32_t i=0; i<512; i++)


			hashmap_entry_t hash_entries[HASHMAP_CAPACITY][BUCKET_SIZE];
			// hashmap_create(hash_entries);
			memset(hash_entries, 0, sizeof(hashmap_entry_t)*BUCKET_SIZE*HASHMAP_CAPACITY);
			memset(associative_map, 0, sizeof(associative_mem)*4*512);


			// std::cout<<"trying to insert in assoc\n";

			// counter = associative_insert(associative_map, values, counter, 370, 500);
			// // std::cout<< "counter returned from ass: "<<counter<<"\n";
			// printf("new counter from insert: %d", counter);

			// // std::cout<<"value from bram "<<values[counter-1]<<"\n";
			// printf("value from bram: %d \n", values[counter-1]);
			// match = associative_lookup(associative_map, values, 370);
			// std::cout<<"From assoc lookup: match at: "<<match<<" counter: "<<counter<<"\n";


			// std::cout<<"created new hash map\n";

			code = 256;
			uint32_t length = l_chunk_boundary - last_boundary + 1;

			int32_t out_code = 0;

			uint8_t c;
			uint8_t p[MAX_NUM_OF_CODES] = {0};
			uint32_t p_idx = 0;
			uint32_t op_idx = 0;

			p[p_idx] = input.read();
			p_idx++;
			
			for (uint32_t i = 0; i < length; i++)
			{
				if (i != length - 1)
				{
					c = input.read();
					p[p_idx] = c;
					p_idx++;
				}

				// Search algorithm for the array
				hash_val = FNVHash((void*)p, p_idx);
				already_sent=false;

				match = hashmap_get(hash_entries, hash_val);

				if(match == -1)
				{
					match = associative_lookup(associative_map, values, hash_val);
				}

				if (match == -1/*table.find(p + c) != table.end()*/) {			// Change the find function
					
					

					if(p_idx <= 2)
					{
						// std::cout<<"sending "<<p[0]<<" for p_idx: "<<p_idx<<"\n";
						out_code = (int32_t)p[0];
						already_sent = true;
					}
					else
					{
						uint32_t hash_val_outcode = FNVHash((void*)p, p_idx - 1);
						out_code = hashmap_get(hash_entries, hash_val_outcode);
						if(out_code == -1)
						{
							out_code = associative_lookup(associative_map, values, hash_val_outcode);
						}
					}
					lzw_encode_out_flag.write(1);
					lzw_encode_out.write(out_code);
					bool put_pass = hashmap_put(hash_entries, hash_val, code);
					if(!put_pass && counter<72)
					{
						// if(counter==0)
						// {
						// 	memset(associative_map, 0, sizeof(associative_mem)*4*512);
						// }
						counter = associative_insert(associative_map, values, counter, hash_val, code);
						// ins_counter++;
						// printf("Insert in assoc counter: %d\n", ins_counter);
					}
					
					code++;
					p_idx = 0;
					p[p_idx] = c;
					p_idx++;
				}
			}
			if(!already_sent)
			{
				lzw_encode_out_flag.write(1);
				// uint32_t out_code = search(table, code, MurmurHash2((void*)p, p_idx));
				// hash_val = FNVHash((void*)p, p_idx);
				out_code = hashmap_get(hash_entries, hash_val);
				if(out_code == -1)
				{
					out_code = associative_lookup(associative_map, values, hash_val);
				}
				lzw_encode_out.write(out_code);
			}
			// std::cout<<"Out code outside if(match<0): "<<out_code<<"\n";
			lzw_encode_out_flag.write(0);
		}
		last_boundary = l_chunk_boundary + 1;
	}
}

void bit_pack(hls::stream<uint32_t> &bit_pack_in,
			  uint32_t num_chunks,

				hls::stream<uint8_t> &lzw_encode_out_flag,
			
				// hls::stream<uint32_t> &boundaries_2,
				hls::stream<uint8_t> &uniques_2,
				hls::stream<uint32_t> &head_2,
			
				// hls::stream<uint32_t> &boundaries_3,
				hls::stream<uint8_t> &uniques_3,
				hls::stream<uint32_t> &head_3,
				
				hls::stream<unsigned char> &bit_pack_out,
				
				hls::stream<uint8_t> &bit_pack_out_flag)
{
	// uint32_t l_chunk_boundary = 0;
	uint8_t unique = false;
	uint32_t l_head = 0;

	for(uint32_t j = 0; j < num_chunks; j++)
	{
		// uint32_t bit_pack_counter = 0;
		
		// l_chunk_boundary = boundaries_2.read();
		unique = uniques_2.read();
		l_head = head_2.read();

		// boundaries_3.write(l_chunk_boundary);
		uniques_3.write(unique);
		head_3.write(l_head);

		if(unique)
		{
			uint32_t offset_pack = 0;
			uint32_t curr_code = 0;
			uint32_t write_data = 0;
			uint32_t old_byte = 0;
			uint32_t rem_bits = 0;
			uint32_t running_bits = 0;
			uint32_t write_byte_size = 0;	// number of bytes to write
			uint8_t write_byte = 0;	// whats written in file

			uint8_t flag = true;
			flag = lzw_encode_out_flag.read();
			while(flag)
			{
				curr_code = bit_pack_in.read();

				write_data = curr_code<<(32 - (uint32_t)CODE_LENGTH - rem_bits);
				write_data |= old_byte;
				running_bits = rem_bits + (uint32_t)CODE_LENGTH;
				write_byte_size = running_bits/8;
				for (uint32_t i=1; i<=write_byte_size; i++)
				{
					bit_pack_out_flag.write(1);
					// bit_pack_counter++;

					write_byte = write_data>>(32-i*8);
					// output_code_packed[offset_pack] = write_byte;
					bit_pack_out.write(write_byte);
					offset_pack += 1;
				}
				old_byte = write_data<<(write_byte_size*8);
				rem_bits = running_bits - write_byte_size*8;

				flag = lzw_encode_out_flag.read();
			}

			if(rem_bits)
			{
				write_byte = 0;
				write_byte = old_byte>>24;
				bit_pack_out_flag.write(1);
				bit_pack_out.write(write_byte);
				// bit_pack_counter++;
				offset_pack +=1;
			}

			bit_pack_out_flag.write(0);
		}

		// std::cout << "\nBit pack counter writing: " << bit_pack_counter <<" bytes.\n";
	}
	// std::cout << "\n";
}

void store(hls::stream<unsigned char> &output,
		   uint32_t	num_chunks,
		   uint8_t* output_file,
		   uint32_t* output_size,
		   
			// hls::stream<uint32_t> &boundaries_3,
			hls::stream<uint8_t> &uniques_3,
			hls::stream<uint32_t> &head_3,

			hls::stream<uint8_t> &bit_pack_out_flag)
{
	uint32_t offset = 0;

	// uint32_t l_chunk_boundary = 0;
	uint8_t unique = false;
	uint32_t l_head = 0;

	for(uint32_t j = 0; j < num_chunks; j++)
	{
		// uint32_t store_counter = 0;

		// l_chunk_boundary = boundaries_3.read();
		unique = uniques_3.read();
		l_head = head_3.read();

		if(unique)
		{
			uint32_t output_chunk_length = 0;
			uint8_t flag = true;
			uint32_t local_offset = offset + 4;
			
			flag = bit_pack_out_flag.read();
			// std::cout << "Flag received in store: " << flag << "\n";

			// Writing unique chunk data to global file pointer
			while(flag)
			{
				output_file[local_offset] = output.read();
				local_offset++;
				output_chunk_length++;
				// store_counter++;

				flag = bit_pack_out_flag.read();
				// std::cout << "Flag received in store: " << flag << "\n";
			}

			// Writing unique chunk header to global file pointer
			uint32_t chunk_header = 0;
			chunk_header = output_chunk_length << 1;
			for(uint32_t j = 0; j < 4; j++)
			{
				output_file[offset] = chunk_header >> (8 * j);
				offset++;
			}
			offset += output_chunk_length;
		}
		else
		{
			// Writing duplicate chunk header to global file pointer
			uint32_t chunk_header = l_head;
			// uint32_t chunk_header = size << 1;
			
			for(uint32_t j = 0; j < 4; j++)
			{
				output_file[offset] = chunk_header >> (8 * j);
				offset++;
			}
		}
		// std::cout << "\nStore reading: " << store_counter <<" bytes.\n\n";
	}

	*output_size = offset;
}

void lzw_kernel(unsigned char* input_packet,
				uint32_t* chunk_bndry,
				uint32_t num_chunks,
				uint8_t* is_chunk_unique,
				uint8_t* output_file,
				uint32_t* output_size,
				uint32_t* dup_chunk_head)
{
	#pragma HLS INTERFACE m_axi port=input_packet bundle=p1 depth=8192
	#pragma HLS INTERFACE m_axi port=output_file bundle=p2 depth=8192
	#pragma HLS INTERFACE m_axi port=chunk_bndry bundle=p0 depth=512
	#pragma HLS INTERFACE m_axi port=is_chunk_unique bundle=p0 depth=512
	#pragma HLS INTERFACE m_axi port=dup_chunk_head bundle=p0 depth=512

	uint32_t local_num_chunks = num_chunks;

	#pragma HLS DATAFLOW

	hls::stream<unsigned char, 8192> input("input_to_store");
	
	hls::stream<uint32_t, 512> boundaries_1("boundaries_1");
	hls::stream<uint8_t, 512> uniques_1("uniques_1");
	hls::stream<uint32_t, 512> head_1("head_1");

	// hls::stream<uint32_t, 512> boundaries_2("boundaries_2");
	hls::stream<uint8_t, 512> uniques_2("uniques_2");
	hls::stream<uint32_t, 512> head_2("head_2");

	// hls::stream<uint32_t, 512> boundaries_3("boundaries_3");
	hls::stream<uint8_t, 512> uniques_3("uniques_3");
	hls::stream<uint32_t, 512> head_3("head_3");

	hls::stream<uint8_t, 512> lzw_encode_out_flag("lzw_encode_out_flag");
	hls::stream<uint8_t, 512> bit_pack_out_flag("bit_pack_out_flag");

	hls::stream<uint32_t, 8192> lzw_encode_out("lzw_encode_out");
	hls::stream<unsigned char, 8192> bit_pack_out("bit_pack_out");
	hls::stream<unsigned char, 8192> output("final_output");

	// #pragma HLS STREAM type=fifo variable=input depth=500
	// #pragma HLS STREAM type=fifo variable=boundaries_1 depth=500
	// #pragma HLS STREAM type=fifo variable=boundaries_2 depth=500
	// #pragma HLS STREAM type=fifo variable=boundaries_3 depth=500
	// #pragma HLS STREAM type=fifo variable=uniques_1 depth=500
	// #pragma HLS STREAM type=fifo variable=uniques_2 depth=500
	// #pragma HLS STREAM type=fifo variable=uniques_3 depth=500
	// #pragma HLS STREAM type=fifo variable=head_1 depth=500
	// #pragma HLS STREAM type=fifo variable=head_2 depth=500
	// #pragma HLS STREAM type=fifo variable=head_3 depth=500
	
	// #pragma HLS STREAM type=fifo variable=lzw_encode_out_flag depth=500
	// #pragma HLS STREAM type=fifo variable=bit_pack_out_flag depth=500

	// #pragma HLS STREAM type=fifo variable=lzw_encode_out depth=500
	// #pragma HLS STREAM type=fifo variable=bit_pack_out depth=500
	// #pragma HLS STREAM type=fifo variable=output depth=500
	

	// load(input_packet, chunk_bndry, is_chunk_unique, local_num_chunks, input);
	load(input_packet, local_num_chunks, chunk_bndry, is_chunk_unique, dup_chunk_head, input, boundaries_1, uniques_1, head_1);

	// lzw_encode(input, chunk_bndry, is_chunk_unique, local_num_chunks, lzw_encode_out);
	lzw_encode(input, boundaries_1, uniques_1, head_1, local_num_chunks, lzw_encode_out, /*boundaries_2,*/ uniques_2, head_2, lzw_encode_out_flag);

	// bit_pack(lzw_encode_out, is_chunk_unique, local_num_chunks, packed_size, bit_pack_out);
	bit_pack(lzw_encode_out, local_num_chunks, lzw_encode_out_flag, /*boundaries_2,*/ uniques_2, head_2, /*boundaries_3,*/ uniques_3, head_3, bit_pack_out, bit_pack_out_flag);

	// store(bit_pack_out, packed_size, is_chunk_unique, local_num_chunks, dup_chunk_head, output_file, output_size);
	store(bit_pack_out, local_num_chunks, output_file, output_size, /*boundaries_3,*/ uniques_3, head_3, bit_pack_out_flag);

}
