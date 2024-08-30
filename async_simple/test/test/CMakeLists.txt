aux_source_directory(. src)
add_executable(s_test ${src})

target_link_libraries(s_test spdlog::spdlog)