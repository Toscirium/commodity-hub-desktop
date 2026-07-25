cmake_minimum_required(VERSION 3.21)
project(CommodityHubDesktop VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network WebEngineWidgets)

if(QT_VERSION VERSION_GREATER_EQUAL 6.3)
    qt_standard_project_setup()
endif()

set(APP_SOURCES
    src/main.cpp
    src/core/Config.h
    src/core/SupabaseClient.h
    src/core/SupabaseClient.cpp
    src/core/Session.h
    src/core/Session.cpp
    src/models/Commodity.h
    src/models/Commodity.cpp
    src/models/OhlcBar.h
    src/models/OhlcBar.cpp
    src/models/Watchlist.h
    src/models/Watchlist.cpp
    src/models/Portfolio.h
    src/models/Portfolio.cpp
    src/models/PortfolioPosition.h
    src/models/PortfolioPosition.cpp
    src/models/PriceAlert.h
    src/models/PriceAlert.cpp
    src/services/CommodityService.h
    src/services/CommodityService.cpp
    src/services/OhlcService.h
    src/services/OhlcService.cpp
    src/services/WatchlistService.h
    src/services/WatchlistService.cpp
    src/services/PortfolioService.h
    src/services/PortfolioService.cpp
    src/services/PriceAlertService.h
    src/services/PriceAlertService.cpp
    src/ui/LoginDialog.h
    src/ui/LoginDialog.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/WatchlistPanel.h
    src/ui/WatchlistPanel.cpp
    src/ui/CommodityCardWidget.h
    src/ui/CommodityCardWidget.cpp
    src/ui/CategoryHeaderWidget.h
    src/ui/CategoryHeaderWidget.cpp
    src/ui/ChartPanel.h
    src/ui/ChartPanel.cpp
    src/ui/DashboardPanel.h
    src/ui/DashboardPanel.cpp
    src/ui/PortfolioPanel.h
    src/ui/PortfolioPanel.cpp
    src/ui/PortfolioTableModel.h
    src/ui/PortfolioTableModel.cpp
    src/ui/AddPositionDialog.h
    src/ui/AddPositionDialog.cpp
    src/ui/AlertsPanel.h
    src/ui/AlertsPanel.cpp
    src/ui/AlertsTableModel.h
    src/ui/AlertsTableModel.cpp
    src/ui/AddAlertDialog.h
    src/ui/AddAlertDialog.cpp
    resources/resources.qrc
)

qt_add_executable(CommodityHubDesktop ${APP_SOURCES})

target_include_directories(CommodityHubDesktop PRIVATE src)

target_link_libraries(CommodityHubDesktop PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    Qt6::WebEngineWidgets
)

if(WIN32)
    set_target_properties(CommodityHubDesktop PROPERTIES
        OUTPUT_NAME "CommodityHub"
    )
endif()

install(TARGETS CommodityHubDesktop
    RUNTIME DESTINATION bin
    BUNDLE DESTINATION .
)
