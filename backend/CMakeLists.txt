cmake_minimum_required(VERSION 3.15)
project(Web-PTT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

set(BOOST_INCLUDE_LIBRARIES asio beast uuid system CACHE STRING "" FORCE)
set(BOOST_ENABLE_CMAKE ON CACHE BOOL "" FORCE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


FetchContent_Declare(
    boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.91.0-1/boost-1.91.0-1-cmake.zip
)

FetchContent_Declare(
    libdatachannel
    GIT_REPOSITORY https://github.com/paullouisageneau/libdatachannel.git
    GIT_TAG v0.24.5
)

FetchContent_Declare(
    spdlog
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.zip
)

FetchContent_Declare(
    glaze
    URL https://github.com/stephenberry/glaze/archive/refs/tags/v7.9.1.zip
)

FetchContent_MakeAvailable(boost libdatachannel spdlog glaze)

find_package(OpenSSL REQUIRED)
add_executable(backend "src/main.cpp" "src/api/Listener.cpp" "src/api/WebsocketSession.cpp" "src/api/HttpSession.cpp" "src/api/WebsocketManager.cpp" "src/api/coordinator.cpp" "src/api/PeerConnection.cpp")
target_link_libraries(backend PRIVATE 
    Boost::asio
    Boost::beast
    Boost::uuid
    datachannel
    spdlog::spdlog
    glaze::glaze
    OpenSSL::SSL
    OpenSSL::Crypto
)