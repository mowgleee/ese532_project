#include "sha.h"

void sha_comp(uint32x4_t MSG0, uint32x4_t MSG1, uint32x4_t MSG2, uint32x4_t MSG3, uint32x4_t* STATE0, uint32x4_t* STATE1, uint32x4_t *ABEF_SAVE,uint32x4_t *CDGH_SAVE)
{
    uint32x4_t TMP0, TMP1, TMP2;

    /* Reverse for little endian */
    MSG0 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG0)));
    MSG1 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG1)));
    MSG2 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG2)));
    MSG3 = vreinterpretq_u32_u8(vrev32q_u8(vreinterpretq_u8_u32(MSG3)));

    TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0x00]));

    /* Rounds 0-3 */
    MSG0 = vsha256su0q_u32(MSG0, MSG1);
    TMP2 = *STATE0;
    TMP1 = vaddq_u32(MSG1, vld1q_u32(&K[0x04]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP0);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP0);
    MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

    /* Rounds 4-7 */
    MSG1 = vsha256su0q_u32(MSG1, MSG2);
    TMP2 = *STATE0;
    TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[0x08]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP1);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP1);
    MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

    /* Rounds 8-11 */
    MSG2 = vsha256su0q_u32(MSG2, MSG3);
    TMP2 = *STATE0;
    TMP1 = vaddq_u32(MSG3, vld1q_u32(&K[0x0c]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP0);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP0);
    MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

    /* Rounds 12-15 */
    MSG3 = vsha256su0q_u32(MSG3, MSG0);
    TMP2 = *STATE0;
    TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0x10]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP1);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP1);
    MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

    /* Rounds 16-19 */
    MSG0 = vsha256su0q_u32(MSG0, MSG1);
    TMP2 = *STATE0;
    TMP1 = vaddq_u32(MSG1, vld1q_u32(&K[0x14]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP0);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP0);
    MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

    /* Rounds 20-23 */
    MSG1 = vsha256su0q_u32(MSG1, MSG2);
    TMP2 = *STATE0;
    TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[0x18]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP1);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP1);
    MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

    /* Rounds 24-27 */
    MSG2 = vsha256su0q_u32(MSG2, MSG3);
    TMP2 = *STATE0;
    TMP1 = vaddq_u32(MSG3, vld1q_u32(&K[0x1c]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP0);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP0);
    MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

    /* Rounds 28-31 */
    MSG3 = vsha256su0q_u32(MSG3, MSG0);
    TMP2 = *STATE0;
    TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0x20]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP1);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP1);
    MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

    /* Rounds 32-35 */
    MSG0 = vsha256su0q_u32(MSG0, MSG1);
    TMP2 = *STATE0;
    TMP1 = vaddq_u32(MSG1, vld1q_u32(&K[0x24]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP0);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP0);
    MSG0 = vsha256su1q_u32(MSG0, MSG2, MSG3);

    /* Rounds 36-39 */
    MSG1 = vsha256su0q_u32(MSG1, MSG2);
    TMP2 = *STATE0;
    TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[0x28]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP1);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP1);
    MSG1 = vsha256su1q_u32(MSG1, MSG3, MSG0);

    /* Rounds 40-43 */
    MSG2 = vsha256su0q_u32(MSG2, MSG3);
    TMP2 = *STATE0;
    TMP1 = vaddq_u32(MSG3, vld1q_u32(&K[0x2c]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP0);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP0);
    MSG2 = vsha256su1q_u32(MSG2, MSG0, MSG1);

    /* Rounds 44-47 */
    MSG3 = vsha256su0q_u32(MSG3, MSG0);
    TMP2 = *STATE0;
    TMP0 = vaddq_u32(MSG0, vld1q_u32(&K[0x30]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP1);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP1);
    MSG3 = vsha256su1q_u32(MSG3, MSG1, MSG2);

    /* Rounds 48-51 */
    TMP2 = *STATE0;
    TMP1 = vaddq_u32(MSG1, vld1q_u32(&K[0x34]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP0);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP0);

    /* Rounds 52-55 */
    TMP2 = *STATE0;
    TMP0 = vaddq_u32(MSG2, vld1q_u32(&K[0x38]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP1);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP1);

    /* Rounds 56-59 */
    TMP2 = *STATE0;
    TMP1 = vaddq_u32(MSG3, vld1q_u32(&K[0x3c]));
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP0);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP0);

    /* Rounds 60-63 */
    TMP2 = *STATE0;
    *STATE0 = vsha256hq_u32(*STATE0, *STATE1, TMP1);
    *STATE1 = vsha256h2q_u32(*STATE1, TMP2, TMP1);

    /* Combine state */
    *STATE0 = vaddq_u32(*STATE0, *ABEF_SAVE);
    *STATE1 = vaddq_u32(*STATE1, *CDGH_SAVE);
}

/* Process multiple blocks. The caller is responsible for setting the initial */
/*  state, and the caller is responsible for padding the final block.        */
void sha256_process_arm(uint32_t state[8], const uint8_t data[], uint32_t length)
{
    // Padding calculation
    uint64_t input_bits = length<<3;
    int64_t zero_padding_bits = 448 - ((input_bits + 1) % 512);
    bool padding_overflow = zero_padding_bits<0;

    if(padding_overflow){
        zero_padding_bits+=512;
    }

    // Padding buffer and calculating last iteration
    uint16_t padding_bytes = (zero_padding_bits + 1) >> 3;
    uint8_t padding[padding_bytes] = {0};
    padding[0] = 1 << 7;
    
    uint8_t padded_data[128]= {0}; // last 512 bits 
    
    memcpy(&padded_data[0], &data[length - (length % 64)], length % 64);//all data written

    memcpy(&padded_data[length % 64], &padding[0], 1);//first byte with MSB 1 copied. proceeding padded_data is already 0

    for(int i=8;i>0;i--){
        uint8_t curr_byte = input_bits>>((i-1)<<3);
        memcpy(&padded_data[length % 64 + padding_bytes+(8-i)], &curr_byte, 1);
    }

    uint32x4_t STATE0, STATE1, ABEF_SAVE, CDGH_SAVE;
    uint32x4_t MSG0, MSG1, MSG2, MSG3;
    // uint32x4_t TMP0, TMP1, TMP2;

    /* Load state */
    STATE0 = vld1q_u32(&state[0]);
    STATE1 = vld1q_u32(&state[4]);

    while (length >= 64)
    {
        /* Save state */
        ABEF_SAVE = STATE0;
        CDGH_SAVE = STATE1;

        /* Load message */
        MSG0 = vld1q_u32((const uint32_t *)(data +  0));
        MSG1 = vld1q_u32((const uint32_t *)(data + 16));
        MSG2 = vld1q_u32((const uint32_t *)(data + 32));
        MSG3 = vld1q_u32((const uint32_t *)(data + 48));

        sha_comp(MSG0, MSG1, MSG2, MSG3, &STATE0, &STATE1, &ABEF_SAVE, &CDGH_SAVE);

        data += 64;
        length -= 64;
    }

    //final 64
    /* Save state */
    ABEF_SAVE = STATE0;
    CDGH_SAVE = STATE1;

    /* Load message */
    MSG0 = vld1q_u32((const uint32_t *)(padded_data +  0));
    MSG1 = vld1q_u32((const uint32_t *)(padded_data + 16));
    MSG2 = vld1q_u32((const uint32_t *)(padded_data + 32));
    MSG3 = vld1q_u32((const uint32_t *)(padded_data + 48));


    sha_comp(MSG0, MSG1, MSG2, MSG3, &STATE0, &STATE1, &ABEF_SAVE, &CDGH_SAVE);

    if(padding_overflow)
    {
        MSG0 = vld1q_u32((const uint32_t *)(padded_data + 64));
        MSG1 = vld1q_u32((const uint32_t *)(padded_data + 80));
        MSG2 = vld1q_u32((const uint32_t *)(padded_data + 96));
        MSG3 = vld1q_u32((const uint32_t *)(padded_data + 112));
        
        sha_comp(MSG0, MSG1, MSG2, MSG3, &STATE0, &STATE1, &ABEF_SAVE, &CDGH_SAVE);
    }

    /* Save state */
    vst1q_u32(&state[0], STATE0);
    vst1q_u32(&state[4], STATE1);
}

void sha(semaphores* sems, packet** packarray)//, wc_Sha3* sha3_384)
{
   while(1) 
    {
        std::cout<<"\nWaiting for SHA Semaphore\n";
        sem_wait(&(sems->sem_sha));
        sha_sem_timer.start();
        packet* pptr;
        uint8_t* buff;
        static uint32_t count = 0;
        buff = input[count%NUM_PACKETS];
		buff = &buff[HEADER];
        pptr = packarray[count%NUM_PACKETS];
        std::cout<<"Semaphore for SHA received\n";
		std::cout<<"SHA Packet Info:\n"<< "SHA Packet Num: "<< pptr->num <<"\n SHA Packet Size: "<< pptr->size <<"\n SHA No of Chunks in Packet: "<<pptr->num_of_chunks<<"\n SHA Count:"<< count <<"\n";


        for(uint32_t chunk_num = 0; chunk_num < pptr->num_of_chunks; chunk_num++)
        {
            //std::cout<<"Calculating SHA for chunk: "<<chunk_num<<"\n";
            

            uint8_t shaChar[32]={0};

            // std::cout<<"calculating sha for buff: "<<buff[0]<<buff[1]<<buff[2]<<"\n";

            /* initial state */
            uint32_t state[8] = {
                0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
            };

            sha256_process_arm(state, &buff[pptr->curr_chunk[chunk_num].lower_bound], pptr->curr_chunk[chunk_num].size);

            for(int i=0; i<8 ; i++)
            {
                for (int j=0; j<=3; j++)
                {
                    shaChar[(i<<2)+j] = state[i]>>((3-j)<<3);
                }
            }

            // printf("SHA256 hash: ");

            // for(int i = 0; i < 32; i++)
            // {
            //     printf("%02X", (uint8_t)shaChar[i]);
            // }
            // printf("\n");

            std::string shaString(reinterpret_cast<char*>(shaChar), 32);
            pptr->curr_chunk[chunk_num].sha = shaString;
        }
        sha_sem_timer.stop();

        sem_post(&(sems->sem_dedup));
        std::cout<<"Dedup Semaphore Posted";
        if(count == total_packets)
        {
            return;
        }
        count++;
   }
}