#include <main.hpp>


std::vector<HOST> SNinitHostsVector(SNparsedNetworkHost &IpPool)
{
    plog->writeLog("Инициализируется сырой массив хостов");

    std::vector<HOST> hosts;
    if (IpPool.hosts == 0)
    { // обработка когда маска 32
        hosts.emplace_back(HOST{.address = IpPool.network});
        plog->writeLog("Отработано как хост (/32)");
    }
    else
    {
        for (uint32_t i = 1; i < IpPool.hosts; i++)
        { // обрабатывает все прочие маски,кроме маски 31 (нулевой результат)
            hosts.emplace_back(HOST{.address = IpPool.network + i});
        }

        // мне нужно из массива хостов удалить все хосты, где айпи совпадает с айпи игнор

        std::vector<std::string> ignoreHost = configer->getIgnoringHosts();
        std::unordered_set<uint32_t> allignoreIP;
        for (std::string &ighost : ignoreHost)
        {
            allignoreIP.emplace(ipToBin(ighost));
        }
        hosts.erase(std::remove_if(
                        hosts.begin(), hosts.end(),
                        [&allignoreIP](const HOST &host)
                        {
                            return allignoreIP.find(host.address) != allignoreIP.end();
                        }),

                    hosts.end());

        // реализации в целом одинаковую задачу выполняют, но кажется первая быстрее
        /* for (auto it = hosts.begin(); it != hosts.end();)
      {
          if (allignoreIP.find(it->address) != allignoreIP.end())
          {
              it = hosts.erase(it);
          }
          else
          {
              ++it;
          }
      }   */

        plog->writeLog("Отработано как сеть, с учётом игнорируемых хостов");
    }
    plog->writeLog("Инициализирован сырой массив хостов");
    return std::move(hosts);
}

void SNcheck_port_async(HOST &host, asio::io_context &io_context, HOST::PORT &port, std::vector<HOST> &validHosts)
{
    auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
    asio::ip::tcp::endpoint endpoint(asio::ip::address_v4(host.address), port.number);

auto timer = std::make_shared<asio::steady_timer>(io_context);
    timer->expires_after(std::chrono::seconds(21)); 
    // выполнение таймера
    timer->async_wait([socket,timer](const asio::error_code &er) {
        if (!er) {
            // если таймер сработал, дроп подключения
            if (socket->is_open()) {
                socket->close();
            }
        }else{
            if (er != asio::error::operation_aborted) {
            plog->writeLog("ошибка таймера для функции одного вектора: "+er.message());
        }}
    });




    try
    {
        socket->async_connect(endpoint, [&host, &port, socket, &validHosts, timer](const asio::error_code &error)
                              {

                            
        if (!error) {
             timer->cancel();
            port.establish=true;
            validHosts.emplace_back(host);
        } });
    }
    catch (std::exception &e)
    {
        plog->writeLog("Ошибка при создании подключения к порту " + std::to_string(port.number) + " в SNcheck_port_async одного вектора: " + std::string(e.what()));
    }
}

void SNcheck_port_async(HOST &host, asio::io_context &io_context, HOST::PORT &port, std::vector<HOST> &validHosts, std::vector<HOST> &refusedHosts)

{

    auto socket = std::make_shared<asio::ip::tcp::socket>(io_context);
    asio::ip::tcp::endpoint endpoint(asio::ip::address_v4(host.address), port.number);


auto timer = std::make_shared<asio::steady_timer>(io_context);
    timer->expires_after(std::chrono::seconds(21)); 
    // выполнение таймера
    timer->async_wait([socket,timer](const asio::error_code &er) {
        if (!er) {
            // если таймер сработал, дроп подключения
            if (socket->is_open()) {
                socket->close();
            }
        }else{if (er != asio::error::operation_aborted) {
            plog->writeLog("ошибка таймера для функции двух векторов: "+er.message());
        }}
    });


    try
    {
        socket->async_connect(endpoint, [&host, &port, socket, &validHosts, &refusedHosts, timer](const asio::error_code &error)
                              {
        if (!error) {
                             timer->cancel();
            port.establish=true;
            validHosts.emplace_back(host);
        } else {
            
            refusedHosts.emplace_back(host);
        } });
    }
    catch (std::exception &e)
    {
        plog->writeLog("Ошибка при создании подключения к порту " + std::to_string(port.number) + " в SNcheck_port_async двух векторов: " + std::string(e.what()));
    }
}

ActiveHOSTS ScanNetwork(std::vector<HOST> &hosts)
{
    ActiveHOSTS activeHosts;
    plog->writeLog("Начата проверка портов");
    std::vector<HOST> NotValidHostsSSH; // выкидывает в ssh функции для обработки телнет

    asio::io_context io_contextSSH; // контекст для ссш
    for (auto &host : hosts)
    {

        SNcheck_port_async(host, io_contextSSH, host.ssh, activeHosts.ssh, NotValidHostsSSH);
        // получаю 2 массива, активные выдаю, неактивные на дальнейшую обработку
    }
    io_contextSSH.run();
    plog->writeLog("Проверка портов SSH пройдена");

    asio::io_context io_contextTELNET; // контекст для телнета
    for (auto &host : NotValidHostsSSH)
    {

        SNcheck_port_async(host, io_contextTELNET, host.telnet, activeHosts.onlyTelnet);
        /*
    для каждого хоста из валидных телнет, проверить нет ли соответствия с хостом инвалидных ssh
    так как я проверяю только инвалидные ssh это уже сделано
*/
    }
    io_contextTELNET.run();
    plog->writeLog("Проверка портов telnet пройдена");

    for (auto &host : activeHosts.ssh)
    {
        wlog->writeLog("Найден хост с активным портом SSH(22) " + asio::ip::address_v4(host.address).to_string());
    }
    for (auto &host : activeHosts.onlyTelnet)
    {
        wlog->writeLog("Найден хост с активным портом telnet(23) " + asio::ip::address_v4(host.address).to_string());
    }
    std::cout << activeHosts.ssh.size() << " хостов с открытым SSH(22)" << std::endl;
    std::cout << activeHosts.onlyTelnet.size() << " хостов где открыт только telnet(23) " << std::endl;
    std::cout << "Подробнее смотреть в logs/log#/working.log" << std::endl;
    std::cout << "Telnet рекомендуется обработать вручную, текущая версия не обрабатывает telnet. Иначе производится неполное изменение настроек сети" << std::endl;
    plog->writeLog("Записан лог найденных хостов");
    return std::move(activeHosts);
}