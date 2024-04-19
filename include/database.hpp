#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <host.hpp>
#include <logging.hpp>
#include <asio/ip/address_v4.hpp>


class dbLite{

std::unique_ptr<SQLite::Database> db; 

public:
 dbLite(const std::filesystem::path &executable_path);

void write_to_database(const std::string& TableName, const std::vector<HOST>& hosts);
void write_one_hostCommit(const std::string& TableName, HOST& host);

void emptyOut(const std::string& TableName);
bool isTableExist(const std::string& TableName);

std::vector<HOST> read_from_database(const std::string& TableName);
std::vector<HOST> read_from_databaseCommit(const std::string& TableName);

};


extern std::unique_ptr<dbLite> sqlite;
