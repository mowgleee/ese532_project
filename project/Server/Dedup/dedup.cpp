#include "dedup.h"

void chunk_matching(chunk *cptr)//, std::unordered_map<std::string, uint32_t> *chunks_map)//, uint32_t* unique_chunks)
{
	static std::unordered_map<std::string, uint32_t> chunks_map;
	uint32_t chunk_header = 1;
	static uint32_t unique_chunks = 0;

	if (chunks_map.find(cptr->sha) == chunks_map.end())
	{
		// Condition if chunk is unique
		cptr->num = unique_chunks;
		chunks_map[cptr->sha] = unique_chunks;
		//std::cout<<"Num assigned to chunk: "<<chunks_map[cptr->sha]<<"\n";
		makelog(VERB_DEBUG,"Num assigned to chunk %d \n", chunks_map[cptr->sha]);
		cptr->is_unique = true;
		unique_chunks++;
		return;
	}
	else
	{
		// Creating the header for a duplicate chunk with LSB 1
		//std::cout<<"This chunk is a copy of chunk no. :"<<chunks_map[cptr->sha]<<" with sha: "<<(cptr->sha)<<"\n";
		makelog(VERB_DEBUG,"This chunk is a copy of chunk no %d with sha %d \n",chunks_map[cptr->sha],cptr->sha);
		chunk_header |= (chunks_map[cptr->sha] << 1);
		//std::cout<<"\nChunk matching Header: "<<chunk_header<<"\n";
		makelog(VERB_DEBUG,"Chunk matching Header %d \n",chunk_header);

		memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
		offset += sizeof(uint32_t);
		cptr->is_unique = false;
		return;
	}
}
