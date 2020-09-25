file(GLOB SRCS
     RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
     ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
     ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set(TARGET_NAME epoll_server)

ADD_LIBRARY(${TARGET_NAME} STATIC ${SRCS})

set(LIBS
    jsoncpp
    spdlog
    )

target_link_libraries(${TARGET_NAME} ${LIBS})
