#include <main.hpp>

int main(const int argc, char const *argv[])
{
    initVars(std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path(), argc, argv); // инициализация логера, базы_данных и конфигера

    // если нет глагола выдать exit 1 и сообщение
    if (argc < 2)
    {
        std::cerr << "Необходимо передать глагол в качестве аргумента.\n"
                  << "Доступные глаголы: scan, identify, script (scripTELN), commit. Допустимо 'scan identify'" << std::endl;
        std::exit(1);
    }


    SWITCH(argv[1]) // в этом свиче нельзя создавать объекты, только операции с ними
    {
        CASE("scan") : // я без понятия почему формотирование вызывает такой баг, в данном случае для табуляций к rootScan (VScode 1.86.2 flatpack )
                       rootScan(argc, argv);
        break; // конец scan

        CASE("identify") : //
                           rootIdentify(argc, argv);
        break; // конец identify

        CASE("commit") : //
                         rootCommit(argc, argv);
        break; // конец commit

        CASE("script") : //
                         rootScript(argc, argv);
        break; // конец script

        CASE("scripTELN") : //
                            rootScriptTELNET(argc, argv);
        break; // конец scripTELN

    default: // выдать ошибку о несоотвествии глагола
        std::cerr << "Неправильный глагол в качестве аргумента.\n"
                  << "Доступные глаголы: scan, identify, script (scripTELN), commit. Допустимо 'scan identify'" << std::endl;
        std::exit(1);
        break;
    }

    return 0;
}
