
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

bool compare_outputs(vector<int> sw_output_code, uint64_t *hw_output_code)
{
    bool Equal = true;
    for (int i=0; i<sw_output_code.size(); i++)
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
    string s = "WYS*WYGWYS*WYSWYSG\"";
    unsigned char s_char[20] = "WYS*WYGWYS*WYSWYSG\"";
    uint64_t hw_output_code[8096]={0};
    uint64_t output_code_size = 0;

    lzw_kernel(s_char, 20, hw_output_code, &output_code_size);
    vector<int> sw_output_code = lzw_SW(s);
    cout << endl;

    bool Equal = compare_outputs(sw_output_code, hw_output_code);
    std::cout << "TEST " << (Equal ? "PASSED" : "FAILED") << std::endl;
}
