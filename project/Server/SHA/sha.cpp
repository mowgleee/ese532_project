#include "sha.h"

void sha(unsigned char* buff, chunk *cptr)//, wc_Sha3* sha3_384)
{
	unsigned char shaSum[SHA3_384_DIGEST_SIZE];

	std::cout<<"calculating sha for buff: "<<buff[0]<<buff[1]<<buff[2]<<"\n";
    
    wc_Sha3 sha3_384;
    wc_InitSha3_384(&sha3_384, NULL, INVALID_DEVID);
    wc_Sha3_384_Update(&sha3_384, (const unsigned char*)buff, cptr->size/*strlen(message)*/);
    wc_Sha3_384_Final(&sha3_384, shaSum);

	std::string str(reinterpret_cast<char*>(shaSum), SHA3_384_DIGEST_SIZE);
	std::cout<<"SHA:\n";
	for (int i=0; i<SHA3_384_DIGEST_SIZE; i++)
	{
		printf("%02x", shaSum[i]);
	}
	std::cout<<"This chunk's SHA: "<<str<<"\n";
	cptr->sha = str;
}