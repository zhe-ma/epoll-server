file(GLOB SRCS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set(EXE_TARGET_NAME epoll_server)

set(LIBS
    jsoncpp
    )

add_executable(${EXE_TARGET_NAME} ${SRCS})
target_link_libraries(${EXE_TARGET_NAME} ${LIBS})
