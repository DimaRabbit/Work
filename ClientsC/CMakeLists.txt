cmake_minimum_required(VERSION 3.20)

project(ClientsC)

set(PostgreSQL_ADDITIONAL_VERSIONS 17)

add_executable(ClientsC main.cpp)
add_subdirectory("C:\\HW\\libpqxx-7.9.2" libpqxx_build)

target_compile_features(ClientsC PRIVATE cxx_std_17)

target_link_libraries(ClientsC PRIVATE pqxx)