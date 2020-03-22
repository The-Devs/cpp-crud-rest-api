#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <algorithm>
#include <typeinfo>

#include "libs/cpp-httplib/httplib.h"
#include "src/phplike-utils/json.cpp"
#include "src/DatabaseConnection.cpp"
#include "src/ResponseTemplate.cpp"

using namespace std;

int main ( int argc, char** argv ) {

    string username = "enriquerene";
    string password = "um2tres45";
    string host = "tcp://127.0.0.1:3306";
    string dbname = "theadmin";
    string jsonFromFile = "{\"0\":\"Operação realizada com sucesso.\",\"1\":\"Recurso não encontrado.\",\"2\":\"Erro ao utilizar recurso.\",\"3\":\"Senha de acesso inválida.\",\"4\":\"Operação interrompida. Já existe uma instalação anterior.\"}";

    httplib::Server svr;
    svr.Get( "/", []( const httplib::Request& req, httplib::Response& res ) {
        // make this "Hello World" as a .html file
        res.set_content( req.path, "text/plain" );
    } );

    svr.Get( R"(/crud/(\w+))", [&](const httplib::Request& req, httplib::Response& res) {
        string tableName = req.matches[1];
        // size_t found = url.find( "?" );
        // vector<string> urlPieces;
        // string strParams;
        // if ( found == string::npos ) {
        //     tableName = url;
        // } else {
        //     urlPieces = plu::Array::explode( '?', url );
        //     tableName = urlPieces[ 0 ];
        //     strParams = urlPieces[ 1 ];
        // }
        thedevs::DatabaseConnection conn( host, username, password, dbname );
        thedevs::ResponseTemplate resTemp( jsonFromFile );

        conn.Select( tableName );
        // if ( strParams.size() > 0 )
        // strPairs = explode strParams
        // 
        // WebData::mapfy( explode & strParams )
        conn.build();
        vector<map<string,string>> dbResult = conn.getRows();
        conn.clear();
        res.set_content( resTemp.build( 0, dbResult ), "application/json" );
    });

    svr.Post( R"(/crud/(\w+))", [&](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader) {
        string tableName = req.matches[1];
        string body;
        vector<map<string,string>> emptyData;
        thedevs::ResponseTemplate resTemp( jsonFromFile );
        thedevs::DatabaseConnection conn( host, username, password, dbname );
        content_reader( [&]( const char *data, size_t data_length ) {
            body.append( data, data_length );
            return true;
        } );
        conn.Insert( tableName, plu::Json::json_decode( body ) );
        try {
            conn.build();
        } catch ( sql::SQLException &e ) {
            res.set_content( resTemp.build( 2, emptyData ), "application/json" );
        }
        conn.clear();
        
        res.set_content( resTemp.build( 0, emptyData ), "application/json" );
    });

    svr.Get( R"(/crud/(\w+)/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        string tableName = req.matches[ 1 ];
        string id = req.matches[ 2 ];
        map<string,string> where = { { "id", id } };
        
        thedevs::DatabaseConnection conn( host, username, password, dbname );
        string jsonFromFile = "{\"0\":\"Operação realizada com sucesso.\",\"1\":\"Recurso não encontrado.\",\"2\":\"Erro ao utilizar recurso.\",\"3\":\"Senha de acesso inválida.\",\"4\":\"Operação interrompida. Já existe uma instalação anterior.\"}";
        thedevs::ResponseTemplate resTemp( jsonFromFile );

        conn.Select( tableName );
        conn.where( where );
        conn.build();
        vector<map<string,string>> dbResult = conn.getRows();
        conn.clear();
        res.set_content( resTemp.build( 0, dbResult ), "application/json" );
    });


    // svr.Get( R"(/crud/(\w+)/(\w+))", [&]( const httplib::Request& req, httplib::Response& res ) {
    //     auto tableName = req.matches[1];
    //     res.set_content( tableName, "text/plain" );
    // });

    svr.Get( "/stop", [&](const httplib::Request& req, httplib::Response& res) {
        svr.stop();
    });


    svr.listen("localhost", 8100);

    // get dbinfo and port via argc and argv
    // if ( argc < 2 ) {
    //     cout << "Missing table name" << endl;
    //     // if ( argc < 2 ) {
    //     //     cout << "Missing table prefix" << endl;
    //     // }
    //     return 1;
    // }

    // string tableName = "firstTable"; // argv[ 1 ];

    
    // cout << endl;

    return EXIT_SUCCESS;
}
// g++ -I /usr/local/include/mysql-connector/include/jdbc/ app.cpp -o crud -lmysqlcppconn
// g++ -std=c++11 -I /usr/local/include/mysql-connector/include/jdbc/ app.cpp -o crud -lmysqlcppconn -pthread