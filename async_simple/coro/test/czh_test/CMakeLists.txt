file(GLOB coro_czh_test_src "*.cpp")
add_executable(async_simple_coro_czh_test ${coro_czh_test_src})

target_link_libraries(async_simple_coro_czh_test async_simple spdlog::spdlog)
