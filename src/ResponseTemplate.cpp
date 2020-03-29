#ifndef TD_RESTEMPLATE_CPP
#define TD_RESTEMPLATE_CPP

#include <vector>
#include <string>
#include <map>

#include "ResponseTemplate.hpp"
#include "phplike-utils/json.cpp"
// #include "../libs/json-for-modern-cpp/json.hpp"

using namespace std;
// private:
// 
// JSON OBJ
// unbrace
// explode ,
// unbrace array_keys
// to Insert method
//

// public:
thedevs::ResponseTemplate::ResponseTemplate ( string str ) {
    map<string,string> m = plu::Json::json_decode( str );
    map<int,string> p;
    for ( map<string,string>::iterator i = m.begin(); i != m.end(); i++ ) {
        p[ stoi( i->first ) ] = i->second;
    }
    this->errorMessage = p;
}

string thedevs::ResponseTemplate::build ( int error, vector<map<string,string>> data ) {
    cout << "error inside: " << to_string( error ) << endl;
    string str = "{";
    str +=  "\"error\":" + to_string( error ) + ",";
    str +=  "\"message\":\"" + this->getMessage( error ) + "\",";
    str +=  "\"dataLenght\":" + to_string( data.size() ) + ",";
    str +=  "\"data\":" + this->handleData( data ) + ",";
    str +=  "\"page\":" + to_string( this->getPage() ) + ",";
    str +=  "\"maxDataSize\":" + to_string( this->getMaxDataSize() );
    str += "}";
    return str;
}

string thedevs::ResponseTemplate::handleData ( vector<map<string,string>> data ) {
    vector<string> v;
    if ( data.empty() ) return "[]";
    for ( int i = 0; i < data.size(); i++ ) {
        v.push_back( plu::Json::json_encode( data[ i ] ) );
    }
    string str = "[" + plu::Array::implode( ",", v ) + "]";
    return str;
}

void thedevs::ResponseTemplate::setMessage ( string message ) {
    this->message = message;
}

string thedevs::ResponseTemplate::getMessage ( int error ) {
    if ( this->message.size() > 0 ) {
        return this->message;
    }
    return this->errorMessage[ error ];
}

void thedevs::ResponseTemplate::setPage ( int page ) {
    this->page = page;
}

int thedevs::ResponseTemplate::getPage () {
    return this->page;
}

void thedevs::ResponseTemplate::setMaxDataSize ( int maxDataSize ) {
    this->maxDataSize = maxDataSize;
}

int thedevs::ResponseTemplate::getMaxDataSize () {
    return this->maxDataSize;
}

#endif