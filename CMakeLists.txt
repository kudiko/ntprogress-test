CMAKE_MINIMUM_REQUIRED(VERSION 3.21)
PROJECT(ClienServerEcn)

FIND_PACKAGE(Boost 1.40 COMPONENTS system REQUIRED)
FIND_PACKAGE(Threads REQUIRED)
FIND_PACKAGE(libpqxx REQUIRED)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIR})

SET(CMAKE_CXX_STANDARD 17)

SET(GCC_ADDITIONAL_FLAGS "-Wall -Werror")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_ADDITIONAL_FLAGS}")

ADD_EXECUTABLE(Server ServerIO.cpp Common.hpp json.hpp
        ServerDatabase.cpp
        ServerDatabase.h
        MarketExecution.cpp
        MarketExecution.h
        Tests.cpp
)
TARGET_LINK_LIBRARIES(Server PRIVATE pqxx pq)
TARGET_LINK_LIBRARIES(Server PRIVATE Threads::Threads ${Boost_LIBRARIES})


ADD_EXECUTABLE(Client Client.cpp Common.hpp json.hpp)
TARGET_LINK_LIBRARIES(Client PRIVATE Threads::Threads ${Boost_LIBRARIES})