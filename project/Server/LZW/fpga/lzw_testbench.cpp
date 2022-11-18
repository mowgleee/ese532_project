#include <bits/stdc++.h>
#include "lzw_kernel.h"

using namespace std;

vector<int> lzw_SW(string s1)
{
    cout << "Encoding\n";
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
    cout << "String\tOutput_Code\tAddition\n";
    for (int i = 0; i < s1.length(); i++) {
        if (i != s1.length() - 1)
            c += s1[i + 1];
        if (table.find(p + c) != table.end()) {
            p = p + c;
        }
        else {
            cout << p << "\t" << table[p] << "\t\t"
                 << p + c << "\t" << code << endl;
            output_code.push_back(table[p]);
            table[p + c] = code;
            code++;
            p = c;
        }
        c = "";
    }
    cout << p << "\t" << table[p] << endl;
    output_code.push_back(table[p]);
    return output_code;
}

bool compare_outputs(uint8_t* sw_output_code, uint8_t *hw_output_code, uint32_t size)
{
    bool Equal = true;
    for (int i=0; i<size; i++)
    {
        if(sw_output_code[i] != hw_output_code[i])
        {
            cout<<"SW out code: "<<sw_output_code[i]<<" HW out code: "<<hw_output_code[i]<<'\n';
            Equal = false;
        }
    }
    return Equal;
}

int main()
{
    string s = "abc";// gdgserge yy66ey   &&**Ggg *GGGGGGGGGGGGGGabc gdgserge yy66ey   &&**Ggg *GGGGGGGGGGGGGGabc gdgserge yy66ey   &&**Ggg *GGGGGGGGGGGGGGabc gdgserge yy66ey   &&**Ggg *GGGGGGGGGGGGGGabc gdgserge yy66ey   &&**Ggg *GGGGGGGGGGGGGGabc gdgserge yy66ey   &&**Ggg *GGGGGGGGGGGGGGG";
    // unsigned char s_char[3] = {'a','b','c'};
    // uint32_t hw_output_code[8096]={0};
    // uint32_t output_code_size = 0;
    uint8_t* s_char = (uint8_t*)calloc(4,sizeof(uint8_t));
    uint8_t* hw_output_code = (uint8_t*)calloc(8096, sizeof(uint8_t));
    // s_char[0] = 'a';
    // s_char[1] = 'b';
    // s_char[2] = 'c';

    for(int i = 0; i < 4; i++)
    {
        s_char[i] = s[i];
    }

    uint32_t* output_code_size = (uint32_t*)calloc(1,sizeof(uint32_t));

    lzw_kernel(s_char, 4, hw_output_code, output_code_size);
    vector<int> sw_output_code = lzw_SW(s);
    cout << endl;


    uint8_t sw_output_code_packed[8096]={0};
    uint32_t offset_pack = 0;

    uint32_t curr_code = 0;
	uint32_t write_data = 0;
	uint32_t old_byte = 0;
	uint32_t rem_bits = 0;
	uint32_t running_bits = 0;
	uint32_t write_byte_size = 0;//number of bytes to write
	uint8_t write_byte = 0;//whats written in file


        for(int idx=0; idx<sw_output_code.size(); idx++)
	{
		curr_code = sw_output_code[idx];
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
		// std::bitset<32> x(old_byte);
		// std::cout<<"No of Remaining Bits:"<<rem_bits<<"\n";
		// std::cout<<"Remaining Bits:"<<x<<"\n";
		write_byte = old_byte>>24;
		// std::bitset<8> y(write_byte);
		// std::cout<<"Remaining Byte:"<<y<<"\n";
		//memcpy(&file[offset], &write_byte, sizeof(unsigned char));

		// offset+= sizeof(unsigned char);

        // write_byte = write_data>>(32-i*8);
        sw_output_code_packed[offset_pack] = write_byte;
        offset_pack +=1;
	}




    bool Equal = compare_outputs(sw_output_code_packed, hw_output_code, offset_pack);
    std::cout << "TEST " << (Equal ? "PASSED" : "FAILED") << std::endl;
}