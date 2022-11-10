#include "encoder.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include "server.h"
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "stopwatch.h"
#include <vector>
#include <bits/stdc++.h>
#include <bitset>

#include <wolfssl/options.h>
#include <wolfssl/wolfcrypt/sha3.h>

#include <math.h>

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

int offset = 0;
unsigned char* file;

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

void lzw_encoding(unsigned char* s1, chunk* cptr)
{
	uint32_t length = cptr->size;
	std::cout<<"length of chunk: "<<length<<"\n";

    // std::cout << "Encoding\n";
    std::unordered_map<std::string, int> table;
    for (int i = 0; i <= 255; i++) {
        std::string ch = "";
        ch += char(i);
        table[ch] = i;
    }
 
    std::string p = "", c = "";
    p += s1[0];
    int code = 256;
    std::vector<int> output_code;
    // cout << "String\tOutput_Code\tAddition\n";
    for (uint32_t i = 0; i < length; i++) {
        if (i != length - 1)
            c += s1[i + 1];
        if (table.find(p + c) != table.end()) {
            p = p + c;
        }
        else {
            // cout << p << "\t" << table[p] << "\t\t"
            //      << p + c << "\t" << code << endl;
            output_code.push_back(table[p]);
            table[p + c] = code;
            code++;
            p = c;
        }
        c = "";
    }
    // cout << p << "\t" << table[p] << endl;
    output_code.push_back(table[p]);
	int output_code_size = output_code.size();
	// out_code=sizeof(output_code);
	std::cout << "output_code size = " << output_code_size << "\n";
	//std::cout << "output_code size(sizeof): = " << out_code << "\n";


	uint32_t curr_code = 0;
	uint32_t write_data = 0;
	uint32_t old_byte = 0;
	uint32_t rem_bits = 0;
	uint32_t running_bits = 0;
	uint32_t write_byte_size = 0;//number of bytes to write
	uint8_t write_byte = 0;//whats written in file
	uint32_t bytes_written = ceil(output_code_size*13.0/8.0);
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
		std::cout<<" No of Remaining Bits:"<<rem_bits<<"\n";
		std::cout<<"Remaining Bits:"<<x<<"\n";
		write_byte = old_byte>>24;
		std::bitset<8> y(write_byte);
		std::cout<<"Remaining Byte:"<<y<<"\n";

		// write_byte_size=rem_bits/8;
		// for (uint32_t i=1; i<=write_byte_size; i++)
		// {
		// 	write_byte = old_byte>>(32-i*8);
			memcpy(&file[offset], &write_byte, sizeof(unsigned char));
			offset+= sizeof(unsigned char);
		// }
	}
}


uint64_t hash_func(unsigned char *input, uint32_t pos)
{
	uint64_t hash = 0;
	for ( int i = 0 ; i < WIN_SIZE ; i++)
	{
		hash += input[pos + WIN_SIZE - 1 - i] * pow(PRIME, i + 1);
	}
	return hash;
}

void sha(unsigned char* buff, chunk *cptr)//, wc_Sha3* sha3_384)
{
	unsigned char shaSum[SHA3_384_DIGEST_SIZE];

	std::cout<<"calculating sha for buff: "<<buff[0]<<buff[1]<<buff[2]<<"\n";
    
    wc_Sha3 sha3_384;
    wc_InitSha3_384(&sha3_384, NULL, INVALID_DEVID);
    wc_Sha3_384_Update(&sha3_384, (const unsigned char*)buff, cptr->size/*strlen(message)*/);
    wc_Sha3_384_Final(&sha3_384, shaSum);

	std::string str(reinterpret_cast<char*>(shaSum), SHA3_384_DIGEST_SIZE);
	std::cout<<"This chunk's SHA: "<<str<<"\n";
	cptr->sha = str;
}

void cdc_eff(unsigned char *buff, chunk *cptr, uint32_t length)
{
	/*
	buff:
	lower_bound:
	hash:		 initial hash for a packet, used for calculating rolling hash
	length:
	*/

	uint64_t hash = hash_func(buff, WIN_SIZE);

	for (int i = WIN_SIZE + 1; i < MAX_CHUNK_SIZE - WIN_SIZE; i++)
	// for (int i = 1; i < MAX_CHUNK_SIZE; i++)
	{
		// Check if condition is working
		if (i - 1 + WIN_SIZE + cptr->lower_bound > length)
		{
			cptr->upper_bound = length-1;
			return;
		}
		

		hash = (hash * PRIME - (buff[i - 1] * pow(PRIME, WIN_SIZE + 1)) + (buff[i - 1 + WIN_SIZE] * PRIME));

		if((hash % MODULUS) == TARGET)
		{
			cptr->upper_bound = i  + cptr->lower_bound;
			return;
		}
	}
	// *starting_hash = hash;
	cptr->upper_bound = MAX_CHUNK_SIZE + cptr->lower_bound;
	return;
}

void handle_input(int argc, char* argv[], int* blocksize) {
	int x;
	extern char *optarg;

	while ((x = getopt(argc, argv, ":b:")) != -1) {
		switch (x) {
		case 'b':
			*blocksize = atoi(optarg);
			printf("blocksize is set to %d optarg\n", *blocksize);
			break;
		case ':':
			printf("-%c without parameter\n", optopt);
			break;
		}
	}
}

void chunk_matching(chunk *cptr)//, std::unordered_map<std::string, uint32_t> *chunks_map)//, uint32_t* unique_chunks)
{
	static std::unordered_map<std::string, uint32_t> chunks_map;
	uint32_t chunk_header = 1;
	static uint32_t unique_chunks = 0;
	static std::vector<unsigned int> chunk_length;

	if (!(chunks_map.find(cptr->sha) == chunks_map.end()))
	{
		if((cptr->size == chunk_length[chunks_map[cptr->sha]])){
			// Creating the header for a duplicate chunk with LSB 1
			std::cout<<"This chunk is a copy of chunk no. :"<<chunks_map[cptr->sha]<<" with sha: "<<(cptr->sha)<<"\n";
			chunk_header |= (chunks_map[cptr->sha] << 1);
			std::cout<<"\nChunk matching Header: "<<chunk_header<<"\n";

			memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
			offset += sizeof(uint32_t);

			cptr->is_unique = false;
			return;
			// return false;
		}
		else
		{
			// Condition if chunk is unique

			cptr->num = unique_chunks;
			chunks_map[cptr->sha] = unique_chunks;
			std::cout<<"Num assigned to chunk: "<<chunks_map[cptr->sha]<<"\n";
			cptr->is_unique = true;
			chunk_length.push_back(cptr->size);
			unique_chunks++;
			return;
			// return true;
		}

	}
	else
	{
		// Condition if chunk is unique

		cptr->num = unique_chunks;
		chunks_map[cptr->sha] = unique_chunks;
		std::cout<<"Num assigned to chunk: "<<chunks_map[cptr->sha]<<"\n";
		cptr->is_unique = true;
		chunk_length.push_back(cptr->size);
		unique_chunks++;
		return;
		// return true;
	}
}

void compress(unsigned char *buffer, uint32_t length)//, std::unordered_map<std::string, uint32_t> *chunks_map)//, uint32_t* unique_chunks)
{
	/*
	buffer: 	pointer to input data (one packet)
	length: 	packet size (may be variable, max allowed is 16KB)
	chunks_map: empty hash map for unique chunks
	*/

	// Structure to store data for current chunk
	chunk curr_chunk;

	curr_chunk.lower_bound = 0;
	curr_chunk.upper_bound = 0;

	chunk *cptr = &curr_chunk;


	while(curr_chunk.lower_bound < length)
	{
		cdc_eff(&buffer[curr_chunk.lower_bound], cptr, length);

		std::cout<<"current chunk lower bound: "<<curr_chunk.lower_bound<<"\n";
		std::cout<<"current chunk upper bound: "<<curr_chunk.upper_bound<<"\n";
		curr_chunk.size = curr_chunk.upper_bound - curr_chunk.lower_bound + 1;
		std::cout<<"Size of chunk: "<<curr_chunk.size<<"\n";

		sha(&buffer[curr_chunk.lower_bound], cptr);
		chunk_matching(cptr);
		
		if (curr_chunk.is_unique)
		{
			lzw_encoding(&buffer[curr_chunk.lower_bound], cptr);
		}

		curr_chunk.lower_bound = curr_chunk.upper_bound +1;

		std::cout<<"CHUNK COMPLETE\n\n\n";
	}
}

int main(int argc, char* argv[]) {
	stopwatch ethernet_timer;
	unsigned char* input[NUM_PACKETS];
	int writer = 0;
	int done = 0;
	uint32_t length = 0;
	int count = 0;
	ESE532_Server server;

	// default is 2k
	int blocksize = BLOCKSIZE;

	// set blocksize if decalred through command line
	handle_input(argc, argv, &blocksize);

	file = (unsigned char*) malloc(sizeof(unsigned char) * 70000000);
	
	if (file == NULL) {
		printf("help\n");
	}

	for (int i = 0; i < NUM_PACKETS; i++) {
		input[i] = (unsigned char*) malloc(
				sizeof(unsigned char) * (NUM_ELEMENTS + HEADER));
		if (input[i] == NULL) {
			std::cout << "aborting " << std::endl;
			return 1;
		}
	}

	server.setup_server(blocksize);

	writer = pipe_depth;
	server.get_packet(input[writer]);
	count++;

	// get packet
	unsigned char* buffer = input[writer];

	// decode
	done = buffer[1] & DONE_BIT_L;
	length = buffer[0] | (buffer[1] << 8);
	length &= ~DONE_BIT_H;


	compress(&buffer[HEADER], length);//, &chunks_map);//, &unique_chunks);

	writer++;

	while (!done) {
		// reset ring buffer
		if (writer == NUM_PACKETS) {
			writer = 0;
		}

		ethernet_timer.start();
		server.get_packet(input[writer]);
		ethernet_timer.stop();

		count++;

		// get packet
		unsigned char* buffer = input[writer];

		// decode
		done = buffer[1] & DONE_BIT_L;
		length = buffer[0] | (buffer[1] << 8);
		length &= ~DONE_BIT_H;
		std::cout<<"Packet Length: "<< length<<"\n";
		
		compress(&buffer[HEADER], length);
		
		writer++;
	}

	// write file to root and you can use diff tool on board
	FILE *outfd = fopen("output_cpu.bin", "wb");
	int bytes_written = fwrite(&file[0], 1, offset, outfd);
	printf("write file with %d\n", bytes_written);
	fclose(outfd);

	for (int i = 0; i < NUM_PACKETS; i++) {
		free(input[i]);
	}

	free(file);
	std::cout << "--------------- Key Throughputs ---------------" << std::endl;
	float ethernet_latency = ethernet_timer.latency() / 1000.0;
	float input_throughput = (bytes_written * 8 / 1000000.0) / ethernet_latency; // Mb/s
	std::cout << "Input Throughput to Encoder: " << input_throughput << " Mb/s."
			<< " (Latency: " << ethernet_latency << "s)." << std::endl;

	return 0;
}
