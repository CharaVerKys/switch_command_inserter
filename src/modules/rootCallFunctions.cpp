#include <main.hpp>

// модуль вообще разросся, стань он ещё немного больше я бы начал его растаскивать по отдельным cpp/hpp

// внутренние функции модуля
void areYouAgreeq();
void commit();
void emptyOutTables();

template <typename T>
void rootCommandsCommit(asio::io_context &io_context,
                        std::vector<HOST> &validForCommitHosts,
                        std::vector<std::pair<std::string, std::vector<COMMANDS>>> models_and_commands,
                        std::vector<std::shared_ptr<T>> &sessions);
ActiveHOSTS rootDoCommandsScan();
void rootDoCommandsIdentify(ActiveHOSTS &I_activeHosts);

//

void rootScript(int argc, char const *argv[])
{

    plog->writeLog("Запущено с глаголом script");

    std::string login;
    std::string password;
    std::cout << "Введите логин: ";
    std::getline(std::cin, login);
    std::cout << "Введите пароль: ";
    std::getline(std::cin, password);

    std::vector<HOST> hosts;
    auto ipList = configer->getScriptIpList();
    for (auto &ip : ipList)
    {
        // заполняю просто пустышкой
        HOST host;
        host.address = ipToBin(ip);
        host.login.name = login;
        host.login.password = password;
        host.model = "script"; // ключевое слово для этого набора
        hosts.push_back(host);
    }
    plog->writeLog("Инициализирован вектор хостов (script)");

    if (sqlite->isTableExist(TableNameForSSH)) // удаляю непосредственно перед использованием
    {
        sqlite->emptyOut(TableNameForSSH);
    }
    sqlite->write_to_database(TableNameForSSH, hosts); // записал полученный список с пустышками

    plog->writeLog("Данные записаны");

    if (!(argc > 2 && std::string(argv[2]) == "-y"))
    {
        std::cout << "Данные записаны, продолжить (commit)?  (Yes/no)" << std::endl;
        std::string S_answer;
        std::getline(std::cin, S_answer);

        while (S_answer == "yes" || S_answer == "YES")
        {
            std::cout << "Пожалуйста ответьте в нужном регистре ( Yes )\n";
            std::getline(std::cin, S_answer);
        }

        if (S_answer == "Yes")
        {
            plog->writeLog("Запущен commit из script");
            areYouAgreeq();
            commit();
            plog->writeLog("Программа завершила работу");
            exit(0);
        }
        else
        {
            std::cout << "Отменено пользоваталем" << std::endl;
            plog->writeLog("Отменено пользоваталем");
            plog->writeLog("Программа завершила работу");
            exit(0); // более понятно чем return 0;
        }
    }
    //
    else
    // если параметр -y
    {
        plog->writeLog("Запущено с -y");
        commit();

        plog->writeLog("Программа завершила работу");
        exit(0);
    }
}

void rootScriptTELNET(int argc, char const *argv[])
{

    plog->writeLog("Запущено с глаголом script");

    std::string login;
    std::string password;
    std::cout << "Введите логин: ";
    std::getline(std::cin, login);
    std::cout << "Введите пароль: ";
    std::getline(std::cin, password);

    std::vector<HOST> hosts;
    auto ipList = configer->getScriptIpList();
    for (auto &ip : ipList)
    {
        HOST host;
        host.address = ipToBin(ip);
        host.login.name = login;
        host.login.password = password;
        host.model = "script";
        hosts.push_back(host);
    }
    plog->writeLog("Инициализирован вектор хостов (scriptTelnet)");

    if (sqlite->isTableExist(TableNameForTELNET)) // удаляю непосредственно перед использованием
    {
        sqlite->emptyOut(TableNameForTELNET);
    }
    sqlite->write_to_database(TableNameForTELNET, hosts);

    plog->writeLog("Данные записаны");

    if (!(argc > 2 && std::string(argv[2]) == "-y"))
    {
        std::cout << "Данные записаны, продолжить (commit)?  (Yes/no)" << std::endl;
        std::string S_answer;
        std::getline(std::cin, S_answer);

        while (S_answer == "yes" || S_answer == "YES")
        {
            std::cout << "Пожалуйста ответьте в нужном регистре ( Yes )\n";
            std::getline(std::cin, S_answer);
        }

        if (S_answer == "Yes")
        {
            plog->writeLog("Запущен commit из script");
            areYouAgreeq();
            commit();
            plog->writeLog("Программа завершила работу");
            exit(0);
        }
        else
        {
            std::cout << "Отменено пользоваталем" << std::endl;
            plog->writeLog("Отменено пользоваталем");
            plog->writeLog("Программа завершила работу");
            exit(0); // более понятно чем return 0;
        }
    }
    //
    else
    // если параметр -y
    {
        plog->writeLog("Запущено с -y");
        commit();

        plog->writeLog("Программа завершила работу");
        exit(0);
    }
}

void rootScan(int argc, char const *argv[])
{
    plog->writeLog("Запущено с глаголом scan");
    ActiveHOSTS S_activeHosts = rootDoCommandsScan(); // вот тут если что основное выполнение
    std::cout << "\n";
    if (argc > 2 && std::string(argv[2]) == "identify")
    {
        plog->writeLog("Запущена идентификация как scan + identify");
        rootDoCommandsIdentify(S_activeHosts);
        plog->writeLog("Программа завершила работу");
        exit(0); // более понятно чем return 0;
    }
    std::cout << "Хотите начать идентификацию? (Требуется заполненный список логинов) (Yes/no)\n";
    std::string S_answer;
    std::getline(std::cin, S_answer);

    while (S_answer == "yes" || S_answer == "YES")
    {
        std::cout << "Пожалуйста ответьте в нужном регистре ( Yes )\n";
        std::getline(std::cin, S_answer);
    }

    if (S_answer == "Yes")
    {
        plog->writeLog("Запущена идентификация из scan");
        rootDoCommandsIdentify(S_activeHosts);
        plog->writeLog("Программа завершила работу");
        exit(0);
    }

    if (sqlite->isTableExist(TableNameForSSH)) // удаляю непосредственно перед использованием
    {
        sqlite->emptyOut(TableNameForSSH);
    }
    sqlite->write_to_database(TableNameForSSH, S_activeHosts.ssh);

    if (sqlite->isTableExist(TableNameForTELNET)) // удаляю непосредственно перед использованием
    {
        sqlite->emptyOut(TableNameForTELNET);
    }
    sqlite->write_to_database(TableNameForTELNET, S_activeHosts.onlyTelnet);
    std::cout << "\n Полученные хосты записаны. Закончите список логиннов и поисковых команд, и запустите программу \"swcmdins identify\"\n";
    plog->writeLog("Программа завершила работу");
    exit(0);
}

ActiveHOSTS rootDoCommandsScan()
{

    SNparsedNetworkHost IpPool = parseIpMaskToIpPool(configer->getIpMask());
    std::vector<HOST> hosts = SNinitHostsVector(IpPool); // все возможные хосты в сети
    ActiveHOSTS activeHosts = ScanNetwork(hosts);        // отдаю массив хостов, получаю 2 массива: ссш и онли телнет
    plog->writeLog("1/3 завершено: получены списки хостов с открытыми портами");
    return std::move(activeHosts);
}

//

void rootIdentify(int argc, char const *argv[])
{

    plog->writeLog("Запущено с глаголом identify");
    ActiveHOSTS I_activeHosts;
    if (sqlite->isTableExist(TableNameForSSH)) // если нет вообще телнета или ссш будет только 1 массив, а второй пустой
    {
        I_activeHosts.ssh = sqlite->read_from_database(TableNameForSSH);
    }
    if (sqlite->isTableExist(TableNameForTELNET))
    {
        I_activeHosts.onlyTelnet = sqlite->read_from_database(TableNameForTELNET);
    }

    rootDoCommandsIdentify(I_activeHosts);
    plog->writeLog("Программа завершила работу");
    exit(0);
}

void rootDoCommandsIdentify(ActiveHOSTS &I_activeHosts)
{
    std::cout << "Запущена идентификация моделей\n";

    std::vector<std::shared_ptr<IdentifySSH>> sessions; // массив указателей для сессий
    std::vector<std::shared_ptr<IdentifyTELNET>> sessionsT;

    asio::io_context io_context;

    auto finding_commands = configer->getFinding_commands(); // std::vector<std::pair<std::string, std::vector<COMMANDS>>> 
    auto logins = configer->getLogins_Passwords(); //std::vector<std::pair<std::string, std::string>>
    
    ActiveHOSTS identifinedHosts; // итоговый набор

// запускаю сам обработчик

    for (auto &host : I_activeHosts.ssh)
    {
        sessions.emplace_back(std::make_shared<IdentifySSH>(io_context, host, logins, finding_commands, identifinedHosts.ssh));
        sessions.back()->connect();
    } // for each

    for (auto &host : I_activeHosts.onlyTelnet)
    {
        sessionsT.emplace_back(std::make_shared<IdentifyTELNET>(io_context, host, logins, finding_commands, identifinedHosts.onlyTelnet));
        sessionsT.back()->connect();
    } // for each

    io_context.run();
    sessions.clear(); // отчистка воизбежание ошибок по памяти
    sessionsT.clear(); // не понятно как они возникают, но методом тыка нашёл решение

    if (sqlite->isTableExist(TableNameForSSH)) // удаляю непосредственно перед использованием
    {
        sqlite->emptyOut(TableNameForSSH);
    }
    sqlite->write_to_database(TableNameForSSH, identifinedHosts.ssh); // записываю из итогового

    if (sqlite->isTableExist(TableNameForTELNET)) // удаляю непосредственно перед использованием
    {
        sqlite->emptyOut(TableNameForTELNET);
    }
    sqlite->write_to_database(TableNameForTELNET, identifinedHosts.onlyTelnet);

    plog->writeLog("Записываются результаты в лог");

    if (sqlite->isTableExist(TableNameForIdentify)) 
    {
        idelog->writeLog("Обращаю внимание что лог отдельного хоста может быть довольно большой, рекомендую загрепать файл по ключевому слову keyword для получения краткого списка где удачно а где нет");
        std::vector<HOST> identifyLogable = sqlite->read_from_databaseCommit(TableNameForIdentify);
        IdentifySSH::filter_to_log_resulting_vector_from_database(identifyLogable);
        for (HOST &host : identifyLogable)
        {
            idelog->writeLog("\n----------------------------------------------------------------------------\n" + asio::ip::address_v4(host.address).to_string() + "\t\tЛогин:" + host.login.name + "\t\t\tkeyword\n\t\tМодель: " + host.model + "\t\t\tkeyword\n_______________\nЛог:\n" + host.log);
        }
    }
}

//
//                                                  commit
//

void rootCommit(int argc, char const *argv[])
{
    plog->writeLog("Запущено с глаголом commit");
    std::cout << "\n";

    if (argc > 2 && std::string(argv[2]) == "-y")
    {
        plog->writeLog("Запущено с -y");
    }
    else
    {
        areYouAgreeq();
    }
    commit();

    plog->writeLog("Программа завершила работу");
    exit(0); // более понятно чем return 0;
}

void areYouAgreeq() // если согласие то просто пропускает код дальше, иначе закрытие программы
{
    std::cout << "\nЭта программа лишь вводит команды и проверяет ответ, она не оснащена средствами отмены действий или что-то типо того."
              << "\nЗапуская применение команд вы отказываетесь от притензий к разработчику, в том случае если ошибка не связанна непосредственно с программным кодом."
              << "\nФайл goodHosts.log перед запуском нового набора команд нужно удалить ВРУЧНУЮ, он просто дозаписывает не проверяя."
              << "\nХотите начать применение команд? (Требуется выполненная идентификация) (Yes/no)\n";
    std::string C_answer;
    std::getline(std::cin, C_answer);

    while (C_answer == "yes" || C_answer == "YES")
    {
        std::cout << "Пожалуйста ответьте в нужном регистре ( Yes )\n";
        std::getline(std::cin, C_answer);
    }

    if (!(C_answer == "Yes"))
    {
        std::cout << "Отменено пользоваталем" << std::endl;
        plog->writeLog("Отменено пользоваталем");
        plog->writeLog("Программа завершила работу");
        exit(0); // более понятно чем return 0;
    }
}

void commit()
{
    plog->writeLog("Запущен коммит");
    std::vector<std::shared_ptr<SSHSession>> sessions; // массив с указателями...
    std::vector<std::shared_ptr<TELNETSession>> sessionsT;
    asio::io_context io_context;
    ActiveHOSTS activeHOSTS;

    if (sqlite->isTableExist(TableNameForSSH))
    {
        activeHOSTS.ssh = sqlite->read_from_database(TableNameForSSH);
    }

    if (sqlite->isTableExist(TableNameForTELNET))
    {
        activeHOSTS.onlyTelnet = sqlite->read_from_database(TableNameForTELNET);
    }

// для логирования делаю красиво, если сделано через script..
    uint16_t i = 0;
    if (!activeHOSTS.ssh.empty() && activeHOSTS.ssh[0].model == "script")
    {
        for (HOST &host : activeHOSTS.ssh)
        {
            host.number = ++i;
        }
    }
    if (!activeHOSTS.onlyTelnet.empty() && activeHOSTS.onlyTelnet[0].model == "script")
    {
        for (HOST &host : activeHOSTS.onlyTelnet)
        {
            host.number = ++i;
        }
    }

    SSHSession::filterHosts(activeHOSTS.ssh); // удаляю хосты где нет логина или модели
    TELNETSession::filterHosts(activeHOSTS.onlyTelnet);
    wlog->writeLog("\n\n\t\tНачался commit.\n");
    rootCommandsCommit(io_context, activeHOSTS.ssh, configer->getModels_and_commands(), sessions);
    rootCommandsCommit(io_context, activeHOSTS.onlyTelnet, configer->getModels_and_commands(), sessionsT);
    emptyOutTables(); // обнуляю таблицы перед записью в них
    io_context.run();
    sessions.clear(); // отчистка воизбежание ошибок по памяти
    sessionsT.clear(); // не понятно как они возникают, но методом тыка нашёл решение

    std::cout << "\n\n---------------------------------\n\n";
    for (const auto &entry : SSHSession::shortlog)
    {
        std::cout << entry.second << std::endl;
    }
    for (const auto &entry : TELNETSession::shortlog)
    {
        std::cout << entry.second << std::endl;
    }
    plog->writeLog("Записываются результаты в лог");

    if (sqlite->isTableExist(TableNameForGoodHosts))
    {
        auto goodHosts = sqlite->read_from_databaseCommit(TableNameForGoodHosts);
        for (HOST &host : goodHosts)
        {
            gHlog->writeLog("\n----------------------------------------------------------------------------\n" + asio::ip::address_v4(host.address).to_string() + "\t\tЛогин:" + host.login.name + "\n\t\tМодель: " + host.model + "\n_______________\nЛог:\n" + host.log);
        }
        // так же добавляю в игнор хостс
        configer->updateNetwork_conf(goodHosts);
    }
    if (sqlite->isTableExist(TableNameForErrorHosts))
    {
        auto errHosts = sqlite->read_from_databaseCommit(TableNameForErrorHosts);
        for (HOST &host : errHosts)
        {
            errHlog->writeLog("\n----------------------------------------------------------------------------\n" + asio::ip::address_v4(host.address).to_string() + "\t\tЛогин:" + host.login.name + "\n\t\tМодель: " + host.model + "\n_______________\nЛог:\n" + host.log);
        }
    }
    if (sqlite->isTableExist(TableNameForProgErrorHosts))
    {
        auto perrHosts = sqlite->read_from_databaseCommit(TableNameForProgErrorHosts);
        errHlog->writeLog("\n\n\t\tДалее идут ошибки связанные с программными проблемами\n\n");
        for (HOST &host : perrHosts)
        {
            errHlog->writeLog("\n----------------------------------------------------------------------------\n" + asio::ip::address_v4(host.address).to_string() + "\t\tЛогин:" + host.login.name + "\n\t\tМодель: " + host.model + "\n_______________\nЛог:\n" + host.log);
        }
    }
}

void emptyOutTables()
{
    if (sqlite->isTableExist(TableNameForGoodHosts)) // обнуляю таблицы перед записью в них
    {
        sqlite->emptyOut(TableNameForGoodHosts);
    }
    if (sqlite->isTableExist(TableNameForProgErrorHosts))
    {
        sqlite->emptyOut(TableNameForProgErrorHosts);
    }
    if (sqlite->isTableExist(TableNameForErrorHosts))
    {
        sqlite->emptyOut(TableNameForErrorHosts);
    }
}

template <typename T>
void rootCommandsCommit(asio::io_context &io_context,
                        std::vector<HOST> &validForCommitHosts,
                        std::vector<                  // вектор для отправки в обработчик
                            std::pair<                // пара модель и команды к ней
                                std::string,          // модель
                                std::vector<COMMANDS> // набор команд
                                >                     //                 жесть ваще вектор
                            >
                            models_and_commands,
                        std::vector<std::shared_ptr<T>> &sessions)
{
    for (auto &host : validForCommitHosts)
    {
        std::regex model_regex;
        std::vector<COMMANDS> currentDoCommands;
        // в каждом отдельном хосте модель будет полная а не регулсярка
        for (auto &pair : models_and_commands)
        {
            model_regex = std::regex(pair.first);
            if (std::regex_search(host.model, model_regex))
            {
                currentDoCommands = pair.second;
                break;
            }
        } // этот перебор для каждого отдельного может занять достаточно времени
          // то есть на каждый существующий конфиг я для каждого существующего хоста (в списке валидных) перебираю пока не найдётся, начиная с первого
          // то есть, если первая регулярка .* то все хосты будут выполняться к первой.

        // инициализировано для текущего хоста
        sessions.emplace_back(std::make_shared<T>(io_context, host, currentDoCommands));
        sessions.back()->connect();

    } // for each

} 