#include <main.hpp>

int main(const int argc, char const *argv[])
{

    // если нет глагола выдать exit 1 и сообщение
    if (argc < 2)
    {
        std::cerr << "Необходимо передать глагол в качестве аргумента.\n"
                  << "Доступные глаголы: scan, identify, showssh, commit. Допустимо 'scan identify'" << std::endl;
        std::exit(1);
    }

    initVars(std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path(), argc, argv); // инициализация логера, базы_данных и конфигера

    // инициализация переменных которые используются в обработчике
    // для уникальности сделал чтото типо особого имени
    // если что от лишник объявленных указателей и переменных размер сильно не увеличется
    // тут что-то было, пока я не вынес в отдельные функции (например rootScan)
HOST host;
std::vector<HOST> oneHost;
std::string str0;
std::string str1;
std::string str2;
std::string str3;

    SWITCH(argv[1]) // в этом свиче нельзя создавать объекты, только операции с ними
    {
        CASE("scan") : // я без понятия почему формотирование вызывает такой баг, в данном случае для табуляций к rootScan (VScode 1.86.2 flatpack )

                       rootScan(argc, argv); // сюда не забыть добавить 2 поинта на функции к identify

        break; // конец scan

        CASE("identify") : //


std::cin >> str0;
std::cin >> str1;
std::cin >> str2;
std::cin >> str3;
host.address = ipToBin(str0);
host.login.name="str1";
host.login.password = "str2";
host.model="str3";
oneHost.push_back(host);
sqlite->write_to_database(TableNameForSSH,oneHost);


                           break; // конец identify

        CASE("commit") : //
                         rootCommit(argc, argv);
        break;            // конец commit
                          //
                          //
                          //
                          //
                          //
                          //
                          //
        CASE("showssh") : // если нет существительного выдать exit 1 и сообщение
                          if (argc < 3)
        {
            std::cerr << "Необходимо ещё передать существительное в качестве аргумента.\n"
                      << "Доступные существительные: modelHost, goodHosts, errHosts, PerrHosts." << std::endl;
            std::exit(1);
        }

        SWITCH(argv[2])
        {
            CASE("modelHost") : // максимум 9 символов для работы хеш функции
                                // возвращает поимённый список хост-модель-логин
                                // надо реализовать так, чтобы в начале шли хосты, где не указана модель, но при этом есть логин
                                // а потом уже шли те где ни модели ни пароля
                                // и в конце те где указано всё

                                break; // конец modelHost

            CASE("goodHosts") : // возвращает список всех пройденных хостов, чтобы добавить их в исключения при повторном запуске, если решим запускать так

                                break; // конец goodHosts

            CASE("errHosts") : // возвращает список хостов где произошло неожиданное поведение

                               break; // конец errHosts

            CASE("PerrHosts") : // возвращает список хостов, где произошла программная ошибка (не открылся сокет, не подошёл пароль и т.п.)

                                break; // конец PerrHosts

        default: // выдать ошибку о несоотвествии существительного
            break;
        }
        break; // конец show
               //
               //
               //
               //
               //

    default: // выдать ошибку о несоотвествии глагола
        std::cerr << "Неправильный глагол в качестве аргумента.\n"
                  << "Доступные глаголы: scan, identify, showssh, commit. Допустимо 'scan identify'" << std::endl;
        std::exit(1);
        break;
    }

    return 0;
}
