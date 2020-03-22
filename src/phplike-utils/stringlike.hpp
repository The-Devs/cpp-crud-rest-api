#ifndef PLU_STRING_H
#define PLU_STRING_H

#include <stdlib.h>
#include <iostream>
#include <string>

using namespace std;
namespace plu {
namespace String {
    string str_unbrace ( string str );

    string str_embrace ( string delimiter, string str );
} // plu namespace
} // thedevs namespace

#endif