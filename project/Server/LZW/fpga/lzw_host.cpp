#include "lzw_host.h"
// void Store_data(const char *Filename, unsigned char *Data, unsigned int Size)
// {
//     FILE *File = fopen(Filename, "wb");
//     if (File == NULL)
//         Exit_with_error("fopen for Store_data failed");

//     if (fwrite(Data, 1, Size, File) != Size)
//         Exit_with_error("fwrite for Store_data failed");

//     if (fclose(File) != 0)
//         Exit_with_error("fclose for Store_data failed");
// }

// void Exit_with_error(const char *s)
// {
//     printf("%s\n", s);
//     exit(EXIT_FAILURE);
// }

// void Load_data(unsigned char *Data)
// {
//     unsigned int Size = FRAMES_NEW * INPUT_FRAME_SIZE;

//     FILE *File = fopen("../data/Input.bin", "rb");
//     if (File == NULL)
//         Exit_with_error("fopen for Load_data failed");

//     if (fread(Data, 1, Size, File) != Size)
//         Exit_with_error("fread for Load_data failed");

//     if (fclose(File) != 0)
//         Exit_with_error("fclose for Load_data failed");
// }

void lzw_host(unsigned char* s1, chunk* cptr, bool first_itr)
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

    unsigned char* output_from_fpga;
    output_from_fpga = (unsigned char *)malloc(MAX_CHUNK_SIZE);

    uint32_t output_size_from_fpga;

    uint32_t* ptr_chunk_size = &cptr->size;
    uint32_t* ptr_output_size = &output_size_from_fpga;

    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
     // ------------------------------------------------------------------------------------
    cl_int err;
    std::string binaryFile = "lzw_kernel.xclbin";
    unsigned fileBufSize;
    std::vector<cl::Device> devices = get_xilinx_devices();
    devices.resize(1);
    cl::Device device = devices[0];
    cl::Context context(device, NULL, NULL, NULL, &err);
    char *fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
    cl::Program program(context, devices, bins, NULL, &err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
    cl::Kernel lzw_kernel(program, "lzw_kernel", &err);
    

    // ------------------------------------------------------------------------------------
    // Step 2: Create buffers and initialize test values
    // ------------------------------------------------------------------------------------

    
    // size_t num_elements_in_chunk = cptr->size;
    size_t input_chunk_bytes = cptr->size * sizeof(unsigned char);
    
    // size_t output_elements_per_iteration = OUTPUT_FRAME_SIZE;
    size_t output_chunk_bytes = MAX_CHUNK_SIZE * sizeof(unsigned char);

    cl::Buffer input_buf;
    cl::Buffer input_size_buf;
    cl::Buffer output_buf;
    cl::Buffer output_size_buf;
    
    // Creating buffer, CL_MEM_READ_ONLY since fpga is reading input from buffers
    input_buf = cl::Buffer(context, CL_MEM_READ_ONLY, input_chunk_bytes, NULL, &err);

    // input_size_buf = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(uint32_t), NULL, &err); // Sending one variable of 32 bits

    // Creating buffer, CL_MEM_WRITE_ONLY since fpga is writing output into buffers
    output_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, output_chunk_bytes, NULL, &err);
    output_size_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, sizeof(uint32_t), NULL, &err); // Receiving one variable of 32 bits
    
    // Connecting host data to OpenCL buffer, CL_MAP_WRITE since writing from host input to fpga buffer
    s1 = (unsigned char*)q.enqueueMapBuffer(input_buf, CL_TRUE, CL_MAP_WRITE, 0, input_chunk_bytes);

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
    
    // for (int i = 0; i < FRAMES_NEW; i++)
    // {
        kernel_timer.start();
        std::vector<cl::Event> exec_events, read_events;
        cl::Event write_ev, exec_ev, read_ev;
        
	    // Scale_SW((Input + i * INPUT_FRAME_SIZE), Temp[0]);
	
        lzw_kernel.setArg(0, input_buf);
        lzw_kernel.setArg(1, cptr->size);
        lzw_kernel.setArg(2, output_buf);
        lzw_kernel.setArg(3, output_size_buf);
        
        if(first_itr) {
            q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, NULL, &write_ev);
            // q.enqueueMigrateMemObjects({input_size_buf}, 0 /* 0 means from host*/, NULL, &write_ev);
        } else {
            q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, &write_events, &write_ev);
        //     q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, &write_events, &write_ev);
        //     q.enqueueMigrateMemObjects({input_size_buf}, 0 /* 0 means from host*/, &write_events, &write_ev);
        //     write_events.pop_back();
        }
        
        write_events.push_back(write_ev);
        q.enqueueTask(lzw_kernel, &write_events, &exec_ev);
        // q.enqueueTask(lzw_kernel, &write_ev, &exec_ev);

        exec_events.push_back(exec_ev);
        q.enqueueMigrateMemObjects({output_buf, output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events, &read_ev);
        // q.enqueueMigrateMemObjects({output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events, &read_ev);

        // q.enqueueMigrateMemObjects({output_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_ev, &read_ev);
        // q.enqueueMigrateMemObjects({output_size_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_ev, &read_ev);
        
        read_events.push_back(read_ev);
        
        // Differentiate_SW(Temp[1], Temp[2]);
        // Size = Compress_SW(Temp[2], Output);
        kernel_timer.stop();

        // Store_data("Output.bin", Output, Size);
    // }

    q.finish();
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
	uint32_t chunk_header = (output_size_from_fpga << 1);
	std::cout<<"\nLZW Header: "<<chunk_header<<"\n";
	memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
	offset += sizeof(uint32_t);

    // Writing compressed chunk reveived from fpga to global file pointer
    memcpy(&file[offset], &output_from_fpga, sizeof(unsigned char));
    offset+= sizeof(unsigned char);

}