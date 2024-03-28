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
struct COMMANDS
{
    std::string cmd;
    std::string expect;
    std::string send_to_step;
    std::string not_expect;

    COMMANDS(){}
    COMMANDS(const std::string &cmd,
             const std::string &expect,
             const std::string &send_to_step,
             const std::string &not_expect) : //
                                    send_to_step(send_to_step),
                                    cmd(cmd),
                                    expect(expect),
                                    not_expect(not_expect)
    {
    }
};

class Configer
{

    bool create_networkconf();
    bool create_perModel_doCommandsconf();
    bool read_networkconf();
    bool read_perModel_doCommandsconf();
    bool read_scriptIpList();
    bool create_scriptIpList();

    std::fstream _scriptIpList;
    std::fstream _networkconf;
    std::fstream _perModel_doCommandsconf;
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
};

extern std::unique_ptr<Configer> configer;
