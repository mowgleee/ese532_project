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
    cl::Kernel        lzw_kernel;
    
    cl::Buffer        input_buf;
    cl::Buffer        output_buf;
    cl::Buffer        output_size_buf;

    std::vector<cl::Event> write_events, exec_events, read_events;
    cl::Event write_ev, exec_ev, read_ev;

    const size_t  output_chunk_bytes = MAX_CHUNK_SIZE * sizeof(unsigned char);
    size_t        input_chunk_bytes;

    public:
    lzw_request();
    void init(uint32_t chunk_size, unsigned char* input_to_fpga, uint32_t* ptr_output_size);
    void run();
    unsigned char* output_from_fpga;
};

void lzw_host(unsigned char *buff, packet* pptr, lzw_request &kernel_cl_obj);

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