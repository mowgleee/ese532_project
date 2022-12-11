#ifndef _LZW_HOST_
#define _LZW_HOST_
// #include "encoder.h"
#define CL_HPP_CL_1_2_DEFAULT_BUILD
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_ENABLE_PROGRAM_CONSTRUCTION_FROM_ARRAY_COMPATIBILITY 1
#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

// #define FRAMES_NEW 200

#include "../../Common/common.h"
#include <CL/cl2.hpp>
#include <cstdint>
#include <cstdlib>
// #include <fstream>
// #include <iostream>
// #include <unistd.h>
// #include <vector>

extern stopwatch kernel_init_timer;
extern stopwatch kernel_timer;
extern stopwatch kernel_mem_timer;


class lzw_request
{
    cl_int            err;
    std::string       binaryFile;
    unsigned          fileBufSize;
    std::vector<cl::Device> devices;
    cl::Device        device;
    cl::Context       context;
    char              *fileBuf;
    cl::Program::Binaries bins;
    cl::Program       program;
    cl::CommandQueue  q;
    cl::Kernel        lzw_kernel_1;
    cl::Kernel        lzw_kernel_2;
    
    cl::Buffer        input_buf_1[NUM_PACKETS/2];
    cl::Buffer        output_buf_1[NUM_PACKETS/2];
    cl::Buffer        output_size_buf_1[NUM_PACKETS/2];
    cl::Buffer        chunk_boundaries_buf_1[NUM_PACKETS/2];
    cl::Buffer        is_unique_buf_1[NUM_PACKETS/2];
    cl::Buffer        dup_chunk_head_buf_1[NUM_PACKETS/2];

    cl::Buffer        input_buf_2[NUM_PACKETS/2];
    cl::Buffer        output_buf_2[NUM_PACKETS/2];
    cl::Buffer        output_size_buf_2[NUM_PACKETS/2];
    cl::Buffer        chunk_boundaries_buf_2[NUM_PACKETS/2];
    cl::Buffer        is_unique_buf_2[NUM_PACKETS/2];
    cl::Buffer        dup_chunk_head_buf_2[NUM_PACKETS/2];

    std::vector<cl::Event> write_events_1, exec_events_1, read_events_1;
    cl::Event write_ev_1, exec_ev_1, read_ev_1;

    std::vector<cl::Event> write_events_2, exec_events_2, read_events_2;
    cl::Event write_ev_2, exec_ev_2, read_ev_2;

    size_t        output_pkt_bytes;// = (BLOCKSIZE*13/8) * sizeof(unsigned char);
    size_t        input_pkt_bytes;// = MAX_CHUNK_SIZE * sizeof(unsigned char);
    size_t        chunk_boundaries_bytes;// = MAX_NUM_CHUNKS + sizeof(uint32_t);
    size_t        is_unique_bytes;// = MAX_NUM_CHUNKS + sizeof(uint8_t);
    size_t        dup_chunk_head_bytes;// = MAX_NUM_CHUNKS + sizeof(uint32_t);;

    public:
    lzw_request();
    ~lzw_request();
    // void init(uint32_t packet_size, uint32_t num_chunks, unsigned char* input_to_fpga, uint8_t* file_ptr, uint32_t* chunk_boundaries, uint32_t* dup_chunk_head, uint8_t* is_unique);
    // void init();
    void set_args_1(uint32_t num_chunks_1, uint32_t);
    void set_args_2(uint32_t num_chunks_2, uint32_t);
    void run_1(uint32_t);
    void run_2(uint32_t);
    void finish();
    void wait_1(uint32_t);
    void wait_2(uint32_t);

    unsigned char* input_to_fpga_1[NUM_PACKETS/2];
    unsigned char* output_from_fpga_1[NUM_PACKETS/2];
    uint32_t* ptr_output_size_1[NUM_PACKETS/2];
    uint32_t* chunk_boundaries_1[NUM_PACKETS/2];
    uint8_t* is_unique_1[NUM_PACKETS/2];
    uint32_t* dup_chunk_head_1[NUM_PACKETS/2];

    unsigned char* input_to_fpga_2[NUM_PACKETS/2];
    unsigned char* output_from_fpga_2[NUM_PACKETS/2];
    uint32_t* ptr_output_size_2[NUM_PACKETS/2];
    uint32_t* chunk_boundaries_2[NUM_PACKETS/2];
    uint8_t* is_unique_2[NUM_PACKETS/2];
    uint32_t* dup_chunk_head_2[NUM_PACKETS/2];

    // uint32_t* chunk_boundaries;
    // uint8_t* is_unique;
    // uint32_t* dup_chunk_head;
};

void lzw_host(lzw_request *kernel_cl_obj, semaphores* sems,packet** packarray);

// OCL_CHECK doesn't work if call has templatized function call
#define OCL_CHECK(error, call)                                                 \
  call;                                                                        \
  if (error != CL_SUCCESS) {                                                   \
    printf("%s:%d Error calling " #call ", error code is: %d\n", __FILE__,     \
           __LINE__, error);                                                   \
    exit(EXIT_FAILURE);                                                        \
  }

std::vector<cl::Device> get_xilinx_devices();
char* read_binary_file(const std::string &xclbin_file_name, unsigned &nb);
void set_callback(cl::Event event, const char *queue_name);

#endif

// When creating a buffer with user pointer (CL_MEM_USE_HOST_PTR), under the
// hood
// User ptr is used if and only if it is properly aligned (page aligned). When
// not
// aligned, runtime has no choice but to create its own host side buffer that
// backs
// user ptr. This in turn implies that all operations that move data to and from
// device incur an extra memcpy to move data to/from runtime's own host buffer
// from/to user pointer. So it is recommended to use this allocator if user wish
// to
// Create Buffer/Memory Object with CL_MEM_USE_HOST_PTR to align user buffer to
// the
// page boundary. It will ensure that user buffer will be used when user create
// Buffer/Mem Object with CL_MEM_USE_HOST_PTR.
// template <typename T> struct aligned_allocator {
//   using value_type = T;

//   aligned_allocator() {}

//   aligned_allocator(const aligned_allocator &) {}

//   template <typename U> aligned_allocator(const aligned_allocator<U> &) {}

//   T *allocate(std::size_t num) {
//     void *ptr = nullptr;

// #if defined(_WINDOWS)
//     {
//       ptr = _aligned_malloc(num * sizeof(T), 4096);
//       if (ptr == NULL) {
//         std::cout << "Failed to allocate memory" << std::endl;
//         exit(EXIT_FAILURE);
//       }
//     }
// #else
//     {
//       if (posix_memalign(&ptr, 4096, num * sizeof(T)))
//         throw std::bad_alloc();
//     }
// #endif
//     return reinterpret_cast<T *>(ptr);
//   }
//   void deallocate(T *p, std::size_t num) {
// #if defined(_WINDOWS)
//     _aligned_free(p);
// #else
//     free(p);
// #endif
//   }
// };