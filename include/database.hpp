#pragma once
#include <main.hpp>
#include <SQLiteCpp/SQLiteCpp.h>



class dbLite{


std::unique_ptr<SQLite::Database> db; 


public:
 dbLite(const std::filesystem::path &executable_path);

void write_to_database(const std::string& TableName, const std::vector<HOST>& hosts);
std::vector<HOST> read_from_database(const std::string& TableName);

void emptyOut(const std::string& TableName);
bool isTableExist(const std::string& TableName);


void write_one_hostCommit(const std::string& TableName, HOST& host);
std::vector<HOST> read_from_databaseCommit(const std::string& TableName);

};


extern std::unique_ptr<dbLite> sqlite;
