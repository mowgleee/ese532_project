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

typedef struct unique_bounds
{
	unsigned int lower_bound;
	unsigned int upper_bound;
}unique_bounds;

std::vector<int> lzw_encoding(unsigned char* s1, unsigned int length)
{
    std::cout << "Encoding\n";
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
    std::cout << "String\tOutput_Code\tAddition\n";
    for (unsigned int i = 0; i < length; i++) {
        if (i != length - 1)
            c += s1[i + 1];
        if (table.find(p + c) != table.end()) {
            p = p + c;
        }
        else {
            std::cout << p << "\t" << table[p] << "\t\t"
                 << p + c << "\t" << code << std::endl;
            output_code.push_back(table[p]);
            table[p + c] = code;
            code++;
            p = c;
        }
        c = "";
    }
    std::cout << p << "\t" << table[p] << std::endl;
    output_code.push_back(table[p]);

	memcpy(&file[offset], &output_code, sizeof(output_code));
	offset += sizeof(output_code);

    return output_code;
}
 
// void lzw_decoding(std::vector<int> op)
// {
//     std::cout << "\nDecoding\n";
//     std::unordered_map<int, std::string> table;
//     for (int i = 0; i <= 255; i++) {
//         std::string ch = "";
//         ch += char(i);
//         table[i] = ch;
//     }
//     int old = op[0], n;
//     std::string s = table[old];
//     std::string c = "";
//     c += s[0];
//     std::cout << s;
//     int count = 256;
//     for (int i = 0; i < op.size() - 1; i++) {
//         n = op[i + 1];
//         if (table.find(n) == table.end()) {
//             s = table[old];
//             s = s + c;
//         }
//         else {
//             s = table[n];
//         }
//         cout << s;
//         c = "";
//         c += s[0];
//         table[count] = table[old] + c;
//         count++;
//         old = n;
//     }
// }

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

bool chunk_matching(uint64_t sha_chunk, unsigned int lower_bound, unsigned int upper_bound, std::unordered_map<uint64_t, unique_bounds> chunks_map)
{
	unique_bounds curr_ub, ub;
	curr_ub.lower_bound = lower_bound;
	curr_ub.upper_bound = upper_bound;

	if (chunks_map.find(sha_chunk) == chunks_map.end())
	{
		chunks_map[sha_chunk] = curr_ub;
		return true;
	}
	else
	{
		// Save chunk number later and make header
		ub = chunks_map[sha_chunk];
		memcpy(&file[offset], &ub, sizeof(ub));
		offset += sizeof(ub);
		return false;
	}
}

void compress(unsigned char *buffer, unsigned int length, std::unordered_map<uint64_t, unique_bounds> chunks_map)
{
	unsigned int lower_bound = HEADER;
	unsigned int upper_bound = 0;
	uint64_t sha_chunk = 0;
	bool is_unique;

	while(upper_bound < length)
	{
		upper_bound = cdc_eff(&buffer[lower_bound], lower_bound, length);
		sha_chunk = sha_dummy(&buffer[HEADER], lower_bound, upper_bound);
		is_unique = chunk_matching(sha_chunk, lower_bound, upper_bound, chunks_map);
		
		if (is_unique)
		{
			lzw_encoding(&buffer[lower_bound], upper_bound - lower_bound);
		}

		// std::cout<<sha_chunk<<"\n";
		lower_bound = upper_bound;
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
	// printing takes time so be weary of transfer rate
	//printf("length: %d offset %d\n",length,offset);

	// we are just memcpy'ing here, but you should call your
	// top function here.

	// unsigned int* boundaries= (unsigned int*) malloc(sizeof(unsigned int) * length);
	// std::vector<unsigned int> boundaries;
	// std::vector<uint64_t> sha_vector;

	// std::vector<int> output_code;
	// output_code.push_back(compress(&buffer[HEADER], length));		// Vector of vector error******


	std::unordered_map<uint64_t, unique_bounds> chunks_map;

	compress(&buffer[HEADER], length, chunks_map);
	
	// chunk_match();
	// lzw_encode();

	// memcpy(&file[offset], output_code, output_code.size());

	// offset += output_code.size();
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
		// cdc_eff(&buffer[HEADER], length);
		compress(&buffer[HEADER], length, chunks_map);
		// memcpy(&file[offset], &buffer[HEADER], length);

		// offset += length;
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

