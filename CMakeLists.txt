cmake_minimum_required(VERSION 3.16...3.24)

if(UNIX AND NOT APPLE)
    execute_process(COMMAND cat /etc/os-release
            OUTPUT_VARIABLE OS_RELEASE_INFO)
    string(REGEX MATCH "ID=ubuntu" FOUND_UBUNTU ${OS_RELEASE_INFO})

    if(FOUND_UBUNTU)
        set(CMAKE_C_COMPILER "/usr/bin/gcc-10")
        SET(CMAKE_CXX_COMPILER "/usr/bin/g++-10")
    endif()
endif()

include(cmake/cable/bootstrap.cmake)
include(CableBuildType)
include(CableCompilerSettings)
include(CablePackage)
include(CableToolchains)
include(CMakePackageConfigHelpers)

cable_configure_toolchain(DEFAULT cxx17-pic)
cable_set_build_type(DEFAULT Release CONFIGURATION_TYPES Release Debug)

include(Hunter/init)

project(evm_server_cpp)

hunter_add_package(ethash)
find_package(ethash CONFIG REQUIRED)
add_subdirectory(evmc)

set(include_dir ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_subdirectory(lib)

set(evmone_private_include_dir ${PROJECT_SOURCE_DIR}/lib)

hunter_add_package(nlohmann_json)
find_package(nlohmann_json CONFIG REQUIRED)
add_subdirectory(state)

hunter_add_package(Beast)
find_package(Beast CONFIG REQUIRED)

add_executable(evm_server_cpp main.cpp http_server.cpp http_server.h evm_tx_executor.cpp evm_tx_executor.h evm_request_processor.cpp evm_request_processor.h help_message.h help_message.cpp)
target_link_libraries(evm_server_cpp PRIVATE Beast::Beast evmone::state nlohmann_json::nlohmann_json)
target_link_libraries(evm_server_cpp PRIVATE evmc::evmc evmone)
