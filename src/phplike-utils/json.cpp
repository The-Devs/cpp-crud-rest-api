#ifndef PLU_JSON_CPP
#define PLU_JSON_CPP

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>

#include "json.hpp"
// #include "../../libs/json-for-modern-cpp/json.hpp"
#include "arraylike.cpp"
#include "stringlike.cpp"

using namespace std;
map<string,string> plu::Json::json_decode ( string json ) {
    map<string,string> m;
    vector<string> pair, strPairs = plu::Array::explode( ',', plu::String::str_unbrace( json ) );
    for ( vector<string>::iterator i = strPairs.begin(); i != strPairs.end(); i++ ) {
        pair = plu::Array::explode( ':', *i );
        m[ plu::String::str_unbrace( pair[ 0 ] ) ] = plu::String::str_unbrace( pair[ 1 ] );
    }
    return m;
}

string plu::Json::json_encode ( map<string,string> data ) {
    vector<string> v;
    string str = "{";
    for ( map<string,string>::iterator i = data.begin(); i != data.end(); i++ ) {
        v.push_back( plu::String::str_embrace( "\"", i->first ) + ":" + plu::String::str_embrace( "\"", i->second ) );
    }
    str += plu::Array::implode( ", ", v ) + "}";
    return str;
}

#endif