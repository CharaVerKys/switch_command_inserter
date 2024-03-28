#pragma once
#include <cstdint>
#include <string>

struct HOST
{
    struct LOGIN
    {
        std::string name;
        std::string password;
    };
struct PORT
    {
        uint16_t number;
        bool establish;
    };

    uint32_t address;
    uint16_t number; // используется сейчас только для script
    PORT ssh ={22,false};
    PORT telnet ={23,false};
    LOGIN login;
    std::string model;
    std::string log;
};

