#include <SSHSession.hpp>

//
//
//
std::string SSHSession::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(1) << ms.count();
    return ss.str();
}

std::map<uint16_t, std::string> SSHSession::shortlog;

void SSHSession::shortErrlog(std::string str)
{
    if (_host.model == "script")
    {

    	shortlog.emplace(_host.number, getCurrentTime() + "\t" + _IPstring + "\tError: " + str);


                std::cout<<  getCurrentTime() << "\t" << _IPstring << "\tХост номер(" << _host.number << ")\tError: " << str<< std::endl;

    }
    else
    {
        	shortlog.emplace(_host.number, getCurrentTime() + "\t" + _IPstring +"\t"+ _host.model + "\tError: " + str);
                std::cout << getCurrentTime() << "\t" << _IPstring << "\t" << _host.model << "\tError: "  << str << std::endl;

    }
}

SSHSession::SSHSession(asio::io_context &io_context, HOST &host, std::vector<COMMANDS> &currentDoCommands)
    : _io_context(io_context), _host(host), _currentDoCommands(currentDoCommands), _socket(_io_context), _timer(_io_context)
{
    this->_iteration = 0;
    _IPstring = asio::ip::address_v4(_host.address).to_string();
    wlog->writeLog("Инициализирован коммит для " + _IPstring);
    _timer.expires_after(std::chrono::minutes(2));
}

//

void SSHSession::connect()
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
               sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
               shortErrlog(_str);
            }
        }
        else
        {
           _str = "Ошибка при подключении "+ ec.message()+" к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
               shortErrlog(_str);
        } });
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на connect" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::handshake()
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
               sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
               shortErrlog(_str);
            } });
        }
        else
        {
            _str = "Ошибка во время handshake(ssh) " + std::to_string(handshake_result) + " к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
            shortErrlog(_str);
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на handshake" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::authenticate()
{
    try
    {
        auto self = shared_from_this();
        int аутен_result = libssh2_userauth_password(_session, _host.login.name.c_str(), _host.login.password.c_str());
        if (аутен_result == 0)
        {
            _str = "Успешная аутентификация к хосту ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);
            init_channel();
        }
        else if (аутен_result == LIBSSH2_ERROR_EAGAIN)
        {
            // Ждем асинхронно, пока handshake завершится
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
                    sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
                    shortErrlog(_str);
                    } });
        }
        else if (аутен_result == -18)
        { // логин пароль код
            _str = "Ошибка во время аутентификации - неправильный логин/пароль на хосте ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForErrorHosts, _host);
            shortErrlog(_str);
        }
        else
        {
            _str = "Ошибка во время аутентификации(ssh) " + std::to_string(аутен_result) + " к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
            shortErrlog(_str);
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на authent" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::init_channel()
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
                                    sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
                                    shortErrlog(_str);
                                   } });
        }
        else
        {
            _str = "Ошибка во время создания канала(ssh) " + std::to_string(libssh2_session_last_errno(_session)) + " к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
            shortErrlog(_str);
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на channel" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::init_shell()
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
                                    sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
                                    shortErrlog(_str);
                                     } });
        }
        else
        {
            _str = "Ошибка во время создания оболочки(ssh) " + std::to_string(rc) + " к хосту ";
            _host.log += ("\n" + _str);
            plog->writeLog(_str + _IPstring);
            sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
            shortErrlog(_str);
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на shell" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::read_label()
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
            sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
            shortErrlog(_str);
        }
       else if (std::regex_search(_part_of_ss.str(), _end_of_read)) // главное чтобы проверка была до открытия сокета
        {
            _timer.cancel();
            
            _part_of_ss.str(""); //  отчищается перед началом цикла
            _str = "Успешное считывание лейбла к хосту ";
            _host.log += ("\n" + _str);
            wlog->writeLog(_str + _IPstring);

            // Добавляю в начало стрима, чтобы отделить то что будет в логе от верхнего аутпута, где будет неожиданное поведение
            _writableCommand = "\n\n\n--------------------------------------------------------\n\tОтправленные команды и полученные ответы:\n\n";
            _ss << _writableCommand;

            one_iteration();
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
                                    sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
                                    shortErrlog(_str);
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

void SSHSession::one_iteration()
{
    try
    {
        if (!(_iteration < _currentDoCommands.size())) // если команд 0 то не выполнит ничего
        {

            _str = ("Закончен цикл для хоста " + _ss.str());
            _host.log += ("\n" + _str);
            wlog->writeLog("Закончен цикл для хоста " + _IPstring);
            sqlite->write_one_hostCommit(TableNameForGoodHosts, _host);
            if (_host.model == "script")
            {
                	shortlog.emplace(_host.number, getCurrentTime() +"\t"+ _IPstring + "\tOK");
                std::cout<<  getCurrentTime() << "\t" << _IPstring << "\tХост номер(" << _host.number << ")\tOK" << std::endl;
            }
            else
            {
                            	shortlog.emplace(_host.number, getCurrentTime()+ "\t" +  _IPstring + "\t" + _host.model + "\tOK");

                std::cout << getCurrentTime() << "\t" << _IPstring << "\t" << _host.model << "\tOK" << std::endl;
            }

            return; // логика завершения, по идее должен вызвать деструктор прям ща
        }
        else
        {

            // выполняю командe
            _writableCommand = "\n........................................................................................\n\nотправленна команда\t " + _currentDoCommands[_iteration].cmd + " \n\tРезультат:\n";
            _ss << _writableCommand;

            if ((_currentDoCommands[_iteration].send_to_step == ""))
            {

                send_to_step = "\x20\n";

            }
            else
            {
                send_to_step = _currentDoCommands[_iteration].send_to_step  + "\n";
            }

            execute_one_command();
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на iteration" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::execute_one_command() // вообще сделал что-то типо pipeline но хз правильно ли, зато понятно, не так востребовано как с предыдущими но да и ладно
{
    try
    {
        command_exec = _currentDoCommands[_iteration].cmd + "\n"; // gpt подсказал так написать ввод
        libssh2_channel_write(_channel, command_exec.c_str(), command_exec.size());
        _str = "Отправленна команда " + _currentDoCommands[_iteration].cmd;
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
                                        plog->writeLog("ошибка таймера чтения в SSHSession: "+er.message()+" для хоста " + _IPstring);
                                    }} });
        read_one_command();
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на exec" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::check_end_of_read(uint16_t buffer_point_add) // не логирую
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
            sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
            shortErrlog("Ошибка во время считывания чего-то, что не заканчивается на ожидаемое значение _end_of_read(ssh) " + std::to_string(rc) + " к хосту ");
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        {
            _one_again_taked = false; // это цикличное ожидание, не нужно отправлять нужный энтер
            auto self = shared_from_this();

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
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
        sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
        shortErrlog("Ошибка во время считывания чего-то, что не заканчивается на _end_of_read(сокет) "+ec.message() +" к хосту ");
                                   } });
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на check" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::read_one_command()
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
            sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
            shortErrlog("Ошибка во время считывания ответа(ssh) " + std::to_string(rc) + " к хосту ");
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
        sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
            shortErrlog("Ошибка во время считывания ответа(сокет) " + ec.message() + " к хосту ");

                                   } });
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на read" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void SSHSession::end_one_command()
{
    try
    {
        _timer.cancel();

        // проверить ответ на равенство ожидаемому
        if (_currentDoCommands[_iteration].expect != "")
        {
            _expect = std::regex(_currentDoCommands[_iteration].expect); // содержет строку в которой описана регулярка
            if (!(std::regex_search(_part_of_ss.str(), _expect)))
            {
                // если не (регулярки совпадают) то выкинуть ошибку

                _str = "неожиданный output команды " + _currentDoCommands[_iteration].cmd;
                wlog->writeLog(_str + " на хосте " + _IPstring + " Подробнее в errHosts.log");
                shortErrlog(_str);
                _str += ". Ожидалось: " + _currentDoCommands[_iteration].expect + "\n\n А в ответе: " + _part_of_ss.str();
                _str += "\n\n\n" + _ss.str() + _part_of_ss.str();
                _host.log += ("\n" + _str);
                sqlite->write_one_hostCommit(TableNameForErrorHosts, _host);
                return;
            }
        }

        // проверить ответ на равенство не ожидаемому
        if (_currentDoCommands[_iteration].not_expect != "")
        {
            _not_expect = std::regex(_currentDoCommands[_iteration].not_expect); // содержет строку в которой описана регулярка
            if ((std::regex_search(_part_of_ss.str(), _not_expect)))
            {
                // если не (регулярки совпадают) то выкинуть ошибку

                _str = "неожиданный output команды " + _currentDoCommands[_iteration].cmd;
                wlog->writeLog(_str + " на хосте " + _IPstring + " Подробнее в errHosts.log");
                shortErrlog(_str);
                _str += ". Ожидалось не получить: " + _currentDoCommands[_iteration].not_expect + "\n\n А в ответе: " + _part_of_ss.str();
                _str += "\n\n\n" + _ss.str() + _part_of_ss.str();
                _host.log += ("\n" + _str);
                sqlite->write_one_hostCommit(TableNameForErrorHosts, _host);
                return;
            }
        }

        _ss << _part_of_ss.str();
        _part_of_ss.str("");

        ++_iteration;

        one_iteration(); // продолжаю пока в массиве команд ещё есть данные
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на end" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

SSHSession::~SSHSession()
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

void SSHSession::filterHosts(std::vector<HOST> &hosts)
{
    auto it = std::remove_if(hosts.begin(), hosts.end(), [](HOST &host)
                             { return host.model == ""; });
    hosts.erase(it, hosts.end());
}
