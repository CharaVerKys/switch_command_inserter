#include <main.hpp>


ActiveHOSTS rootDoCommandsScan();
void rootScan(int argc, char const *argv[]){

// если что от лишник объявленных указателей и переменных размер сильно не увеличется

plog->writeLog("Запущено с глаголом scan");
ActiveHOSTS S_activeHosts = rootDoCommandsScan();
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

if (sqlite->isTableExist(TableNameForSSH))
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