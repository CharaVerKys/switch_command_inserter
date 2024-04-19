#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>

#define isWRITEq if(!isWriteThisLog){return;} // управляющая функция

// часть функционала вынесена в initVars.cpp

class Logging
{
protected:
    std::string logFilePath;
    std::ofstream logFile;
    bool isWriteThisLog; // управляющая переменная

    //для получения текущего времени в нужном формате
    std::string getCurrentTime();

public:
    Logging(const std::string &filePath,bool isWriteThisLog);
    ~Logging();

    virtual void writeLog(const std::string &message);
};

extern std::unique_ptr<Logging> plog;
extern std::unique_ptr<Logging> wlog;
extern std::unique_ptr<Logging> errHlog;
extern std::unique_ptr<Logging> idelog;

// этот класс в 2 места записывает лог
class GoodHostsLogging : public Logging
{
    std::ofstream overridableLogFile;
    std::string overridablelogFilePath;

public:
    GoodHostsLogging(const std::string &filePath, const std::string &overridableFilePath,bool isWriteThisLog);
    ~GoodHostsLogging();
    void writeLog(const std::string &message) override;
};

extern std::unique_ptr<GoodHostsLogging> gHlog;