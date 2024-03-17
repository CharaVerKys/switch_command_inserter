# Получение списка всех исходных файлов в директории libs/
# file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# Настройка сборки статической библиотеки
# add_library(my_static_lib STATIC ${SOURCES})

# Если ваша статическая библиотека имеет зависимости, укажите их здесь
# target_link_libraries(my_static_lib PUBLIC dependency1 dependency2)




execute_process(
    COMMAND make
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/libs/asio-1.28.0
)
add_subdirectory(rapidjson)
add_subdirectory(SQliteCpp)
add_subdirectory(libssh2)