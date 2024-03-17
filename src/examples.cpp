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