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
    OCL_CHECK(err, q = cl::CommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE/* | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE*/, &err));
    OCL_CHECK(err, lzw_kernel = cl::Kernel(program, "lzw_kernel", &err));

    // output_from_fpga = (unsigned char *)calloc(MAX_NUM_CHUNKS*13/8, sizeof(unsigned char));
    ptr_output_size = (uint32_t *)calloc(1, sizeof(uint32_t));
    chunk_boundaries = (uint32_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint32_t));
    dup_chunk_head = (uint32_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint32_t));
    is_unique = (uint8_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint8_t));
}

lzw_request::~lzw_request()
{
    // free(output_from_fpga);
    free(chunk_boundaries);
    free(dup_chunk_head);
    free(is_unique);
    free(ptr_output_size);
}

void lzw_request::init(uint32_t packet_size, uint32_t num_chunks, unsigned char* input_to_fpga, uint8_t* file_ptr)//, uint32_t* ptr_output_size)
{
    input_pkt_bytes = packet_size * sizeof(unsigned char);

    chunk_boundaries_bytes = num_chunks * sizeof(uint32_t);
    is_unique_bytes = num_chunks * sizeof(uint8_t);
    dup_chunk_head_bytes = num_chunks * sizeof(uint32_t);


    OCL_CHECK(err, input_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_pkt_bytes, input_to_fpga, &err));
    OCL_CHECK(err, output_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, output_pkt_bytes/*CONST CLASS MEMBER*/, file_ptr, &err));
    OCL_CHECK(err, output_size_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(uint32_t), ptr_output_size, &err));

    OCL_CHECK(err, chunk_boundaries_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, chunk_boundaries_bytes, chunk_boundaries, &err));
    OCL_CHECK(err, is_unique_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, is_unique_bytes, is_unique, &err));
    OCL_CHECK(err, dup_chunk_head_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, dup_chunk_head_bytes, dup_chunk_head, &err));

    OCL_CHECK(err, err = lzw_kernel.setArg(0, input_buf));
    OCL_CHECK(err, err = lzw_kernel.setArg(1, chunk_boundaries_buf));
    OCL_CHECK(err, err = lzw_kernel.setArg(2, num_chunks));
    OCL_CHECK(err, err = lzw_kernel.setArg(3, is_unique_buf));
    OCL_CHECK(err, err = lzw_kernel.setArg(4, output_buf));
    OCL_CHECK(err, err = lzw_kernel.setArg(5, output_size_buf));
    OCL_CHECK(err, err = lzw_kernel.setArg(6, dup_chunk_head_buf));
}

void lzw_request::run()
{
    kernel_mem_timer.start();
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({input_buf, chunk_boundaries_buf, is_unique_buf, dup_chunk_head_buf}, 0 /* 0 means from host*/, NULL, &write_ev));
    write_events.push_back(write_ev);
    kernel_mem_timer.stop();
    printf("Enqueueing the kernel.\n");
    kernel_timer.start();
    OCL_CHECK(err, err = q.enqueueTask(lzw_kernel, &write_events, &exec_ev));
    exec_events.push_back(exec_ev);
    kernel_timer.stop();
    kernel_mem_timer.start();
    OCL_CHECK(err, err = q.enqueueMigrateMemObjects({output_buf, output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events, &read_ev));
    OCL_CHECK(err, err = read_ev.wait());
    kernel_mem_timer.stop();
    read_events.push_back(read_ev);
    q.finish();
}

void lzw_host(lzw_request *kernel_cl_obj, semaphores* sems, packet** packarray)
{
    while(1)
    {
        static uint32_t count = 0;
        sem_wait(&(sems->sem_lzw));
        lzw_sem_timer.start();
        packet* pptr;
        uint8_t* buff;
        buff = input[count%NUM_PACKETS];
		buff = &buff[HEADER];        
        pptr = packarray[count%NUM_PACKETS];
        makelog(VERB_DEBUG,"Semaphore for LZW Received");
        makelog(VERB_DEBUG,"LZW Packet Info:\n LZW Packet Num: %d\n LZW Packet Size: %d\n LZW No of Chunks in Packet: %d\n LZW Count: %d\n",pptr->num,pptr->size,pptr->num_of_chunks,count);


        uint32_t packet_size = pptr->size;
        uint32_t num_chunks = pptr->num_of_chunks;

        for(uint32_t i = 0; i < num_chunks; i++)
        {
            kernel_cl_obj->chunk_boundaries[i] = pptr->curr_chunk[i].upper_bound;
            kernel_cl_obj->dup_chunk_head[i] = pptr->curr_chunk[i].header;
            kernel_cl_obj->is_unique[i] = pptr->curr_chunk[i].is_unique;
        }
            
        kernel_init_timer.start();
        kernel_cl_obj->init(packet_size, num_chunks, &buff[pptr->curr_chunk[0].lower_bound], &file[offset]);//, kernel_cl_obj->ptr_output_size);
        kernel_init_timer.stop();

        kernel_cl_obj->run();

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

        // Writing chunk header to global file pointer
        // uint32_t chunk_header = (*(kernel_cl_obj->ptr_output_size) << 1);
        //std::cout<<"\nLZW Header: "<<chunk_header<<"\n";
        // memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
        // offset += sizeof(uint32_t);

        // Writing compressed chunk reveived from fpga to global file pointer
        // memcpy(&file[offset], kernel_cl_obj->output_from_fpga, *(kernel_cl_obj->ptr_output_size));
        offset+= *(kernel_cl_obj->ptr_output_size);

        std::cout<<"\nLZW PACKET DONE\n";
        lzw_sem_timer.stop();
        sem_post(&(sems->sem_getpacket));
        std::cout<<"\nPosted getpacket semaphore\n";
        if(count == total_packets)
        {
            std::cout<<"LZW count= "<<count<<" total_packets= "<<total_packets<<"\n";
            return;
        }
        count++;
    }
}