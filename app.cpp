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
    string dbhost = "127.0.0.1";
    string protocol = "tcp";
    int dbport = 3306;
    string host = protocol + "://" + dbhost + ":" + to_string( dbport );
    string dbname = "theadmin";
    int port = 8100;

    string jsonFromFile = "{\"0\":\"Operação realizada com sucesso.\",\"1\":\"Recurso não encontrado.\",\"2\":\"Erro ao utilizar recurso.\",\"3\":\"Senha de acesso inválida.\",\"4\":\"Operação interrompida. Já existe uma instalação anterior.\"}";

    httplib::Server svr;
    cout << "Inicializando servidor C++ CRA" << endl;
    thedevs::ResponseTemplate resTemp( jsonFromFile );
    
    svr.Get( "/", []( const httplib::Request& req, httplib::Response& res ) {
        string html = "<a href=\"https://github.com/The-Devs\">The Devs - Desenvolvimento de Software</a>";
        res.set_content( html, "text/html" );
    } );
    svr.Delete( "/", [&](const httplib::Request& req, httplib::Response& res) {
        cout << "Encerrando servidor C++ CRA" << endl;
        svr.stop();
    });

    svr.Get( R"(/crud/(\w+))", [&](const httplib::Request& req, httplib::Response& res) {
        string tableName = req.matches[1];
        cout << req.path << endl;
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

        conn.Select( tableName );
        // where (here)
        conn.build();
        vector<map<string,string>> dbResult = conn.getRows();
        conn.clear();
        int respStatus = ( dbResult.size() > 0 ) ? 0 : 1;
        res.set_content( resTemp.build( respStatus, dbResult ), "application/json" );
    });
    svr.Post( R"(/crud/(\w+))", [&](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader) {
        string tableName = req.matches[1];
        string body;
        vector<map<string,string>> emptyData;
        thedevs::DatabaseConnection conn( host, username, password, dbname );
        content_reader( [&]( const char *data, size_t data_length ) {
            body.append( data, data_length );
            return true;
        } );
        conn.Insert( tableName, plu::Json::json_decode( body ) );
        try {
            conn.build();
            conn.clear();
            res.set_content( resTemp.build( 0, emptyData ), "application/json" );
        } catch ( sql::SQLException &e ) {
            res.set_content( resTemp.build( 2, emptyData ), "application/json" );
        }
    });
    svr.Delete( R"(/crud/(\w+))", [&]( const httplib::Request& req, httplib::Response& res ) {
        string tableName = req.matches[ 1 ];
        thedevs::DatabaseConnection conn( host, username, password, dbname );
        conn.Delete( tableName );
        conn.build();
        conn.clear();
        res.set_content( resTemp.build( 0, {} ), "application/json" );
    } );

    /* Query params here? */
    svr.Get( R"(/crud/(\w+)/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        string tableName = req.matches[ 1 ];
        string id = req.matches[ 2 ];
        map<string,string> where = { { "id", id } };
        
        thedevs::DatabaseConnection conn( host, username, password, dbname );

        conn.Select( tableName );
        conn.where( where );
        conn.build();
        vector<map<string,string>> dbResult = conn.getRows();
        conn.clear();
        int respStatus = ( dbResult.size() > 0 ) ? 0 : 1;
        res.set_content( resTemp.build( respStatus, dbResult ), "application/json" );
    });
    svr.Delete( R"(/crud/(\w+)/(\d+))", [&]( const httplib::Request& req, httplib::Response& res ) {
        string tableName = req.matches[ 1 ];
        string id = req.matches[ 2 ];
        map<string,string> where = { { "id", id } };

        thedevs::DatabaseConnection conn( host, username, password, dbname );
        conn.Delete( tableName );
        conn.where( where );
        conn.build();
        conn.clear();
        res.set_content( resTemp.build( 0, {} ), "application/json" );
    } );
    svr.Put( R"(/crud/(\w+)/(\d+))", [&](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader) {
        /*
        string tableName = req.matches[1];
        string body;
        vector<map<string,string>> emptyData;
        thedevs::DatabaseConnection conn( host, username, password, dbname );
        content_reader( [&]( const char *data, size_t data_length ) {
            body.append( data, data_length );
            return true;
        } );
        conn.Update( tableName, plu::Json::json_decode( body ) );
        try {
            conn.build();
            conn.clear();
            res.set_content( resTemp.build( 0, emptyData ), "application/json" );
        } catch ( sql::SQLException &e ) {
            res.set_content( resTemp.build( 2, emptyData ), "application/json" );
        }
        */
    });

    

    svr.set_error_handler([](const httplib::Request & /*req*/, httplib::Response &res) {
        const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
        char buf[BUFSIZ];
        snprintf(buf, sizeof(buf), fmt, res.status);
        res.set_content(buf, "text/html");
    });


    svr.listen( "localhost", port );

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