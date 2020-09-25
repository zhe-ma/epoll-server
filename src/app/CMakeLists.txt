file(GLOB SRCS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

set(EXE_TARGET_NAME Server)

set(LIBS
    jsoncpp
    spdlog
    epoll_server
    "${CMAKE_THREAD_LIBS_INIT}"
    )

add_executable(${EXE_TARGET_NAME} ${SRCS})
target_link_libraries(${EXE_TARGET_NAME} ${LIBS})
