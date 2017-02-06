#include <iostream>
#include <string>

using namespace std;

string int_to_string(int x){
    string s;
    while(x != 0){
        if(x % 2 == 0) s += "C";
        else s += "G";
        x /= 2;
    }
    return s;
}

int main(int argc, char** argv){
    if(argc != 3){
        cerr << "Usage: ./program number_of_strings overlap_length" << endl;
        return 0;
    }

    int n_strings = stoi(argv[1]);
    int overlap_length = stoi(argv[2]);

    for(int i = 0; i < n_strings; i++){
        cout << ">" << i << "\n";
        string read = "";
        for(int j = 0; j < overlap_length; j++) read += "A";
        read += int_to_string(i);
        for(int j = 0; j < overlap_length; j++) read += "A";
        cout << read << "\n";
    }
}
