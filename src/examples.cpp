

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
    const char *currentCstrCommand;
    char buffer[4096];
    int cmd_exit_status;
    std::regex expect;
    std::regex model_regex;
    std::regex end_of_read = std::regex("\\S+[#$>]\\s$", std::regex::ECMAScript);
    std::regex moreRegex = std::regex("More", std::regex::ECMAScript);
    size_t iteration;
    bool is_end_of_readq;
    bool one_again_taked;

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

        init_chanel(); // всё ещё тело аутен калбека кста
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

    void init_chanel()
    {
        // всё ещё тело аутен калбека кста
        auto self = shared_from_this();
        _channel = libssh2_channel_open_session(_session);

        if (_channel)
        {
            std::cout << "канал открылся для " << _команда << std::endl;
            init_shell();
        }
        else if (libssh2_session_last_errno(_session) == LIBSSH2_ERROR_EAGAIN)
        {

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   std::cout << "и я зашёл в тело асинх старт ченел " << _команда << " \n"; 

                                   if (!ec)
                                   {
                                       init_chanel();
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

    void init_shell()
    {

        auto self = shared_from_this();
        int rc = libssh2_channel_shell(_channel);
        if (rc)
        {
            std::cout << "канал шел открылся для " << _команда << std::endl;

            read_label(); // логика для запуска большого количества команд асинхронно (цикл while для асинхронной реализации в event loop)
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN)
        {

            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   std::cout << "и я зашёл в тело асинх старт shell " << _команда << " \n"; 

                                   if (!ec)
                                   {
                                       init_shell();
                                   }
                                   else
                                   {
                                       std::cerr << "Error during shell create - " << ec.message() << std::endl;
                                   } });
        }
        else
        {
            std::cerr << "Failed to open shell with err_code " << rc << std::endl;
        }
    }

    void read_label()
    {
        std::regex end_of_read1 = std::regex("applicable", std::regex::ECMAScript);
        int rc = libssh2_channel_read(_channel, buffer, sizeof(buffer));
        if (rc > 0)
        {
            std::cout << "считало в read_label для " << _команда << " \n";
            part_of_ss.write(buffer, rc); // здесь находится лейбл, сейчас не используется и отчищается перед началом цикла
            read_label();
        }
        else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN)
        {
            str += " не удалось считать вывод от устройства с кодом ошибки " + std::to_string(rc) + " в read_a_fuck \n" + ss.str() + part_of_ss.str();
            std::cerr << "Error: " << str << std::endl;
        }

        else if (std::regex_search(part_of_ss.str(), end_of_read1)) // главное чтобы проверка была до открытия сокета
        {
            part_of_ss.str(""); //  отчищается перед началом цикла
            one_iteration();
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        {
            auto self = shared_from_this();
            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self](const asio::error_code &ec)
                               {
                                   std::cout << "и я зашёл в тело асинх read_label для " << _команда << " \n";

                                   if (!ec)
                                   {
                                       read_label();
                                   }
                                   else
                                   {
                                       std::cerr << "Error during read_label answer - " << ec.message() << std::endl;
                                   } });
        }
    }

    void one_iteration()
    {

        if (!(iteration < currentDoCommands.size())) // если команд 0 то не выполнит ничего
        {
            str += ss.str();
            std::cout << "\tи я полученная строка лога" << str << " \n";
            return; // логика завершения, по идее должен вызвать деструктор прям ща
        }
        else
        {
            // выполняю командe
            writableCommand = "\n\nотправленна команда\t " + currentDoCommands[iteration].cmd + " \n\tРезультат:\n";
            ss << writableCommand;
            execute_one_command();
        }
    }

    void execute_one_command() // вообще сделал что-то типо pipeline но хз правильно ли, зато понятно, не так востребовано как с предыдущими но да и ладно
    {
        currentCstrCommand = (currentDoCommands[iteration].cmd += "\n").c_str();
        libssh2_channel_write(_channel, currentCstrCommand, strlen(currentCstrCommand));
        std::cout << "отправленна команда " + currentDoCommands[iteration].cmd + " для " + _hostStringIp + "\n";
        is_end_of_readq = false;
        one_again_taked = true; // чтобы выполнелась отправка первого энтера
        read_one_command();
    }

    void check_end_of_read(uint16_t buffer_point_add)
    { // обработчик проверки получения всех отправленных данных
        if (std::regex_search(buffer, buffer + buffer_point_add, end_of_read))
        {
            is_end_of_readq = true;
            return;
        }
        if (!(std::regex_search(buffer, buffer + buffer_point_add, moreRegex)))
        {
            return;
        }
        // else contunue

        if (one_again_taked)
        {
            libssh2_channel_write(_channel, "\n", strlen("\n"));
        } // отправляется только если было прочитано до этого (или при старте)
        int rc = libssh2_channel_read(_channel, buffer, sizeof(buffer));

        if (rc > 0)
        {
            std::cout << "считало в check_end_of_read для " << _команда << " \n";
            part_of_ss.write(buffer, rc);
            one_again_taked = true;
            check_end_of_read(rc);
        }
        else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN)
        {
            str += " не удалось считать вывод от устройства с кодом ошибки " + std::to_string(rc) + " \n" + ss.str() + part_of_ss.str();
            std::cerr << "Error: " << str << std::endl;
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        {
            one_again_taked = false; // это цикличное ожидание, не нужно отправлять нужный энтер
            auto self = shared_from_this();
            _socket.async_wait(asio::ip::tcp::socket::wait_read, [this, self, rc](const asio::error_code &ec)
                               {
                                   std::cout << "и я зашёл в тело асинх check_end_of_read для " << _команда << " \n"; 

                                   if (!ec)
                                   {
                                       check_end_of_read(rc);
                                   }
                                   else
                                   {
                                       std::cerr << "Error during check_end_of_read answer - " << ec.message() << std::endl;
                                   } });
        }
        else
        {
        } // это произойдёт только в том случае если отправится eof
    }

    // РИД НУЖНО ОБЕРНУТЬ В ТАЙМЕР НА 15 МИНУТ В ИТОГОВОЙ РЕАЛИЗАЦИИ (от экзек до енд оф ком)

    void read_one_command()
    {
        int rc = libssh2_channel_read(_channel, buffer, sizeof(buffer));

        if (rc > 0)
        {
            std::cout << "считало в read_one_command для " << _команда << " \n";
            part_of_ss.write(buffer, rc);
            check_end_of_read(rc);
            read_one_command();
        }
        else if (rc < 0 && rc != LIBSSH2_ERROR_EAGAIN)
        {
            str += " не удалось считать вывод от устройства с кодом ошибки " + std::to_string(rc) + " \n" + ss.str() + part_of_ss.str();
            std::cerr << "Error: " << str << std::endl;
        }

        else if (is_end_of_readq) // главное чтобы проверка была до открытия сокета
        {
            end_one_command(); // продолжить выполнение (этот же калбек просто вынес в другую функцию)
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        {
            auto self = shared_from_this();

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

        ++iteration;

        one_iteration(); // продолжаю пока в массиве команд ещё есть данные
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















































Логика здесь от части не рабочая, я помню что там нужно поменять, но на неё можно не ориентироваться вообще

// к комиту добавить параметр -y




// определитель моделей
// внутри он берёт этот массив логин пароль и передаёт его в свою асинх функцию,
// где перебирает в цикле while их - пытаясь подключиться
// подключился - начинает по своей логике искать строку с моделью
// как она будет реализована (логика) чёрт знает - задача общая

FindAllModelsSSH modelfinderssh;

modelfinderssh.startSearch(activeHosts.ssh, configer->getLogins_Passwords()); 
                                                                                      // надо чтобы в конце старт сёрча записывал в базу данных весь массив хостов из первого параметра
                                                                                      // собственно обычный врайт запустить, всё там норм должно быть

// реализация внутри старт сёрча для получения строки
std::string allmodels;
allmodels += newmodel;
allmodels += " /:\\ "; // блямба по приколу такая между именами моделей
// "модель /:\ модель /:\ модель /:\ модель /:\ " // точный вид строки
// вернёт в консоль и в воркинг.лог эту строку

// обработчик ssh
// в первую очередь кидает 2 предупреждения требующие подтверждение,
// о том что для корректной работы требуется заполненный массив хостов, в котором не будет таких,
// где будут логин пароль, но не будет моделей, если таковые будут они запишутся в error_host_list ещё до того как начнётся обработка подключений
// защитит от случайного запуска комита и снимает ответственность с разработчика

// то есть требует чтобы перед запуском юзер убедился что show modelHost соответствует этому условию

// после запуска commit в отличии от scan и identify массив хостов не перезаписывается, и можно посмотреть на него через шоу после запуска комита
// то есть доступны весь массив, массив удачных, массив ошибок хоста и массив ошибок программы при обработке подключения
// для подмоссивов используется host.log который хранит весь лог полученных и отправленных команд
//         не знаю как долго я эту логику буду реализовывать

// подтаблицы уничтожаются перед каждым новым запуском комита

CommandsCommiterSSH commitssh(std::vector<HOST>valid_to_commitssh, configer->getModels_and_commands()); // берёт готовый к обработке

//-----------------------------------------------------------

//    асинх функция абстракция
/*

try{
    asio::io_context io_context;
}поймать - маловероятная ошибка - ошибка программы на глобальном уровне - не получилось создать контекст - вылет


для каждого хоста из всех хостов{

        try{

            объявить_асинхронную_функцию(host, io_context,&modelCMD)
                // тут тип нужен для выбора обработки подключения

        }поймать - ошибка программы на уровне определённого хоста - не получилось объявить корневую асинх функцию
                не вылет, а запись этого хоста в таблицу PerrHostsSSH

}



try{
    io_context.run();
}поймать - ошибка программы на глобальном уровне - не получилось запустить контекст - вылет




-------------------------------------------------------------









объявить_асинхронную_функцию(&хост, &io_context, &modelCMD{ 

try{
    auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
}поймать - маловероятная ошибка - ошибка программы на уровне определённого хоста - не получилось объявить сокет
                не вылет, а запись этого хоста в таблицу PerrHostsSSH

try{
    asio::ip::tcp::endpoint endpoint(asio::ip::address_v4(host.address), 22);

 }поймать - маловероятная ошибка - ошибка программы на уровне определённого хоста - не получилось объявить эндпоинт
                не вылет, а запись этого хоста в таблицу PerrHostsSSH


    try{
        socket->async_connect(endpoint, callback); // не забыть определить колбек выше этого вызова, в примере он чисто для структуры находится ниже

    }поймать - ошибка программы на уровне определённого хоста - не получилось создать асинх коннект
                не вылет, а запись этого хоста в таблицу PerrHostsSSH

}//конец для функции объявить_асинхронную_функцию



---------------------------------------------------------------------------------

auto callback = [&host, socket, &modelCMD](const asio::error_code &error) // захватываю указатели по значению

    вся логика для входа,ввода,выхода, учитывая все исключения    
    {
создаю строку str;
создать типа айпи_ви4 хостИп 

str += инициализирован сокет для хоста + хостИп
влог это же


try{
 LIBSSH2_SESSION *session = libssh2_session_init();
    if (!session)
    {
          throw рантайм эрор вернуть значение сессии
    }
}поймать - ошибка программы на уровне инициализации - не получилось инициализиоровать сессию ssh
                не вылет, а запись этого хоста в таблицу PerrHostsSSH
                     то есть:
                        str += не получилось инициализиоровать сессию ssh. Ошибка: (исключение)
                        записать стр в host.log
                        влог тоже + хостИп
                        запись в БД
                return;




try{
    int rc = libssh2_session_handshake(session, socket.native_handle());
        if (rc != 0)
        {
            ошибка программы на уровне определённого хоста - не получилось подключить ssh сессию к сокету
                    не вылет, а запись этого хоста в таблицу PerrHostsSSH
                        то есть:
                            str += не получилось подключить ssh сессию к сокету, возможно ssh клиент на хосте отключили. Код ошибки :(код ошибки) (это rc)
                            записать стр в host.log
                            влог тоже + хостИп
                            запись в БД
            libssh2_session_free(session);
            return;
        }
}поймать - ошибка программы на уровне определённого хоста - не получилось подключить ssh сессию к сокету
                не вылет, а запись этого хоста в таблицу PerrHostsSSH
                     то есть:
                        str += не получилось подключить ssh сессию к сокету. Ошибка: (исключение)
                        записать стр в host.log
                        влог тоже + хостИп
                        запись в БД
                return;




try{
        rc = libssh2_userauth_password(session, host.login.name, host.login.password );
            if (rc != 0)
            {
                libssh2_session_disconnect(session, "Authentication failed");
                libssh2_session_free(session);
                throw std::runtime_error("Не удалось подключиться по известным логин/пароль");
            }


}поймать - ошибка программы на уровне определённого хоста - Не удалось подключиться по известным логин/пароль
                не вылет, а запись этого хоста в таблицу errHostsSSH
                     то есть:
                        str += Ошибка: (исключение). \tЕсли ошибка НЕ о логин/пароль это програмная ошибка.
                        записать стр в host.log
                        влог тоже + хостИп
                        запись в БД
                return;



try{
    channel = libssh2_channel_open_session(session);
        if (!channel) {
throw Не удалось открыть канал
        }
}поймать - ошибка программы на уровне определённого хоста - Не удалось создать канал.
                не вылет, а запись этого хоста в таблицу PerrHostsSSH
                     то есть:
                        str += Ошибка: (исключение). Не удалось создать канал.
                        записать стр в host.log
                        влог тоже + хостИп
                        запись в БД






=-вайу=айцу=а-цу=а-цу=ацу=а-цу=а-ацу=а-цу=а-цу=а-цу=а-ц=у-цуа=-вайу=айцу=а-цу=а-цу=ацу=а-цу=а-ацу=а-цу=а-цу=а-цу=а-ц=у-цуа


try {
    // ваш код для обработки команд
// буду выкидывать исключения, после того как запишу в БД
        // так что функция воид

        str += инициализировано успешно, начинаю ввод команд
        влог тоже самое + хостИП
    
    inputOutputSSHdoCommands(channal,&host,&modelCMD,&str);



=-вайу=айцу=а-цу=а-цу=ацу=а-цу=а-ацу=а-цу=а-цу=а-цу=а-ц=у-цуа=-вайу=айцу=а-цу=а-цу=ацу=а-цу=а-ацу=а-цу=а-цу=а-цу=а-ц=у-цуа




 // закрытие канала
libssh2_channel_close(channel);
libssh2_channel_free(channel); 
    // отключение сессии SSH
    libssh2_session_disconnect(session, "Normal shutdown");
    libssh2_session_free(session);
} catch (const std::exception& e) {
        try{

            libssh2_session_disconnect(session, "Error occurred");
            libssh2_session_free(session);
            
        }поймать - серьёзная системная ошибка вызывающая утечку паияти (прям так и написать)   +  отправить в плог
}


str += сессия ssh закрыта
влог тоже самое + хостИП
хост.лог = str
    записываю хост в таблицу goodHosts



    }// закрывает калбек (для самой асинх вызова)














                                                            std::vector<            // вектор для отправки в обработчик
                                                                    std::pair<                // пара модель и команды к ней
                                                                        std::string,          // модель
                                                                        std::vector<COMMANDS> // набор команд
                                                                        >                     //                 жесть ваще вектор
                                                                    > modelCMD

 void inputOutputSSHdoCommands(&channal,&host,&modelCMD,&str){

создать типа айпи_ви4 хостИп 

            std::vector<COMMANDS> currentDoCommands;
                for( auto &el : modelCMD){

            _"№);*:_(%?*№%(_?*№%_(:*№%(?*№%(:*№%(?_*№%:(_*%?_(№%?*_№%):(№%):*№%?(_№%:*_)%:(_%?)*№_%):*_)№%?*№%):№%_(:*№%?(№%():*%№(?*)))))))
            _"№);*:_(%?*№%(_?*№%_(:*№%(?*№%(:*№%(?_*№%:(_*%?_(№%?*_№%):(№%):*№%?(_№%:*_)%:(_%?)*№_%):*_)№%?*№%):№%_(:*№%?(№%():*%№(?*)))))))
            _"№);*:_(%?*№%(_?*№%_(:*№%(?*№%(:*№%(?_*№%:(_*%?_(№%?*_№%):(№%):*№%?(_№%:*_)%:(_%?)*№_%):*_)№%?*№%):№%_(:*№%?(№%():*%№(?*)))))))
            _"№);*:_(%?*№%(_?*№%_(:*№%(?*№%(:*№%(?_*№%:(_*%?_(№%?*_№%):(№%):*№%?(_№%:*_)%:(_%?)*№_%):*_)№%?*№%):№%_(:*№%?(№%():*%№(?*)))))))

                }
            найти пару, для host.model, где модели совпадают, и из неё достать currentDoCommands
            // тут не может быть хоста без модели, потому что подаётся массив отфильтрованных.
            // это не стоит наверное логировать



+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
влог- начато исполнение команд для хоста + хостИП 
хост лог тоже самое


std::stringstream ss;
    for(COMMANDS &command : currentDoCommands){

исполнить----------------------------------------------------------------------------

в ss писать \n\n\t команда \n 
    // ss.write(buffer, rc); - посмотреть можно ли сдесь вставлять строку

 libssh2_channel_exec(channel, command.cmd);


влог- отправлена команда + command.cmd + на + хостИП 
хост лог тоже самое 

получить ответ по частям-------------------------------------------------------------------------------

    
    char buffer[4096];
    

// если когдато повиснет на этом моменте - обернуть в таймер на 150 секунд который будет прерывать 
    do
    {
        int rc = libssh2_channel_read_nonblocking(channel, buffer, sizeof(buffer));
        if (rc > 0)
        {
            ss.write(buffer, rc);
        }
        else if (rc == LIBSSH2_ERROR_EAGAIN) // ошибка говорящая что не все байты получены
        { 
            continue;
        }
        else if (rc<0){


            str += не удалось считать весь вывод от устройства.
            str += ss.str();
            host.log = str;
            записать хост в таблицу errHosts
            throw не удалось считать данные из вывода коммутатора
        }else // if (rc == 0)
        {
            break;
        }
    } while (true);



проверить код
    int cmd_exit_status = libssh2_channel_get_exit_status(channel);
    if(!(cmd_exit_status == command.code)){

            str += неверный код результата для команды + command.cmd + для + hostIP +. Ожидалось + command.code +, а в ответе + cmd_exit_status
            влог- это же + подробнее для этого хоста можно посмотреть в show errHosts
            str += \n\n + ss.str();
            host.log = str;
            записать хост в таблицу errHosts
            throw неверный код результата для команды
    }





проверить ответ на равенство ожидаемому


if (command.expect != ""){
    if( !(regex(command.expect,ss.str())) ){ //может другая очерёдность
    // если не (регулярки совпадают) то выкинуть ошибку

                str += неожиданный output команды + command.cmd + для + hostIP +. Ожидалось \n\n+ command.expect +\n\n а в ответе\n\n + ss.str()
                host.log = str;
                влог- неожиданный output команды + command.cmd + для + hostIP +. подробнее для этого хоста можно посмотреть в show errHosts
                записать хост в таблицу errHosts
                throw неожиданный output команды
    }
}









успешно выполнена команда command.cmd для хоста + hostIP
    влог 
    host.log

    } //for(COMMANDS &command : currentDoCommands)+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-
str += ss.str();





 } // void inputOutputSSHdoCommands(&host,&modelCMD)





*/


                        /* Some environment variables may be set,
                            * It's up to the server which ones it'll allow though
                            */ 
                            libssh2_channel_setenv(channel, "FOO", "bar");

libssh2_channel_send_eof(channel);
    libssh2_channel_wait_eof(channel);
    libssh2_channel_wait_closed(channel);

shutdown:
    if (channel) {
        libssh2_channel_free(channel); // есть
        channel = NULL;
    }
    if (session) {
        libssh2_session_disconnect(session, "Normal Shutdown"); // есть
        libssh2_session_free(session); // есть
    }
    close(sock); // само
    libssh2_exit(); // не динам


     libssh2_init(0);
libssh2_exit();
libssh2_thread_init();
 libssh2_thread_cleanup();














#include <libssh2.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream> // не проверено ответ от гпт

int main() {
    const char* hostname = "example.com";
    const char* username = "user";
    const char* password = "password";
    const int port = 22;
    
    int rc;
    int sock;
    struct sockaddr_in sin;
    LIBSSH2_SESSION *session = NULL;
    LIBSSH2_CHANNEL *channel = NULL;

    // Инициализация libssh2
    rc = libssh2_init(0);
    if (rc != 0) {
        std::cerr << "libssh2 initialization failed (" << rc << ")\n";
        return 1;
    }

    // Создание сокета int rc = libssh2_session_startup(session, socket.native_handle());
    sock = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = inet_addr(hostname);
    if (connect(sock, (struct sockaddr*)(&sin), sizeof(sin)) != 0) {
        std::cerr << "Failed to connect!\n";
        return 1;
    }

    // Создание сессии SSH
    session = libssh2_session_init();
    if (!session) {
        std::cerr << "Failed to create SSH session!\n";
        return 1;
    }

    // Подключение к SSH серверу
    rc = libssh2_session_handshake(session, sock);
    if (rc) {
        std::cerr << "Failure establishing SSH session: " << rc << "\n";
        return 1;
    }

    // Авторизация
    if (libssh2_userauth_password(session, username, password)) {
        std::cerr << "Authentication by password failed.\ int rc = libssh2_session_startup(session, socket.native_handle());n";
        goto shutdown;
    }

    // Открытие канала
    channel = libssh2_channel_open_session(session);
    if (!channel) {
        std::cerr << "Failed to open a channel!\n";
        goto shutdown;
    }

    // Выполнение команды
    const char* command = "uptime";
    rc = libssh2_channel_exec(channel, command);
    if (rc != 0) {
        std::cerr << "Failed to execute command!\n";
        goto shutdown;
    }

    // Чтение ответа
    char buffer[1024];
    int bytes_read;
    do {
        bytes_read = libssh2_channel_read(channel, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            std::cout.write(buffer, bytes_read);
        }
    } while (bytes_read > 0);

    // Завершение
    libssh2_channel_send_eof(channel);
    libssh2_channel_wait_eof(channel);
    libssh2_channel_wait_closed(channel);

shutdown:
    if (channel) {
        libssh2_channel_free(channel);
        channel = NULL;
    }
    if (session) {
        libssh2_session_disconnect(session, "Normal Shutdown");
        libssh2_session_free(session);
    }
    close(sock);
    libssh2_exit();

    return 0;
}