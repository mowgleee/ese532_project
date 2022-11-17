#include "lzw.h"

void lzw_encoding(unsigned char* s1, chunk* cptr)
{
	uint32_t length = cptr->size;
	//static uint32_t total_length_compressed=0;
	//static uint32_t total_length_uncompressed=0;
	//total_length_uncompressed += length;
	
	std::cout<<"length of chunk: "<<length<<"\n";

    // std::cout << "Encoding\n";
    std::unordered_map<std::string, int> table;
    for (int i = 0; i <= 255; i++) {
        std::string ch = "";
        ch += char(i);
        table[ch] = i;
    }
 
    std::string p = "", c = "";
    p += s1[0];
    int code = 256;
    std::vector<int> output_code;
    // cout << "String\tOutput_Code\tAddition\n";
    for (uint32_t i = 0; i < length; i++) {
        if (i != length - 1)
            c += s1[i + 1];
        if (table.find(p + c) != table.end()) {
            p = p + c;
        }
        else {
            // cout << p << "\t" << table[p] << "\t\t"
            //      << p + c << "\t" << code << endl;
            output_code.push_back(table[p]);
            table[p + c] = code;
            code++;
            p = c;
        }
        c = "";
    }
    // cout << p << "\t" << table[p] << endl;
    output_code.push_back(table[p]);
	int output_code_size = output_code.size();
	//std::cout << "output_code size = " << output_code_size << "\n";

	bit_pack_timer.start();
	uint32_t curr_code = 0;
	uint32_t write_data = 0;
	uint32_t old_byte = 0;
	uint32_t rem_bits = 0;
	uint32_t running_bits = 0;
	uint32_t write_byte_size = 0;//number of bytes to write
	uint8_t write_byte = 0;//whats written in file
	uint32_t bytes_written = ceil(output_code_size*13.0/8.0);
	//total_length_compressed += bytes_written;
	//total_length_compressed += 4;
	uint32_t chunk_header = (bytes_written<<1);
	//std::cout<<"\nLZW Header: "<<chunk_header<<"\n";
	memcpy(&file[offset], &chunk_header, sizeof(uint32_t));
	offset += sizeof(uint32_t);

	for(int idx=0; idx<output_code.size(); idx++)
	{
		curr_code = output_code[idx];
		write_data = curr_code<<(32 - (int)CODE_LENGTH - rem_bits);
		write_data |= old_byte;
		running_bits = rem_bits + (int)CODE_LENGTH;
		write_byte_size = running_bits/8;
		for (uint32_t i=1; i<=write_byte_size; i++)
		{
			write_byte = write_data>>(32-i*8);
			memcpy(&file[offset], &write_byte, sizeof(unsigned char));
			offset += sizeof(unsigned char);
		}
		old_byte = write_data<<(write_byte_size*8);
		rem_bits = running_bits - write_byte_size*8;
	}
	if(rem_bits){
		//std::bitset<32> x(old_byte);
		//std::cout<<"No of Remaining Bits:"<<rem_bits<<"\n";
		//std::cout<<"Remaining Bits:"<<x<<"\n";
		write_byte = old_byte>>24;
		//std::bitset<8> y(write_byte);
		//std::cout<<"Remaining Byte:"<<y<<"\n";
		memcpy(&file[offset], &write_byte, sizeof(unsigned char));
		offset+= sizeof(unsigned char);
	}
	bit_pack_timer.stop();
	//std::cout<<"RUNNING TOTAL BYTES(LZW): "<<total_length_compressed<<"\n";
	//std::cout<<"RUNNING TOTAL BYTES OF UNIQUE CHUNKS BEFORE COMPRESSION(LZW): "<<total_length_uncompressed<<"\n";
	//logger(VERB_DEBUG,"Running Total Bytes %d",total_length_compressed)
	//logger(VERB_HIGH,"Total bytes of unique chunks before compression %d",total_length_uncompressed);
}