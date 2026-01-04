cmake_minimum_required(VERSION 3.16)

project(Web)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets WebEngineWidgets)

add_executable(Web MACOSX_BUNDLE main.cpp)


target_link_libraries(Web PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::WebEngineWidgets
)

