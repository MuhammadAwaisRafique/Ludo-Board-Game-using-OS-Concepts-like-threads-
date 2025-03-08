cmake_minimum_required(VERSION 3.5)
project(Ludo)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui Network)

add_executable(Ludo main.cpp)
target_link_libraries(Ludo Qt5::Core Qt5::Widgets Qt5::Gui Qt5::Network)