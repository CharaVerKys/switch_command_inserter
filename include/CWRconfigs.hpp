#pragma once
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <cerrno>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <logging.hpp>
#include <unordered_set>
#include <regex>
#include <host.hpp>
#include <asio.hpp>
#include <nameDefinition.hpp>

class Configer
{

    bool create_networkconf();
    bool create_perModel_doCommandsconf();
    bool read_networkconf();
    bool read_perModel_doCommandsconf();
    bool create_scriptIpList();
    bool read_scriptIpList();
    bool create_perCommands_findModelconf();
    bool read_perCommands_findModelconf();


    std::fstream _scriptIpList;
    std::fstream _networkconf;
    std::fstream _perModel_doCommandsconf;
    std::fstream _perCommands_findModelconf;
    std::filesystem::path executable_path;

    std::vector<std::string> _sIpList;
    std::vector<std::string> _ignoreHosts;
    std::vector<std::pair<std::string, std::string>> _Logins_Passwords;
    std::string _IpMask;

    std::vector<                  // вектор для отправки в обработчик
        std::pair<                // пара модель и команды к ней
            std::string,          // модель
            std::vector<COMMANDS> // набор команд
            >                     //                 жесть ваще вектор
        >
        _models_and_commands; // название вектора

    std::vector<                  // вектор для отправки в обработчик
        std::pair<                // пара модель и команды к ней
            std::string,          // модель
            std::vector<COMMANDS> // набор команд
            >                     //                 жесть ваще вектор
        >
        _finding_commands; // название вектора
public:
    Configer(const std::filesystem::path &executable_path);

    const std::vector<std::string> &getIgnoringHosts();
    const std::string &getIpMask() const;
    const std::vector<std::pair<std::string, std::string>> &getLogins_Passwords() const;
    const std::vector<std::string> &getScriptIpList();
    bool updateNetwork_conf(std::vector<HOST> &goodHosts);

    const std::vector<            // вектор для отправки в обработчик
        std::pair<                // пара модель и команды к ней
            std::string,          // модель
            std::vector<COMMANDS> // набор команд
            >                     //                 жесть ваще вектор
        > &
    getModels_and_commands() const; // название метода

     const std::vector<            // вектор для отправки в обработчик
        std::pair<                // пара модель и команды к ней
            std::string,          // модель
            std::vector<COMMANDS> // набор команд
            >                     //                 жесть ваще вектор
        > &
    getFinding_commands() const; // название метода
};

extern std::unique_ptr<Configer> configer;
