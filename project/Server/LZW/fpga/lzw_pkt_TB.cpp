#include <bits/stdc++.h>
#include "lzw_kernel.h"

using namespace std;

void lzw_SW(const unsigned char* input_packet,
            uint32_t* chunk_bndry,
            uint32_t num_chunks,
            bool* is_chunk_unique,
            uint8_t* output_file_SW,
            uint32_t* output_size_SW,
            uint32_t* dup_chunk_head)
{

    // cout<<"\nsw length of string: "<<s1.length();
    // cout << "Encoding\n";
    uint32_t offset = 0;
    uint32_t last_boundary = 0;

    for(uint32_t z = 0; z < num_chunks; z++)
    {
        if(is_chunk_unique[z])
		{
            string s1="";
            
            for(uint32_t j = 0; j < chunk_bndry[z] + 1; j++)
			{
				s1 += input_packet[j + last_boundary];
			}

            unordered_map<string, int> table;
            for (int i = 0; i <= 255; i++) {
                string ch = "";
                ch += char(i);
                table[ch] = i;
            }
        
            string p = "", c = "";
            p += s1[0];
            int code = 256;
            vector<int> output_code;
            // cout << "String\tOutput_Code\tAddition\n";
            for (int i = 0; i < s1.length(); i++) {
                if (i != s1.length() - 1)
                    c += s1[i + 1];
                
                if (table.find(p + c) != table.end()) {
                    p = p + c;
                }
                else {
                    // cout << p << "\t" << table[p] << "\t\t"
                    //     << p + c << "\t" << code << endl;
                    output_code.push_back(table[p]);
                    table[p + c] = code;
                    code++;
                    p = c;
                }
                c = "";
            }
            // cout << p << "\t" << table[p] << endl;
            output_code.push_back(table[p]);

            uint8_t sw_output_code_packed[8096]={0};
            uint32_t offset_pack = 0;
            uint32_t curr_code = 0;
            uint32_t write_data = 0;
            uint32_t old_byte = 0;
            uint32_t rem_bits = 0;
            uint32_t running_bits = 0;
            uint32_t write_byte_size = 0;//number of bytes to write
            uint8_t write_byte = 0;//whats written in file


            for(int idx=0; idx < output_code.size(); idx++)
            {
                curr_code = output_code[idx];
                write_data = curr_code<<(32 - (int)CODE_LENGTH - rem_bits);
                write_data |= old_byte;
                running_bits = rem_bits + (int)CODE_LENGTH;
                write_byte_size = running_bits/8;
                for (uint32_t i=1; i<=write_byte_size; i++)
                {
                    write_byte = write_data>>(32-i*8);
                    sw_output_code_packed[offset_pack] = write_byte;
                    // memcpy(&file[offset], &write_byte, sizeof(unsigned char));
                    offset_pack += 1;
                }
                old_byte = write_data<<(write_byte_size*8);
                rem_bits = running_bits - write_byte_size*8;
            }
            if(rem_bits){
                write_byte = old_byte>>24;
                sw_output_code_packed[offset_pack] = write_byte;
                offset_pack +=1;
            }

			// Writing unique chunk header to global file pointer
			uint32_t size = offset_pack;
			uint32_t chunk_header = size << 1;
			for(uint32_t j = 0; j < 4; j++)
			{
				output_file_SW[offset] = chunk_header >> (8 * (3-j));
				offset++;
			}
			
			// Writing unique chunk data to global file pointer
			for(uint32_t j = 0; j < size; j++)
			{
				output_file_SW[offset] = sw_output_code_packed[j];
				offset++;
			}
        }
        else
        {
			// Writing duplicate chunk header to global file pointer
			uint32_t size = dup_chunk_head[z];
			uint32_t chunk_header = size << 1;
			
			for(uint32_t j = 0; j < 4; j++)
			{
				output_file_SW[offset] = chunk_header >> (8 * (3-j));
				offset++;
			}
        }
        last_boundary = chunk_bndry[z] + 1;
    }

    *output_size_SW = offset;
}

bool compare_outputs(uint8_t* sw_output_code, uint8_t *hw_output_code, uint32_t size)
{
    bool Equal = true;
    for (int i=0; i<size; i++)
    {
        if(sw_output_code[i] != hw_output_code[i])
        {
            // cout<<"SW out code: "<<sw_output_code[i]<<" HW out code: "<<hw_output_code[i]<<'\n';
            std::bitset<8> x(sw_output_code[i]);
            std::bitset<8> y(hw_output_code[i]);
            std::cout <<"SW out code: "<< x << " HW out code: " << y<<"\n";
            Equal = false;
        }
        else
        {
            std::cout<<"passed for iter: "<<i<<"\n";
        }
    }
    return Equal;
}

int main()
{
    // Generating input data for testing
    // input packet size = 622 (MAX = 1024)
    const unsigned char* input_packet = reinterpret_cast<const unsigned char *>("tors swallow their prey whole, without chewing it. After that they are not able to move, and they sleep through the six months that they need for digestion. I pondered deeply, then, over the adventures of the jungle. And after some work with a colored pencil I succeeded in making my first drawing. My Drawing Number One. The Little Prince Chapter I\nOnce when I was six years old I saw a magnificent picture in a book, called True Stories from Nature, about the primeval forest. It was a picture of a boa constrictor in the act of swallowing an animal. Here is a copy of the drawing. Boa In the book it said: Boa constric");
    uint32_t chunk_bndry[] = {12, 280, 471, 621};
    uint32_t num_chunks = 1;
    bool is_chunk_unique[] = {1, 1, 0, 1};
    uint32_t dup_chunk_head[] = {0, 0, 12, 0};
    
    // Output file pointer
    uint8_t* output_file_HW = (uint8_t*)calloc(8096, sizeof(uint8_t));
    uint8_t* output_file_SW = (uint8_t*)calloc(8096, sizeof(uint8_t));
    // Output size pointer
    uint32_t* output_size_HW = (uint32_t*)calloc(1, sizeof(uint32_t));
    uint32_t* output_size_SW = (uint32_t*)calloc(1, sizeof(uint32_t));

    lzw_kernel(input_packet, chunk_bndry, num_chunks, is_chunk_unique, output_file_HW, output_size_HW, dup_chunk_head);

    lzw_SW(input_packet, chunk_bndry, num_chunks, is_chunk_unique, output_file_SW, output_size_SW, dup_chunk_head);

    // std::cout<<"\nOUTPUT SIZE SW: "<< offset_pack<<"  HW: "<<(*output_code_size)<<"\n";

    bool data_Equal = compare_outputs(output_file_SW, output_file_HW, (*output_size_SW));
    bool size_Equal = (*output_size_SW == *output_size_HW);

    free(output_file_HW);
    free(output_file_SW);
    free(output_size_HW);
    free(output_size_SW);

    std::cout << "Hardware output size = " << *output_size_HW << "\nSoftware output size = " << *output_size_SW << std::endl;
    std::cout << "SIZE " << (size_Equal ? "EQUAL" : "NOT EQUAL") << " for hardware and software output." << std::endl;
    std::cout << "TESTBENCH " << (data_Equal ? "PASSED" : "FAILED") << std::endl;
}
