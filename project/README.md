# Data Depulication and Compression

* Ddup uses Rolling Hash/(Rabin Fingerprinting) to create content defined chunks.
* Deduplication requires creating a hash table of all unique chunks.
* Hashes for dedup table calculated using SHA256 on NEON vector units of device.
* Comp uses LZW compression algorithm.

## To Do
* Implement LZW on FPGA (done - array implementation)
* Logger with verbosity levels (done)
* Prefetch reads and issue writes
* Power Table in CDC (done)
* Hash only once in CDC (using min chunk size to improve throughput)
* Write a script for compilation, file copying and execution process.

## Benchmarks
Before porting LZW to H/W (SHA Neon, Other Blocks in S/W)

* LittlePrince.txt


Input File Size 14336 Bytes = 14KB <br />
write file with 4220 Bytes = 4.12KB<br />
--------------- Key Throughputs ---------------<br />
Input Throughput to Encoder: 5573.4 Mb/s. (Latency: 2.045e-05s).<br />
CDC Latency: 0.46205ms)	AVG: 0.00810614 ms<br />
SHA Latency: 1.31895ms)	AVG: 0.0231395 ms<br />
Chunk matching Latency: 0.29966ms)	AVG: 0.00525719 ms<br />
LZW Latency: 6.06583ms)	AVG: 0.404389 ms<br />
Bitpack Latency: 0.31592ms)	AVG: 0.0210613 ms<br />
Output Throughput from Encoder: 13.4962 Mb/s. (Latency: 0.00844506s).<br />


* LittlePrince.txt after SHA Neon and no prints


write file with 4220 = 4.12KB <br />
--------------- Key Throughputs ---------------<br />
Input Throughput to Encoder: 2487.47 Mb/s. (Latency: 4.582e-05s).<br />
CDC Latency: 0.18996ms	AVG: 0.00333263 ms<br />
SHA Latency: 0.28775ms	AVG: 0.00504825 ms<br />
Chunk matching Latency: 0.09973ms	AVG: 0.00174965 ms<br />
LZW Latency: 5.91974ms	AVG: 0.394649 ms<br />
Bitpack Latency: 0.06459ms	AVG: 0.004306 ms<br />
Output Throughput from Encoder: 17.3579 Mb/s. (Latency: 0.00656624s).<br />


* Franklin.txt


Input File Size = 389KB<br />
write file with 446075 Bytes = 435.6KB<br />
--------------- Key Throughputs ---------------<br />
Input Throughput to Encoder: 2981.36 Mb/s. (Latency: 0.00107174s).<br />
CDC Latency: 11.9776ms)	AVG: 0.0077575 ms<br />
SHA Latency: 34.9261ms)	AVG: 0.0226205 ms<br />
Chunk matching Latency: 4.16384ms)	AVG: 0.00269679 ms<br />
LZW Latency: 631.48ms)	AVG: 0.40899 ms<br />
Bitpack Latency: 15.9523ms)	AVG: 0.0103318 ms<br />
Output Throughput from Encoder: 4.62473 Mb/s. (Latency: 0.690902s)<br />


Packet Level Granularity

* LittlePrince.txt with SHA Neon and no prints and software LZW

--------------- Key Throughputs ---------------<br />
Input Throughput to Encoder: 4170.36 Mb/s. (Latency: 2.733e-05s). <br />
CDC Latency: 0.12873ms	AVG: 0.064365 ms <br />
SHA Latency: 0.1578ms	AVG: 0.0789 ms <br />
Chunk matching Latency: 0.11822ms	AVG: 0.05911 ms <br />
LZW Latency: 5.59005ms	AVG: 2.79503 ms <br />
Bitpack Latency: 0.0689ms	AVG: 0.00459333 ms <br />
Output Throughput from Encoder: 18.1656 Mb/s. (Latency: 0.00627429s).<br />
Output Throughput from Encoder without LZW: 166.573 Mb/s. (Latency: 0.00068424s).<br />


* Franklin.txt with SHA Neon, no prints and software LZW



--------------- Key Throughputs ---------------<br />
Input Throughput to Encoder: 2917.97 Mb/s. (Latency: 0.00109406s).<br />
CDC Latency: 3.56341ms	AVG: 0.0727227 ms<br />
SHA Latency: 2.39094ms	AVG: 0.0487947 ms<br />
Chunk matching Latency: 2.13314ms	AVG: 0.0435335 ms<br />
LZW Latency: 598.017ms	AVG: 12.2044 ms<br />
Bitpack Latency: 8.02541ms	AVG: 0.00518437 ms<br />
Output Throughput from Encoder: 5.24989 Mb/s. (Latency: 0.608095s).<br />
Output Throughput from Encoder without LZW: 316.763 Mb/s. (Latency: 0.0100783s).<br />


## To Do Later
* Check if we need to do rabin fingerprinting
* Account for chunks across packet boundaries
* Implement SHA on the NEON vector SIMD unit (Done)
* Packet Level Granularity (Done)
* Multithreading and Pipelining
* Streaming in Kernel
* Associative memory impl in Kernel
