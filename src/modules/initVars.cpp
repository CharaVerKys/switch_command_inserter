#include <main.hpp>

std::string getCurrentDateTimeForLogDir();

void initVars(std::filesystem::path executable_path, const int &argc, char const *argv[])
{
    // создаю дирикторию логов
    if (!std::filesystem::exists(executable_path / "logs"))
    {

        if (!(std::filesystem::create_directory(executable_path / "logs")))
        {
            std::cerr << "Failed to create directory logs.\n";
        }
    }
    // и поддирикторию для запуска
    std::string pathHereLogFormat = "log(" + getCurrentDateTimeForLogDir() + ")";
    std::filesystem::path logs_directory = executable_path / "logs" / pathHereLogFormat;

    if (!std::filesystem::exists(logs_directory))
    {
        if (!std::filesystem::create_directories(logs_directory))
        {
            std::cerr << "Failed to create logs sub-directory.\n";
        }
    }

// удаляю из корня те что не должны повторяться по логике
    std::filesystem::remove(executable_path / "identify.log");
    std::filesystem::remove(executable_path / "errHosts.log");
    
    // инициализация логера
    
    idelog = std::make_unique<Logging>((executable_path / "identify.log").string(), true);
    plog = std::make_unique<Logging>((logs_directory / "programm.log").string(), true);
    wlog = std::make_unique<Logging>((logs_directory / "working.log").string(), true);

    // писать эти логи только если параметр запуска комит
    bool isWriteThisLog = false;
    if (argc >= 2 && (std::string(argv[1]) == "commit" || std::string(argv[1]) == "script") )
    {
        isWriteThisLog = true;
    }
    errHlog = std::make_unique<Logging>((executable_path / "errHosts.log").string(), isWriteThisLog);
    gHlog = std::make_unique<GoodHostsLogging>((logs_directory / "goodHosts.log").string(),                   // я вообще мог бы это спокойно вставить внутрь блока if, но не стану уже переделывать
                                               (executable_path / "goodHosts.log").string(), isWriteThisLog); // просто идея как это должно работать поменялась

    // сразу же, как возможно, начинаю лог
    plog->writeLog("Программа стартовала");

    // создаю дирикторию конфигов
    if (!std::filesystem::exists(executable_path / "configs"))
    {
        plog->writeLog("Создаётся дириктория configs");
        if (!std::filesystem::create_directory(executable_path / "configs"))
        {
            std::cerr << "Failed to create directory configs.\n";
            plog->writeLog("Failed to create directory configs.");
        }
    }

    // инициализирую конфигер
    configer = std::make_unique<Configer>(executable_path);

    // инициализирую базы данных
    sqlite = std::make_unique<dbLite>(executable_path);
}

std::string getCurrentDateTimeForLogDir()
{
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%y-%m-%d_%H:%M");
    return ss.str();
}
