#pragma once
#include <asio.hpp>
#include <libssh2.h>
#include <host.hpp>
#include <nameDefinition.hpp>
#include <regex>
#include <map>
#include <iostream>
#include <logging.hpp>
#include <database.hpp>

class SSHSession : public std::enable_shared_from_this<SSHSession>
{
    asio::io_context &_io_context;
    HOST &_host;
    std::vector<COMMANDS> _currentDoCommands;

    asio::steady_timer _timer;
    asio::ip::tcp::socket _socket;
    std::string _IPstring; // айпи текущего хоста для логов
    LIBSSH2_SESSION *_session = nullptr;
    LIBSSH2_CHANNEL *_channel = nullptr;
    std::string _writableCommand; // используется в 1(2) месте и определена просто для оптимизации (псевдо бл)
    std::string _str; // тоже самое, но тут уже для читаемости скорее
    std::stringstream _ss; // весь лог
    std::stringstream _part_of_ss; // текущая часть команды
    std::string command_exec; // команда отправляемая на исполнение
    const char *_currentCstrCommand; 
    std::string send_to_step; // послать чтобы продолжить последовательность
    char _buffer[1048576];
    std::regex _expect; 
    std::regex _not_expect;
    std::regex _end_of_read = std::regex("\\S+[#$>]\\s?$", std::regex::ECMAScript);
    std::regex _moreRegex = std::regex("(--More--)|(Next\\sEntry)|(Quit.*Next\\sPage.*Previous\\sPage)", std::regex::ECMAScript);
    size_t _iteration;
    bool _is_end_of_readq; // достигнут ли конец чтения
    bool _one_again_taked; // как бы в пару слов сказать, ну крч отвечает за внутреннюю работу чтения конца

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
