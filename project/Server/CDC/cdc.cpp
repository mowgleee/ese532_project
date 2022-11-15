#include "cdc.h"

static const uint64_t power_table[16] = {3, 9, 27, 81, 243, 729, 2187, 6561, 19683, 59049, 177147, 531441, 1594323, 4782969, 14348907, 43046721};

uint64_t hash_func(unsigned char *input, uint32_t pos)
{
	uint64_t hash = 0;
	for ( int i = 0 ; i < WIN_SIZE ; i++)
	{
		hash += input[pos + WIN_SIZE - 1 - i] * power_table[i];
	}
	return hash;
}

void cdc_eff(unsigned char *buff, chunk *cptr, uint32_t length)
{
	uint64_t hash = hash_func(buff, WIN_SIZE);
	static const uint64_t pow_const = pow(PRIME, WIN_SIZE + 1);

	for (int i = WIN_SIZE + 1; i < MAX_CHUNK_SIZE - WIN_SIZE; i++)
	// for (int i = 1; i < MAX_CHUNK_SIZE; i++)
	{
		// Check if condition is working
		if (i - 1 + WIN_SIZE + cptr->lower_bound > length)
		{
			cptr->upper_bound = length-1;
			return;
		}

		hash = (hash * PRIME - (buff[i - 1] * pow_const) + (buff[i - 1 + WIN_SIZE] * PRIME));

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