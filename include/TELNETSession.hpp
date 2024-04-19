#pragma once
#include <asio.hpp>
#include <host.hpp>
#include <nameDefinition.hpp>
#include <regex>
#include <iostream>
#include <logging.hpp>
#include <database.hpp>

// не заходите сюда лучше даже
// тут в целом всё как в ssh но бля этот телнет...

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
    std::stringstream _last_read_one_it_inside_for_send_to_step;
    std::string _writableCommand;
    std::string line;
    std::regex _expect;
    std::regex _not_expect;
    std::string send_to_step;
    std::regex _control_1 = std::regex("\xFF\xFB\x03\xFF\xFD\x01", std::regex::ECMAScript);
    std::regex _control_2 = std::regex("\xFF\xFD\x18\xFF\xFD\x20\xFF\xFD\x23", std::regex::ECMAScript);
    std::regex _regex_login = std::regex("([lL]ogin:)|(User[Nn]ame:)", std::regex::ECMAScript);
    std::regex _regex_password = std::regex("Pass[wW]ord:", std::regex::ECMAScript);
    std::regex _regex_end_of_read = std::regex("\\S+[#$>]\\s?", std::regex::ECMAScript);
    std::regex _moreRegex = std::regex("(--More--)|(Next\\sEntry)|(Quit.*Next\\sPage.*Previous\\sPage)", std::regex::ECMAScript);
    bool _is_end_of_readq;
    bool _is_this_moreq;
    asio::error_code asio_error;

public:
    TELNETSession(asio::io_context &io_context, HOST &host, std::vector<COMMANDS> &currentDoCommands);
    void connect();
	static std::map<uint16_t, std::string> shortlog;
    static void filterHosts(std::vector<HOST> &hosts);

private:
    void read_from_host(const std::regex &regex, std::function<void()> next_callback);
    void check_end_of_read(const std::regex &regex);
    void read_from_host_init(const char* what_send_to_step);
    void read_from_host_init();
    void send_login();
    void send_password();
    void start_timer();
    void one_iteration();
    void exec_com();
    void end_of_com();
    void shortErrlog(std::string str);
    std::string getCurrentTime();   
};
