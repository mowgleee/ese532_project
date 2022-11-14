#include "sha.h"



void sha_comp(uint32x4_t MSG0, uint32x4_t MSG1, uint32x4_t MSG2, uint32x4_t MSG3, uint32x4_t* STATE0, uint32x4_t* STATE1, uint32x4_t *ABEF_SAVE,uint32x4_t *CDGH_SAVE)
{
    // /* Save state */
    //     ABEF_SAVE = STATE0;
    //     CDGH_SAVE = STATE1;

    //     /* Load message */
    //     MSG0 = vld1q_u32((const uint32_t *)(data +  0));
    //     MSG1 = vld1q_u32((const uint32_t *)(data + 16));
    //     MSG2 = vld1q_u32((const uint32_t *)(data + 32));
    //     MSG3 = vld1q_u32((const uint32_t *)(data + 48));
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

    // data += 64;
    // length -= 64;
}

/* Process multiple blocks. The caller is responsible for setting the initial */
/*  state, and the caller is responsible for padding the final block.        */
void sha256_process_arm(uint32_t state[8], const uint8_t data[], uint32_t length)
{

    // Padding calculation
    uint64_t input_bits = length * 8;
    int zero_padding_bits_temp = 448 - ((input_bits + 1) % 512);
    bool run_again=zero_padding_bits_temp<0;
    if(run_again){
        zero_padding_bits_temp+=512;
    }
    uint64_t zero_padding_bits= zero_padding_bits_temp;

    // Padding buffer and calculating last iteration
    uint16_t padding_bytes = (zero_padding_bits + 1) / 8;
    unsigned char padding[padding_bytes] = {0};
    padding[0] = 1 << 7;

    // {data[(length - length%64), (length + length%64)], padding, input_bits}
    unsigned char padded_data[128]= {0}; // last 512 bits 
    
    memcpy(&padded_data[0], &data[length - (length % 64)], length % 64);
    memcpy(&padded_data[length % 64], &padding[0], padding_bytes);

    for(int i=8;i>0;i--){
        unsigned char curr_byte = input_bits>>(8*(i-1));
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

    if(run_again){
        
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

void sha(unsigned char* buff, chunk *cptr)//, wc_Sha3* sha3_384)
{
	unsigned char shaChar[32]={0};

	std::cout<<"calculating sha for buff: "<<buff[0]<<buff[1]<<buff[2]<<"\n";

    /* initial state */
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    sha256_process_arm(state, buff, cptr->size);

    for(int i=0; i<8 ; i++)
    {
        for (int j=0; j<=3; j++)
        {
            shaChar[(i*4)+j] = state[i]>>((3-j)*8);
        }
    }

    printf("SHA256 hash: ");

    for(int i = 0; i < 32; i++)
    {
        printf("%02X", (uint8_t)shaChar[i]);
    }
    printf("\n");

    std::string shaString(reinterpret_cast<char*>(shaChar), 32);
    cptr->sha = shaString;
}