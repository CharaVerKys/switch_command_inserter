#include <main.hpp>

ActiveHOSTS rootDoCommandsScan();
void rootScan(int argc, char const *argv[])
{
    // если что от лишник объявленных указателей и переменных размер сильно не увеличется

    plog->writeLog("Запущено с глаголом scan");
    ActiveHOSTS S_activeHosts = rootDoCommandsScan(); // вот тут если что основное выполнение
    std::cout << "\n";
    if (argc > 2 && std::string(argv[2]) == "identify")
    {
        plog->writeLog("Запущена идентификация как scan + identify");
        // выполнить функцию идентификации
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
        // выполнить функцию идентификации
        plog->writeLog("Программа завершила работу");
        exit(0);
    }

    // вот тут и выше если будем делать телнет нужно добавить логику для телнета
    // вообще нужно будет если телнет то всё что обращается как ssh просто задублировать для телнета
    // гланое не забыть сделать методы для телнета на show, после того как продублирую остальное

    if (sqlite->isTableExist(TableNameForSSH)) // удаляю непосредственно перед использованием
    {
        sqlite->emptyOut(TableNameForSSH);
    }
    sqlite->write_to_database(TableNameForSSH, S_activeHosts.ssh);
    std::cout << "\n Полученные хосты записаны. Закончите список логиннов и запустите программу \"swcmdins identify\"\n";
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
//                                                  commit
//
void emptyOutTables();
void rootCommandsCommit(asio::io_context &io_context,
                        std::vector<HOST> &validForCommitHosts,
                        std::vector<std::pair<std::string, std::vector<COMMANDS>>> models_and_commands,
                        std::vector<std::shared_ptr<SSHSession>> &sessions);
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

    plog->writeLog("Запущен коммит");
    emptyOutTables(); // обнуляю таблицы перед записью в них
    std::vector<std::shared_ptr<SSHSession>> sessions;
    asio::io_context io_context;
    auto ForCommitHosts = sqlite->read_from_database(TableNameForSSH);
    SSHSession::filterHosts(ForCommitHosts);
    rootCommandsCommit(io_context, ForCommitHosts, configer->getModels_and_commands(), sessions);
    io_context.run();

    plog->writeLog("Записываются результаты в лог");
    if (sqlite->isTableExist(TableNameForGoodHosts))
    {
        auto goodHosts = sqlite->read_from_databaseCommit(TableNameForGoodHosts);
        for (HOST &host : goodHosts)
        {
            gHlog->writeLog(asio::ip::address_v4(host.address).to_string() + "\t" + host.login.name + "\n\t\t" + host.model+"\n"+host.log);
        }
    }
    if (sqlite->isTableExist(TableNameForErrorHosts))
    {
        auto errHosts = sqlite->read_from_databaseCommit(TableNameForErrorHosts);
        for (HOST &host : errHosts)
        {
            errHlog->writeLog(asio::ip::address_v4(host.address).to_string() + "\t" + host.login.name + "\n\t\t" + host.model+"\n"+host.log);
        }
    }
    if (sqlite->isTableExist(TableNameForProgErrorHosts))
    {
        auto perrHosts = sqlite->read_from_databaseCommit(TableNameForProgErrorHosts);
        errHlog->writeLog("\n\n\t\tДалее идут ошибки связанные с программными проблемами\n\n");
        for (HOST &host : perrHosts)
        {
            errHlog->writeLog(asio::ip::address_v4(host.address).to_string() + "\t" + host.login.name + "\n\t\t" + host.model+"\n"+host.log);
        }
    }

    plog->writeLog("Программа завершила работу");
    // само закроется нормально   exit(0); // более понятно чем return 0;
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

void rootCommandsCommit(asio::io_context &io_context,
                        std::vector<HOST> &validForCommitHosts,
                        std::vector<                  // вектор для отправки в обработчик
                            std::pair<                // пара модель и команды к ней
                                std::string,          // модель
                                std::vector<COMMANDS> // набор команд
                                >                     //                 жесть ваще вектор
                            >
                            models_and_commands,
                        std::vector<std::shared_ptr<SSHSession>> &sessions)
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
        sessions.emplace_back(std::make_shared<SSHSession>(io_context, host, currentDoCommands));
        sessions.back()->connect();
    }
} // CommandCommiter(io_context,ForCommitHosts,configer->getmodels)
// первое вне, второе должно быть инициализировано заранее, а третье передаётся по значению
