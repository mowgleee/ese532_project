// #include "../Server/encoder.h"
#include "../Common/common.h"

float power_table[16] = {3.0, 9.0, 27.0, 81.0, 243.0, 729.0, 2187.0, 6561.0, 19683.0, 59049.0, 177147.0, 531441.0, 1594323.0, 4782969.0, 14348907.0, 43046721.0};
uint64_t hash_func(unsigned char *input, uint32_t pos);
void cdc_eff(unsigned char *buff, chunk *cptr, uint32_t length);