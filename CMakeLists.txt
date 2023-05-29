cmake_minimum_required(VERSION 3.16...3.24)


#set(CMAKE_CXX_STANDARD 20)

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

add_executable(evm_server_cpp main.cpp)
target_link_libraries(evm_server_cpp PRIVATE evmone::state nlohmann_json::nlohmann_json)
target_link_libraries(evm_server_cpp PRIVATE evmc::evmc evmone)
