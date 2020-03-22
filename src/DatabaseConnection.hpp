#ifndef DATABASE_CONNECTION_H
#define DATABASE_CONNECTION_H

#include <vector>
#include <map>
#include <string>
#include <algorithm>

#include "mysql_connection.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"

using namespace std;
namespace thedevs {
class DatabaseConnection {
    private:
        map<string,map<string,string>> tableStructure = {
            { "customers", {
                { "prefix", "customer_" },
                { "primary", "id" },
                { "unique", "fullName,email,mobileNumber" },
                { "fields", "id[int(11)],fullName[varchar(100)],email[varchar(100)],mobileNumber[varchar(16)],birthday[date],addressCode[varchar(50)],address[varchar(200)]" },
            } },
            { "products", {
                { "prefix", "product_" },
                { "primary", "id" }
            } },
            { "services", {
                { "prefix", "service_" },
                { "primary", "id" }
            } },
            { "staff", {
                { "prefix", "staff_" },
                { "primary", "id" }
            } }
        };
        string prefix = "tda_";
        string name;
        string host;
        string charset;
        string sql;
        string tableName;
        string crud;
        char crudType;
        // map<string,string> fieldsMap;
        map<string,string> whereConditions;
        string whereJoinner = "=";
        vector<map<string,string>> orderConditions;
        int pagination = 0;
        int page = 1;
        vector<map<string,string>> rows;
        sql::Connection *connection;
        // See more props in PHP slimframework repo


        /** 
         * Set prefix of tables
         * 
         * @param prefix Prefix of tables
         */
        void setPrefix ( string prefix );

        /** 
         * Returns prefix of tables
         * 
         * @return Prefix of tables
         */
        string getPrefix ();

        /** 
         * Get the prefix of fields in specific table
         * 
         * @param tableName The specific table name
         * @return The prefix of table
         */
        string getTablePrefix ( string tableName );

        /** 
         * Preppend prefix on a field of specific table
         * 
         * @param tableName The specific table name
         * @param fieldName The specific field name
         * @return The prefixed field
         */
        string prefixIt ( string tableName, string fieldName );
        
        /** 
         * Preppend prefix on all fields of specific table
         * 
         * @param tableName The specific table name
         * @param fieldNames The {key,value} map, prefixing just keys
         * @return The provided map with preffixed keys
         */
        string whereHandler ();

    public:
        /** 
         * DatabaseConnection set host, username, password and database name
         * 
         * @param host database host name
         * @param username database username
         * @param password database user password
         * @param dbname database name
         */
        DatabaseConnection( string host, string username, string password, string dbname );

        /** 
         * Clear all data of DatabaseConnection
         */
        void clear ( void );

        /** 
         * Reset where conditions of sql statement
         */
        void clearWhere ( void );

        /** 
         * Set where conditions in sql statement
         * 
         * @param whereConditions {key,value} map with conditions
         */
        void where ( map<string,string> whereConditions );

        /** 
         * Get where conditions of DatabaseConnection
         * 
         * @return where conditions {key,value} map
         */
        map<string,string> getWhereConditions ();

        /** 
         * Set string placed between key and value of where conditions
         * 
         * @param whereJoinner string placed between key and value of where conditions
         */
        void setWhereJoinner ( string whereJoinner );

        /** 
         * Get string placed between key and value of where conditions
         * 
         * @return string for comparison field and value in where statement.
         */
        string getWhereJoinner ();

        /** 
         * Set sql select statement results after execution.
         * 
         * @param rows Rows in select case
         */
        void setRows ( vector<map<string,string>> rows );

        /** 
         * Get sql select statement results after execution.
         * 
         * @return Rows in select case
         */
        vector<map<string,string>> getRows ();

        /** 
         * Set data per page ratio
         * 
         * @param pagination Number of resulsts per page
         */
        void setPagination ( int pagination );

        /** 
         * Get data per page ratio.
         * 
         * @return Number of resulsts per page
         */
        int getPagination ();

        /** 
         * Set current page
         * 
         * @param page Number of resulsts per page
         */
        void setPage ( int page );

        /** 
         * Get current page
         * 
         * @return Number of resulsts per page
         */
        int getPage ();

        /** 
         * Set {fieldName,fieldValue} map for sql statement.
         * 
         * @param fieldsMap string to string map of {fieldName,fieldValue}
         */
        // void setFieldsMap ( map<string,string> fieldsMap );

        /** 
         * Get {fieldName,fieldValue} map for sql statement.
         * 
         * @return string to string map of {fieldName,fieldValue}
         */
        // map<string,string> getFieldsMap ();

        /** 
         * Set sql to be executed.
         * 
         * @param sql Sql to be executed
         */
        void setSql ( string sql );

        /** 
         * Get sql to be executed.
         * 
         * @return Sql to be executed
         */
        string getSql ();

        /** 
         * Set CRUD statement.
         * 
         * @param crud CRUD statement
         */
        void setCRUD ( string crud );

        /** 
         * Get crud statement.
         * 
         * @return CRUD statement
         */
        string getCRUD ();

        /** 
         * Set Table name.
         * 
         * @param tableName Table name
         */
        void setTableName ( string tableName );

        /** 
         * Get Table name.
         * 
         * @return Table name
         */
        string getTableName ();

        /** 
         * Set CRUD type char.
         * 
         * @param tableName CRUD type char
         */
        void setCRUDType ( char crudType );

        /** 
         * Get CRUD type char.
         * 
         * @return CRUD type char
         */
        char getCRUDType ();

        /** 
         * Get list of fields in a specific table
         * 
         * @param tableName specific table name
         * @return List of fields in table
         */
        vector<string> getTableFields ( string tableName, bool withPrefix );

        /** 
         * Get select statement SQL result. Make use of set[WhereJoinner,whereConditions,...] to order and filter result
         * 
         * @param tableName specific table name
         * @param fields list of fields coming in result
         */
        void Select ( string tableName, vector<string> fields = {} );

        /** 
         * Get insert statement SQL result.
         * 
         * @param tableName table name
         * @param data {fieldName,fieldValue} map relation
         */
        void Insert ( string tableName, map<string,string> data );

        /** 
         * Get update statement SQL result.
         * 
         * @param tableName table name
         * @param data {fieldName,fieldValue} map relation
         */
        void Update ( string tableName, map<string,string> data );

        /** 
         * Get delete statement SQL result.
         * 
         * @param tableName table name
         */
        void Delete ( string tableName );

        /** 
         * Execute the sql statement
         * 
         * @return true in success, false otherwise
         */
        bool build ();

};

} // namespace thedevs

#endif