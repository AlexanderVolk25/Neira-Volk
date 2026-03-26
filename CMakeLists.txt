cmake_minimum_required(VERSION 3.21)
project(NeiraBotPanel VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Network Sql)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/configservice.cpp
    src/telegramapiclient.cpp
    src/botengine.cpp
    src/orderservice.cpp
    src/adminworkflow.cpp
    src/pages/dashboardpage.cpp
    src/pages/settingspage.cpp
    src/pages/planspage.cpp
    src/pages/paymentspage.cpp
    src/pages/messagespage.cpp
    src/pages/channelpage.cpp
    src/pages/orderspage.cpp
)

set(HEADERS
    src/mainwindow.h
    src/configservice.h
    src/telegramapiclient.h
    src/botengine.h
    src/orderservice.h
    src/adminworkflow.h
    src/pages/dashboardpage.h
    src/pages/settingspage.h
    src/pages/planspage.h
    src/pages/paymentspage.h
    src/pages/messagespage.h
    src/pages/channelpage.h
    src/pages/orderspage.h
)

qt_add_executable(NeiraBotPanel WIN32
    ${SOURCES} ${HEADERS}
    resources/resources.qrc
)

target_link_libraries(NeiraBotPanel PRIVATE Qt6::Widgets Qt6::Network Qt6::Sql)
target_include_directories(NeiraBotPanel PRIVATE src)
