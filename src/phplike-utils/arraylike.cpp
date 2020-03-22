#ifndef PLU_ARRAY_CPP
#define PLU_ARRAY_CPP

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <string>

#include "arraylike.hpp"

using namespace std;
vector<string> plu::Array::explode ( char delimiter, string str ) {
    string buff = "";
    vector<string> v;

    for ( auto n:str ) {
        if ( n != delimiter ) {
            buff+=n;
        } else
        if ( n == delimiter && buff != "" ) {
            v.push_back( buff );
            buff = "";
        }
    }
    if ( buff != "" ) {
        v.push_back( buff );
    }
    return v;
}

string plu::Array::implode ( string delimiter, vector<string> arr ) {
    string buff = "";

    for ( vector<string>::iterator i = arr.begin(); i != arr.end(); i++ ) {
        buff += *i;
        
        if ( i != arr.end() && ( i + 1 ) != arr.end() ) {
            buff += delimiter;
        }
    }
    return buff;
}

vector<string> plu::Array::array_keys ( map<string,string> arr ) {
    vector<string> v;
    string key;
    for ( map<string,string>::iterator i = arr.begin(); i != arr.end(); i++ ) {
        key = i->first;
        v.push_back( key );
    }
    return v;
}

vector<string> plu::Array::array_values ( map<string,string> arr ) {
    vector<string> v;
    string val;
    for ( map<string,string>::iterator i = arr.begin(); i != arr.end(); i++ ) {
        val = i->second;
        v.push_back( val );
    }
    return v;
}

map<string,string> plu::Array::array_combine ( vector<string> keys, vector<string> vals ) {
    map<string,string> m;
    string k, v;
    if ( keys.size() != vals.size() ) {
        throw "The keys and vals vectors must be the same std::string::size() returned value.";
    }
    vector<string>::iterator i, j;
    for ( i = keys.begin(); i != keys.end(); i++, j++ ) {
        k = *i;
        v = *j;
        m[ k ] = v;
    }
    return m;
}

vector<string> plu::Array::array_xppend ( string prefix, vector<string> arr, bool asPrefix ) {
    string entry;
    vector<string> v;
    string str;
    for ( vector<string>::iterator i = arr.begin(); i != arr.end(); i++ ) {
        entry = *i;
        if ( asPrefix ) {
            str = prefix + entry;
        } else {
            str = entry + prefix;
        }
        v.push_back( str );
    }
    return v;
}

vector<string> plu::Array::array_embrace ( string str, vector<string> arr ) {
    vector<string> v = plu::Array::array_xppend( str, arr, true );
    return plu::Array::array_xppend( str, v, false );
}

vector<string> plu::Array::array_unbrace ( vector<string> arr ) {
    vector<string> v;
    string s, o;
    for ( vector<string>::iterator i = arr.begin(); i != arr.end(); i++ ) {
        s = *i;
        o = "";
        for ( string::iterator j = s.begin() + 1; j != s.end() - 1; j++ ) {
            o += *j;
        }
        v.push_back( o );
    }
    return v;
}

#endif