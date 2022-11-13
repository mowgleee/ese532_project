#include "cdc.h"

uint64_t hash_func(unsigned char *input, uint32_t pos)
{
	uint64_t hash = 0;
	for ( int i = 0 ; i < WIN_SIZE ; i++)
	{
		hash += input[pos + WIN_SIZE - 1 - i] * pow(PRIME, i + 1);
	}
	return hash;
}

void cdc_eff(unsigned char *buff, chunk *cptr, uint32_t length)
{
	uint64_t hash = hash_func(buff, WIN_SIZE);

	for (int i = WIN_SIZE + 1; i < MAX_CHUNK_SIZE - WIN_SIZE; i++)
	// for (int i = 1; i < MAX_CHUNK_SIZE; i++)
	{
		// Check if condition is working
		if (i - 1 + WIN_SIZE + cptr->lower_bound > length)
		{
			cptr->upper_bound = length-1;
			return;
		}
		

		hash = (hash * PRIME - (buff[i - 1] * pow(PRIME, WIN_SIZE + 1)) + (buff[i - 1 + WIN_SIZE] * PRIME));

		if((hash % MODULUS) == TARGET)
		{
			cptr->upper_bound = i  + cptr->lower_bound;
			return;
		}
	}
	// *starting_hash = hash;
	cptr->upper_bound = MAX_CHUNK_SIZE + cptr->lower_bound;
	return;
}