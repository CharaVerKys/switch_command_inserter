#include <identifyRegex.hpp>

//
// for identify единственная из БД таблица
//

void IdentifySSH::filter_to_log_resulting_vector_from_database(std::vector<HOST> &hosts)
{
    // сортировочка поехала

    auto hasLoginNoModel = [](const HOST &host)
    {
        return !host.login.name.empty() && host.model.empty();
    };
    auto hasNoModelNoLogin = [](const HOST &host)
    {
        return host.login.name.empty() && host.model.empty();
    };
    auto hasModelAndLogin = [](const HOST &host)
    {
        return !host.login.name.empty() && !host.model.empty();
    };

    std::vector<HOST> hostsWithLoginNoModel;
    std::vector<HOST> hostsWithNoModelNoLogin;
    std::vector<HOST> hostsWithModelAndLogin;

    // 3 массива потом склеиваю
    for (const auto &host : hosts)
    {
        if (hasLoginNoModel(host))
        {
            hostsWithLoginNoModel.emplace_back(std::move(host));
        }
        else if (hasNoModelNoLogin(host))
        {
            hostsWithNoModelNoLogin.emplace_back(std::move(host));
        }
        else if (hasModelAndLogin(host))
        {
            hostsWithModelAndLogin.emplace_back(std::move(host));
        }
        else
        {
            std::cerr << "После identify у хоста " << asio::ip::address_v4(host.address).to_string() << " каким то образом есть модель но нет логина, не представляю как это возможно\n";
        }
    }
    hosts.clear();

    // это можно, и впринципе стоило бы переделать на move_iterator но я забил
    hosts.insert(hosts.end(), hostsWithLoginNoModel.begin(), hostsWithLoginNoModel.end());
    hosts.insert(hosts.end(), hostsWithNoModelNoLogin.begin(), hostsWithNoModelNoLogin.end());
    hosts.insert(hosts.end(), hostsWithModelAndLogin.begin(), hostsWithModelAndLogin.end());
}

//
//
//
IdentifySSH::IdentifySSH(asio::io_context &io_context, HOST &host,
                         std::vector<std::pair<std::string, std::string>> &logins,
                         std::vector<std::pair<std::string, std::vector<COMMANDS>>> &finding_commands,
                         std::vector<HOST> &identifined_hosts__vector_of_ref) //
    : _io_context(io_context),
      _host(host),
      _finding_commands(finding_commands),
      _logins(logins),
      _identifined_hosts__vector_of_ref(identifined_hosts__vector_of_ref), // результаты
      _socket(_io_context),
      _timer(_io_context)
{
    this->_login_iteration = 0;
    this->_cover_iteration = 0;
    this->_command_iteration = 0;

    _IPstring = asio::ip::address_v4(_host.address).to_string();
    wlog->writeLog("Инициализирован identify для " + _IPstring);
    _timer.expires_after(std::chrono::minutes(2));
}

//

void IdentifySSH::connect()
{
    try
    {
        asio::ip::tcp::endpoint endpoint(asio::ip::address_v4(_host.address), 22);
        auto self = shared_from_this(); // по какойто причине нужно объявлять каждый раз именно здесь! если вынести в конструктор пераметры класса то не работает
        _socket.async_connect(endpoint, [this, self](const asio::error_code &ec)
                              {
        if (!ec)
        {
            _session = libssh2_session_init();
            if (_session)
            {
                libssh2_session_set_blocking(_session, 0);
               _str = "Успешное создание сессии к хосту ";
               _host.log += ("\n"+_str); 
                wlog->writeLog(_str+_IPstring);
                handshake();
            }
            else
            {
               _str = "Ошибка при создании сессии к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForIdentify, _host);
            }
        }
        else
        {
           _str = "Ошибка при подключении "+ ec.message()+" к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForIdentify, _host);
        } });
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на connect" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::handshake()
{
    try
    {
        auto self = shared_from_this();
        int handshake_result = libssh2_session_handshake(_session, _socket.native_handle());
        if (handshake_result == 0)
        {
            _str = "Успешный handshake к хосту ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);
            authenticate();
        }
        else if (handshake_result == LIBSSH2_ERROR_EAGAIN)
        {
            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {                                
            if (!ec)
            {
                handshake();
            }
            else
            {
               _str = "Ошибка во время handshake(сокет) "+ec.message()+" к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForIdentify, _host);
            } });
        }
        else
        {
            _str = "Ошибка во время handshake(ssh) " + std::to_string(handshake_result) + " к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на handshake" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::authenticate()
{
    try
    {
        // если закончились логины
        if (_logins.size() == _login_iteration)
        {
            _str = "Не удалось подобрать логин/пароль к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);
            return;
        }

        auto self = shared_from_this();
        int аутен_result = libssh2_userauth_password(_session, _logins[_login_iteration].first.c_str(), _logins[_login_iteration].second.c_str());

        if (аутен_result == 0)
        {
            _host.login.name = _logins[_login_iteration].first;
            _host.login.password = _logins[_login_iteration].second;
            _str = "Успешная аутентификация( " + _host.login.name + " ) к хосту ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);
            init_channel();
        }
        else if (аутен_result == LIBSSH2_ERROR_EAGAIN)
        {
            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                    if (!ec)
                    {

                        authenticate();
                    }
                    else
                    {
                    _str = "Ошибка во время аутентификации(сокет) "+ec.message()+" к хосту ";
                    _host.log += ("\n"+_str); 
                    plog->writeLog(_str + _IPstring);
                    sqlite->write_one_hostCommit(TableNameForIdentify, _host);
                    } });
        }
        else if (аутен_result == LIBSSH2_ERROR_AUTHENTICATION_FAILED)
        { // логин пароль код
            _str = "Не подошли логин/пароль " + _logins[_login_iteration].first + " к хосту ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);
            _login_iteration++;

            authenticate();
        }
        else
        {
            _str = "Ошибка во время аутентификации(ssh) " + std::to_string(аутен_result) + " к хосту ";
            _host.log += ("\n" + _str + "\nИз-за специфики работы используемой библиотеки и невозможности абстрактно получить результат " +
                          "'исчерпано количество попыток подключения' : эта ошибка может означать как программную ошибку, так и проблему с исчерпанием количества попыток коннекта");
            wlog->writeLog(_str + _IPstring + " не информативная (и не точная) ошибка из-за специфики работы библиотеки");
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на authent" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::init_channel()
{
    try
    {
        auto self = shared_from_this();
        _channel = libssh2_channel_open_session(_session);

        if (_channel)
        {
            _str = "Успешное создание канала к хосту ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);
            init_shell();
        }
        else if (libssh2_session_last_errno(_session) == LIBSSH2_ERROR_EAGAIN)
        {

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   if (!ec)
                                   {
                                       init_channel();
                                   }
                                   else
                                   {
                                    _str = "Ошибка во время создания канала(сокет) "+ec.message()+" к хосту ";
                                    _host.log += ("\n"+_str);
                                    plog->writeLog(_str + _IPstring);
                                    sqlite->write_one_hostCommit(TableNameForIdentify, _host);
                                   } });
        }
        else
        {
            _str = "Ошибка во время создания канала(ssh) " + std::to_string(libssh2_session_last_errno(_session)) + " к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на channel" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::init_shell()
{
    try
    {
        auto self = shared_from_this();
        int rc = libssh2_channel_shell(_channel);
        if (rc)
        {
            _str = "Успешное создание оболочки к хосту ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);

            // таймер чтобы не блокировать всё приложение если не приходит ответ (время настраивать в конструкторе)
            auto self = shared_from_this();
            _timer.async_wait([this, self](const asio::error_code &er)
                              {
                                    if (!er) {
                                        // если таймер сработал, дроп подключения

                                        _ss << "-----------------------------------------------------------\n"<<
                                        "\tОшибка: слишком долго нет ответа(таймаут)\n"
                                        <<"-----------------------------------------------------------\n";
                                        plog->writeLog("Ошибка: слишком долго нет ответа(таймаут) к хосту "+ _IPstring);
                                        // будет op aborted на текущем калбеке
                                        if (_socket.is_open()) {
                                            _socket.close();
                                        }
                                    }else{
                                        if (er != asio::error::operation_aborted) {
                                        plog->writeLog("ошибка таймера чтения в SSHSession: "+er.message()+" для хоста " + _IPstring);
                                    }} });

            read_label();
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN)
        {

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   if (!ec)
                                   {
                                       init_shell();
                                   }
                                   else
                                   {
                                         _str = "Ошибка во время создания оболочки(сокет) "+ec.message()+" к хосту ";
                                    _host.log += ("\n"+_str); 
                                    plog->writeLog(_str + _IPstring);
                                    sqlite->write_one_hostCommit(TableNameForIdentify, _host);
                                     } });
        }
        else
        {
            _str = "Ошибка во время создания оболочки(ssh) " + std::to_string(rc) + " к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на shell" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::read_label()
{
    try
    {
        int rc = libssh2_channel_read(_channel, _buffer, sizeof(_buffer));
        if (rc > 0)
        {
            _part_of_ss.write(_buffer, rc); // здесь находится лейбл, сейчас не используется и отчищается перед началом цикла
            read_label();
        }
        else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN)
        {
            _str = "Ошибка во время считывания лейбла(ssh) " + std::to_string(rc) + " аутпут(" + _part_of_ss.str() + ") " + " к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog("Ошибка во время считывания лейбла(ssh) " + std::to_string(rc) + " " + _IPstring);
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);
        }
        else if (std::regex_search(_part_of_ss.str(), _end_of_read)) // главное чтобы проверка была до открытия сокета
        {
            _part_of_ss.str(""); //  отчищается перед началом цикла
            _str = "Успешное считывание лейбла к хосту ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);

            // Добавляю в начало стрима, чтобы отделить то что будет в логе от верхнего аутпута, где будет неожиданное поведение
            _writableCommand = "\n\n\n--------------------------------------------------------\n\tОтправленные команды и полученные ответы:\n\n";
            _ss << _writableCommand;

            _timer.cancel();
            one_iteration_cover_vector();
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        {
            auto self = shared_from_this();
            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   if (!ec)
                                   {
                                       read_label();
                                   }
                                   else
                                   {
                                     _str = "Ошибка во время считывания лейбла(сокет) "+ec.message() + " аутпут(" + _part_of_ss.str() + ") " +" к хосту ";
                                    _host.log += ("\n"+_str); 
                                    plog->writeLog("Ошибка во время считывания лейбла(сокет) "+ec.message()+" " + _IPstring);
                                    sqlite->write_one_hostCommit(TableNameForIdentify, _host);
                                    } });
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на label" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::one_iteration_cover_vector()
{
    try
    {
        if (!(_cover_iteration < _finding_commands.size())) // если набор пустой то выход
        {
            _str = ("Закончен поиск для хоста " + _ss.str());
            _host.log += ("\n" + _str);
            wlog->writeLog("Закончен поиск для хоста " + _IPstring);
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);

            return; // логика завершения, по идее должен вызвать деструктор прям ща
        }
        else
        {
            // нужно обнулять каждый раз будет
            // тут запускается только каждый новый
            _command_iteration = 0;

            one_iteration_inside();
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на iteration (cover)" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::one_iteration_inside()
{
    try
    {
        // неожиданный ответ вызываю функцию одной итерации ковера

        if (!(_command_iteration < _finding_commands.at(_cover_iteration).second.size())) // если команд 0 то не выполнит ничего
        {
            // если команд не осталось - нашёл
            _host.model = _finding_commands.at(_cover_iteration).first; // ковер соответственно...
            return;                                                     // логика завершения, по идее должен вызвать деструктор прям ща
        }
        else
        {
            _writableCommand = "\n........................................................................................\n\nотправленна команда\t " + _finding_commands.at(_cover_iteration).second.at(_command_iteration).cmd + " \n\tРезультат:\n";
            _ss << _writableCommand;

            if (_finding_commands.at(_cover_iteration).second.at(_command_iteration).send_to_step == "")
            {
                send_to_step = "\x20\n";
            }
            else
            {
                send_to_step = _finding_commands.at(_cover_iteration).second.at(_command_iteration).send_to_step + "\n";
            }

            // сейчас я во внутреннем цикле
            execute_one_command();
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на iteration (inside)" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::execute_one_command() // вообще сделал что-то типо pipeline но хз правильно ли, зато понятно, не так востребовано как с предыдущими но да и ладно
{
    try
    {
        command_exec = _finding_commands.at(_cover_iteration).second.at(_command_iteration).cmd + "\n"; // gpt подсказал так написать ввод
        libssh2_channel_write(_channel, command_exec.c_str(), command_exec.size());
        _str = "Отправленна команда " + _finding_commands.at(_cover_iteration).second.at(_command_iteration).cmd;
        wlog->writeLog(_str + " для " + _IPstring);

        _is_end_of_readq = false;
        _one_again_taked = true; // чтобы выполнелась отправка первого энтера

        // таймер чтобы не блокировать всё приложение если не приходит ответ (время настраивать в конструкторе)
        auto self = shared_from_this();
        _timer.async_wait([this, self](const asio::error_code &er)
                          {
                                    if (!er) {
                                        // если таймер сработал, дроп подключения

                                        _ss << "-----------------------------------------------------------\n"<<
                                        "\tОшибка: слишком долго нет ответа(таймаут)\n"
                                        <<"-----------------------------------------------------------\n";
                                        plog->writeLog("Ошибка: слишком долго нет ответа(таймаут) к хосту "+ _IPstring);
                                        // будет op aborted на текущем калбеке
                                        if (_socket.is_open()) {
                                            _socket.close();
                                        }
                                    }else{
                                        if (er != asio::error::operation_aborted) {
                                        plog->writeLog("ошибка таймера чтения в IdentifySSH: "+er.message()+" для хоста " + _IPstring);
                                    }} });
        read_one_command();
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на exec" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::check_end_of_read(uint16_t buffer_point_add) // не логирую
{
    try
    { // обработчик проверки получения всех отправленных данных
        if (std::regex_search(_buffer, _buffer + buffer_point_add, _end_of_read))
        {
            _is_end_of_readq = true;
            return;
        }
        if (!(std::regex_search(_buffer, _buffer + buffer_point_add, _moreRegex)) && _one_again_taked)
        {
            return;
        }
        // else contunue
        if (_one_again_taked)
        {
            std::cerr << send_to_step << std::endl;
            libssh2_channel_write(_channel, send_to_step.c_str(), send_to_step.size());
        } // отправляется только если было прочитано до этого (или при старте)
        int rc = libssh2_channel_read(_channel, _buffer, sizeof(_buffer));

        if (rc > 0)
        {
            _part_of_ss.write(_buffer, rc);
            _one_again_taked = true;
            check_end_of_read(rc);
        }
        else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN)
        {
            _str = "Ошибка во время считывания чего-то, что не заканчивается на _end_of_read(ssh) " + std::to_string(rc) + " аутпут(" + _ss.str() + _part_of_ss.str() + ") ";
            _host.log += ("\n" + _str);
            plog->writeLog("Ошибка во время считывания чего-то, что не заканчивается на ожидаемое значение _end_of_read(ssh) " + std::to_string(rc) + " к хосту " + _IPstring);
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        {
            _one_again_taked = false; // это цикличное ожидание, не нужно отправлять нужный энтер
            auto self = shared_from_this();

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self, rc](const asio::error_code &ec)
                               {
                                   if (!ec)
                                   {

                                       check_end_of_read(0);
                                   }
                                   else
                                   {
        _str = "Ошибка во время считывания чего-то, что не заканчивается на _end_of_read(сокет) "+ec.message() + " аутпут(" +_ss.str()+ _part_of_ss.str() + ") "  ;
        _host.log += ("\n" + _str);
        plog->writeLog("Ошибка во время считывания чего-то, что не заканчивается на _end_of_read(сокет) "+ec.message() +" к хосту "+ _IPstring);
        sqlite->write_one_hostCommit(TableNameForIdentify, _host);
                                   } });
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на check" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::read_one_command()
{
    try
    {
        int rc = libssh2_channel_read(_channel, _buffer, sizeof(_buffer));

        if (rc > 0)
        {
            _part_of_ss.write(_buffer, rc);
            check_end_of_read(rc);
            read_one_command();
        }
        else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN)
        {
            _str = "Ошибка во время считывания ответа(ssh) " + std::to_string(rc) + " аутпут(" + _ss.str() + _part_of_ss.str() + ") ";
            _host.log += ("\n" + _str);
            plog->writeLog("Ошибка во время считывания ответа(ssh) " + std::to_string(rc) + " к хосту " + _IPstring);
            sqlite->write_one_hostCommit(TableNameForIdentify, _host);
        }

        else if (_is_end_of_readq) // главное чтобы проверка была до открытия сокета
        {
            _str = "Успешное считывание ответа к хосту ";
            wlog->writeLog(_str + _IPstring);
            end_one_command(); // продолжить выполнение (этот же калбек просто вынес в другую функцию)
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        {
            auto self = shared_from_this();

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   if (!ec)
                                   {
                                       read_one_command();
                                   }
                                   else
                                   {
        _str = "Ошибка во время считывания ответа(сокет) "+ec.message() + " аутпут(" +_ss.str()+ _part_of_ss.str() + ") " ;
        _host.log += ("\n" + _str);
        plog->writeLog("Ошибка во время считывания ответа(сокет) "+ec.message() +" к хосту "+ _IPstring);
        sqlite->write_one_hostCommit(TableNameForIdentify, _host);
                                   } });
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на read" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void IdentifySSH::end_one_command()
{
    try
    {
        _timer.cancel();

        // проверить ответ на равенство ожидаемому
        if (_finding_commands.at(_cover_iteration).second.at(_command_iteration).expect != "")
        {
            _expect = std::regex(_finding_commands.at(_cover_iteration).second.at(_command_iteration).expect); // содержет строку в которой описана регулярка
            if (!(std::regex_search(_part_of_ss.str(), _expect)))
            {
                // если не (регулярки совпадают) то перейти к следующему массиву

                _str = "неожиданный output команды " + _finding_commands.at(_cover_iteration).second.at(_command_iteration).cmd;
                _str += ". Ожидалось: " + _finding_commands.at(_cover_iteration).second.at(_command_iteration).expect + "\n\n А в ответе: " + _part_of_ss.str();
                _host.log += ("\n" + _str);

                ++_cover_iteration;
                one_iteration_cover_vector();
            }
        }

        // проверить ответ на равенство не ожидаемому
        if (_finding_commands.at(_cover_iteration).second.at(_command_iteration).not_expect != "")
        {
            _not_expect = std::regex(_finding_commands.at(_cover_iteration).second.at(_command_iteration).not_expect); // содержет строку в которой описана регулярка
            if ((std::regex_search(_part_of_ss.str(), _not_expect)))
            {
                // если не (регулярки совпадают) то выкинуть ошибку

                _str = "неожиданный output команды " + _finding_commands.at(_cover_iteration).second.at(_command_iteration).cmd;
                wlog->writeLog(_str + " на хосте " + _IPstring + " Подробнее в errHosts.log");
                _str += ". Ожидалось не получить: " + _finding_commands.at(_cover_iteration).second.at(_command_iteration).not_expect + "\n\n А в ответе: " + _part_of_ss.str();
                _host.log += ("\n" + _str);

                ++_cover_iteration;
                one_iteration_cover_vector();
            }
        }

        _ss << _part_of_ss.str();
        _part_of_ss.str("");

        ++_command_iteration;

        one_iteration_inside(); // продолжаю пока в массиве команд ещё есть данные
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch (identify) " + std::string(e.what()) + " к хосту " + _IPstring + " на end" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

IdentifySSH::~IdentifySSH()
{
    if (_channel)
    {
        libssh2_channel_close(_channel);
        libssh2_channel_free(_channel);
        _channel = nullptr;
    }
    if (_session)
    {
        libssh2_session_disconnect(_session, "shutdown session");
        libssh2_session_free(_session);
        _session = nullptr;
    }
}
