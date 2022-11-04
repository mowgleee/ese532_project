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

int offset = 0;
unsigned char* file;


uint64_t hash_func(unsigned char *input, unsigned int pos)
{
	// put your hash function implementation here
	uint64_t hash =0; 
	for ( int i = 0 ; i<WIN_SIZE ; i++)
	{
		hash += (input[pos + WIN_SIZE-1-i]) * (pow(PRIME,i+1)); 
	}
	return hash; 

}

uint64_t sha_dummy(unsigned char* buff, unsigned int lower_bound, unsigned int upper_bound)
{
	// put your hash function implementation here
	uint64_t hash = 0;
	hash=hash_func(&buff[lower_bound], upper_bound-lower_bound);
	return hash; 

}

unsigned int cdc_eff(unsigned char *buff, unsigned int lower_bound, unsigned int length)
{

	uint64_t hash = hash_func(buff, WIN_SIZE);

	// std::cout<<"Hash = "<<hash<<" lower_bound = "<<lower_bound<<std::endl;

	if((hash % MODULUS) == TARGET)
		{
				printf( " %d \t", lower_bound+1);
				// boundaries.push_back(WIN_SIZE);
				return lower_bound+1;
		}

	for (int i = WIN_SIZE + 1; i < MAX_CHUNK_SIZE - WIN_SIZE; i++)
	{

		if (i + lower_bound > length)
		{
			return length;
		}

		hash = (hash * PRIME - (buff[i - 1] * pow(PRIME, WIN_SIZE + 1)) + (buff[i - 1 + WIN_SIZE] * PRIME));

		if((hash % MODULUS) == TARGET)
		{
			std::cout<<i+lower_bound<<"\t";
			return i + lower_bound;
		}
	}
	return MAX_CHUNK_SIZE + lower_bound;
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

void compress(unsigned char *buffer, int length)
{
	unsigned int lower_bound = HEADER;
	unsigned int upper_bound = 0;
	uint64_t sha_chunk = 0;

	while(upper_bound < length)
	{
		upper_bound = cdc_eff(&buffer[lower_bound], lower_bound, length);
		sha_chunk = sha_dummy(&buffer[HEADER], lower_bound, upper_bound);
		// std::cout<<sha_chunk<<"\n";
		lower_bound = upper_bound;
	}
}

int main(int argc, char* argv[]) {
	stopwatch ethernet_timer;
	unsigned char* input[NUM_PACKETS];
	int writer = 0;
	int done = 0;
	int length = 0;
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
	// printing takes time so be weary of transfer rate
	//printf("length: %d offset %d\n",length,offset);

	// we are just memcpy'ing here, but you should call your
	// top function here.

	// unsigned int* boundaries= (unsigned int*) malloc(sizeof(unsigned int) * length);
	// std::vector<unsigned int> boundaries;
	// std::vector<uint64_t> sha_vector;

	compress(&buffer[HEADER], length);
	// chunk_match();
	// lzw_encode();

	memcpy(&file[offset], &buffer[HEADER], length);

	offset += length;
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
		//printf("length: %d offset %d\n",length,offset);
		// cdc_eff(&buffer[HEADER], length);
		// compress(&buffer[HEADER], length);
		memcpy(&file[offset], &buffer[HEADER], length);

		offset += length;
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

