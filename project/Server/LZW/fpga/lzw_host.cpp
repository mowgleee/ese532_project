#include "lzw_host.h"

void lzw_host(unsigned char* s1, chunk* cptr)
{   
    // unsigned char *Input = (unsigned char *)malloc(FRAMES_NEW * INPUT_FRAME_SIZE);
    // unsigned char *Output = (unsigned char *)malloc(FRAMES_NEW * OUTPUT_FRAME_SIZE);
    
    // if (Input == NULL)
    //   Exit_with_error("malloc failed for Input");

    // if (Output == NULL)
    //   Exit_with_error("malloc failed for Output");
    
    // unsigned char *Temp[STAGES - 1];
    
    // for (int i = 0; i < (STAGES - 1); i++) {
    // 	Temp[i] = (unsigned char *)malloc(SCALED_FRAME_SIZE);
    // }
    
    // Load_data(Input);

    // size_t num_elements_in_chunk = cptr->size;
    size_t input_chunk_bytes = cptr->size * sizeof(unsigned char);
    
    // size_t output_elements_per_iteration = OUTPUT_FRAME_SIZE;
    size_t output_chunk_bytes = MAX_CHUNK_SIZE * sizeof(unsigned char);

    unsigned char* input_to_fpga = (unsigned char *)calloc(cptr->size, sizeof(unsigned char));
    memcpy(input_to_fpga, s1, cptr->size);

    unsigned char* output_from_fpga = (unsigned char *)calloc(MAX_CHUNK_SIZE, sizeof(unsigned char));


    // uint32_t* ptr_chunk_size = &cptr->size;

    uint32_t* ptr_output_size = (uint32_t *)calloc(1, sizeof(uint32_t));

    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
     // ------------------------------------------------------------------------------------
    cl_int err;
    std::string binaryFile = "lzw_kernel.xclbin";
    unsigned fileBufSize;
    std::vector<cl::Device> devices = get_xilinx_devices();
    devices.resize(1);
    cl::Device device = devices[0];

    OCL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
    // cl::Context context(device, NULL, NULL, NULL, &err);
    
    char *fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};

    OCL_CHECK(err, cl::Program program(context, devices, bins, NULL, &err));
    // cl::Program program(context, devices, bins, NULL, &err);

    OCL_CHECK(err, cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE/* | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE*/, &err));
    // cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE/* | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE*/, &err);
    
    OCL_CHECK(err, cl::Kernel lzw_kernel(program, "lzw_kernel", &err));
    // cl::Kernel lzw_kernel(program, "lzw_kernel", &err);
    

    // ------------------------------------------------------------------------------------
    // Step 2: Create buffers and initialize test values
    // ------------------------------------------------------------------------------------

    OCL_CHECK(err, cl::Buffer input_buf(context, CL_MEM_READ_ONLY, input_chunk_bytes, NULL, &err));
    // cl::Buffer input_buf(context, CL_MEM_READ_ONLY, input_chunk_bytes, NULL, &err);

    // cl::Buffer input_size_buf;
    OCL_CHECK(err, cl::Buffer output_buf(context, CL_MEM_WRITE_ONLY, output_chunk_bytes, NULL, &err));
    // cl::Buffer output_buf(context, CL_MEM_WRITE_ONLY, output_chunk_bytes, NULL, &err);

    OCL_CHECK(err, cl::Buffer output_size_buf(context, CL_MEM_WRITE_ONLY, sizeof(uint32_t), NULL, &err););
    // cl::Buffer output_size_buf(context, CL_MEM_WRITE_ONLY, sizeof(uint32_t), NULL, &err); // Receiving one variable of 32 bits;
    
    // Creating buffer, CL_MEM_READ_ONLY since fpga is reading input from buffers
//    // input_buf = cl::Buffer(context, CL_MEM_READ_ONLY, input_chunk_bytes, NULL, &err);

    // input_size_buf = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(uint32_t), NULL, &err); // Sending one variable of 32 bits

    // Creating buffer, CL_MEM_WRITE_ONLY since fpga is writing output into buffers
//    // output_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, output_chunk_bytes, NULL, &err);
//    // output_size_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(uint32_t), NULL, &err); // Receiving one variable of 32 bits
    
    // Connecting host data to OpenCL buffer, CL_MAP_WRITE since writing from host input to fpga buffer
    input_to_fpga = (unsigned char*)q.enqueueMapBuffer(input_buf, CL_TRUE, CL_MAP_WRITE, 0, input_chunk_bytes);

    // ptr_chunk_size = (uint32_t*)q.enqueueMapBuffer(input_size_buf, CL_TRUE, CL_MAP_WRITE, 0, sizeof(uint32_t));

    // Connecting host data to OpenCL buffer, CL_MAP_READ since reading from fpga buffer to host
    output_from_fpga = (unsigned char*)q.enqueueMapBuffer(output_buf, CL_TRUE, CL_MAP_READ, 0, output_chunk_bytes);
    ptr_output_size = (uint32_t*)q.enqueueMapBuffer(output_size_buf, CL_TRUE, CL_MAP_READ, 0, sizeof(uint32_t));

    // ------------------------------------------------------------------------------------
    // Step 3: Run the kernel
    // ------------------------------------------------------------------------------------

    // void lzw_kernel(unsigned char* input, uint32_t size, uint8_t* output_code_packed, uint32_t* output_code_size)

    std::vector<cl::Event> write_events;
    // kernel_total_time.start();

    stopwatch kernel_timer;
    
    // for (int i = 0; i < FRAMES_NEW; i++)
    // {
        kernel_timer.start();
        std::vector<cl::Event> exec_events, read_events;
        cl::Event write_ev, exec_ev, read_ev;
        
	    // Scale_SW((Input + i * INPUT_FRAME_SIZE), Temp[0]);
	
        OCL_CHECK(err, err = lzw_kernel.setArg(0, input_buf));
        OCL_CHECK(err, err = lzw_kernel.setArg(1, cptr->size));
        OCL_CHECK(err, err = lzw_kernel.setArg(2, output_buf));
        OCL_CHECK(err, err = lzw_kernel.setArg(3, output_size_buf));

        // lzw_kernel.setArg(0, input_buf);
        // lzw_kernel.setArg(1, cptr->size);
        // lzw_kernel.setArg(2, output_buf);
        // lzw_kernel.setArg(3, output_size_buf);
        
        // if(first_itr) {

            OCL_CHECK(err, err = q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, NULL, &write_ev));
            // q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, NULL, &write_ev);

            // q.enqueueMigrateMemObjects({input_size_buf}, 0 /* 0 means from host*/, NULL, &write_ev);
        // } else {
        //     q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, &write_events, &write_ev);
        //     q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, &write_events, &write_ev);
        //     q.enqueueMigrateMemObjects({input_size_buf}, 0 /* 0 means from host*/, &write_events, &write_ev);
        //     write_events.pop_back();
        // }

        write_events.push_back(write_ev);

        printf("Enqueueing the kernel.\n");
        // This event needs to wait for the write buffer operations to complete
        // before executing. We are sending the write_events into its wait list to
        // ensure that the order of operations is correct.
        //Launch the Kernel
        // std::vector<cl::Event> waitList;
        // waitList.push_back(write_event);

        // q.enqueueNDRangeKernel(krnl_vadd, 0, 1, 1, &waitList, &kernel_events[flag]);

        OCL_CHECK(err, err = q.enqueueTask(lzw_kernel, &write_events, &exec_ev));
        // q.enqueueTask(lzw_kernel, &write_events, &exec_ev);

        // q.enqueueTask(lzw_kernel, &write_ev, &exec_ev);

        exec_events.push_back(exec_ev);

        OCL_CHECK(err, err = q.enqueueMigrateMemObjects({output_buf, output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events, &read_ev));
        // q.enqueueMigrateMemObjects({output_buf, output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events, &read_ev);

        // q.enqueueMigrateMemObjects({output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events, &read_ev);

        // q.enqueueMigrateMemObjects({output_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_ev, &read_ev);
        // q.enqueueMigrateMemObjects({output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_ev, &read_ev);
        
        OCL_CHECK(err, err = read_ev.wait());
        // read_ev.wait();

        read_events.push_back(read_ev);
        
        // Differentiate_SW(Temp[1], Temp[2]);
        // Size = Compress_SW(Temp[2], Output);
        kernel_timer.stop();

        // Store_data("Output.bin", Output, Size);
    // }

    q.finish();

    std::cout << "Output size received from kernel: " << *ptr_output_size << std::endl;

    std::cout << "Data received from kernel: ";
    for(uint32_t i = 0; i < 10; i++) {
        std::cout << output_from_fpga[i];
    }
    std::cout << "\n";

    // kernel_total_time.stop();
    std::cout << "Average latency of lzw_hw_kernel: " << kernel_timer.avg_latency() << std::endl;
    std::cout << "TOtal latency of lzw_kernel: " << kernel_timer.latency() << std::endl;

    // ------------------------------------------------------------------------------------
    // Step 4: Release Allocated Resources
    // ------------------------------------------------------------------------------------

    // free(Input);
    // free(Output);

    // delete[] fileBuf;

    // uint32_t bytes_written = ceil(output_code_size*13.0/8.0);

    // Writing chunk header to global file pointer
	uint32_t chunk_header = (*ptr_output_size << 1);
	std::cout<<"\nLZW Header: "<<chunk_header<<"\n";
	memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    // Writing compressed chunk reveived from fpga to global file pointer
    memcpy(&file[offset], output_from_fpga, *ptr_output_size);
    offset+= *ptr_output_size;

}