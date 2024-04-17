#pragma once
#include <iostream>
#include <logging.hpp>
#include <asio.hpp>
#include <regex>
#include <host.hpp>
#include <nameDefinition.hpp>
#include <database.hpp>
#include <functional>

class IdentifyTELNET : public std::enable_shared_from_this<IdentifyTELNET>
{
     asio::io_context &_io_context;
    HOST &_host;
    std::vector<std::pair<std::string, std::vector<COMMANDS>>> &_finding_commands;
    std::vector<std::pair<std::string, std::string>> &_logins;
    std::vector<HOST> &_identifined_hosts__vector_of_ref; // то есть каждый хост в этом векторе хостов - ссылка на хост поданный в объект из родительского вектора
                                                          // то есть он не самостоятелен а зависит от того в котором по циклу обходится

    asio::steady_timer _timer;
    asio::ip::tcp::socket _socket;
    std::string _IPstring;
    std::string _str;
    std::stringstream _ss;
    std::stringstream _last_read;
    std::string line;
    asio::streambuf _read_buffer;
    std::string _writableCommand;
   std::regex _expect;
    std::regex _not_expect;

    size_t _login_iteration;
    size_t _cover_iteration;
    size_t _command_iteration;

    std::regex _regex_login = std::regex("login:\\s$", std::regex::ECMAScript);
    std::regex _regex_password = std::regex("Password:\\s$", std::regex::ECMAScript);
    std::regex _regex_end_of_read = std::regex("\\S+[#$>]\\s?$", std::regex::ECMAScript);

public:
    IdentifyTELNET(asio::io_context &io_context, HOST &host,
                std::vector<std::pair<std::string, std::string>> &logins,
                std::vector<std::pair<std::string, std::vector<COMMANDS>>> &finding_commands,
                std::vector<HOST> &identifined_hosts__vector_of_ref);
                void connect();

private:
    void start_timer();
    void read_from_host(const std::regex &regex, std::function<void()> next_callback);
    void one_it_authent_try();
    void send_password();
    void extra_read_logic();
    void one_iteration_cover_vector();
    void one_iteration_inside();
void exec_com();
void end_of_com();
};