cmake_minimum_required(VERSION 3.8)

project(psc)

add_definitions(-DBOOST_LOG_DYN_LINK)

add_executable(psc
    main.cpp)

set(BOOST_LIB_SUFFIX)

if(MINGW)
    target_link_libraries(psc
        wsock32
        ws2_32)

    set(BOOST_LIB_SUFFIX "-mt")
endif()

# target_link_libraries(psc)
