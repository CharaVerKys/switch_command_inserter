#pragma once
#include <iostream>
#include <asio.hpp>

#include <host.hpp> // структура хоста
#include <CWRconfigs.hpp> //чтение и запись конфига (create/write/read)
#include <nameDefinition.hpp> // имена и некоторые структуры
#include <identifyRegex.hpp> // обработчики поиска
#include <identifyRegexT.hpp>
#include <SSHSession.hpp> // обработчики сессий
#include <TELNETSession.hpp>
#include <database.hpp> // работа с sqlite
#include <headerlibSwitchCase.hpp> // даёт возможность писать символы в SwitchCase
// #include <logging.hpp> // уже добавил в CWRconfigs.hpp, чисто на всякий случай закоментил


// initVars.cpp
void initVars(std::filesystem::path executable_path, const int &argc, char const *argv[]);

// rootCallFunctions.cpp
void rootScan(int argc, char const *argv[]);
void rootIdentify(int argc, char const *argv[]);
void rootCommit(int argc, char const *argv[]);
void rootScript(int argc, char const *argv[]);
void rootScriptTELNET(int argc, char const *argv[]);
void showHelp();

// scanNetwork.cpp
ActiveHOSTS ScanNetwork(std::vector<HOST> &hosts);
std::vector<HOST> SNinitHostsVector(SNparsedNetworkHost &IpPool);
void SNcheck_port_async(HOST &host, asio::io_context &io_context, HOST::PORT &port, std::vector<HOST> &validHosts);
void SNcheck_port_async(HOST &host, asio::io_context &io_context, HOST::PORT &port, std::vector<HOST> &validHosts, std::vector<HOST> &refusedHosts);


// parseIpMaskToIpPool.cpp
SNparsedNetworkHost parseIpMaskToIpPool(std::string ipAndMask);
uint32_t ipToBin(const std::string &ip);
