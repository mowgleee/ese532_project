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

    output_from_fpga = (unsigned char *)calloc(MAX_NUM_CHUNKS*13/8, sizeof(unsigned char));
    ptr_output_size = (uint32_t *)calloc(1, sizeof(uint32_t));
 
    chunk_boundaries = (uint32_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint32_t));
    dup_chunk_head = (uint32_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint32_t));
    is_unique = (uint8_t*)calloc(MAX_NUM_CHUNKS, sizeof(uint8_t));
}

lzw_request::~lzw_request()
{
    free(output_from_fpga);
    free(ptr_output_size);
}

void lzw_request::init(uint32_t packet_size, uint32_t num_chunks, unsigned char* input_to_fpga)//, uint32_t* ptr_output_size)
{
    input_chunk_bytes = packet_size * sizeof(unsigned char);

    chunk_boundaries_bytes = num_chunks * sizeof(uint32_t);
    is_unique_bytes = num_chunks * sizeof(uint8_t);
    dup_chunk_head_bytes = num_chunks * sizeof(uint32_t);


    OCL_CHECK(err, input_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_chunk_bytes, input_to_fpga, &err));
    OCL_CHECK(err, output_buf = cl::Buffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, output_chunk_bytes/*CONST CLASS MEMBER*/, output_from_fpga, &err));
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


    // lzw_kernel(unsigned char* input_packet,
	// 			uint32_t* chunk_bndry,
	// 			uint32_t num_chunks,
	// 			uint8_t* is_chunk_unique,
	// 			uint8_t* output_file,
	// 			uint32_t* output_size,
	// 			uint32_t* dup_chunk_head)
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


        // uint32_t chunk_header = 0;
        // for(uint32_t chunk_num = 0; chunk_num < pptr->num_of_chunks; chunk_num++)
        // {
        //     if(pptr->curr_chunk[chunk_num].is_unique)
        //     {       
                uint32_t packet_size = pptr->size;
                uint32_t num_chunks = pptr->num_of_chunks;

                // uint32_t* pkt_chunk_boundaries;
                // uint32_t* pkt_dup_chunk_head;
                // uint8_t* pkt_is_unique;

                for(uint32_t i = 0; i < num_chunks; i++)
                {
                    kernel_cl_obj->chunk_boundaries[i] = pptr->curr_chunk[i].upper_bound;
                    kernel_cl_obj->dup_chunk_head[i] = pptr->curr_chunk[i].header;
                    kernel_cl_obj->is_unique[i] = pptr->curr_chunk[i].is_unique;
                }

                // unsigned char* input_to_fpga = (unsigned char *)calloc(curr_chunk_size, sizeof(unsigned char));
                // memcpy(input_to_fpga, &buff[pptr->curr_chunk[chunk_num].lower_bound], curr_chunk_size);
                // unsigned char* output_from_fpga = (unsigned char *)calloc(MAX_CHUNK_SIZE, sizeof(unsigned char));
                // uint32_t* ptr_output_size = (uint32_t *)calloc(1, sizeof(uint32_t));
            
                kernel_init_timer.start();
                kernel_cl_obj->init(packet_size, num_chunks, &buff[pptr->curr_chunk[0].lower_bound]);//, kernel_cl_obj->ptr_output_size);
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
                memcpy(&file[offset], kernel_cl_obj->output_from_fpga, *(kernel_cl_obj->ptr_output_size));
                offset+= *(kernel_cl_obj->ptr_output_size);
        //     }
        //     else
        //     {
        //         chunk_header = pptr->curr_chunk[chunk_num].header;
        //         memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
        //         offset += sizeof(uint32_t);
        //     }
        // }
        // sem_post(&(sems->sem_cdc));

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

// void lzw_host(unsigned char* s1, chunk* cptr)
// {

//     unsigned char* input_to_fpga = (unsigned char *)calloc(cptr->size, sizeof(unsigned char));
//     memcpy(input_to_fpga, s1, cptr->size);

//     unsigned char* output_from_fpga = (unsigned char *)calloc(MAX_CHUNK_SIZE, sizeof(unsigned char));

//     uint32_t* ptr_output_size = (uint32_t *)calloc(1, sizeof(uint32_t));

//     // ------------------------------------------------------------------------------------
//     // Step 1: Initialize the OpenCL environment
//      // ------------------------------------------------------------------------------------
//     cl_int err;
//     std::string binaryFile = "lzw_kernel.xclbin";
//     unsigned fileBufSize;
//     std::vector<cl::Device> devices = get_xilinx_devices();
//     devices.resize(1);
//     cl::Device device = devices[0];

//     OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    
//     char *fileBuf = read_binary_file(binaryFile, fileBufSize);
//     cl::Program::Binaries bins{{fileBuf, fileBufSize}};

//     OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));

//     OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE/* | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE*/, &err));
    
//     OCL_CHECK(err, cl::Kernel lzw_kernel(program, "lzw_kernel", &err));
    

//     // ------------------------------------------------------------------------------------
//     // Step 2: Create buffers and initialize test values
//     // ------------------------------------------------------------------------------------

    
//     // size_t num_elements_in_chunk = cptr->size;
//     size_t input_chunk_bytes = (cptr->size) * sizeof(unsigned char);
    
//     size_t output_chunk_bytes = MAX_CHUNK_SIZE * sizeof(unsigned char);

//     OCL_CHECK(err, cl::Buffer input_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, input_chunk_bytes, input_to_fpga, &err));

//     // cl::Buffer input_size_buf;
//     OCL_CHECK(err, cl::Buffer output_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, output_chunk_bytes, output_from_fpga, &err));

//     OCL_CHECK(err, cl::Buffer output_size_buf(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(uint32_t), ptr_output_size, &err););

//     // ------------------------------------------------------------------------------------
//     // Step 3: Run the kernel
//     // ------------------------------------------------------------------------------------

//     std::vector<cl::Event> write_events;

//     stopwatch kernel_timer;
    
//     kernel_timer.start();
//     std::vector<cl::Event> exec_events, read_events;
//     cl::Event write_ev, exec_ev, read_ev;
    
//     // Scale_SW((Input + i * INPUT_FRAME_SIZE), Temp[0]);

//     OCL_CHECK(err, err = lzw_kernel.setArg(0, input_buf));
//     OCL_CHECK(err, err = lzw_kernel.setArg(1, cptr->size));
//     OCL_CHECK(err, err = lzw_kernel.setArg(2, output_buf));
//     OCL_CHECK(err, err = lzw_kernel.setArg(3, output_size_buf));

//     OCL_CHECK(err, err = q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, NULL, &write_ev));


//     write_events.push_back(write_ev);

//     printf("Enqueueing the kernel.\n");
//     // This event needs to wait for the write buffer operations to complete
//     // before executing. We are sending the write_events into its wait list to
//     // ensure that the order of operations is correct.
//     //Launch the Kernel
//     OCL_CHECK(err, err = q.enqueueTask(lzw_kernel, &write_events, &exec_ev));

//     exec_events.push_back(exec_ev);

//     OCL_CHECK(err, err = q.enqueueMigrateMemObjects({output_buf, output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events, &read_ev));

//     OCL_CHECK(err, err = read_ev.wait());
//     // read_ev.wait();

//     read_events.push_back(read_ev);
    
//     kernel_timer.stop();


//     q.finish();

//     std::cout << "Output size received from kernel: " << *ptr_output_size << std::endl;

//     // std::cout << "Data received from kernel: ";
//     // for(uint32_t i = 0; i < 100; i++) {
//     //     std::cout << output_from_fpga[i];
//     // }
//     // std::cout << "\n";

//     std::cout << "Average latency of lzw_hw_kernel: " << kernel_timer.avg_latency() << std::endl;
//     std::cout << "TOtal latency of lzw_kernel: " << kernel_timer.latency() << std::endl;

//     // ------------------------------------------------------------------------------------
//     // Step 4: Release Allocated Resources
//     // ------------------------------------------------------------------------------------

//     // free(Input);
//     // free(Output);

//     // delete[] fileBuf;

//     // uint32_t bytes_written = ceil(output_code_size*13.0/8.0);

//     // Writing chunk header to global file pointer
//     uint32_t chunk_header = (*ptr_output_size << 1);
// 	std::cout<<"\nLZW Header: "<<chunk_header<<"\n";
// 	memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
// 	offset += sizeof(uint32_t);

//     // Writing compressed chunk reveived from fpga to global file pointer
//     memcpy(&file[offset], output_from_fpga, *ptr_output_size);
//     offset+= *ptr_output_size;
// }