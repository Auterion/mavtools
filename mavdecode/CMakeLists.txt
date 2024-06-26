
project(mavdecode)
add_executable(mavdecode mavdecode.cpp)

find_package(nlohmann_json REQUIRED)

target_include_directories(mavdecode PRIVATE ../dependencies)
target_include_directories(mavdecode PRIVATE ../dependencies/libmav/include)

target_link_libraries(mavdecode PRIVATE nlohmann_json::nlohmann_json)

