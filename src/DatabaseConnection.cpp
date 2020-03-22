#ifndef TD_DBCONNECTION_CPP
#define TD_DBCONNECTION_CPP

#include <vector>
#include <string>
#include <map>
#include <algorithm>

#include "DatabaseConnection.hpp"
#include "phplike-utils/arraylike.cpp"

#include "mysql_connection.h"

#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"


using namespace std;


// private:
void thedevs::DatabaseConnection::setPrefix ( string prefix ) {
    this->prefix = prefix;
}

string thedevs::DatabaseConnection::getPrefix () {
    return this->prefix;
}

string thedevs::DatabaseConnection::getTablePrefix ( string tableName ) {
    return this->tableStructure[ tableName ][ "prefix" ];
}

void thedevs::DatabaseConnection::setTableName ( string tableName ) {
    this->tableName = tableName;
}

string thedevs::DatabaseConnection::getTableName () {
    return this->tableName;
}

string thedevs::DatabaseConnection::prefixIt ( string tableName, string fieldName ) {
    return this->getTablePrefix( tableName ) + fieldName;
}

string thedevs::DatabaseConnection::whereHandler () {
    map<string,string> whereConditions = thedevs::DatabaseConnection::getWhereConditions();
    string prefix = thedevs::DatabaseConnection::getTablePrefix( thedevs::DatabaseConnection::getTableName() );
    vector<string> vals = plu::Array::array_values( whereConditions );
    vector<string> keys = plu::Array::array_keys( whereConditions );
    keys = plu::Array::array_xppend( prefix, keys, true );
    vector<string> sqlPieces;
    string keyString;
    string valString;
    vector<string>::iterator vIt = vals.begin();
    for ( vector<string>::iterator kIt = keys.begin(); kIt != keys.end(); kIt++, vIt++ ) {
        keyString = *kIt;
        valString = *vIt;
        sqlPieces.push_back( keyString + " " + thedevs::DatabaseConnection::getWhereJoinner() + " '" + valString + "'" );
    }
    string sql = " WHERE ";
    sql += plu::Array::implode( " AND ", sqlPieces );
    
    return sql;
}

// public:
thedevs::DatabaseConnection::DatabaseConnection( string host, string username, string password, string dbname ) {
    try {
        sql::Driver *driver;

        /* Create a connection */
        driver = get_driver_instance();
        this->connection = driver->connect( host, username, password );
        this->connection->setSchema( dbname );
        // delete this->connection;

    } catch (sql::SQLException &e) {
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
    }
}

void thedevs::DatabaseConnection::clear ( void ) {
    delete this->connection;
}

void thedevs::DatabaseConnection::clearWhere ( void ) {
    this->whereConditions.clear();
}

void thedevs::DatabaseConnection::where ( map<string,string> whereConditions ) {
    this->whereConditions = whereConditions;
}

map<string,string> thedevs::DatabaseConnection::getWhereConditions () {
    return this->whereConditions;
}

void thedevs::DatabaseConnection::setWhereJoinner ( string whereJoinner ) {
    this->whereJoinner = whereJoinner;
}

string thedevs::DatabaseConnection::getWhereJoinner () {
    return this->whereJoinner;
}

vector<string> thedevs::DatabaseConnection::getTableFields ( string tableName, bool withPrefix ) {
    string inlineFields = this->tableStructure[ tableName ][ "fields" ];
    vector<string> arr = plu::Array::explode( ',', inlineFields );
    string tmpField = "";
    size_t pos; 
    vector<string> v;
    for ( vector<string>::iterator i = arr.begin(); i != arr.end(); i++ ) {
        tmpField = *i;
        pos = tmpField.find( '[' );

        v.push_back( tmpField.substr( 0, pos ) );
    }
    if ( withPrefix ) {
        return plu::Array::array_xppend( this->getTablePrefix( tableName ), v, true );
    }
    return v;
}

void thedevs::DatabaseConnection::setCRUD ( string crud ) {
    this->crud = crud;
}

string thedevs::DatabaseConnection::getCRUD () {
    return this->crud;
}

void thedevs::DatabaseConnection::setCRUDType ( char crudType ) {
    this->crudType = crudType;
}

char thedevs::DatabaseConnection::getCRUDType () {
    return this->crudType;
}

void thedevs::DatabaseConnection::setRows ( vector<map<string,string>> rows ) {
    this->rows = rows;
}

vector<map<string,string>> thedevs::DatabaseConnection::getRows () {
    return this->rows;
}

void thedevs::DatabaseConnection::Select ( string tableName, vector<string> fields ) {
    string sqlFields;
    if ( fields.empty() ) {
        sqlFields = "*";
    } else {
        string prefix = this->getTablePrefix( tableName );
        vector<string> pfxFields = plu::Array::array_xppend( prefix, fields, true );
        sqlFields = plu::Array::implode( ",", pfxFields );
    }
    this->setTableName( tableName );
    this->setCRUDType( 'R' );
    this->setCRUD( "SELECT " + sqlFields + " FROM " + this->getPrefix() + tableName );
}

void thedevs::DatabaseConnection::Insert ( string tableName, map<string,string> data ) {
    string sqlFields, sqlVals;
    vector<string> keys, vals;
    keys = plu::Array::array_keys( data );
    vals = plu::Array::array_values( data );
    keys = plu::Array::array_xppend( this->getTablePrefix( tableName ), keys, true );
    vals = plu::Array::array_embrace( "'", vals );
    sqlFields = plu::Array::implode( ",", keys );
    sqlVals = plu::Array::implode( ",", vals );
    
    this->setTableName( tableName );
    this->setCRUDType( 'C' );
    this->setCRUD( "INSERT INTO " + this->getPrefix() + tableName + " (" + sqlFields  + ") VALUES (" + sqlVals  + ")" );
}

bool thedevs::DatabaseConnection::build () {
    if ( this->getTableName() == "" ) return false;
    string tableName = this->getTableName();
    string sql = this->getCRUD();
    vector<string> fields = this->getTableFields( tableName, true );
    sql::Statement *stmt;
    sql::ResultSet *res;
    map<string,string> row;
    vector<map<string,string>> rows;

    try {
        stmt = this->connection->createStatement();
        if ( ! this->getWhereConditions().empty() ) {
            sql += this->whereHandler();
        }
        // if ( ! this->getOrderConditions().empty() ) {
        //     sql += this->orderHandler( tableName );
        // }
        // if ( this->getPagination() > 0 ) {
        //     sql += this->paginationHandler( tableName );
        // }
        // cout << sql << endl;
        if ( this->getCRUDType() == 'R' ) {
            res = stmt->executeQuery( sql );
            while ( res->next() ) {
                for ( vector<string>::iterator i = fields.begin(); i !=  fields.end(); i++ ) {
                    string colName = *i;
                    row[ colName ] = res->getString( colName );
                }
                rows.push_back( row );
                row.clear();
            }
            this->setRows( rows );
        } else {
            stmt->execute( sql );
            return stmt->getUpdateCount() > 0;
        }
        delete res;
        delete stmt;
        return true;
    } catch ( sql::SQLException &e ) {
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
        return false;
    }
}

#endif