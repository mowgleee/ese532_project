# Data Depulication and Compression

* Ddup uses Rolling Hash/(Rabin Fingerprinting) to create content defined chunks.
* Comp uses LZW compression algorithm.

## To Do
* Implement LZW on FPGA
* Logger with verbosity levels
* Prefetch reads and issue writes

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


## To Do Later
* Check if we need to do rabin fingerprinting
* Account for chunks across packet boundaries
* Implement SHA on the NEON vector SIMD unit (Done)
