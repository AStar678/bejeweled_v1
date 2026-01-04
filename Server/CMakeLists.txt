cmake_minimum_required(VERSION 3.17)
project(GameBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


find_package(Qt6 COMPONENTS Core Sql REQUIRED)

include_directories("src")
include_directories("lib")

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(GameBackend ${SOURCES})

target_link_libraries(GameBackend PRIVATE Qt6::Core Qt6::Sql pthread)


add_custom_command(TARGET GameBackend POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/src/static
        $<TARGET_FILE_DIR:GameBackend>/static
    COMMENT "Copying static assets to output directory"
)