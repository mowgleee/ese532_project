# Data Depulication and Compression

* Ddup uses Rolling Hash/(Rabin Fingerprinting) to create content defined chunks.
* Comp uses LZW compression algorithm.

## To Do
* Check if we need to do rabin fingerprinting
* Account for chunks across packet boundaries
* Implement SHA on the NEON vector SIMD unit
* Implement LZW on FPGA
