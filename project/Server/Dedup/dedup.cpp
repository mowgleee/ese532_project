#include "dedup.h"

void chunk_matching(packet *pptr)//, std::unordered_map<std::string, uint32_t> *chunks_map)//, uint32_t* unique_chunks)
{
	static uint32_t unique_chunks = 0;
	static std::unordered_map<std::string, uint32_t> chunks_map;
	
	for(uint32_t chunk_num = 0; chunk_num < pptr->num_of_chunks; chunk_num++)
	{
		uint32_t chunk_header = 1;

		//makelog(VERB_DEBUG,"Entering dedup for chunk: %d \n", chunk_num);

		if (chunks_map.find(pptr->curr_chunk[chunk_num].sha) == chunks_map.end())
		{
			// Condition if chunk is unique
			pptr->curr_chunk[chunk_num].num = unique_chunks;
			chunks_map[pptr->curr_chunk[chunk_num].sha] = unique_chunks;
			//std::cout<<"Num assigned to chunk: "<<chunks_map[cptr->sha]<<"\n";
			//makelog(VERB_DEBUG,"Num assigned to chunk %d \n", chunks_map[pptr->curr_chunk[chunk_num].sha]);
			pptr->curr_chunk[chunk_num].is_unique = true;
			unique_chunks++;
			// return;
		}
		else
		{
			// Creating the header for a duplicate chunk with LSB 1
			//std::cout<<"This chunk is a copy of chunk no. :"<<chunks_map[cptr->sha]<<" with sha: "<<(cptr->sha)<<"\n";
			//makelog(VERB_DEBUG,"This chunk is a copy of chunk no %d with sha %d \n", chunks_map[pptr->curr_chunk[chunk_num].sha], pptr->curr_chunk[chunk_num].sha);
			chunk_header |= (chunks_map[pptr->curr_chunk[chunk_num].sha] << 1);
			//std::cout<<"\nChunk matching Header: "<<chunk_header<<"\n";
			//makelog(VERB_DEBUG,"Chunk matching Header %d \n",chunk_header);

			// memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
			// offset += sizeof(uint32_t);
			pptr->curr_chunk[chunk_num].header = chunk_header;
			pptr->curr_chunk[chunk_num].is_unique = false;
			// return;
		}
	}
	std::cout<<"Dedup Done \n";
}
