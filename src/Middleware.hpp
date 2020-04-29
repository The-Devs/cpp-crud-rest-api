#ifndef CRA_MIDDLEWARE_H
#define CRA_MIDDLEWARE_H

#include <vector>
#include <string>
#include <map>

using namespace std;
namespace thedevs {
class ResponseTemplate {
    private:
        string message = "";
        string resourceName = "";
        map<int,string> errorMessage;
        int page = 0;
        int maxPages = 0;
        int maxDataSize = 0;
        // 
        // JSON OBJ
        // unbrace
        // explode ,
        // unbrace array_keys
        // to Insert method
        // 

    public:
        /** 
         * ResponseTemplate initialize with json string/file with predefined error messages as map<int,string> style.
         * 
         * @param json A json string/file with predefined error messages as map<int,string> style
         */
        ResponseTemplate ( string json );

        /**
         * ...
         *
         * @param error
         * @param data
         * @return string
         */
        string build ( int error, vector<map<string,string>> data );

        /**
         * ...
         *
         * @param data
         * @return string
         */
        string handleData ( vector<map<string,string>> data );
        
        /**
         * Set a default message to be delivered from ::build method
         * 
         * @param message message to be set as default when ::build
         */
        void setMessage ( string message );

        /**
         * Get message to be delivered from ::build method
         * 
         * @param error error code from json file
         * @return message to be delivered when ::build
         */
        string getMessage ( int error );

        /**
         * Set page number of current data.
         * 
         * @param page the page of current data
         */
        void setPage ( int page );

        /**
         * Get page number of current data.
         * 
         * @return the page of current data
         */
        int getPage ();

        /**
         * Set max data size of whole set of results. Use just with pagination
         * 
         * @param maxDataSize max data size of whole set of results
         */
        void setMaxDataSize ( int maxDataSize );

        /**
         * Get max data size of whole set of results. Used just with pagination
         * 
         * @return maxDataSize max data size of whole set of results
         */
        int getMaxDataSize ();

};

} // namespace thedevs

#endif