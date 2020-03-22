#ifndef PLU_JSON_H
#define PLU_JSON_H

#include <stdlib.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;
namespace plu {
namespace Json {
    map<string,string> json_decode ( string json );

    string json_encode ( map<string,string> data );
} // Json namespace
} // plu namespace

#endif