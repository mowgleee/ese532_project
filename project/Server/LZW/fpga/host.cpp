#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#define FRAMES_NEW 200

// #include <CL/cl2.hpp>
// #include <cstdint>
// #include <cstdlib>
// #include <fstream>
// #include <iostream>
// #include <unistd.h>
// #include <vector>

#include "../../Common/common.h"

#include <CL/cl2.hpp>
#include <cstdint>
#include <cstdlib>

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

// int main(int argc, char *argv[])
void lzw_encoding(unsigned char* s1, chunk* cptr)
{
    int Size = 0;
    
    // unsigned char *Input = (unsigned char *)malloc(FRAMES_NEW * INPUT_FRAME_SIZE);
    // unsigned char *Output = (unsigned char *)malloc(FRAMES_NEW * OUTPUT_FRAME_SIZE);
    
    // if (Input == NULL)
    //   Exit_with_error("malloc failed for Input");

    // if (Output == NULL)
    //   Exit_with_error("malloc failed for Output");
    
    unsigned char *Temp[STAGES - 1];
    
    for (int i = 0; i < (STAGES - 1); i++) {
    	Temp[i] = (unsigned char *)malloc(SCALED_FRAME_SIZE);
    }
    
    Load_data(Input);

    // ------------------------------------------------------------------------------------
    // Step 1: Initialize the OpenCL environment
     // ------------------------------------------------------------------------------------
    cl_int err;
    std::string binaryFile = argv[1];
    unsigned fileBufSize;
    std::vector<cl::Device> devices = get_xilinx_devices();
    devices.resize(1);
    cl::Device device = devices[0];
    cl::Context context(device, NULL, NULL, NULL, &err);
    char *fileBuf = read_binary_file(binaryFile, fileBufSize);
    cl::Program::Binaries bins{{fileBuf, fileBufSize}};
    cl::Program program(context, devices, bins, NULL, &err);
    cl::CommandQueue q(context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err);
    cl::Kernel Filter_HW(program, "Filter_HW", &err);
    

    // ------------------------------------------------------------------------------------
    // Step 2: Create buffers and initialize test values
    // ------------------------------------------------------------------------------------

    size_t input_elements_per_iteration = SCALED_FRAME_SIZE;
    size_t input_bytes_per_iteration = input_elements_per_iteration * sizeof(unsigned char);
    
    size_t output_elements_per_iteration = OUTPUT_FRAME_SIZE;
    size_t output_bytes_per_iteration = output_elements_per_iteration * sizeof(unsigned char);

    cl::Buffer input_buf;
    cl::Buffer output_buf;
    
    input_buf = cl::Buffer(context, CL_MEM_READ_ONLY, input_bytes_per_iteration, NULL, &err);
    output_buf = cl::Buffer(context, CL_MEM_WRITE_ONLY, output_bytes_per_iteration, NULL, &err);
    
    Temp[0] = (unsigned char*)q.enqueueMapBuffer(input_buf, CL_TRUE, CL_MAP_WRITE, 0, input_bytes_per_iteration);
    Temp[1] = (unsigned char*)q.enqueueMapBuffer(output_buf, CL_TRUE, CL_MAP_READ, 0, output_bytes_per_iteration);

    stopwatch total_time;

    // ------------------------------------------------------------------------------------
    // Step 3: Run the kernel
    // ------------------------------------------------------------------------------------

    std::vector<cl::Event> write_events;
    // total_time.start();
    
    for (int i = 0; i < FRAMES_NEW; i++)
    {
        total_time.start();
        std::vector<cl::Event> exec_events, read_events;
        cl::Event write_ev, exec_ev, read_ev;
        
	    Scale_SW((Input + i * INPUT_FRAME_SIZE), Temp[0]);
	
        Filter_HW.setArg(0, input_buf);
        Filter_HW.setArg(1, output_buf);
        
        if(i == 0) {
            q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, NULL, &write_ev);
        } else {
            q.enqueueMigrateMemObjects({input_buf}, 0 /* 0 means from host*/, &write_events, &write_ev);
            write_events.pop_back();
        }
        
        write_events.push_back(write_ev);
        q.enqueueTask(Filter_HW, &write_events, &exec_ev);
        exec_events.push_back(exec_ev);
        q.enqueueMigrateMemObjects({output_buf}, CL_MIGRATE_MEM_OBJECT_HOST, &exec_events, &read_ev);
        read_events.push_back(read_ev);
        
        Differentiate_SW(Temp[1], Temp[2]);
        Size = Compress_SW(Temp[2], Output);
        total_time.stop();

        Store_data("Output.bin", Output, Size);
    }

    q.finish();
    // total_time.stop();
    std::cout << "Average latency: " << total_time.avg_latency() << std::endl;
    std::cout << "Latency: " << total_time.latency() << std::endl;

    // ------------------------------------------------------------------------------------
    // Step 4: Release Allocated Resources
    // ------------------------------------------------------------------------------------

    // free(Input);
    // free(Output);

    delete[] fileBuf;

    
    return 0;
}
