#pragma once
#include <string>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <filesystem>

#define isWRITEq if(!isWriteThisLog){return;}

class Logging
{
protected:
    std::string logFilePath;
    std::ofstream logFile;
    bool isWriteThisLog;



    // Функция для получения текущего времени в нужном формате
    std::string getCurrentTime();

public:
    // Измененный конструктор
    Logging(const std::string &filePath,bool isWriteThisLog);
    // Деструктор
    ~Logging();

    // Метод для записи строки в файл
    virtual void writeLog(const std::string &message);
};

extern std::unique_ptr<Logging> plog;
extern std::unique_ptr<Logging> wlog;
extern std::unique_ptr<Logging> errHlog;

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