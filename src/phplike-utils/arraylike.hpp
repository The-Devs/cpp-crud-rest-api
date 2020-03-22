#ifndef PLU_ARRAY_H
#define PLU_ARRAY_H

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>

using namespace std;
namespace plu {
namespace Array {
    vector<string> explode ( char delimiter, string str );

    string implode ( string delimiter, vector<string> arr );

    vector<string> array_keys ( map<string,string> arr );

    vector<string> array_values ( map<string,string> arr );

    map<string,string> array_combine ( vector<string> keys, vector<string> vals );

    vector<string> array_xppend ( string prefix, vector<string> arr, bool asPrefix);

    vector<string> array_embrace ( string str, vector<string> arr );

    vector<string> array_unbrace ( vector<string> arr );


} // plu namespace
} // thedevs namespace
#endif