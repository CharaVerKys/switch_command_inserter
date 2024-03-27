#include <CWRconfigs.hpp>

Configer::Configer(const std::filesystem::path &executable_path) : executable_path(executable_path)
{

    // проверить существуют ли файлы, создать если не существуют через функции
    if (!std::filesystem::exists(executable_path / "configs" / "network.conf"))
    {
        if (!create_networkconf())
        {
            std::cerr << "Failed to create network.conf.\n";
            exit(EACCES);
        }
        plog->writeLog("Созданн пример network.conf");
    }
    if (!std::filesystem::exists(executable_path / "configs" / "perModel_doCommands.conf"))
    {
        if (!create_perModel_doCommandsconf())
        {
            std::cerr << "Failed to create perModel_doCommands.conf.\n";
            exit(EACCES);
        }
        plog->writeLog("Созданн пример perModel_doCommands.conf");
    }
    if (!std::filesystem::exists(executable_path / "configs" / "scriptIp.list"))
    {
        if (!create_scriptIpList())
        {
            std::cerr << "Failed to create scriptIp.list.\n";
            exit(EACCES);
        }
        plog->writeLog("Созданн пример scriptIp.list");
    }

    if (!read_scriptIpList())
    {
        std::cerr << "Failed to read scriptIp.list.\n";
        exit(ENOENT);
    }
    plog->writeLog("Считан конфиг scriptIp.list");

    // и теперь открыть их и считать
    if (!read_networkconf())
    {
        std::cerr << "Failed to read network.conf.\n";
        exit(ENOENT);
    }
    plog->writeLog("Считан конфиг network.conf");

    if (!read_perModel_doCommandsconf())
    {
        std::cerr << "Failed to read perModel_doCommands.conf.\n";
        exit(ENOENT);
    }
    plog->writeLog("Считан конфиг perModel_doCommands.conf");
}

// функции обработчики для конструктора

bool Configer::create_networkconf()
{
    _networkconf.open(executable_path / "configs" / "network.conf", std::ios::out);
    const char *example = R"(
{
    "network": "192.168.1.145/29",
    "logins": [
        {
            "login": "username1",
            "password": "password1"
        },
         {
            "login": "ignorehost может быть пустым",
            "password": "но он должен существовать"
        },
        {
            "login": "самый_обычный",
            "password": "джейсон_формат"
        }
    ],
    "ignorehosts" : [
        "192.168.1.146",
        "192.168.1.147",
        "192.168.1.148",
        "192.168.1.149",
        "192.168.1.150"       
    ]
}
    )";

    if (!_networkconf.is_open())
    {
        std::cerr << "Файл network.conf не открыт. \n Странная ошибка учитывая контекст." << std::endl;
        return false;
    }
    _networkconf << example;
    if (!_networkconf)
    {
        std::cerr << "Ошибка при записи в файл network.conf.\n Понятия не имею что вообще может пойти не так, не буду даже обрабатывать ошибку" << std::endl;
        _networkconf.close();
        return false;
    }
    _networkconf.close();
    if (_networkconf.is_open())
    {
        std::cerr << "Файл network.conf всё ещё открыт, хотя не должен. \n Крайне вероятно возникновение дополнительных ошибок!!" << std::endl;
    }

    return true;
}

bool Configer::create_perModel_doCommandsconf()
{
    _perModel_doCommandsconf.open(executable_path / "configs" / "perModel_doCommands.conf", std::ios::out);
    const char *example = R"(
{"root_MC":[
        {
        "model" : "cisco\\.\\d{1,2}\\/",
          "commandsForThisModel" : [
                            {
                                "cmd" : "su",
                                "expect" : "^Password:",
                                "code" : 0

                            },
                             {
                                "cmd" : "myPassword",
                                "expect" : "",
                                "code" : 0

                            },
                             {
                                "cmd" : "systemctl restart apache2",
                                "expect" : "",
                                "code" : 0

                            }
            ]
        },

   
        {
        "model" : "tplink\\s?\\.model.*",
           "commandsForThisModel" : [
                            {
                                "cmd" : "conf t",
                                "expect" : "",
                                "code" : 0

                            },
                             {
                                "cmd" : "no logging console",
                                "expect" : "(вроде не выдаёт ответ, поэтому тут тоже оставить пустое, пустое означает не проверять) ",
                                "code" : 0

                            }
            ]
        },
   
        {
        "model" : "это регулярки, передаются как строки, нужно экранировать знаки (\"\") ковычек и слеш (\\) ",
           "commandsForThisModel" : [
                            {
                                "cmd" : " если не экранировать, поведение будет неожидаемое (вылетит на парсинге) ",
                                "expect" : "ожидание тоже регулярки",
                                "code" : 0

                            }
            ]
        },

   
        {
        "model" : "tplinkO?\\.modl.*",
           "commandsForThisModel" : [
                            {
                                "cmd" : "для того чтобы не сбиться, не запутаться при состовлении конфига",
                                "expect" : "просто соблюдайте этот паттерн и дальше",
                                "code" : 0

                            },
                             {
                                "cmd" : "копировать от точки, до закрывающей скобки",
                                "expect" : "новую модель так же, главное скопировать ту скобку, которая относится именно к этому объекту",
                                "code" : 0

                            }
            ]
        }
   
]}

    )";

    if (!_perModel_doCommandsconf.is_open())
    {
        std::cerr << "Файл perModel_doCommands.conf не открыт. \n Странная ошибка учитывая контекст." << std::endl;
        return false;
    }
    _perModel_doCommandsconf << example;
    if (!_perModel_doCommandsconf)
    {
        std::cerr << "Ошибка при записи в файл perModel_doCommands.conf.\n Понятия не имею что вообще может пойти не так, не буду даже обрабатывать ошибку" << std::endl;
        return false;
    }
    _perModel_doCommandsconf.close();
    if (_perModel_doCommandsconf.is_open())
    {
        std::cerr << "Файл perModel_doCommands.conf всё ещё открыт, хотя не должен. \n Крайне вероятно возникновение дополнительных ошибок!!" << std::endl;
    }

    return true;
}

bool Configer::create_scriptIpList()
{
    _scriptIpList.open(executable_path / "configs" / "scriptIp.list", std::ios::out);

    const char *example = R"(192.168.8.1;192.168.8.2;
10.90.90.90;
10.90.90.100;172.16.0.1;172.16.0.2;192.168.1.1;
192.168.1.2;
10.0.0.1;10.0.0.2;

192.168.0.1;192.168.0.2;
172.31.0.1;
172.31.0.2;10.1.1.1;

10.1.1.2;192.168.2.1;192.168.2.2;
172.17.0.1;172.17.0.2;)";

    if (!_scriptIpList.is_open())
    {
        std::cerr << "Файл scriptIp.list не открыт. \n Странная ошибка учитывая контекст." << std::endl;
        return false;
    }
    _scriptIpList << example;
    if (!_scriptIpList)
    {
        std::cerr << "Ошибка при записи в файл scriptIp.list.\n Понятия не имею что вообще может пойти не так, не буду даже обрабатывать ошибку" << std::endl;
        return false;
    }
    _scriptIpList.close();
    if (_scriptIpList.is_open())
    {
        std::cerr << "Файл scriptIp.list всё ещё открыт, хотя не должен. \n Крайне вероятно возникновение дополнительных ошибок!!" << std::endl;
    }

    return true;
}

bool Configer::read_scriptIpList()
{

    plog->writeLog("Считывается конфиг scriptIp.list");

    _scriptIpList.open(executable_path / "configs" / "scriptIp.list", std::ios::in);
    if (!_scriptIpList.is_open())
    {
        std::cerr << "Failed to open file scriptIp.list." << std::endl;
        return false;
    }

    auto parse = [](std::string line)
    {
        std::vector<std::string> ips;
        std::string ip;
        std::istringstream iss(line);
        while (std::getline(iss, ip, ';'))
        {
            ips.push_back(ip);
        }
        return ips;
    };

    std::string line;
    while (std::getline(_scriptIpList, line))
    {

        std::vector<std::string> ips = parse(line);

        for (const auto &ip : ips)
        {
            if (ip.size() > 0 && ip.find_first_not_of("0123456789.") == std::string::npos)
            {
                _sIpList.push_back(ip);
            }
            else
            {
                std::cerr << "Неправильное IP: " << ip << std::endl;
            }
        }
    }
    return true;
}

bool Configer::read_networkconf()
{
    plog->writeLog("Считывается конфиг network.conf");
    // открываю сам файл
    _networkconf.open(executable_path / "configs" / "network.conf", std::ios::in);
    if (!_networkconf.is_open())
    {
        std::cerr << "Failed to open config file network.conf." << std::endl;
        return false;
    }
    // получаю строку джейсон
    std::string jsonStr;
    std::string line;
    while (std::getline(_networkconf, line))
    {
        jsonStr += line;
    }
    _networkconf.close();

    // пропарсинг строки для дальнейшего извлечения данных
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());

    if (doc.HasParseError())
    {
        std::cerr << "Error parsing JSON." << std::endl;
        return false;
    }

    // начинаю получать сами данные
    if (doc.HasMember("network") && doc["network"].IsString())
    {
        _IpMask = doc["network"].GetString();
    }
    else
    {
        std::cerr << "Missing or invalid 'network' field in JSON." << std::endl;
        return false;
    }

    // проверяю по регулярке айпи
    std::regex ipRegex("^\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}(\\/\\d{1,2})?$");

    if (!std::regex_match(_IpMask, ipRegex))
    {
        return false;
    }
    plog->writeLog("Адрес соответствует регулярке, но регулярка просто проверяет что знаки - числа.");

    // долбанутое разложение массива, и это ещё не массив команд
    if (doc.HasMember("logins") && doc["logins"].IsArray())
    {
        const rapidjson::Value &logins = doc["logins"];
        for (rapidjson::SizeType i = 0; i < logins.Size(); ++i)
        {

            if (logins[i].HasMember("login") && logins[i].HasMember("password") && logins[i]["login"].IsString() && logins[i]["password"].IsString())
            {
                _Logins_Passwords.emplace_back(logins[i]["login"].GetString(), logins[i]["password"].GetString());
            }
            else
            {
                std::cerr << "Invalid login/password format in JSON." << std::endl;
                return false;
            }
        }
    }
    else
    {
        std::cerr << "Missing or invalid 'logins' field in JSON." << std::endl;
        return false;
    }

    if (!doc.HasMember("ignorehosts") || !doc["ignorehosts"].IsArray())
    {
        std::cerr << "Missing or invalid 'ignorehosts' field in JSON." << std::endl;
        return false;
    }

    for (rapidjson::SizeType i = 0; i < doc["ignorehosts"].Size(); ++i)
    {
        _ignoreHosts.emplace_back(doc["ignorehosts"][i].GetString());
    }

    if (_networkconf.is_open())
    {
        std::cerr << "Файл network.conf всё ещё открыт, хотя не должен." << std::endl;
    }

    return true;
}

bool Configer::read_perModel_doCommandsconf()
{
    plog->writeLog("Считывается конфиг perModel_doCommands.conf");
    // открываю сам файл
    _perModel_doCommandsconf.open(executable_path / "configs" / "perModel_doCommands.conf", std::ios::in);
    if (!_perModel_doCommandsconf.is_open())
    {
        std::cerr << "Failed to open config file perModel_doCommands.conf." << std::endl;
        return false;
    }
    // получаю строку джейсон
    std::string jsonStr;
    std::string line;
    while (std::getline(_perModel_doCommandsconf, line))
    {
        jsonStr += line;
    }
    _perModel_doCommandsconf.close();
    // пропарсинг строки для дальнейшего извлечения данных
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());

    if (doc.HasParseError())
    {
        std::cerr << "Error parsing JSON." << std::endl;
        return false;
    }

    // начинаю получать сами данные

    // doc["root_MC"] = корень, то есть через это я обращаюсь к...
    // _models_and_commands
    const rapidjson::Value &root = doc["root_MC"];

    if (!root.IsArray())
    {
        std::cerr << "Invalid input format: models should be an array.\n Не знаю даже когда может возникнуть, генерируемый конфиг это точно описывает" << std::endl;
        return false;
    }

    for (rapidjson::SizeType i = 0; i < root.Size(); ++i)
    {
        const rapidjson::Value &elementByRoot_i = root[i];
        if (!elementByRoot_i.HasMember("model") || !elementByRoot_i.HasMember("commandsForThisModel") || !elementByRoot_i["model"].IsString() || !elementByRoot_i["commandsForThisModel"].IsArray())
        {
            std::cerr << "Invalid model format in JSON. \n Что-то не так с регуляркой в элементе №" << ++i << std::endl;
            return false;
        }

        std::pair<std::string, std::vector<COMMANDS>> Model_and_Commands_i;

        Model_and_Commands_i.first = elementByRoot_i["model"].GetString();

        const rapidjson::Value &allcommands = elementByRoot_i["commandsForThisModel"];
        for (rapidjson::SizeType j = 0; j < allcommands.Size(); ++j)
        {

            const rapidjson::Value &command = allcommands[j];
            if (!command.HasMember("cmd") || !command.HasMember("expect") || !command.HasMember("code") || !command["cmd"].IsString() || !command["expect"].IsString() || !command["code"].IsInt())
            {
                std::cerr << "Invalid command format in JSON. \n Проблема с каким то объектом команды в объекте №" << ++i << " команда №" << ++j << std::endl;
                return false;
            }
            COMMANDS commands;

            commands.cmd = command["cmd"].GetString();
            commands.expect = command["expect"].GetString();
            commands.code = command["code"].GetInt();

            Model_and_Commands_i.second.push_back(commands);
        }

        _models_and_commands.push_back(Model_and_Commands_i);
    }

    if (_perModel_doCommandsconf.is_open())
    {
        std::cerr << "Файл perModel_doCommands.conf всё ещё открыт, хотя не должен." << std::endl;
    }
    return true;
}

//  функции ниже
//  просто возвращают значения константными,
//  ничего больше не делают

const std::string &Configer::getIpMask() const
{
    return _IpMask;
}

const std::vector<std::pair<std::string, std::string>> &Configer::getLogins_Passwords() const
{
    return _Logins_Passwords;
}

const std::vector<            // вектор для отправки в обработчик
    std::pair<                // пара модель и команды к ней
        std::string,          // модель
        std::vector<COMMANDS> // набор команд
        >                     //                 жесть ваще вектор
    > &
Configer::getModels_and_commands() const
{
    return _models_and_commands;
}

const std::vector<std::string> &Configer::getScriptIpList(){
   return _sIpList;
}

const std::vector<std::string> &Configer::getIgnoringHosts()
{
    return _ignoreHosts;
}

// а ври эти вот делают

bool Configer::updateNetwork_conf(std::vector<HOST> &goodHosts)
{

    plog->writeLog("Начинаю изменять конфиг network.conf");
    // открываю сам файл
    _networkconf.open(executable_path / "configs" / "network.conf", std::ios::out | std::ios::trunc);
    if (!_networkconf.is_open())
    {
        std::cerr << "Failed to open config file network.conf. (for modify)" << std::endl;
        return false;
    }
    // Создание JSON объекта
    rapidjson::Document doc;
    doc.SetObject();

    rapidjson::Value network(_IpMask.c_str(), doc.GetAllocator());
    doc.AddMember("network", network, doc.GetAllocator());

    rapidjson::Value logins(rapidjson::kArrayType);
    for (const auto &login_password : _Logins_Passwords)
    {
        rapidjson::Value login_object(rapidjson::kObjectType);
        rapidjson::Value login(login_password.first.c_str(), doc.GetAllocator());
        rapidjson::Value password(login_password.second.c_str(), doc.GetAllocator());
        login_object.AddMember("login", login, doc.GetAllocator());
        login_object.AddMember("password", password, doc.GetAllocator());
        logins.PushBack(login_object, doc.GetAllocator());
    }
    doc.AddMember("logins", logins, doc.GetAllocator());

    // начинаю заполнять хосты
    rapidjson::Value ignorehosts(rapidjson::kArrayType);

    for (const auto &ignoreHost : _ignoreHosts)
    {
        rapidjson::Value host(ignoreHost.c_str(), doc.GetAllocator());
        ignorehosts.PushBack(host, doc.GetAllocator());
    }

    // добавляю хосты из goodHosts
    for (const auto &goodHost : goodHosts)
    {
        const char *host_str = asio::ip::address_v4(goodHost.address).to_string().c_str();
        rapidjson::Value host(host_str, doc.GetAllocator());
        ignorehosts.PushBack(host, doc.GetAllocator());
    }

    doc.AddMember("ignorehosts", ignorehosts, doc.GetAllocator());

    // Запись JSON объекта в файл

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> prettyWriter(buffer);
    doc.Accept(prettyWriter);

    _networkconf << buffer.GetString() << std::endl;
    _networkconf.close();

    if (_networkconf.is_open())
    {
        std::cerr << "Файл network.conf всё ещё открыт, хотя не должен. (for modify)" << std::endl;
    }
    plog->writeLog("Изменил конфиг network.conf");
    return true;
}

std::unique_ptr<Configer> configer;
