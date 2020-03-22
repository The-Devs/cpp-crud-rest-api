#ifndef PLU_STRING_CPP
#define PLU_STRING_CPP

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "stringlike.hpp"

using namespace std;
string plu::String::str_unbrace ( string str ) {
    string s = "";
    for ( string::iterator i = str.begin() + 1; i != str.end() - 1; i++ ) {
        s += *i;
    }
    return s;
}

string plu::String::str_embrace ( string delimiter, string str ) {
    return delimiter + str + delimiter;
}

#endif