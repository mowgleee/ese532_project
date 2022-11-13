#include "dedup.h"

void chunk_matching(chunk *cptr)//, std::unordered_map<std::string, uint32_t> *chunks_map)//, uint32_t* unique_chunks)
{
	static std::unordered_map<std::string, uint32_t> chunks_map;
	uint32_t chunk_header = 1;
	static uint32_t unique_chunks = 0;
	static std::vector<unsigned int> chunk_length;
	static uint32_t total_length_compressed = 0;

	if (!(chunks_map.find(cptr->sha) == chunks_map.end()))
	{
		if((cptr->size == chunk_length[chunks_map[cptr->sha]])){
			// Creating the header for a duplicate chunk with LSB 1
			std::cout<<"This chunk is a copy of chunk no. :"<<chunks_map[cptr->sha]<<" with sha: "<<(cptr->sha)<<"\n";
			chunk_header |= (chunks_map[cptr->sha] << 1);
			std::cout<<"\nChunk matching Header: "<<chunk_header<<"\n";

			memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
			offset += sizeof(uint32_t);
			total_length_compressed += 4;
			std::cout<<"RUNNING TOTAL BYTES(CHUNK MATCHING): "<<total_length_compressed<<"\n";

			cptr->is_unique = false;
			return;
			// return false;
		}
		else
		{
			// Condition if chunk is unique

			cptr->num = unique_chunks;
			chunks_map[cptr->sha] = unique_chunks;
			std::cout<<"Num assigned to chunk: "<<chunks_map[cptr->sha]<<"\n";
			cptr->is_unique = true;
			chunk_length.push_back(cptr->size);
			unique_chunks++;
			return;
			// return true;
		}

	}
	else
	{
		// Condition if chunk is unique

		cptr->num = unique_chunks;
		chunks_map[cptr->sha] = unique_chunks;
		std::cout<<"Num assigned to chunk: "<<chunks_map[cptr->sha]<<"\n";
		cptr->is_unique = true;
		chunk_length.push_back(cptr->size);
		unique_chunks++;
		return;
		// return true;
	}
}
