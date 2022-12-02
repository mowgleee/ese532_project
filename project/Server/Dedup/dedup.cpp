#include "dedup.h"

void chunk_matching(semaphores* sems, packet** packarray)//, std::unordered_map<std::string, uint32_t> *chunks_map)//, uint32_t* unique_chunks)
{
	while(1)
	{
		static uint32_t count=0;
		makelog(VERB_DEBUG, "Waiting for Dedup Semaphore\n");
		sem_wait(&(sems->sem_dedup));
		dedup_sem_timer.start();



		packet* pptr;
		pptr = packarray[count%NUM_PACKETS];
		makelog(VERB_DEBUG, "Semaphore for Dedup received\n");
		makelog(VERB_DEBUG,"Dedup Packet Info:\n Dedup Packet Num: %d\n Dedup Packet Size: %d\n Dedup No of Chunks in Packet: %d\n Dedup Count: %d\n",pptr->num,pptr->size,pptr->num_of_chunks,count);

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
		makelog(VERB_DEBUG,"DEDUP Packet Done");
		dedup_sem_timer.stop();
		sem_post(&(sems->sem_lzw));
		makelog(VERB_DEBUG,"LZW Sempahore Posted");
		if(count == total_packets)
        {
            return;
        }
		count++;
	}
}
