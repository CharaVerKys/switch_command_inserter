# Получение списка всех исходных файлов в директории src/
file(GLOB_RECURSE SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/modules/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/classes/*.cpp
)
# Сборка исполняемого файла из исходных файлов
add_executable(swcmdins ${SOURCES})
target_link_libraries(swcmdins SQLiteCpp)
target_link_libraries(swcmdins libssh2)
