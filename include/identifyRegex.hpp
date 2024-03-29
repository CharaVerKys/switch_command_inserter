#pragma once
#include <vector>
#include <libssh2.h>
#include <string>
#include <iostream>
#include <logging.hpp>
#include <asio.hpp>
#include <regex>
#include <host.hpp>
#include <nameDefinition.hpp>
#include <database.hpp>

class IdentifySSH : public std::enable_shared_from_this<IdentifySSH>
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
    LIBSSH2_SESSION *_session = nullptr;
    LIBSSH2_CHANNEL *_channel = nullptr;
    std::string _writableCommand;
    std::stringstream _ss;
    std::stringstream _part_of_ss;
    std::string _str;
    std::string command_exec;
    std::string send_to_step;
    const char *_currentCstrCommand;
    char _buffer[1048576];
    std::regex _expect;
    std::regex _not_expect;
    std::regex _end_of_read = std::regex("\\S+[#$>]\\s?$", std::regex::ECMAScript);
    std::regex _moreRegex = std::regex("(--More--)|(Next\\sEntry)|(Quit.*Next\\sPage.*Previous\\sPage)", std::regex::ECMAScript);
    size_t _login_iteration;
    bool _is_end_of_readq;
    bool _one_again_taked;

public:
    IdentifySSH(asio::io_context &io_context, HOST &host,
                std::vector<std::pair<std::string, std::string>> &logins,
                std::vector<std::pair<std::string, std::vector<COMMANDS>>> &finding_commands,
                std::vector<HOST> &identifined_hosts__vector_of_ref);
    ~IdentifySSH();
    void connect();
    static void  filter_to_log_resulting_vector_from_database(std::vector<HOST> &hosts);

private:
    void handshake();
    void authenticate();
    // void init_channel();
    // void init_shell();
    // void read_label();
    // 
    // void one_iteration();
    // void execute_one_command();
    // void check_end_of_read(uint16_t buffer_point_add);
    // void read_one_command();
    // void end_one_command();
};
