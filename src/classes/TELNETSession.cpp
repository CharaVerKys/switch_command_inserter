#include <TELNETSession.hpp>

//
//
//
std::string TELNETSession::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(1) << ms.count();
    return ss.str();
}

void TELNETSession::start_timer()
{
    auto self = shared_from_this(); // совершенно не уверен что это хорошая идея его сюда пихать
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
                                        plog->writeLog("ошибка таймера чтения в TELNETSession: "+er.message()+" для хоста " + _IPstring);
                                    }} });
}

std::map<uint16_t, std::string> TELNETSession::shortlog;

void TELNETSession::shortErrlog(std::string str)
{
    if (_host.model == "script")
    {

        shortlog.emplace(_host.number, getCurrentTime() + "\t" + _IPstring + "\tError: " + str);

        std::cout << getCurrentTime() << "\t" << _IPstring << "\tХост номер(" << _host.number << ")\tError: " << str << std::endl;
    }
    else
    {
        shortlog.emplace(_host.number, getCurrentTime() + "\t" + _IPstring + "\t" + _host.model + "\tError: " + str);
        std::cout << getCurrentTime() << "\t" << _IPstring << "\t" << _host.model << "\tError: " << str << std::endl;
    }
}

TELNETSession::TELNETSession(asio::io_context &io_context, HOST &host, std::vector<COMMANDS> &currentDoCommands)
    : _io_context(io_context), _host(host), _currentDoCommands(currentDoCommands), _socket(_io_context), _timer(_io_context)
{
    this->_iteration = 0;
    _IPstring = asio::ip::address_v4(_host.address).to_string();
    wlog->writeLog("Инициализирован коммит(telnet) для " + _IPstring);
    _timer.expires_after(std::chrono::minutes(2));
}

void TELNETSession::connect()
{
    try
    {
        asio::ip::tcp::endpoint endpoint(asio::ip::address_v4(_host.address), 23);
        auto self = shared_from_this(); // по какойто причине нужно объявлять каждый раз именно здесь! если вынести в конструктор пераметры класса то не работает
                                        // возможно (вероятнее всего) не нужно когда используется массив указателей, потому что shared ptr живут долго...
        _socket.async_connect(endpoint, [this, self](const asio::error_code &ec)
                              {
        if (!ec)
        {
               _str = "Успешное создание подключения(telnet) к хосту ";
               _host.log += ("\n"+_str); 
                wlog->writeLog(_str+_IPstring);
start_timer();

            read_from_host(_regex_login,std::bind(&TELNETSession::send_login, this));

        }
        else
        {
            _str = "Ошибка при подключении(telnet) "+ ec.message()+" к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
               shortErrlog(_str);

        } });
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на connect (telnet)" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void TELNETSession::send_login()
{
    try
    {
        _timer.cancel();

        auto self = shared_from_this();
        asio::async_write(_socket, asio::buffer(_host.login.name), [this, self](const asio::error_code &error, size_t bytes_transferred)
                          {
        if (!error) {

            start_timer();

            read_from_host(_regex_password,std::bind(&TELNETSession::send_password, this));



        } else {
        _str = "Ошибка при авторизации(логин)(telnet) "+ error.message()+" к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
               shortErrlog(_str);
        } });
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на send_login" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void TELNETSession::send_password()
{
    try
    {
        _timer.cancel();
        _last_read.str("");

        auto self = shared_from_this();
        asio::async_write(_socket, asio::buffer(_host.login.password), [this, self](const asio::error_code &error, size_t bytes_transferred)
                          {
        if (!error) {

            start_timer();
            read_from_host(_regex_end_of_read, std::bind(&TELNETSession::one_iteration, this));

        } else {
          _str = "Ошибка при авторизации(пароль)(telnet) "+ error.message()+" к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
               shortErrlog(_str);
        } });
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на send_password" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void TELNETSession::one_iteration()
{

    try
    {
        _timer.cancel();
        if (!(_iteration < _currentDoCommands.size())) // если команд 0 то не выполнит ничего
        {

            _str = ("Закончен цикл для хоста(telnet) " + _ss.str());
            _host.log += ("\n" + _str);
            wlog->writeLog("Закончен цикл для хоста(telnet) " + _IPstring);
            sqlite->write_one_hostCommit(TableNameForGoodHosts, _host);
            if (_host.model == "script")
            {
                shortlog.emplace(_host.number, getCurrentTime() + "\t" + _IPstring + "\tOK");
                std::cout << getCurrentTime() << "\t" << _IPstring << "\tХост номер(" << _host.number << ")\tOK (telnet)" << std::endl;
            }
            else
            {
                shortlog.emplace(_host.number, getCurrentTime() + "\t" + _IPstring + "\t" + _host.model + "\tOK (telnet)");

                std::cout << getCurrentTime() << "\t" << _IPstring << "\t" << _host.model << "\tOK (telnet)" << std::endl;
            }

            return; // логика завершения, по идее должен вызвать деструктор прям ща
        }
        else
        {

            // выполняю командe
            _writableCommand = "\n........................................................................................\n\nотправленна команда\t " + _currentDoCommands[_iteration].cmd + " \n\tРезультат:\n";
            _ss << _writableCommand;

            exec_com();
        }
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на iteration (telnet)" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void TELNETSession::exec_com()
{

    try
    {
        _timer.cancel();

        auto self = shared_from_this();
        asio::async_write(_socket, asio::buffer(_currentDoCommands[_iteration].cmd), [this, self](const asio::error_code &error, size_t bytes_transferred)
                          {
        if (!error) {

start_timer();


            read_from_host(_regex_end_of_read, std::bind(&TELNETSession::end_of_com, this));


        } else {
           _str = "Ошибка при отправки команды(telnet) "+ error.message()+" к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
               shortErrlog(_str);
        } });
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на exec_com" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void TELNETSession::end_of_com()
{
    try
    {
        _timer.cancel();

        // проверить ответ на равенство ожидаемому
        if (_currentDoCommands[_iteration].expect != "")
        {
            _expect = std::regex(_currentDoCommands[_iteration].expect); // содержет строку в которой описана регулярка
            if (!(std::regex_search(_last_read.str(), _expect)))
            {
                // если не (регулярки совпадают) то выкинуть ошибку

                _str = "неожиданный output команды " + _currentDoCommands[_iteration].cmd;
                wlog->writeLog(_str + " на хосте " + _IPstring + " Подробнее в errHosts.log");
                shortErrlog(_str);
                _str += ". Ожидалось: " + _currentDoCommands[_iteration].expect + "\n\n А в ответе: " + _last_read.str();
                _str += "\n\n\n" + _ss.str() + _last_read.str();
                _host.log += ("\n" + _str);
                sqlite->write_one_hostCommit(TableNameForErrorHosts, _host);
                return;
            }
        }

        // проверить ответ на равенство не ожидаемому
        if (_currentDoCommands[_iteration].not_expect != "")
        {
            _not_expect = std::regex(_currentDoCommands[_iteration].not_expect); // содержет строку в которой описана регулярка
            if ((std::regex_search(_last_read.str(), _not_expect)))
            {
                // если не (регулярки совпадают) то выкинуть ошибку

                _str = "неожиданный output команды " + _currentDoCommands[_iteration].cmd;
                wlog->writeLog(_str + " на хосте " + _IPstring + " Подробнее в errHosts.log");
                shortErrlog(_str);
                _str += ". Ожидалось не получить: " + _currentDoCommands[_iteration].not_expect + "\n\n А в ответе: " + _last_read.str();
                _str += "\n\n\n" + _ss.str() + _last_read.str();
                _host.log += ("\n" + _str);
                sqlite->write_one_hostCommit(TableNameForErrorHosts, _host);
                return;
            }
        }

        _ss << _last_read.str();
        _last_read.str("");

        ++_iteration;

        one_iteration(); // продолжаю пока в массиве команд ещё есть данные
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на end_of_com" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

// вот эту хню нужно с двух сторон обкладывать таймером
void TELNETSession::read_from_host(const std::regex &regex, std::function<void()> next_callback)
{
    try
    {

        auto self = shared_from_this();
        asio::async_read_until(_socket, _read_buffer, '\n', [this, self, &regex, next_callback](const asio::error_code &error, size_t bytes_transferred)
                               {
            if (!error)
            {
                std::istream is(&_read_buffer);
                std::getline(is, line);
                _last_read << line; // сохраняем строку в stringstream для дальнейшей обработки

                if (std::regex_search(line, regex))
                {
                    // Если найдено совпадение с регуляркой, вызываем обратный вызов
                    next_callback();
                }
                else
                {
                    // Продолжаем читать из сокета до тех пор, пока не найдем совпадение
                    read_from_host(regex, next_callback);
                }
            }
            else
            {
              _str = "Ошибка при считывания команды(telnet) "+ error.message()+" к хосту ";
               _host.log += ("\n"+_str); 
               plog->writeLog(_str + _IPstring);
               sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
               shortErrlog(_str);
            } });
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на read_from_host" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }
}

void TELNETSession::filterHosts(std::vector<HOST> &hosts)
{
    auto it = std::remove_if(hosts.begin(), hosts.end(), [](HOST &host)
                             { return host.model == ""; });
    hosts.erase(it, hosts.end());
}