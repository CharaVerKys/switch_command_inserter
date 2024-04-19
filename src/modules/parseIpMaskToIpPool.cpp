#include <main.hpp>

// просто приводит айпи формат в бинарный вид
uint32_t ipToBin(const std::string &ip)
{
    unsigned int a, b, c, d;
    char dot;
    std::istringstream(ip) >> a >> dot >> b >> dot >> c >> dot >> d;
    return (a << 24) | (b << 16) | (c << 8) | d;
}
// локальная для модуля функция
uint32_t maskToBin(uint8_t prefixLength)
{
    return 0xffffffff << (32 - prefixLength);
}

SNparsedNetworkHost parseIpMaskToIpPool(std::string ipAndMask)
{
    SNparsedNetworkHost result;
    size_t slashIndex = ipAndMask.find('/');
    std::string ip_address_NotCompl;
    uint8_t prefixLength;

    if (slashIndex != std::string::npos) // проверка на то указана ли маска
    {
        ip_address_NotCompl = ipAndMask.substr(0, slashIndex);
        prefixLength = std::stoi(ipAndMask.substr(slashIndex + 1));
        // соответственно если указана делю
    }
    else
    {
        plog->writeLog("Маска не указана в конфигурации. Пример правильной конфигурации: X.X.X.X/MM");
        ip_address_NotCompl = ipAndMask;
        prefixLength = 32; // как хост
    }
    

    uint32_t ip_address = ipToBin(ip_address_NotCompl);
    uint32_t ip_mask = maskToBin(prefixLength);
    result.network = ip_address & ip_mask;
    result.hosts = ~ip_mask;
    plog->writeLog("Прошёл парсинг сеть/маска");
    return result;
}
