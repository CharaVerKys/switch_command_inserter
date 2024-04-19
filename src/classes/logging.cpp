#include <logging.hpp>

// конструктор
Logging::Logging(const std::string &filePath, bool isWriteThisLog) : logFilePath(filePath)
{
    this->isWriteThisLog = isWriteThisLog;
    isWRITEq // функция просто ничего не делает

        // удаления существующего файла лога, чтобы писать по новой
        // std::filesystem::remove(logFilePath); // это уже типо легаси, я переопределил логику логирования, теперь не перезаписывается файл а список папок с временем

        logFile.open(logFilePath, std::ios::out | std::ios::app);
    if (!logFile.is_open())
    {
        std::cerr << "Failed to open log file: " << logFilePath << std::endl;
    }
}

// Деструктор
Logging::~Logging()
{
    isWRITEq

        if (logFile.is_open())
    {
        logFile.close();
    }
}

// для получения текущего времени в формате
std::string Logging::getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();

// этот код я чисто стыбзил
    return ss.str();
}

void Logging::writeLog(const std::string &message)
{
    isWRITEq

        if (!logFile.is_open())
    {
        std::cerr << "Log file is not open. (" + logFilePath + ")" << std::endl;
        return;
    }
    logFile << getCurrentTime() << " -> " << message << std::endl;
}

// вот такой простинький логер

std::unique_ptr<Logging> plog;
std::unique_ptr<Logging> wlog;
std::unique_ptr<Logging> errHlog;
std::unique_ptr<Logging> idelog;

// а этот уже чуть чуть посложнее

GoodHostsLogging::GoodHostsLogging(const std::string &filePath,
                                   const std::string &overridableFilePath,
                                   bool isWriteThisLog) : Logging(filePath, isWriteThisLog),
                                                          overridablelogFilePath(overridableFilePath)
{
    isWRITEq
        overridableLogFile.open(overridablelogFilePath, std::ios::out | std::ios::app);
    if (!overridableLogFile.is_open())
    {
        std::cerr << "Failed to open log file: " << overridablelogFilePath << std::endl;
    }
}

GoodHostsLogging::~GoodHostsLogging()
{   isWRITEq

     if (overridableLogFile.is_open())
    {
        overridableLogFile.close();
    }
}

void GoodHostsLogging::writeLog(const std::string &message)
{isWRITEq
    Logging::writeLog(message);
    if (!overridableLogFile.is_open())
    {
        std::cerr << "Log file is not open. (" + overridablelogFilePath + ")" << std::endl;

        return;
    }
    overridableLogFile << getCurrentTime() << " -> " << message << std::endl;
}

std::unique_ptr<GoodHostsLogging> gHlog;