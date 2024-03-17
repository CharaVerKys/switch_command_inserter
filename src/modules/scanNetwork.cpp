#include <main.hpp>
// мусорная функция, не используется
bool is_port_open(const uint32_t ip_address, int port)
{
    using asio::ip::tcp;
    static asio::io_context io_context;
    asio::ip::tcp::endpoint endpoint(asio::ip::address_v4(ip_address), port);
    tcp::socket socket(io_context);
    try
    {
        socket.connect(endpoint);
        return true;
    }
    catch (std::exception &e)
    {
        return false;
    }
}

// часть icmp
void calculate_checksumICMP(std::vector<unsigned char> &data)
{
    std::size_t length = data.size();
    uint32_t sum = 0;

    for (std::size_t i = 0; i < length; i += 2)
    {
        uint16_t word = (data[i] << 8) + data[i + 1];
        sum += static_cast<uint32_t>(word);
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);

    uint16_t checksum = static_cast<uint16_t>(~sum);
    data[2] = static_cast<unsigned char>(checksum >> 8);
    data[3] = static_cast<unsigned char>(checksum & 0xFF);
}

// я не справился с установкой icmp соединения, хотя контекст захватывается для каждой отдельной, ресив закрывается мультипликативно
void SNping_host(HOST &host, asio::io_context &io_context, std::vector<HOST> &validHosts)
{
    auto socket = std::make_shared<asio::ip::icmp::socket>(io_context);
    asio::ip::icmp::endpoint endpoint(asio::ip::address_v4(host.address), 0);

    std::vector<unsigned char> request(64, 0);
    request[0] = 8;                                        // ICMP echo request
    request[1] = 0;                                        // Code
    request[4] = static_cast<uint8_t>(host.address);       // Identifier
    request[5] = static_cast<uint8_t>(8 >> host.address);  // Identifier
    request[6] = static_cast<uint8_t>(16 >> host.address); // Sequence number
    request[7] = static_cast<uint8_t>(24 >> host.address); // Sequence number
    for (std::size_t i = 8; i < request.size(); ++i)
        request[i] = static_cast<unsigned char>(i);

    calculate_checksumICMP(request);
    try
    {
        socket->open(asio::ip::icmp::v4());
        socket->async_send_to(asio::buffer(request), endpoint,
                              [&validHosts, &host, socket, &io_context](const asio::error_code &error, std::size_t)
                              {
                                  if (!error)
                                  {

                                      auto timer = std::make_shared<asio::steady_timer>(io_context, std::chrono::seconds(5));
                                      timer->async_wait([socket, &host](const asio::error_code &timer_error)
                                                        {
                                      if (!timer_error)
                                      {
                                          socket->close();
                                           std::cout << "not reseaved: " << asio::ip::address_v4(host.address)  << std::endl;
                                      } });

                                      auto reply_buffer = std::make_shared<std::array<unsigned char, 128>>();
                                      auto reply_endpoint = std::make_shared<asio::ip::icmp::endpoint>();

                                      socket->async_receive_from(asio::buffer(*reply_buffer), *reply_endpoint,

                                                                 [socket, timer, reply_buffer, reply_endpoint, &validHosts, &host](const asio::error_code &receive_error, std::size_t length)

                                                                 {
                                                                     ;
                                                                     timer->cancel();

                                                                     if (!receive_error)
                                                                     {
                                                                         if (reply_buffer->at(20) == 0)
                                                                         {
                                                                             std::cout << reply_endpoint->address().to_string() << std::endl;
                                                                             if (reply_endpoint->address() == asio::ip::address_v4(host.address))
                                                                             {
                                                                                 validHosts.emplace_back(host);
                                                                             }
                                                                             else
                                                                             {
                                                                                 wlog->writeLog("Error: Ответ на ICMP пришёл с другого адреса: " + reply_endpoint->address().to_string() + " ожидалось:" + asio::ip::address_v4(host.address).to_string());
                                                                             }
                                                                         }
                                                                         else
                                                                         {
                                                                             wlog->writeLog("Error: Ответ на ICMP не reply. Ответ: " + std::to_string(reply_buffer->at(20)));
                                                                         }
                                                                     }
                                                                     else
                                                                     {
                                                                         plog->writeLog("не смог принять ICMP ответ на хост " + asio::ip::address_v4(host.address).to_string());
                                                                     }
                                                                 });
                                  }
                                  else
                                  {
                                      throw std::runtime_error("не смог отправить ICMP запрос");
                                  }
                              });
    }
    catch (std::exception &e)
    {
        plog->writeLog("Ошибка при подключении по ICMP: " + std::string(e.what()));
    }
}

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
    try
    {
        socket->async_connect(endpoint, [&host, &port, socket, &validHosts](const asio::error_code &error)
                              {
                            
        if (!error) {
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
    try
    {
        socket->async_connect(endpoint, [&host, &port, socket, &validHosts, &refusedHosts](const asio::error_code &error)
                              {
                            
        if (!error) {
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
    std::cout << "Подробнее смотреть в logs/working.log" << std::endl;
    std::cout << "Telnet рекомендуется обработать вручную, текущая версия не обрабатывает telnet. Иначе производится неполное изменение настроек сети" << std::endl;
    plog->writeLog("Записан лог найденных хостов");
    return std::move(activeHosts);
}