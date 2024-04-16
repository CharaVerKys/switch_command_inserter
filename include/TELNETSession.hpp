#pragma once
#include <asio.hpp>
#include <host.hpp>
#include <nameDefinition.hpp>
#include <regex>
#include <iostream>
#include <logging.hpp>
#include <database.hpp>

class TELNETSession : public std::enable_shared_from_this<TELNETSession>
{

    asio::io_context &_io_context;
    HOST &_host;
    std::vector<COMMANDS> _currentDoCommands;

    asio::steady_timer _timer;
    asio::ip::tcp::socket _socket;

    std::string _str;
    std::string _IPstring;
    size_t _iteration;
    asio::streambuf _read_buffer;
    std::stringstream _ss;
    std::stringstream _last_read;
    std::string _writableCommand;
    std::string line;
    std::regex _expect;
    std::regex _not_expect;
    std::regex _regex_login = std::regex("login:\\s$", std::regex::ECMAScript);
    std::regex _regex_password = std::regex("Password:\\s$", std::regex::ECMAScript);
    std::regex _regex_end_of_read = std::regex("\\S+[#$>]\\s?$", std::regex::ECMAScript);

public:
    TELNETSession(asio::io_context &io_context, HOST &host, std::vector<COMMANDS> &currentDoCommands);
    void connect();
	static std::map<uint16_t, std::string> shortlog;
    static void filterHosts(std::vector<HOST> &hosts);

private:
    void read_from_host(const std::regex &regex, std::function<void()> next_callback);
    void send_login();
    void send_password();
    void start_timer();
    void one_iteration();
    void exec_com();
    void end_of_com();
    void shortErrlog(std::string str);
    std::string getCurrentTime();   
};