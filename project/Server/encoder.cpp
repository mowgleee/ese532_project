#include "encoder.h"


stopwatch lzw_timer;
stopwatch cdc_eff_timer;
stopwatch chunk_matching_timer;
stopwatch sha_timer;


void pin_thread_to_cpu(std::thread &t, int cpu_num)
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__APPLE__)
    return;
#else
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(cpu_num, &cpuset);
    int rc =
        pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
    {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
#endif
}

void pin_main_thread_to_cpu0()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__) || defined(__APPLE__)
    return;
#else
    pthread_t thread;
    thread = pthread_self();
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    int rc =
        pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
    {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
#endif
}


void handle_input(int argc, char* argv[], int* blocksize) {
	int x;
	extern char *optarg;

	while ((x = getopt(argc, argv, ":f:b:")) != -1) {
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

void compress(unsigned char *buffer, packet* pptr, lzw_request *kernel_cl_obj, semaphores* sems)
{
	cdc_eff_timer.start();
	makelog(VERB_DEBUG,"CDC Timer started and entering CDC \n");


	// cdc_eff(&buffer[0], pptr);

	std::vector<std::thread> ths;
    ths.push_back(std::thread(&cdc_eff, &buffer[0], pptr, sems));
	pin_thread_to_cpu(ths[0], 0);
	
	
	makelog(VERB_DEBUG,"CDC exit succesfully and timer stop \n");
	cdc_eff_timer.stop();

	sha_timer.start();




	// sha(&buffer[0], pptr);

	ths.push_back(std::thread(&sha, &buffer[0], pptr, sems));
	pin_thread_to_cpu(ths[1], 1);




	sha_timer.stop();
	
	chunk_matching_timer.start();



	// chunk_matching(pptr);
	
	ths.push_back(std::thread(&chunk_matching, pptr, sems));
	pin_thread_to_cpu(ths[2], 2);
	
	
	
	chunk_matching_timer.stop();
	
	lzw_timer.start();
	// lzw_encoding(&buffer[0], pptr);



	// lzw_host(&buffer[0], pptr, kernel_cl_obj);

	ths.push_back(std::thread(&lzw_host, &buffer[0], pptr, kernel_cl_obj, sems));
	pin_thread_to_cpu(ths[3], 3);

	lzw_timer.stop();


	for (auto &th : ths)
    {
      th.join();
    }
	
	makelog(VERB_DEBUG,"Packet Complete");
}

int main(int argc, char* argv[]) {

	pin_main_thread_to_cpu0();
	
	char *fileName[50];
	if(argc==4){
		if(argv[1]=="-"){
		*fileName = argv[3];
		}
		else{
			*fileName = argv[1];
		}
	}
	else if(argc==2)
	{
		*fileName = argv[1];
	}
	else
	{
		*fileName = "compressed.bin";
	}

	stopwatch ethernet_timer;
	stopwatch output_timer;
	unsigned char* input[NUM_PACKETS];
	int writer = 0;
	int done = 0;
	uint32_t length = 0;
	int count = 0;
	ESE532_Server server;
	uint32_t total_input_size = 0;

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
	total_input_size += length;
	output_timer.start();

	makelog(VERB_DEBUG,"Initialize Pointer to Packet \n");
	std::cout<<"packet to pointer \n";

	// Creating open cl object for host configuration and kernel run
	lzw_timer.start();
	lzw_request kernel_cl_obj;
	lzw_timer.stop();

	packet curr_packet;
	packet* pptr = &curr_packet;
	curr_packet.num = 0;
	curr_packet.size = length;


	semaphores sems;
	int ret = 0;
	// int count = 0;

	/* to be used within this process only */
	ret |= sem_init(&(sems.sem_cdc), 0, 1); 
	ret |= sem_init(&(sems.sem_sha), 0, 0);
	ret |= sem_init(&(sems.sem_dedup), 0, 0);
	ret |= sem_init(&(sems.sem_lzw), 0, 0);
	
	makelog(VERB_DEBUG,"Entering Compress \n");
	compress(&buffer[HEADER], pptr, &kernel_cl_obj, &sems);
	makelog(VERB_DEBUG,"Exit Compress Sucessfully \n");

	writer++;

	while (!done) {
		// reset ring buffer
		if (writer == NUM_PACKETS) {
			writer = 0;
		}

		ethernet_timer.start();
		server.get_packet(input[writer]);
		ethernet_timer.stop();

		// get packet
		unsigned char* buffer = input[writer];

		// decode
		done = buffer[1] & DONE_BIT_L;
		length = buffer[0] | (buffer[1] << 8);
		length &= ~DONE_BIT_H;
		total_input_size += length;
		//std::cout<<"Packet Length: "<< length<<"\n";
		makelog(VERB_DEBUG,"Packet Length %d \n", length);

		packet curr_packet;
		packet* pptr = &curr_packet;
		curr_packet.num = count;
		curr_packet.size = length;
		
		compress(&buffer[HEADER], pptr, &kernel_cl_obj, &sems);
		
		count++;	//next packet count
		writer++;
	}

	// write file to root and you can use diff tool on board
	output_timer.stop();

	FILE *outfd = fopen(*fileName, "wb");
	int bytes_written = fwrite(&file[0], 1, offset, outfd);
	printf("write file with %d\n", bytes_written);
	fclose(outfd);

	for (int i = 0; i < NUM_PACKETS; i++) {
		free(input[i]);
	}

	free(file);
	std::cout << "--------------- Key Throughputs ---------------" << std::endl;
	float ethernet_latency = ethernet_timer.latency() / 1000.0;
	float output_latency = output_timer.latency() / 1000.0;//in sec
	float op_latency_wo_lzw=(output_timer.latency()-lzw_timer.latency())/1000.0; //in sec

	float input_throughput = (total_input_size * 8 / 1000000.0) / ethernet_latency; // Mb/s
	float output_throughput = (total_input_size * 8 / 1000000.0) / output_latency;
	float output_throughput_wo_lzw = (total_input_size * 8 / 1000000.0) / op_latency_wo_lzw;


	std::cout << "Input Throughput to Encoder: " << input_throughput << " Mb/s."
			<< " (Latency: " << ethernet_latency << "s)." << std::endl;

	std::cout << "CDC Latency: " << cdc_eff_timer.latency() << "ms\t" << "AVG: "<< cdc_eff_timer.avg_latency() <<" ms"<< std::endl;
	std::cout << "SHA Latency: " << sha_timer.latency() << "ms\t" << "AVG: "<< sha_timer.avg_latency() <<" ms"<< std::endl;
	std::cout << "Chunk matching Latency: " << chunk_matching_timer.latency() << "ms\t" << "AVG: "<< chunk_matching_timer.avg_latency() <<" ms"<< std::endl;
	std::cout << "LZW Latency: " << lzw_timer.latency() << "ms\t" << "AVG: "<< lzw_timer.avg_latency() <<" ms"<< std::endl;
	std::cout << "Bitpack Latency: " << bit_pack_timer.latency() << "ms\t" << "AVG: "<< bit_pack_timer.avg_latency() <<" ms"<< std::endl;

	std::cout << "Output Throughput from Encoder: " << output_throughput << " Mb/s."
			<< " (Latency: " << output_latency << "s)." << std::endl;
	std::cout << "Output Throughput from Encoder without LZW: " << output_throughput_wo_lzw << " Mb/s."
			<< " (Latency: " << op_latency_wo_lzw<< "s)." << std::endl;

	return 0;
}
