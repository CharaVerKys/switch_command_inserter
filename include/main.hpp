#pragma once
#include <iostream>
#include <host.hpp>
#include <asio.hpp>
#include <CWRconfigs.hpp>
#include <nameDefinition.hpp>
#include <identifyRegex.hpp>
#include <SSHSession.hpp>
#include <database.hpp>
#include <headerlibSwitchCase.hpp>
#include <TELNETSession.hpp>
#include <identifyRegexT.hpp>
// #include <logging.hpp> // уже добавил в CWRconfigs.hpp, чисто на всякий случай закоментил
// initVars.cpp
void initVars(std::filesystem::path executable_path, const int &argc, char const *argv[]);

// rootCallFunctions.cpp
void rootScan(int argc, char const *argv[]);
void rootIdentify(int argc, char const *argv[]);
void rootCommit(int argc, char const *argv[]);
void rootScript(int argc, char const *argv[]);
void rootScriptTELNET(int argc, char const *argv[]);


// scanNetwork.cpp
ActiveHOSTS ScanNetwork(std::vector<HOST> &hosts);

std::vector<HOST> SNinitHostsVector(SNparsedNetworkHost &IpPool);
void SNcheck_port_async(HOST &host, asio::io_context &io_context, HOST::PORT &port, std::vector<HOST> &validHosts);
void SNcheck_port_async(HOST &host, asio::io_context &io_context, HOST::PORT &port, std::vector<HOST> &validHosts, std::vector<HOST> &refusedHosts);

// мусорная функция, не используется
bool is_port_open(const uint32_t ip_address, int port);

// parseIpMaskToIpPool.cpp
SNparsedNetworkHost parseIpMaskToIpPool(std::string ipAndMask);
uint32_t ipToBin(const std::string &ip);
