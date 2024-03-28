#pragma once
#include <main.hpp>
#include <libssh2.h>

class SSHSession : public std::enable_shared_from_this<SSHSession>
{
    asio::io_context &_io_context;
    HOST &_host;
    std::vector<COMMANDS> _currentDoCommands;

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
    int _cmd_exit_status;
    std::regex _expect;
    std::regex _not_expect;
    std::regex _end_of_read = std::regex("\\S+[#$>]\\s?$", std::regex::ECMAScript);
    std::regex _moreRegex = std::regex("(--More--)|(Next\\sEntry)|(Quit.*Next\\sPage.*Previous\\sPage)", std::regex::ECMAScript);
    size_t _iteration;
    bool _is_end_of_readq;
    bool _one_again_taked;

public:
    SSHSession(asio::io_context &io_context, HOST &host, std::vector<COMMANDS> &currentDoCommands);
    ~SSHSession();
    void connect();
    static void filterHosts(std::vector<HOST> &hosts);
	static std::map<uint16_t, std::string> shortlog;

private:
    void handshake();
    void authenticate();
    void init_channel();
    void init_shell();
    void read_label();
    void one_iteration();
    void execute_one_command();
    void check_end_of_read(uint16_t buffer_point_add);
    void read_one_command();
    void end_one_command();
    void shortErrlog(std::string str);
    std::string getCurrentTime();    
};
