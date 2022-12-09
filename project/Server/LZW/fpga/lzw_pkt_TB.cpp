#include <bits/stdc++.h>
#include "lzw_kernel.h"

using namespace std;

void lzw_SW(unsigned char* input_packet,
            uint32_t* chunk_bndry,
            uint32_t num_chunks,
            uint8_t* is_chunk_unique,
            uint8_t* output_file_SW,
            uint32_t* output_size_SW,
            uint32_t* dup_chunk_head)
{


    // cout<<"\nsw length of string: "<<s1.length();
    cout << "New Software Encoding\n";
    uint32_t offset = 0;
    uint32_t last_boundary = 0;

    for(uint32_t z = 0; z < num_chunks; z++)
    {
        if(is_chunk_unique[z])
		{
            string s1;
            
            for(uint32_t j = last_boundary; j < chunk_bndry[z] + 1; j++)
			{
				s1 += input_packet[j];
			}

            cout << "\nChunk in SW: " << s1 << "\n";

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
				output_file_SW[offset] = chunk_header >> (8 * j);
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
			// uint32_t size = dup_chunk_head[z];
			// uint32_t chunk_header = size << 1;
			uint32_t chunk_header = dup_chunk_head[z];
			
			for(uint32_t j = 0; j < 4; j++)
			{
				output_file_SW[offset] = chunk_header >> (8 * j);
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
    uint32_t mismatches = 0;
    for (int i=0; i<size; i++)
    {
        if(sw_output_code[i] != hw_output_code[i])
        {
            // cout<<"SW out code: "<<sw_output_code[i]<<" HW out code: "<<hw_output_code[i]<<'\n';
            std::cout<<"***********failed for iter: "<<i<<"************\n";
            std::bitset<8> x(sw_output_code[i]);
            std::bitset<8> y(hw_output_code[i]);
            std::cout <<"SW out code: "<< x << " HW out code: " << y<<"\n\n";
            Equal = false;
            mismatches++;
        }
        else
        {
            // std::cout<<"passed for iter: "<<i<<"\n";
            std::bitset<8> x(sw_output_code[i]);
            std::bitset<8> y(hw_output_code[i]);
            // std::cout <<"SW out code: "<< x << " HW out code: " << y<<"\n\n";
        }
    }

    std::cout <<"--------------MISMATCHES: "<< mismatches <<"---------------\n\n";

    return Equal;
}

int main()
{
    // Generating input data for testing
    // input packet size = 622 (MAX = 1024)
//    unsigned char input_packet[] = "01abc tors swallow their prey whole, without chewing it. After that they are not able to move, and they sleep through the six months that they need for digestion. I pondered deeply, then, over the adventures of the jungle. And after some work with a colored pencil I succeeded in making my first drawing. My Drawing Number One. The Little Prince Chapter I\nOnce when I was six years old I saw a magnificent picture in a book, called True Stories from Nature, about the primeval forest. It was a picture of a boa constrictor in the act of swallowing an animal. Here is a copy of the drawing. Boa In the book it said: Boa constric";
	unsigned char input_packet[] = "12The Autobiography of Benjamin Franklin edited by Charles Eliot presented\nby Project Gutenberg\n\nThis eBook is for the use of anyone anywhere at no cost and with almost\nno restrictions whatsoever. You may copy it, give it away or re-use it\nunder the terms of the Project Gutenberg License included with this\neBook or online at www.gutenberg.net\n\nTitle: The Autobiography of Benjamin Franklin\nEditor: Charles Eliot\nRelease Date: May 22, 2008 [EBook #148]\nLast Updated: July 2016\nLanguage: English\n\nProduced by Project Gutenberg. HTML version by Robert Homa.\n\n*** START OF THIS PROJECT GUTENBERG EBOOK THE AUTOBIOGRAPHY OF BENJAMIN\nFRANKLIN ***\n\nTitle: The Autobiography of Benjamin Franklin\n\nAuthor: Benjamin Franklin\n\nFirst Released: August 4, 1995 [Ebook: #148]\n[Last updated: August 2, 2016]\n\nLanguage: English\n\n*** START OF THIS PROJECT GUTENBERG EBOOK AUTOBIOGRAPHY OF BENJAMIN\nFRANKLIN ***\n\nTHE AUTOBIOGRAPHY OF BENJAMIN FRANKLIN\n\nThe Harvard Classics\n\nWITH INTRODUCTION AND NOTES\n\nEDITED BY\n\nCHARLES W ELLIOT LLD\n\nP F COLLIER & SON COMPANY\nNEW YORK\n1909\n\n\n\n\nNavigation\n\n    Letter from Mr. Abel James.\n    Publishes the first number of \"Poor Richard's Almanac.\n    Proposes a Plan of Union for the colonies\n    Chief events in Franklin's life.\n\nINTRODUCTORY NOTE\n\nBenjamin Franklin was born in Milk Street, Boston, on January 6, 1706.\nHis father, Josiah Franklin, was a tallow chandler who married twice,\nand of his seventeen children Benjamin was the youngest son. His\nschooling ended at ten, and at twelve he was bound apprentice to his\nbrother James, a printer, who published the \"New England Courant.\" To\nthis journal he became a contributor, and later was for a time its\nnominal editor. But the brothers quarreled, and Benjamin ran away, going\nfirst to New York, and thence to Philadelphia, where he arrived in\nOctober, 1723. He soon obtained work as a printer, but after a few\nmonths he was induced by Governor Keith to go to London, where, finding\nKeith's promises empty, he again worked as a compositor till he was\nbrought back to Philadelphia by a merchant named Denman, who gave him a\nposition in his business. On Denman's death he returned to his former\ntrade, and shortly set up a printing house of his own from which he\npublished \"The Pennsylvania Gazette,\" to which he contributed many\nessays, and which he made a medium for agitating a variety of local\nreforms. In 1732 he began to issue his famous \"Poor Richard's Almanac\"\nfor the enrichment of which he borrowed or composed those pithy\nutterances of worldly wisdom which are the basis of a large part of his\npopular reputation. In 1758, the year in which he ceased writing for the\nAlmanac, he printed in it \"Father Abraham's Sermon,\" now regarded as the\nmost famous piece of literature produced in Colonial America.";
    // uint8_t* input_packet = &_input_packet[0];
    // uint32_t chunk_bndry[] = {2, 10, 471, 621};     // WARNING: Hls::stream 'bit_pack_out_flag' is read while empty, which may result in RTL simulation hanging.
    // uint32_t chunk_bndry[] = {50, 200, 471, 624};
    uint32_t chunk_bndry[] = {2500,1500,2000,2500,1636,1794,1821,2084,2479,2786};
    // uint32_t *chunk_bndry = (uint32_t*)calloc(4, sizeof(uint32_t));
    // chunk_bndry[0] = 50;
    // chunk_bndry[1] = 200;
    // chunk_bndry[2] = 471;
    // chunk_bndry[3] = 626;
    // uint32_t chunk_bndry[] = {152, 300, 471, 621};
  
    uint32_t num_chunks = 1;
    // uint8_t is_chunk_unique[] = {1, 1, 0, 1};
    uint8_t is_chunk_unique[] = {1, 1, 0, 1, 1, 1, 0, 1, 0, 1};
    // uint8_t *is_chunk_unique=&_is_chunk_unique[0];
    // uint32_t dup_chunk_head[] = {13, 22, 4, 6};
    uint32_t dup_chunk_head[] = {13, 22, 4, 6, 56, 87, 234, 6, 55, 99};
    // uint32_t* dup_chunk_head = &_dup_chunk_head[0];
    
    // Output file pointer
    uint8_t* output_file_HW = (uint8_t*)calloc(12000, sizeof(uint8_t));
    uint8_t* output_file_SW = (uint8_t*)calloc(12000, sizeof(uint8_t));
    // Output size pointer
    uint32_t* output_size_HW = (uint32_t*)calloc(1, sizeof(uint32_t));
    uint32_t* output_size_SW = (uint32_t*)calloc(1, sizeof(uint32_t));

    std::cout<<"Starting kernel\n";

    lzw_kernel(input_packet, chunk_bndry, num_chunks, is_chunk_unique, output_file_HW, output_size_HW, dup_chunk_head);
    lzw_SW(&input_packet[2], chunk_bndry, num_chunks, is_chunk_unique, output_file_SW, output_size_SW, dup_chunk_head);
    

    bool data_Equal = compare_outputs(output_file_SW, output_file_HW, (*output_size_HW));
    bool size_Equal = (*output_size_SW == *output_size_HW);

    std::cout << "Hardware output size = " << *output_size_HW << "\nSoftware output size = " << *output_size_SW << std::endl;
    std::cout << "OUTPUT SIZE " << (size_Equal ? "EQUAL" : "NOT EQUAL") << std::endl;
    std::cout << "TESTBENCH " << (data_Equal ? "PASSED" : "FAILED") << "\n\n";

    free(output_file_HW);
    free(output_file_SW);
    free(output_size_HW);
    free(output_size_SW);
    // free(chunk_bndry);
}
