#include "lzw_host.h"

// Best code till now

// Constructor for lzw_request class to be called once when creating object
lzw_request::lzw_request()
{
    binaryFile = "lzw_kernel.xclbin";
    devices = get_xilinx_devices();
    devices.resize(1);
    device = devices[0];
    OCL_CHECK(err, context = cl::Context(device, NULL, NULL, NULL, &err));
    fileBuf = read_binary_file(binaryFile, fileBufSize);
    bins = cl::Program::Binaries{{fileBuf, fileBufSize}};
    OCL_CHECK(err, program = cl::Program(context, devices, bins, NULL, &err));
    OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));
    OCL_CHECK(err, lzw_kernel_1 = cl::Kernel(program, "lzw_kernel:{lzw_kernel_1}", &err));
    OCL_CHECK(err, lzw_kernel_2 = cl::Kernel(program, "lzw_kernel:{lzw_kernel_2}", &err));

    input_pkt_bytes = (BLOCKSIZE + 2) * sizeof(unsigned char);    // +2 for packet header
    output_pkt_bytes = (NUM_ELEMENTS) * sizeof(unsigned char);
    chunk_boundaries_bytes = MAX_NUM_CHUNKS * sizeof(uint32_t);
    is_unique_bytes = MAX_NUM_CHUNKS * sizeof(uint8_t);
    dup_chunk_head_bytes = MAX_NUM_CHUNKS * sizeof(uint32_t);

    for(uint32_t i = 0; i < NUM_PACKETS/2; i++)
    {
        posix_memalign((void**)&input_to_fpga_1[i], 4096, input_pkt_bytes);
        // output_from_fpga = (unsigned char *)calloc(BLOCKSIZE*13/8, sizeof(unsigned char));
        posix_memalign((void**)&output_from_fpga_1[i], 4096, output_pkt_bytes);
        // ptr_output_size = (uint32_t *)calloc(1, sizeof(uint32_t));
        posix_memalign((void**)&ptr_output_size_1[i], 4096, sizeof(uint32_t));
        posix_memalign((void**)&chunk_boundaries_1[i], 4096, sizeof(uint32_t) * (MAX_NUM_CHUNKS));
        posix_memalign((void**)&is_unique_1[i], 4096, sizeof(uint8_t) * (MAX_NUM_CHUNKS));
        posix_memalign((void**)&dup_chunk_head_1[i], 4096, sizeof(uint32_t) * (MAX_NUM_CHUNKS));



        //for 2nd kernel
        posix_memalign((void**)&input_to_fpga_2[i], 4096, input_pkt_bytes);
        // output_from_fpga = (unsigned char *)calloc(BLOCKSIZE*13/8, sizeof(unsigned char));
        posix_memalign((void**)&output_from_fpga_2[i], 4096, output_pkt_bytes);
        // ptr_output_size = (uint32_t *)calloc(1, sizeof(uint32_t));
        posix_memalign((void**)&ptr_output_size_2[i], 4096, sizeof(uint32_t));
        posix_memalign((void**)&chunk_boundaries_2[i], 4096, sizeof(uint32_t) * (MAX_NUM_CHUNKS));
        posix_memalign((void**)&is_unique_2[i], 4096, sizeof(uint8_t) * (MAX_NUM_CHUNKS));
        posix_memalign((void**)&dup_chunk_head_2[i], 4096, sizeof(uint32_t) * (MAX_NUM_CHUNKS));
    }
    // chunk_boundaries = (uint32_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint32_t));
    // dup_chunk_head = (uint32_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint32_t));
    // is_unique = (uint8_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint8_t));

    for(uint32_t i = 0; i < NUM_PACKETS; i=i+2)
    {
        OCL_CHECK(err, input_buf_1[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_pkt_bytes, input_to_fpga_1[i], &err));
        OCL_CHECK(err, output_buf_1[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, output_pkt_bytes, output_from_fpga_1[i], &err));
        OCL_CHECK(err, output_size_buf_1[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(uint32_t), ptr_output_size_1[i], &err));
        OCL_CHECK(err, chunk_boundaries_buf_1[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, chunk_boundaries_bytes, chunk_boundaries_1[i], &err));
        OCL_CHECK(err, is_unique_buf_1[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, is_unique_bytes, is_unique_1[i], &err));
        OCL_CHECK(err, dup_chunk_head_buf_1[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, dup_chunk_head_bytes, dup_chunk_head_1[i], &err));



        OCL_CHECK(err, input_buf_2[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_pkt_bytes, input_to_fpga_2[i], &err));
        OCL_CHECK(err, output_buf_2[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, output_pkt_bytes, output_from_fpga_2[i], &err));
        OCL_CHECK(err, output_size_buf_2[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(uint32_t), ptr_output_size_2[i], &err));
        OCL_CHECK(err, chunk_boundaries_buf_2[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, chunk_boundaries_bytes, chunk_boundaries_2[i], &err));
        OCL_CHECK(err, is_unique_buf_2[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, is_unique_bytes, is_unique_2[i], &err));
        OCL_CHECK(err, dup_chunk_head_buf_2[i] = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, dup_chunk_head_bytes, dup_chunk_head_2[i], &err));    
    }
    makelog(VERB_DEBUG, "LZW object constructor completed.\n");
}

lzw_request::~lzw_request()
{
    for(uint32_t i = 0; i < NUM_PACKETS; i=i+2)
    {
        free(input_to_fpga_1[i]);
        free(output_from_fpga_1[i]);
        free(ptr_output_size_1[i]);
        free(chunk_boundaries_1[i]);
        free(dup_chunk_head_1[i]);
        free(is_unique_1[i]);

        free(input_to_fpga_2[i]);
        free(output_from_fpga_2[i]);
        free(ptr_output_size_2[i]);
        free(chunk_boundaries_2[i]);
        free(dup_chunk_head_2[i]);
        free(is_unique_2[i]);
    }
}

void lzw_request::set_args_1(uint32_t num_chunks, uint32_t count)
{
    OCL_CHECK(err, err = lzw_kernel_1.setArg(0, input_buf[count]));
    OCL_CHECK(err, err = lzw_kernel_1.setArg(1, chunk_boundaries_buf[count]));
    OCL_CHECK(err, err = lzw_kernel_1.setArg(2, num_chunks));
    OCL_CHECK(err, err = lzw_kernel_1.setArg(3, is_unique_buf[count]));
    OCL_CHECK(err, err = lzw_kernel_1.setArg(4, output_buf[count]));
    OCL_CHECK(err, err = lzw_kernel_1.setArg(5, output_size_buf[count]));
    OCL_CHECK(err, err = lzw_kernel_1.setArg(6, dup_chunk_head_buf[count]));
}

void lzw_request::set_args_2(uint32_t num_chunks, uint32_t count)
{
    OCL_CHECK(err, err = lzw_kernel_2.setArg(0, input_buf_2[count]));
    OCL_CHECK(err, err = lzw_kernel_2.setArg(1, chunk_boundaries_buf_2[count]));
    OCL_CHECK(err, err = lzw_kernel_2.setArg(2, num_chunks));
    OCL_CHECK(err, err = lzw_kernel_2.setArg(3, is_unique_buf_2[count]));
    OCL_CHECK(err, err = lzw_kernel_2.setArg(4, output_buf_2[count]));
    OCL_CHECK(err, err = lzw_kernel_2.setArg(5, output_size_buf_2[count]));
    OCL_CHECK(err, err = lzw_kernel_2.setArg(6, dup_chunk_head_buf_2[count]));
}

void lzw_request::run_1(uint32_t count_1)
{
    kernel_mem_timer.start();
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({input_buf_1[count_1], chunk_boundaries_buf_1[count_1], is_unique_buf_1[count_1], dup_chunk_head_buf_1[count_1]}, 0 /* 0 means from host*/, NULL, &write_ev_1));
    write_events_1.push_back(write_ev_1);
    kernel_mem_timer.stop();
    printf("Enqueueing the kernel.\n");
    kernel_timer.start();
    OCL_CHECK(err, err = q.enqueueTask(lzw_kernel_1, &write_events_1, &exec_ev_1));
    exec_events_1.push_back(exec_ev_1);
    kernel_timer.stop();
    kernel_mem_timer.start();
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({output_buf_1[count_1], output_size_buf_1[count_1]}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events_1, &read_ev_1));
    kernel_mem_timer.stop();
    read_events_1.push_back(read_ev_1);
}

void lzw_request::run_2(uint32_t count_2)
{
    kernel_mem_timer.start();
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({input_buf_2[count_2], chunk_boundaries_buf_2[count_2], is_unique_buf_2[count_2], dup_chunk_head_buf_2[count_2]}, 0 /* 0 means from host*/, NULL, &write_ev_2));
    write_events_2.push_back(write_ev_2);
    kernel_mem_timer.stop();
    printf("Enqueueing the kernel.\n");
    kernel_timer.start();
    OCL_CHECK(err, err = q.enqueueTask(lzw_kernel_2, &write_events_2, &exec_ev_2));
    exec_events_2.push_back(exec_ev_2);
    kernel_timer.stop();
    kernel_mem_timer.start();
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({output_buf_2[count_2], output_size_buf_2[count_2]}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events_2, &read_ev_2));
    kernel_mem_timer.stop();
    read_events_2.push_back(read_ev_2);
}

void lzw_request::finish()
{
    q.finish();
}

void lzw_request::wait(uint32_t count)
{
    OCL_CHECK(err, err = read_events_1[count].wait());
}

void lzw_host(lzw_request *kernel_cl_obj, semaphores* sems, packet** packarray)
{
	// lzw_request kernel_cl_obj;

    uint32_t write_counter = 0;
    uint32_t total_pkts_read = 0;

    while(1)
    {
        static uint32_t count = 0;
        sem_wait(&(sems->sem_lzw));
        lzw_sem_timer.start();
        packet* pptr;
        uint8_t* buff;
        // uint8_t* buff_2;
        buff = input[count % NUM_PACKETS];
        // buff_2 = input[(count+1) % NUM_PACKETS];
		// buff = &buff[0];   ///for memory allignment. send full packet including header
        pptr = packarray[count % NUM_PACKETS];
        // pptr_2 = packarray[(count+1) % NUM_PACKETS];
        makelog(VERB_DEBUG,"Semaphore for LZW Received");
        makelog(VERB_DEBUG,"LZW Packet Info:\n LZW Packet Num: %d\n LZW Packet Size: %d\n LZW No of Chunks in Packet: %d\n LZW Count: %d\n",pptr->num,pptr->size,pptr->num_of_chunks,count);


        uint32_t packet_size = pptr->size;
        uint32_t num_chunks = pptr->num_of_chunks;

        uint32_t curr_buff_num = (count % NUM_PACKETS);

        // uint32_t packet_size_2 = pptr_2->size;
        // uint32_t num_chunks_2 = pptr_2->num_of_chunks;

        // uint32_t curr_buff_num_2 = ((count+1) % NUM_PACKETS);

        if(count%2 == 0)
        {
            for(uint32_t i = 0; i < num_chunks; i++)
            {
                kernel_cl_obj->chunk_boundaries_1[curr_buff_num/2][i] = pptr->curr_chunk[i].upper_bound;
                kernel_cl_obj->dup_chunk_head_1[curr_buff_num/2][i] = pptr->curr_chunk[i].header;
                kernel_cl_obj->is_unique_1[curr_buff_num/2][i] = pptr->curr_chunk[i].is_unique;
            }

            memcpy(kernel_cl_obj->input_to_fpga_1[curr_buff_num/2], buff, packet_size + 2);     // Copying 2 extra bytes of packet HEADER

            kernel_cl_obj->set_args_1(num_chunks, curr_buff_num/2);

            kernel_cl_obj->run_1(curr_buff_num/2);
        }

        else
        {
            for(uint32_t i = 0; i < num_chunks; i++)
            {
                kernel_cl_obj->chunk_boundaries_2[curr_buff_num/2][i] = pptr_1->curr_chunk[i].upper_bound;
                kernel_cl_obj->dup_chunk_head_2[curr_buff_num/2][i] = pptr_1->curr_chunk[i].header;
                kernel_cl_obj->is_unique_2[curr_buff_num/2][i] = pptr_1->curr_chunk[i].is_unique;
            }

            memcpy(kernel_cl_obj->input_to_fpga_2[curr_buff_num/2], buff_1, packet_size + 2);     // Copying 2 extra bytes of packet HEADER

            kernel_cl_obj->set_args_2(num_chunks, curr_buff_num);

            kernel_cl_obj->run_2(curr_buff_num/2);
        }

        // kernel_cl_obj->run_1(curr_buff_num, curr_buff_num_2);

        // ------------------------------------------------------------------------------------
        // Step 4: Release Allocated Resources
        // ------------------------------------------------------------------------------------

        // std::cout << "Output size received from kernel: " << *(kernel_cl_obj->ptr_output_size) << std::endl;
        // std::cout << "Average latency of lzw_hw_kernel call only: " << kernel_timer.avg_latency() << std::endl;
        // std::cout << "Total latency of lzw_kernel call only: " << kernel_timer.latency() << std::endl;
        // std::cout << "Average latency of lzw_hw_kernel MEM TRANSFER only: " << kernel_mem_timer.avg_latency() << std::endl;
        // std::cout << "Total latency of lzw_kernel MEM TRANSFER only: " << kernel_mem_timer.latency() << std::endl;
        // std::cout << "Average latency of lzw_hw_kernel initialization only: " << kernel_init_timer.avg_latency() << std::endl;
        // std::cout << "Total latency of lzw_kernel initialization only: " << kernel_init_timer.latency() << std::endl;

        // Writing compressed chunk reveived from fpga to global file pointer
        if(count >= 49)
        {
            kernel_cl_obj->wait(total_pkts_read);
            memcpy(&file[offset], kernel_cl_obj->output_from_fpga[write_counter], *(kernel_cl_obj->ptr_output_size[write_counter]));
            offset+= *(kernel_cl_obj->ptr_output_size[write_counter]);
            
            if(write_counter < 49) {
                write_counter++;
            } else {
                write_counter = 0;
            }

            total_pkts_read++;
        }

        std::cout<<"\nLZW PACKET DONE\n";
        sem_post(&(sems->sem_getpacket));
        std::cout<<"\nPosted getpacket semaphore for packet: " << count << "\n";
        if(count == total_packets)
        {
            // uint32_t flag = write_counter - 1;
            while(total_pkts_read != (total_packets + 1))
            {
                kernel_cl_obj->wait(total_pkts_read);
                memcpy(&file[offset], kernel_cl_obj->output_from_fpga[write_counter], *(kernel_cl_obj->ptr_output_size[write_counter]));
                offset+= *(kernel_cl_obj->ptr_output_size[write_counter]);
                
                if(write_counter < 49) {
                    write_counter++;
                } else {
                    write_counter = 0;
                }

                total_pkts_read++;
            }
            kernel_cl_obj->finish();
            std::cout<<"LZW count= "<<count<<" total_packets= "<<total_packets<<"\n";
            // free(chunk_boundaries);
            // free(is_unique);
            // free(dup_chunk_head);
            lzw_sem_timer.stop();
            return;
        }
        count++;
    }
}