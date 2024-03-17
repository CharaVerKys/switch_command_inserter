#include <main.hpp>

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

public:
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

        // std::cout << "инициализирован конструктор для команды " + команда << std::endl;
    }

    void connect()
    {
        asio::ip::tcp::endpoint endpoint(asio::ip::address::from_string(_hostStringIp), 22);

        std::cout << asio::ip::address::from_string(_hostStringIp) << std::endl;
        auto self = shared_from_this();

        _socket.async_connect(endpoint, [this, self](const asio::error_code &ec)
                              {
                                   int handshake_result = 2147483647;
                                  if (!ec)
                                  {

                                      _session = libssh2_session_init();
                                      if (_session)
                                      {
                                        std::cout<< "\nя открылся "<<_команда<<" \n";
                                          libssh2_session_set_blocking(_session, 0);
                                          handshake_result = libssh2_session_handshake(_session, _socket.native_handle());
                                          // оно будет повторяться пока не другая ошибка или удачный...
                                        while (handshake_result == LIBSSH2_ERROR_EAGAIN)
                                            {
                                                handshake_result = libssh2_session_handshake(_session, _socket.native_handle());
                                            }

                                          if (
                                               handshake_result == 0)
                                          {
                                                std::cout<< "и я выполнился "<<_команда<<" \n";
                                                    
                                                 authenticate();
                                                return;
                                          }
                                      }
                                    // поменять выводы логов для конечной реализации, остальное так же
                                      std::cerr << "Error initializing SSH session/handshake: "<< std::to_string(handshake_result) << std::endl;
                                  }else{
                                         std::cerr << "Error connecting - " << ec.message()<< std::endl;
                                  } }); // колбек
    }

    void authenticate()
    {

        while (true)
        {
            // Обработка событий (может быть использован select(), epoll() или другие механизмы)

            // Попытка выполнения операции (например, рукопожатие или аутентификация)
            int аутен_result = libssh2_userauth_password(_session, _username.c_str(), _password.c_str());

            // Проверка результата операции
            if (аутен_result == 0)
            {
                // Операция завершилась успешно
                std::cout << "и я аутентифицировался " << _команда << " \n";
                break; // Выход из цикла, если не нужно ничего еще делать
            }
            else if (аутен_result == LIBSSH2_ERROR_EAGAIN)
            {
                // Операция не завершилась немедленно, продолжаем ожидать
                continue;
            }
            else
            {
                // Обработка других ошибок
                // Например, если аутентификация не удалась, выходим из цикла
                std::cerr << "Error: " << аутен_result << std::endl;
                break;
            }
        }
        executeCommands();
    }

    void executeCommands()
    {

        std::vector<COMMANDS> currentDoCommands = {
            COMMANDS(_команда, "", 0),
            COMMANDS("echo expect_string", ".*ct.st.*", 0),
            COMMANDS("sleep 5", "", 0)};

        std::string writableCommand;
        std::stringstream ss;
        std::string str;
        char buffer[4096];
        int rc;
int cmd_exit_status;
 std::regex expect;

        for (COMMANDS &command : currentDoCommands)
        {
            writableCommand = "\n\nотправленна команда\t " + command.cmd + " \n\tРезультат:\n";
            ss << writableCommand;

            // открываю канал для одной блять команды
            while (true)
            {
                _channel = libssh2_channel_open_session(_session);

                if (_channel)
                {
                    // Канал успешно открыт, выходим из цикла
                    std::cout << "канал открылся" << std::endl;
                    // выполняю команды
                    break;
                }
                else if (libssh2_session_last_errno(_session) == LIBSSH2_ERROR_EAGAIN)
                {
                    // без ожидания, сейчас проверяю работоспособность
                    continue;
                }
                else
                {
                    std::cerr << "Failed to open channel with err_code " << libssh2_session_last_errno(_session) << std::endl;
                    break; // Выход из цикла в случае ошибки
                }
            }
            // открыл

            libssh2_channel_exec(_channel, command.cmd.c_str());
            std::cout << "отправленна команда " + command.cmd + " для " + _hostStringIp + "\n";

            do
            {
                rc = libssh2_channel_read(_channel, buffer, sizeof(buffer));
                if (rc > 0)
                {
                    ss.write(buffer, rc);
                }
                else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
                {
                    continue;
                }
                else if (rc < 0)
                {
                    str += " не удалось считать вывод от устройства с кодом ошибки "+std::to_string(rc)+" \n" + ss.str();
                    std::cerr << "Error: " << str << std::endl;
                    break;
                }
                else // if (rc == 0)
                {
                    break;
                }
            } while (true);

            cmd_exit_status = libssh2_channel_get_exit_status(_channel);
            if (!(cmd_exit_status == command.code))
            {
                str += "неверный код результата для команды" + command.cmd +".Ожидалось" + std::to_string(command.code) +", а в ответе" + std::to_string(cmd_exit_status);
                //влог - это же + подробнее для этого хоста можно посмотреть в show errHosts str += \n\n + ss.str();
                std::cerr << "Error: " << str << std::endl;
               // записать хост в таблицу errHosts throw неверный код результата для команды
            }



            //проверить ответ на равенство ожидаемому
                if (command.expect != "")
            {
                expect = std::regex(command.expect); //содержет строку в которой описана регулярка
                if (!(std::regex_match(ss.str(), expect)))
                { 
                    // если не (регулярки совпадают) то выкинуть ошибку
                                     
                 //   str += неожиданный output команды + command.cmd + для + hostIP +.Ожидалось \n\n + command.expect +\n\n а в ответе\n\n + ss.str() host.log = str;
                 //   влог - неожиданный output команды + command.cmd + для + hostIP +.подробнее для этого хоста можно посмотреть в show errHosts записать хост в таблицу errHosts throw неожиданный output команды
                }
            }

            libssh2_channel_close(_channel);
            libssh2_channel_free(_channel);
            _channel = nullptr;

        } // for(COMMANDS &command : currentDoCommands)

        str += ss.str();
        std::cout << "\tи я полученная строка лога" << str << " \n";
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
int main()
{

    char **com = new char *[20];

    try
    {
        asio::io_context io;
        for (int i = 0; i < 2; ++i)
        {
            std::string str = "touch testssh-" + std::to_string(i);
            com[i] = new char[str.length() + 1];
            strcpy(com[i], str.c_str());
            auto session = std::make_shared<SSHSession>(io, "192.168.1.145", "charaverk", "", com[i]);
            session->connect();
        }

        for (int i = 9; i < 11; ++i)
        {
            std::string str = "touch testssh-" + std::to_string(i);
            com[i] = new char[str.length() + 1];
            strcpy(com[i], str.c_str());
            auto session = std::make_shared<SSHSession>(io, "192.168.1.79", "debian", "", com[i]);
            session->connect();
        }

        io.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    for (int i = 0; i < 20; ++i)
    {
        delete[] com[i];
    }
    delete[] com;
    return 0;
}

int cmain(const int argc, char const *argv[])
{

    // если нет глагола выдать exit 1 и сообщение
    if (argc < 2)
    {
        std::cerr << "Необходимо передать глагол в качестве аргумента.\n"
                  << "Доступные глаголы: scan, identify, showssh, commit. Допустимо 'scan identify'" << std::endl;
        std::exit(1);
    }

    initVars(std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path(), argc, argv); // инициализация логера, базы_данных и конфигера

    // инициализация переменных которые используются в обработчике
    // для уникальности сделал чтото типо особого имени
    // если что от лишник объявленных указателей и переменных размер сильно не увеличется
    // тут что-то было, пока я не вынес в отдельные функции (например rootScan)

    SWITCH(argv[1]) // в этом свиче нельзя создавать объекты, только операции с ними
    {
        CASE("scan") : // я без понятия почему формотирование вызывает такой баг, в данном случае для табуляций к rootScan (VScode 1.86.2 flatpack )

                       rootScan(argc, argv); // сюда не забыть добавить 2 поинта на функции к identify

        break; // конец scan
               //
               //
               //
               //
               //

        CASE("identify") : //

                           break; // конец identify
                                  //
                                  //
                                  //
                                  //
                                  //

        CASE("commit") : //

                         break; // конец commit
                                //
                                //
                                //
                                //
                                //

        CASE("showssh") : // если нет существительного выдать exit 1 и сообщение
                          if (argc < 3)
        {
            std::cerr << "Необходимо ещё передать существительное в качестве аргумента.\n"
                      << "Доступные существительные: modelHost, goodHosts, errHosts, PerrHosts." << std::endl;
            std::exit(1);
        }

        SWITCH(argv[2])
        {
            CASE("modelHost") : // максимум 9 символов для работы хеш функции
                                // возвращает поимённый список хост-модель-логин
                                // надо реализовать так, чтобы в начале шли хосты, где не указана модель, но при этом есть логин
                                // а потом уже шли те где ни модели ни пароля
                                // и в конце те где указано всё

                                break; // конец modelHost

            CASE("goodHosts") : // возвращает список всех пройденных хостов, чтобы добавить их в исключения при повторном запуске, если решим запускать так

                                break; // конец goodHosts

            CASE("errHosts") : // возвращает список хостов где произошло неожиданное поведение

                               break; // конец errHosts

            CASE("PerrHosts") : // возвращает список хостов, где произошла программная ошибка (не открылся сокет, не подошёл пароль и т.п.)

                                break; // конец PerrHosts

        default: // выдать ошибку о несоотвествии существительного
            break;
        }
        break; // конец show
               //
               //
               //
               //
               //

    default: // выдать ошибку о несоотвествии глагола
        std::cerr << "Неправильный глагол в качестве аргумента.\n"
                  << "Доступные глаголы: scan, identify, showssh, commit. Допустимо 'scan identify'" << std::endl;
        std::exit(1);
        break;
    }

    return 0;
}
