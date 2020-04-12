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
bool thedevs::DatabaseConnection::isDebugOn () {
    return this->debug;
}
void thedevs::DatabaseConnection::debugOn () {
    this->debug = true;
}
void thedevs::DatabaseConnection::debugOff () {
    this->debug = false;
}

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

string thedevs::DatabaseConnection::orderHandler () {
    map<string,string> orderConditions = thedevs::DatabaseConnection::getOrderConditions();
    string prefix = thedevs::DatabaseConnection::getTablePrefix( thedevs::DatabaseConnection::getTableName() );
    vector<string> vals = plu::Array::array_values( orderConditions );
    vector<string> keys = plu::Array::array_keys( orderConditions );
    keys = plu::Array::array_xppend( prefix, keys, true );
    vector<string> sqlPieces;
    string keyString;
    string valString;
    vector<string>::iterator vIt = vals.begin();
    for ( vector<string>::iterator kIt = keys.begin(); kIt != keys.end(); kIt++, vIt++ ) {
        keyString = *kIt;
        valString = *vIt;
        sqlPieces.push_back( keyString + " " + valString );
    }
    string sql = " ORDER BY ";
    sql += plu::Array::implode( ", ", sqlPieces );
    
    return sql;
}

string thedevs::DatabaseConnection::paginationHandler () {
    int pagination = this->getPagination();
    int offset = ( this->getPage() - 1 ) * pagination;
    return " LIMIT " + to_string( pagination ) + " OFFSET " + to_string( offset );
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

void thedevs::DatabaseConnection::order ( map<string,string> orderConditions ) {
    this->orderConditions = orderConditions;
}

map<string,string> thedevs::DatabaseConnection::getWhereConditions () {
    return this->whereConditions;
}

map<string,string> thedevs::DatabaseConnection::getOrderConditions () {
    return this->orderConditions;
}

void thedevs::DatabaseConnection::setWhereJoinner ( string whereJoinner ) {
    this->whereJoinner = whereJoinner;
}

string thedevs::DatabaseConnection::getWhereJoinner () {
    return this->whereJoinner;
}

int thedevs::DatabaseConnection::getPagination () {
    return this->pagination;
}

int thedevs::DatabaseConnection::getPage () {
    return this->page;
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

void thedevs::DatabaseConnection::Update ( string tableName, map<string,string> data ) {
    string sqlFields, sqlVals;
    string key, val;
    vector<string> pairs;
    for ( map<string,string>::iterator i = data.begin(); i != data.end(); i++ ) {
        key = this->getTablePrefix( tableName ) + i->first;
        val = plu::String::str_embrace( "'", i->second );
        pairs.push_back( key + "=" + val );
    }
    
    string sqlPairs = plu::Array::implode( ",", pairs );
    
    this->setTableName( tableName );
    this->setCRUDType( 'U' );
    this->setCRUD( "UPDATE " + this->getPrefix() + tableName + " SET " + sqlPairs );
}

void thedevs::DatabaseConnection::Delete ( string tableName ) {
    this->setTableName( tableName );
    this->setCRUDType( 'D' );
}

bool thedevs::DatabaseConnection::build () {
    if ( this->getTableName() == "" ) return false;
    string tableName = this->getTableName();
    vector<string> fields = this->getTableFields( tableName, true );
    sql::Statement *stmt;
    sql::ResultSet *res;
    map<string,string> row;
    vector<map<string,string>> rows;

    // Fixing Delete request, change between delete and truncate statements
    if ( this->getCRUDType() == 'D' ) {
        if ( ! this->getWhereConditions().empty() ) {
            this->setCRUD( "DELETE FROM " + this->getPrefix() + this->getTableName() );
        } else {
            this->setCRUD( "TRUNCATE " + this->getPrefix() + this->getTableName() );
        }
    }

    try {
        string sql = this->getCRUD();
        stmt = this->connection->createStatement();
        if ( ! this->getWhereConditions().empty() ) {
            sql += this->whereHandler();
        }
        if ( ! this->getOrderConditions().empty() ) {
            sql += this->orderHandler();
        }
        if ( this->getPagination() > 0 ) {
            sql += this->paginationHandler();
        }
        if ( this->isDebugOn() ) {
            cout << "Building sql: " << sql << endl;
        }
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
        cout << endl;
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
        return false;
    }
}

#endif