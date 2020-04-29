#include <stdlib.h>
#include <iostream>
#include <vector>
#include <map>
#include <bits/stdc++.h>
#include <algorithm>
#include <typeinfo>
#include <fstream>
#include <time.h>

#include "libs/cpp-httplib/httplib.h"
#include "src/phplike-utils/json.cpp"
#include "src/DatabaseConnection.cpp"
#include "src/ResponseTemplate.cpp"

using namespace std;


long timeBetween( long before, long after ) {
	return ( after - before ) * 1000 / CLOCKS_PER_SEC;
}

string fileContent ( string fileName ) {
	cout << "fileContent function" << endl;
	fstream file( fileName );
	if ( ! file.is_open() ) {
		exit( 1 );
	}
	string line;
	string content = "";
	while ( getline( file, line ) ) {
		content += line;
	}
	file.close();
	
	cout << "file closed, read content:  " << content << endl << endl;
	return content;
}

int main ( int argc, char** argv ) {
	if ( argc != 4 ) {
		if ( argc > 4 ) {
			cout << "Muitos argumentos inseridos!" << endl;
		}
		if ( argc < 4 ) {
			cout << "Faltam argumentos!" << endl;
		}
		cout << "1. Nome do servidor (ex: localhost)" << endl;
		cout << "2. Porta de acesso (ex: 8100)" << endl;
		cout << "3. Arquivo .json com informações do banco de dados (ex: database.json)" << endl;
		return 1;
	}
	time_t t[ 2 ];

	/* Read file argv[ 3 ] */
	string jsonString = fileContent( argv[ 3 ] );

	map<string,string> db = plu::Json::json_decode( jsonString );
	string host = db[ "protocol" ] + "://" + db[ "host" ] + ":" + db[ "port" ];
	

	/* Read file response-messages.json */
	string jsonFromFile = fileContent( "response-messages.json" );


	httplib::Server svr;
	cout << "Inicializando servidor C++ CRA" << endl;
	cout << "Nome do servidor: " << argv[ 1 ] << endl;
	cout << "Porta de acesso: " << argv[ 2 ] << endl;
	cout << "Arquivo para conexão com Banco de dados: " << argv[ 3 ] << endl;

	thedevs::ResponseTemplate resTemp( jsonFromFile );

	svr.set_logger( []( const httplib::Request& req, const httplib::Response& res ) {
		string lineLogFile = "ip=" + req.get_header_value( "REMOTE_ADDR" ) + "\t";
		lineLogFile += "ver=" + req.version + "\t";
		lineLogFile += "meth=" + req.method + "\t";
		lineLogFile += "targ=" + req.target + "\t";
		lineLogFile += "body=" + req.body + "\t";
		lineLogFile += "token=" + ( req.has_header( "X-Auth-Token" ) ? req.get_header_value( "X-Auth-Token" ) : "no" );
	
		cout << endl << lineLogFile << endl;
	} );

	svr.set_error_handler( [&]( const httplib::Request& req, httplib::Response& res )  {
		res.set_content( resTemp.build( 5, {} ), "application/json" );
	} );
	
	svr.Get( "/", []( const httplib::Request& req, httplib::Response& res ) {
		string html = "<a href=\"https://github.com/The-Devs\">The Devs - Desenvolvimento de Software</a>";
		res.set_content( html, "text/html" );
	} );

	svr.Delete( "/", [&]( const httplib::Request& req, httplib::Response& res ) {
		cout << "Encerrando servidor C++ CRA" << endl;
		svr.stop();
	});


	svr.Get( R"(/crud/(\w+))", [&](const httplib::Request& req, httplib::Response& res) {
		t[ 0 ] = clock();
		string tableName = req.matches[ 1 ];
		string key, value;
		map<string,string> whereConds;
		for ( auto quPrms = req.params.begin(); quPrms != req.params.end(); quPrms++ ) {
			key = quPrms->first;
			value = quPrms->second;
			cout << key << ": " << value << endl;
			whereConds[ key ] = value;
		}
		
		thedevs::DatabaseConnection conn( host, db[ "username" ], db[ "password" ], db[ "name" ] );

		conn.Select( tableName );
		conn.where( whereConds );
		conn.build();
		vector<map<string,string>> dbResult = conn.getRows();
		conn.clear();
		int respStatus = ( dbResult.size() > 0 ) ? 0 : 1;
		t[ 1 ] = clock();
		cout << endl << "Tempo de execução no servidor: " << timeBetween( t[ 0 ], t[ 1 ] ) << endl << endl;
		res.set_content( resTemp.build( respStatus, dbResult ), "application/json" );
	});

	svr.Post( R"(/crud/(\w+))", [&](const httplib::Request &req, httplib::Response &res, const httplib::ContentReader &content_reader) {
		string tableName = req.matches[1];
		string body;
		vector<map<string,string>> emptyData;
		thedevs::DatabaseConnection conn( host, db[ "username" ], db[ "password" ], db[ "name" ] );
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
		thedevs::DatabaseConnection conn( host, db[ "username" ], db[ "password" ], db[ "name" ] );
		conn.Delete( tableName );
		conn.build();
		conn.clear();
		res.set_content( resTemp.build( 0, {} ), "application/json" );
	} );


	svr.Get( R"(/crud/(\w+)/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
		string tableName = req.matches[ 1 ];
		string id = req.matches[ 2 ];
		map<string,string> where = { { "id", id } };
		
		thedevs::DatabaseConnection conn( host, db[ "username" ], db[ "password" ], db[ "name" ] );

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

		thedevs::DatabaseConnection conn( host, db[ "username" ], db[ "password" ], db[ "name" ] );
		conn.Delete( tableName );
		conn.where( where );
		conn.build();
		conn.clear();
		res.set_content( resTemp.build( 0, {} ), "application/json" );
	} );

	svr.Put( R"(/crud/(\w+)/(\d+))", [&](const httplib::Request& req, httplib::Response& res, const httplib::ContentReader &content_reader) {
		string tableName = req.matches[ 1 ];
		string id = req.matches[ 2 ];
		map<string,string> where = { { "id", id } };
		string body;
		vector<map<string,string>> emptyData;
		thedevs::DatabaseConnection conn( host, db[ "username" ], db[ "password" ], db[ "name" ] );
		content_reader( [&]( const char *data, size_t data_length ) {
			body.append( data, data_length );
			return true;
		} );
		conn.Update( tableName, plu::Json::json_decode( body ) );
		conn.where( where );
		try {
			conn.build();
			conn.clear();
			res.set_content( resTemp.build( 0, emptyData ), "application/json" );
		} catch ( sql::SQLException &e ) {
			res.set_content( resTemp.build( 2, emptyData ), "application/json" );
		}
	});

	cout << "Listen to ..." << argv[ 1 ] << ":" << argv[ 2 ] << endl;
	svr.listen( argv[ 1 ], atoi( argv[ 2 ] ) );

	cout << "Finishing..." << endl;
	return EXIT_SUCCESS;
}