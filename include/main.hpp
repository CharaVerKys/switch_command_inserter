#pragma once
#include <iostream>
#include <host.hpp>
#include <asio.hpp>
#include <CWRconfigs.hpp>
#define TableNameForSSH "activeHostsSSH"
#include <database.hpp>
#include <headerlibSwitchCase.hpp>
#include <CommandsCommiterSSH.hpp>
// #include <logging.hpp> // уже добавил в CWRconfigs.hpp, чисто на всякий случай закоментил



// класс тестовый
//
//
//
//
//
#include <libssh2.h>
class SSHSession : public std::enable_shared_from_this<SSHSession>
{

    asio::io_context &_io_context;
    asio::ip::tcp::socket _socket;
    std::string _hostStringIp;
    std::string _username;
    std::string _password;
    std::string _команда;
    LIBSSH2_SESSION *_session;
    LIBSSH2_CHANNEL *_channel;
    std::vector<COMMANDS> currentDoCommands;
    std::string writableCommand;
    std::stringstream ss;
    std::stringstream part_of_ss;
    std::string str;
    char buffer[4096];
    int cmd_exit_status;
    std::regex expect;
    std::regex model_regex;
    size_t iteration;

public:
    void currentCommands_vector_init()
    {
        auto allMaC = configer->getModels_and_commands();

        std::string model = "testing"; // захардкодил типо, это host.model
        // в каждом отдельном хосте модель будет полная а не регулсярка

        for (auto &pair : allMaC)
        {
            model_regex = std::regex(pair.first);
            if (std::regex_search(model, model_regex))
            {
                currentDoCommands = pair.second;
            }
        } // этот перебор для каждого отдельного может занять достаточно времени
          // то есть на каждый существующий конфиг я для каждого существующего хоста (в списке валидных) перебираю пока не найдётся, начиная с первого
          // то есть, если первая регулярка .* то все хосты будут выполняться к первой.

        one_iteration(); // всё ещё тело аутен калбека кста
    }

    SSHSession(asio::io_context &io_context,
               const std::string &hostStringIp,
               const std::string &username,
               const std::string &password,
               const std::string &команда) : //
                                             _io_context(io_context),
                                             _socket(_io_context),
                                             _username(username),
                                             _password(password),
                                             _hostStringIp(hostStringIp),
                                             _команда(команда)

    {
        this->iteration = 0;
        std::cout << "инициализирован конструктор для команды " + _команда << std::endl;
    }

    void connect()
    {
        asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(_hostStringIp), 22);

        auto self = shared_from_this();
        _socket.async_connect(endpoint, [this, self](const asio::error_code &ec)
                              {
        if (!ec)
        {
            _session = libssh2_session_init();
            if (_session)
            {
        std::cout << asio::ip::address::from_string(_hostStringIp) << std::endl;
                libssh2_session_set_blocking(_session, 0);
                // Выполняем handshake асинхронно
                async_handshake();
            }
            else
            {
                std::cerr << "Error initializing SSH session" << std::endl;
            }
        }
        else
        {
            std::cerr << "Error connecting - " << ec.message() << std::endl;
        } });
    }

    void async_handshake()
    {
        auto self = shared_from_this();
        int handshake_result = libssh2_session_handshake(_session, _socket.native_handle());
        if (handshake_result == 0)
        {
            std::cerr << "выполнилось хендшейк для команды " << _команда << std::endl;

            authenticate();
        }
        else if (handshake_result == LIBSSH2_ERROR_EAGAIN)
        {
            // Ждем асинхронно, пока handshake завершится
            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
            std::cerr << "запустился асинх авейт хендшейк для "<< _команда << std::endl;
                                
            if (!ec)
            {
                async_handshake();
            }
            else
            {
                std::cerr << "Error during handshake - " << ec.message() << std::endl;
            } });
        }
        else
        {
            std::cerr << "Error during handshake - " << handshake_result << std::endl;
        }
    }

    void authenticate()
    {

        auto self = shared_from_this();
        int аутен_result = libssh2_userauth_password(_session, _username.c_str(), _password.c_str());
        if (аутен_result == 0)
        {
            std::cout << "и я аутентифицировался " << _команда << " \n";
            currentCommands_vector_init();
        }
        else if (аутен_result == LIBSSH2_ERROR_EAGAIN)
        {
            // Ждем асинхронно, пока handshake завершится
            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   std::cout << "и я зашёл в тело асинх аутент " << _команда << " \n"; // для аутен ошибка неверный логин пароль -18

                                   if (!ec)
                                   {
                                       authenticate();
                                   }
                                   else
                                   {
                                       std::cerr << "Error during userauth - " << ec.message() << std::endl;
                                   } });
        }
        else
        {
            std::cerr << "Error during userauth - " << аутен_result << std::endl;
        }
    }

    void one_iteration()
    {

        if (!(iteration < currentDoCommands.size())) // всё ещё калбек аутентифика, то есть если команд 0 то не выполнит ничего
        {
            str += ss.str();
            std::cout << "\tи я полученная строка лога" << str << " \n";
            return; // логика завершения, по идее должен вызвать деструктор прям ща
        }

        auto self = shared_from_this();
        _channel = libssh2_channel_open_session(_session);

        if (_channel)
        {
            writableCommand = "\n\nотправленна команда\t " + currentDoCommands[iteration].cmd + " \n\tРезультат:\n";
            ss << writableCommand;
            std::cout << "канал открылся (вызов) для" << _команда << std::endl;
            // выполняю команды
            execute_one_command();
        }
        else if (libssh2_session_last_errno(_session) == LIBSSH2_ERROR_EAGAIN)
        {

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   std::cout << "и я зашёл в тело асинх старт ченел (вон этерейшн) " << _команда << " \n"; 

                                   if (!ec)
                                   {
                                       one_iteration();
                                   }
                                   else
                                   {
                                       std::cerr << "Error during channal create - " << ec.message() << std::endl;
                                   } });
        }
        else
        {
            std::cerr << "Failed to open channel with err_code " << libssh2_session_last_errno(_session) << std::endl;
        }
    }

    void execute_one_command()
    {
        libssh2_channel_exec(_channel, currentDoCommands[iteration].cmd.c_str());
        std::cout << "отправленна команда " + currentDoCommands[iteration].cmd + " для " + _hostStringIp + "\n";
        read_one_command();
    }

    void read_one_command()
    {

        int rc = libssh2_channel_read(_channel, buffer, sizeof(buffer));
        auto self = shared_from_this();
        if (rc > 0)
        {
            std::cout << "считало в read_one_command для " << _команда << " \n";
            part_of_ss.write(buffer, rc);
            read_one_command();
        }
        else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN)
        {
            str += " не удалось считать вывод от устройства с кодом ошибки " + std::to_string(rc) + " \n" + ss.str() + part_of_ss.str();
            std::cerr << "Error: " << str << std::endl;
        }
        else if (rc == 0)
        {

            end_one_command(); // продолжить выполнение (этот же калбек просто вынес в другую функцию)
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        {

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   std::cout << "и я зашёл в тело асинх read_one_command для " << _команда << " \n"; 

                                   if (!ec)
                                   {

                                       read_one_command();
                                   }
                                   else
                                   {
                                       std::cerr << "Error during read answer - " << ec.message() << std::endl;
                                   } });
        }
    }

    void end_one_command()
    {

        cmd_exit_status = libssh2_channel_get_exit_status(_channel);
        if (!(cmd_exit_status == currentDoCommands[iteration].code))
        {
            str += "неверный код результата для команды" + currentDoCommands[iteration].cmd + ".Ожидалось" + std::to_string(currentDoCommands[iteration].code) + ", а в ответе" + std::to_string(cmd_exit_status);
            // влог - это же + подробнее для этого хоста можно посмотреть в show errHosts
            str += "\n\n" + ss.str() + part_of_ss.str();
            std::cerr << "Error: " << str << std::endl;
            // записать хост в таблицу errHosts throw неверный код результата для команды
        }

        // проверить ответ на равенство ожидаемому
        if (currentDoCommands[iteration].expect != "")
        {
            expect = std::regex(currentDoCommands[iteration].expect); // содержет строку в которой описана регулярка
            if (!(std::regex_search(part_of_ss.str(), expect)))
            {
                // если не (регулярки совпадают) то выкинуть ошибку

                str += "неожиданный output команды " + currentDoCommands[iteration].cmd + " для " + _hostStringIp + ". Ожидалось \n\n" + currentDoCommands[iteration].expect + "\n\n а в ответе\n\n" + part_of_ss.str();
                // host.log = str;
                str += "\n\n" + ss.str() + part_of_ss.str();
                std::cerr << "Error: " << str << std::endl;
                //   влог - неожиданный output команды + command.cmd + для + hostIP +.подробнее для этого хоста можно посмотреть в show errHosts записать хост в таблицу errHosts throw неожиданный output команды
            }
        }

        ss << part_of_ss.str();
        part_of_ss.str("");
        libssh2_channel_close(_channel);
        libssh2_channel_free(_channel);
        _channel = nullptr;
        ++iteration;

        one_iteration(); // ахуеть а теперь из калбека рид команд вызываю
    }

    ~SSHSession()
    {
        if (_channel)
        {
            libssh2_channel_close(_channel);
            libssh2_channel_free(_channel);
            _channel = nullptr;
            std::cout << "и я выполнился из деструктора для канала " << _команда << " \n";
        }
        if (_session)
        {
            libssh2_session_disconnect(_session, "Bye!");
            libssh2_session_free(_session);
            _session = nullptr;
            std::cout << "и я выполнился из деструктора " << _команда << " \n";
        }
    }
};
//
//
//
//
//
//


// initVars.cpp
void initVars(std::filesystem::path executable_path, const int & argc, char const* argv[]);


// rootCallFunctions.cpp
void rootScan(int argc, char const *argv[]);










// scanNetwork.cpp 
struct SNparsedNetworkHost
{
    uint32_t network;
    uint32_t hosts;
};
struct ActiveHOSTS
{
    std::vector<HOST> ssh;
    std::vector<HOST> onlyTelnet;
};

ActiveHOSTS ScanNetwork(std::vector<HOST> &hosts);

std::vector<HOST> SNinitHostsVector(SNparsedNetworkHost &IpPool);
void SNcheck_port_async(HOST &host, asio::io_context &io_context, HOST::PORT &port, std::vector<HOST> &validHosts);
void SNcheck_port_async(HOST &host, asio::io_context &io_context, HOST::PORT &port, std::vector<HOST> &validHosts,  std::vector<HOST> &refusedHosts);


//мусорная функция, не используется
bool is_port_open(const uint32_t ip_address, int port);


// parseIpMaskToIpPool.cpp
SNparsedNetworkHost parseIpMaskToIpPool(std::string ipAndMask);
uint32_t ipToBin(const std::string &ip);



