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

void cdc_eff(unsigned char *buff, packet* pptr)
{
	uint64_t hash = hash_func(buff, MIN_CHUNK_SIZE);
	static const uint64_t pow_const = pow(PRIME, WIN_SIZE + 1);

	uint32_t length = pptr->size;
	// First chunk
	uint32_t chunk_num = 0;
	pptr->curr_chunk[chunk_num].lower_bound = 0;
	uint32_t chunk_size = MIN_CHUNK_SIZE;

	for (uint32_t i = MIN_CHUNK_SIZE; i < length - MIN_CHUNK_SIZE; i=i)
	{
		if(((hash % MODULUS) == TARGET))
		{
			pptr->curr_chunk[chunk_num].upper_bound = chunk_size  + pptr->curr_chunk[chunk_num].lower_bound;
			makelog(VERB_DEBUG,"Upper bound of chunk if equal to target %d \n", pptr->curr_chunk[chunk_num].upper_bound );
			pptr->curr_chunk[chunk_num].size = chunk_size;
			chunk_size = MIN_CHUNK_SIZE;
			// Next chunk
			chunk_num++;
			pptr->curr_chunk[chunk_num].lower_bound = pptr->curr_chunk[chunk_num - 1].upper_bound + 1;
			makelog(VERB_DEBUG,"Lower bound of chunk if equal to target %d \n", pptr->curr_chunk[chunk_num].lower_bound );
			i = i + MIN_CHUNK_SIZE;
			hash = hash_func(buff, i);
			continue;
		}
		
		hash = (hash * PRIME - (buff[i - 1] * pow_const) + (buff[i - 1 + WIN_SIZE] * PRIME));
		chunk_size+=1;
		i=i+1;
	}
	pptr->curr_chunk[chunk_num].upper_bound = length;
	pptr->curr_chunk[chunk_num].size = pptr->curr_chunk[chunk_num].upper_bound - pptr->curr_chunk[chunk_num].lower_bound;
	pptr->num_of_chunks = chunk_num + 1;
	makelog(VERB_DEBUG,"Upper bound of chunk %d \n", pptr->curr_chunk[chunk_num].upper_bound );
	makelog(VERB_DEBUG,"Size  of chunk %d \n", pptr->curr_chunk[chunk_num].size );
	makelog(VERB_DEBUG,"Num  of chunk %d \n", pptr->num_of_chunks );
	return;
}