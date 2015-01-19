cmake_minimum_required (VERSION 2.6)
include(ExternalProject)

project (NetStack)

IF (WIN32)
    ExternalProject_Add(
        winpcap
        URL http://www.winpcap.org/install/bin/WpdPack_4_1_2.zip
        PREFIX ${CMAKE_CURRENT_LIST_DIR}/dependencies
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DOWNLOAD ON
    )

    include_directories(${CMAKE_CURRENT_LIST_DIR}/dependencies/src/winpcap/Include)
    link_directories(${CMAKE_CURRENT_LIST_DIR}/dependencies/src/winpcap/Lib)

    add_definitions(-D_ALLOW_KEYWORD_MACROS -DHAVE_REMOTE)
ELSE()
    # download pcap
ENDIF()

set(SOURCES src/NetTest.cpp src/Client.h src/EventEmitter.h src/Events.h src/Packet.h src/Request.h src/Response.h src/Server.h src/Transport.h)

add_executable(NetStack ${SOURCES})

IF (WIN32)
    target_link_libraries(NetStack wpcap)
    target_link_libraries(NetStack Ws2_32)
ELSE()
    # link pcap
ENDIF()