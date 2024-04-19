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
read_from_host_init();
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
        asio::async_write(_socket, asio::buffer(_host.login.name + "\r\n"), [this, self](const asio::error_code &error, size_t bytes_transferred)
                          {
        if (!error) {

            start_timer();
read_from_host_init();
            read_from_host(_regex_password,std::bind(&TELNETSession::send_password, this));

std::cout << "triger 3\n";

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
        std::cout << "triger 4\n";
        auto self = shared_from_this();
        asio::async_write(_socket, asio::buffer(_host.login.password + "\r\n"), [this, self](const asio::error_code &error, size_t bytes_transferred)
                          {
        if (!error) {
std::cout << "triger 5\n";
            start_timer();
            read_from_host_init();
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
        _last_read.str("");

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

        auto self = shared_from_this();
        asio::async_write(_socket, asio::buffer((_currentDoCommands[_iteration].cmd + "\r\n")), [this, self](const asio::error_code &error, size_t bytes_transferred)
                          {
        if (!error) {

            start_timer();
            read_from_host_init(_currentDoCommands[_iteration].send_to_step.c_str());
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

void TELNETSession::check_end_of_read(const std::regex &regex) // не логирую
{
    try
    {                                                                                  // обработчик проверки получения всех отправленных данных
        if (std::regex_search(_last_read_one_it_inside_for_send_to_step.str(), regex)) // условие выхода - совпадение с regex конца чтения
        {
            _is_end_of_readq = true;
            return;
        }

        bool temp_for_no_duplicate = std::regex_search(_last_read_one_it_inside_for_send_to_step.str(), _moreRegex);

        if (!(temp_for_no_duplicate || _is_this_moreq))
        // если это сейчас мор, или уже было мор то выполняется функция, а не выход для дальнейшего чтения
        {
            return;
        }
        // else contunue

        if (temp_for_no_duplicate)
        {
            _last_read << _last_read_one_it_inside_for_send_to_step.str(); // чтобы обрабатывать новые данные а не дублировать старые
            _last_read_one_it_inside_for_send_to_step.str("");
            // условие выхода всё тоже...
            asio::write(_socket, asio::buffer(send_to_step));
            // каждое новое обнаружение more означает что предыдущие данные прочитаны до конца
        }
        _is_this_moreq = true; // чтобы не выходить если дошёл до сюда

        // в ssh там логика такая что одинаково читает что из чтения вызова что из проверки,
        //  а тут я не придумал как так же сделать, и чтобы избежать дублирования (это вызов асинхронной функции внутри которой вызывается другая асинхронная функция)
        //  дк вот я решил оставить синхронную реализацию
        //  опасный момент какой-то
        //  было бы круто остановить выполнение программы пока не выполнится вычисление в check_end_of_read, но кажется это усложнит логику сильно,
        //  прийдётся использовать фьючеры как то (а я их не изучал ещё как и корутины) и для каждого отдельного вызова check_end_of_read ждать...
        //  а так - я внутрь вхожу только в том случае если !(std::regex_search(_last_read_one_it_inside_for_send_to_step.str(), _moreRegex)
        //  то есть просто чтение выполняется как обычная функция, а не как асинхронная (которую всё равно синхронной сделал но в ssh асинхронная)
        //
        asio::read(_socket, _read_buffer, asio::transfer_at_least(1), asio_error);
    }
    catch (const std::exception &e)
    {
        std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на check(telnet)" + _host.log;
        std::cerr << str << std::endl;
        plog->writeLog(str);
    }

    // разделяю области работы

    if (!asio_error)
    {
        try
        {
            std::istream is(&_read_buffer);
            while (std::getline(is, line))
            {
                _last_read_one_it_inside_for_send_to_step << line + "\n"; // сохраняем строку в stringstream для дальнейшей обработки
            }
            check_end_of_read(regex);
        }
        catch (const std::exception &e)
        {
            std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на check(telnet)" + _host.log;
            std::cerr << str << std::endl;
            plog->writeLog(str);
        }
    }
    else
    {
        _str = "Ошибка во время считывания чего-то, что не заканчивается на _end_of_read(telnet) " + asio_error.message() + " аутпут(" + _ss.str() + _last_read.str() + _last_read_one_it_inside_for_send_to_step.str() + ") ";
        _host.log += ("\n" + _str);
        plog->writeLog("Ошибка во время считывания чего-то, что не заканчивается на ожидаемое значение _end_of_read(telnet) " + asio_error.message() + " к хосту " + _IPstring);
        sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
        shortErrlog("Ошибка во время считывания чего-то, что не заканчивается на ожидаемое значение _end_of_read(telnet) " + asio_error.message() + " к хосту ");
        throw std::runtime_error("ошибка из подфункции чтения, запись отработала нормально");
    }
}

// вот эту хню нужно с двух сторон обкладывать таймером
void TELNETSession::read_from_host(const std::regex &regex, std::function<void()> next_callback)
{
    try
    {
        auto self = shared_from_this();
        asio::async_read(_socket, _read_buffer, asio::transfer_at_least(1), [this, self, &regex, next_callback](const asio::error_code &error, size_t bytes_transferred)
                         {
            if (!error)
            {
                std::istream is(&_read_buffer);
                while (std::getline(is, line)) {
                _last_read_one_it_inside_for_send_to_step << line + "\n"; // сохраняем строку в stringstream для дальнейшей обработки
                        }


                        if(std::regex_search(line, _control_1)){

        asio::async_write(_socket, asio::buffer(std::string("\xff\xfb\x01\xff\xfc\x1f\xff\xfe\x05\xff\xfc\x21")), [this, self, &regex, next_callback](const asio::error_code &error, size_t bytes_transferred)
                          {
                            if (!error) {
                                    read_from_host(regex, next_callback);

                            } else {
                                _str = "Ошибка при чтении - последовательность вариант 1 (telnet) "+ error.message()+" к хосту ";
                                    _host.log += ("\n"+_str); 
                                    plog->writeLog(_str + _IPstring);
                                    sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
                                    shortErrlog(_str);
                            } }); 
                            return;
                        }

          if(std::regex_search(line, _control_2)){

        asio::async_write(_socket, asio::buffer(std::string("\xff\xfc\x18\xff\xfc\x20\xff\xfc\x23\xff\xfc\x27")), [this, self, &regex, next_callback](const asio::error_code &error, size_t bytes_transferred)
                          {
                            if (!error) {
                                    read_from_host(regex, next_callback);

                            } else {
                                _str = "Ошибка при чтении - последовательность вариант 2 (telnet) "+ error.message()+" к хосту ";
                                    _host.log += ("\n"+_str); 
                                    plog->writeLog(_str + _IPstring);
                                    sqlite->write_one_hostCommit(TableNameForProgErrorHosts, _host);
                                    shortErrlog(_str);
                            } }); 
                            return;
                        }


                check_end_of_read(regex); // в телнет это синхронная операция, здесь поток остановится
                if (_is_end_of_readq){
                    next_callback();
                    _last_read << _last_read_one_it_inside_for_send_to_step.str();
                    _last_read_one_it_inside_for_send_to_step.str("");
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
        std::string errorMsg = e.what();
        if (errorMsg != "ошибка из подфункции чтения, запись отработала нормально")
        {
            std::string str = "Сработал глобальный try-catch " + std::string(e.what()) + " к хосту " + _IPstring + " на read_from_host" + _host.log;
            std::cerr << str << std::endl;
            plog->writeLog(str);
        }
    }
}

void TELNETSession::read_from_host_init(const char *what_send_to_step = "")
{
    _is_end_of_readq = false;
    _is_this_moreq = false;
    if (what_send_to_step[0]=='\0')
    {
        send_to_step = "\x20\n";
    }
    else
    {
        send_to_step = what_send_to_step;
    }
}

void TELNETSession::filterHosts(std::vector<HOST> &hosts)
{
    auto it = std::remove_if(hosts.begin(), hosts.end(), [](HOST &host)
                             { return host.model == ""; });
    hosts.erase(it, hosts.end());
}
