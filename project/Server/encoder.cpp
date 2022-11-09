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
	unsigned int lower_bound = 0;
	unsigned int upper_bound = 0;
	unsigned int size = 0;
	std::string sha;
	bool is_unique;
	int num;
}chunk;

void lzw_encoding(unsigned char* s1, chunk* cptr)
{
	unsigned int length = cptr->size;
	unsigned int chunk_header=0;

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
    std::vector<unsigned int> output_code;
    // std::cout << "String\tOutput_Code\tAddition\n";
    for (unsigned int i = 0; i < length; i++) {
        if (i != length - 1)
            c += s1[i + 1];
        if (table.find(p + c) != table.end()) {
            p = p + c;
        }
        else {
            // std::cout << p << "\t" << table[p] << "\t\t"
            //      << p + c << "\t" << code << std::endl;
            output_code.push_back(table[p]);
            table[p + c] = code;
            code++;
            p = c;
        }
        c = "";
    }
    // std::cout << p << "\t" << table[p] << std::endl;
    output_code.push_back(table[p]);

	// uint32_t total_bits = 0;
	uint32_t curr_code = 0;
	unsigned int write_data = 0;
	unsigned int old_byte = 0;
	unsigned int rem_bits = 0;
	unsigned int running_bits = 0;
	unsigned int write_byte_size = 0;//number of bytes to write
	uint8_t write_byte = 0;//whats written in file
	int orig_offset = offset;
	unsigned int bytes_written = 0;
	offset += sizeof(unsigned int);

	for(int idx=0; idx<output_code.size(); idx++)
	{
		curr_code = output_code[idx];
		write_data = curr_code<<(32 - (int)CODE_LENGTH - rem_bits);
		write_data |= old_byte;
		running_bits = rem_bits + (int)CODE_LENGTH;
		write_byte_size = running_bits/8;
		// write_byte = write_data>>24;//(32 - running_bits - 8);
		for (unsigned int i=1; i<=write_byte_size; i++)
		{
			write_byte = write_data>>(32-i*8);
			memcpy(&file[offset], &write_byte, sizeof(unsigned char));
			offset += sizeof(unsigned char);
		}
		bytes_written += write_byte_size;
		old_byte = write_data<<(write_byte_size*8);
		rem_bits = running_bits - write_byte_size*8;
	}

	// Creating header for unique chunk with LSB 0
	chunk_header = (chunk_header & 0) | bytes_written<<1;
	std::cout<<"\nLZW Header: "<<chunk_header<<"\n";
	memcpy(&file[orig_offset], &chunk_header, sizeof(unsigned int));
}


uint64_t hash_func(unsigned char *input, unsigned int pos)
{
	uint64_t hash = 0;
	for ( int i = 0 ; i < WIN_SIZE ; i++)
	{
		hash += input[pos + WIN_SIZE - 1 - i] * pow(PRIME, i + 1);
	}
	return hash;
}

void sha_dummy(unsigned char* buff, chunk *cptr)
{
	// cptr->sha = hash_func(buff, cptr->size);

	char shaSum[SHA3_384_DIGEST_SIZE];
    
    wc_Sha3 sha3_384;
    wc_InitSha3_384(&sha3_384, NULL, INVALID_DEVID);
    wc_Sha3_384_Update(&sha3_384, (const unsigned char*)buff, cptr->size/*strlen(message)*/);
    wc_Sha3_384_Final(&sha3_384, (unsigned char*)shaSum);

	cptr->sha = std::string(shaSum);

	// return;

    // for(int x = 0; x < SHA3_384_DIGEST_SIZE; x++)
    // {
    //     printf("%x",shaSum[x]);
    // }

}

void cdc_eff(unsigned char *buff, chunk *cptr, uint64_t* starting_hash, unsigned int length)
{
	/*
	buff:
	lower_bound:
	hash:		 initial hash for a packet, used for calculating rolling hash
	length:
	*/

	uint64_t hash = *starting_hash;

	// for (int i = WIN_SIZE + 1; i < MAX_CHUNK_SIZE - WIN_SIZE; i++)
	for (int i = 1; i < MAX_CHUNK_SIZE; i++)
	{
		// Check if condition is working
		if (i - 1 + WIN_SIZE + cptr->lower_bound > length)
		{
			cptr->upper_bound = length;
			return;
			// return length;
		}

		hash = (hash * PRIME - (buff[i - 1] * pow(PRIME, WIN_SIZE + 1)) + (buff[i - 1 + WIN_SIZE] * PRIME));

		if((hash % MODULUS) == TARGET)
		{
			std::cout<<"\nBoundary found at: "<<i + cptr->lower_bound<<"\t";
			*starting_hash = hash;
			cptr->upper_bound = i + cptr->lower_bound;
			// return i + lower_bound;
			return;
		}
	}
	*starting_hash = hash;
	cptr->upper_bound = MAX_CHUNK_SIZE + cptr->lower_bound;
	return;
	// return MAX_CHUNK_SIZE + lower_bound;
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

void chunk_matching(chunk *cptr, std::unordered_map<std::string, unsigned int> *chunks_map, unsigned int* unique_chunks)
{

	unsigned int chunk_header=0;

	if (chunks_map->find(cptr->sha) == chunks_map->end())
	{
		// Condition if chunk is unique

		cptr->num = *unique_chunks;
		(*chunks_map)[cptr->sha] = cptr->num;
		cptr->is_unique = true;
		*unique_chunks= *unique_chunks + 1;
		return;
		// return true;
	}
	else
	{
		// Condition if chunk is duplicate

		// Creating the header for a duplicate chunk with LSB 1
		chunk_header = (chunk_header | 1) | ((*chunks_map)[cptr->sha] << 1);
		std::cout<<"\nChunk matching Header: "<<chunk_header<<"\n";

		memcpy(&file[offset], &chunk_header, sizeof(unsigned int));
		offset += sizeof(unsigned int);

		cptr->is_unique = false;
		return;
		// return false;
	}
}

void compress(unsigned char *buffer, unsigned int length, std::unordered_map<std::string, unsigned int> *chunks_map, unsigned int* unique_chunks)
{
	/*
	buffer: 	pointer to input data (one packet)
	length: 	packet size (may be variable, max allowed is 16KB)
	chunks_map: empty hash map for unique chunks
	*/

	// Structure to store data for current chunk
	chunk curr_chunk;

	curr_chunk.lower_bound = HEADER;
	curr_chunk.upper_bound = HEADER;

	chunk *cptr = &curr_chunk;

	uint64_t starting_hash = hash_func(&buffer[curr_chunk.lower_bound], WIN_SIZE);
	
	curr_chunk.lower_bound = HEADER + WIN_SIZE;
	curr_chunk.upper_bound = HEADER + WIN_SIZE;


	while(curr_chunk.upper_bound < length)
	{
		// curr_chunk.upper_bound = cdc_eff(&buffer[curr_chunk.lower_bound], cptr, &starting_hash, length);
		cdc_eff(&buffer[curr_chunk.lower_bound], cptr, &starting_hash, length);
		curr_chunk.size = curr_chunk.upper_bound - curr_chunk.lower_bound;

		std::cout<<"Size of chunk: "<<curr_chunk.size<<"\t";

		// curr_chunk.sha = sha_dummy(&buffer[curr_chunk.lower_bound], &curr_chunk);
		sha_dummy(&buffer[curr_chunk.lower_bound], cptr);

		// curr_chunk.is_unique = chunk_matching(curr_chunk.sha, &curr_chunk, chunks_map);
		chunk_matching(cptr, chunks_map, unique_chunks);
		
		if (curr_chunk.is_unique)
		{
			lzw_encoding(&buffer[curr_chunk.lower_bound], cptr);
		}

		// std::cout<<sha_chunk<<"\n";
		curr_chunk.lower_bound = curr_chunk.upper_bound;
	}
}

int main(int argc, char* argv[]) {
	stopwatch ethernet_timer;
	unsigned char* input[NUM_PACKETS];
	int writer = 0;
	int done = 0;
	unsigned int length = 0;
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

	std::unordered_map<std::string, unsigned int> chunks_map;
	unsigned int unique_chunks = 0;

	compress(&buffer[HEADER], length, &chunks_map, &unique_chunks);

	writer++;

	//last message
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
		// printf("length: %d offset %d\n",length,offset);
		
		compress(&buffer[HEADER], length, &chunks_map, &unique_chunks);
		
		writer++;
	}

	std::cout<< "Unique chunks: "<<(unique_chunks)<<"\n";

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
