#include <database.hpp>

dbLite::dbLite(const std::filesystem::path &executable_path)
{
    try
    {

        db = std::make_unique<SQLite::Database>(executable_path / "sqlite.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        plog->writeLog("Открыта/создана БД");
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка при открытии базы данных: " << e.what() << std::endl;
        plog->writeLog("Ошибка при открытии базы данных: " + std::string(e.what()));
        exit(EACCES);
    }
}

void dbLite::write_to_database(const std::string &TableName, const std::vector<HOST> &hosts)
{
    try
    {
        db->exec("CREATE TABLE IF NOT EXISTS " + TableName + " (address INTEGER, name TEXT, password TEXT, model TEXT);");

        // Проходимся по всем хостам
        for (const HOST &host : hosts)
        { // Подготавливаем запрос INSERT
            SQLite::Statement query(*db, "INSERT INTO " + TableName + " (address, name, password, model) VALUES (?, ?, ?, ?);");

            // Привязываем значения к параметрам запроса
            query.bind(1, static_cast<int64_t>(host.address));
            query.bind(2, host.login.name);
            query.bind(3, host.login.password);
            query.bind(4, host.model);

            // Выполняем запрос
            query.exec();
        }
        plog->writeLog("Данные успешно записаны в базу данных, таблица: " + TableName);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка при записи в базу данных: " << e.what() << std::endl;
        plog->writeLog("Ошибка при записи в базу данных: " + std::string(e.what()));
        exit(1);
    }
}

std::vector<HOST> dbLite::read_from_database(const std::string &TableName)
{
    std::vector<HOST> hosts;

    try
    {
        // Подготавливаем запрос SELECT
        SQLite::Statement query(*db, "SELECT  address, name, password, model FROM " + TableName + ";");

        // Выполняем запрос и обрабатываем результаты
        while (query.executeStep())
        {
            HOST host;
            host.address = static_cast<uint32_t>(query.getColumn(0).getInt());
            host.login.name = query.getColumn(1).getString();
            host.login.password = query.getColumn(2).getString();
            host.model = query.getColumn(3).getString();

            hosts.push_back(host);
        }

        plog->writeLog("Данные успешно прочитаны из базы данных, таблица: " + TableName);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка при чтении из базы данных: " << e.what() << std::endl;
        plog->writeLog("Ошибка при чтении из базы данных: " + std::string(e.what()));
        exit(1);
    }

    return std::move(hosts);
}

void dbLite::emptyOut(const std::string &TableName)
{

    try
    {
        // Проверяем существует ли таблица
        SQLite::Statement query(*db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + TableName + "';");
        query.executeStep();
        bool tableExists = static_cast<bool>(query.getColumn(0).getInt());

        // Если таблица существует, очищаем ее

        if (tableExists)
        {
            db->exec("DELETE FROM " + TableName + ";");
            plog->writeLog("Таблица " + TableName + " успешно очищена.");
        }
        // Кста пример когда гпт капец касячит, я ему описал что нужно чтобы он мне сгенерил запрос правильно,
        // но он зараза мне левый функционал предлагает, я с 4 раза только добился такого результата, фактически когда уже сам код написал
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка при очистке таблицы " << TableName << ": " << e.what() << std::endl;
        plog->writeLog("Ошибка при очистке таблицы " + TableName + " из базы данных: " + std::string(e.what()));
    }
}

bool dbLite::isTableExist(const std::string &TableName)
{
    try
    {
        // Проверяем существует ли таблица
        SQLite::Statement query(*db, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='" + TableName + "';");
        query.executeStep();
        return static_cast<bool>(query.getColumn(0).getInt());
    }
    catch (const std::exception &e)
    {
        std::cerr << "Ошибка при попытки узнать, существует ли таблица " << TableName << ": " << e.what() << std::endl;
        plog->writeLog("Ошибка при попытки узнать, существует ли таблица  " + TableName + " в базе данных: " + std::string(e.what()));
        throw std::runtime_error("Ошибка при попытки узнать, существует ли таблица  " + TableName + " в базе данных: " + std::string(e.what()));
    }
}

std::unique_ptr<dbLite> sqlite;