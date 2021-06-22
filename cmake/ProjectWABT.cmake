include(ExternalProject)

if (APPLE)
    set(PATCH_COMMAND "")
else()
    set(PATCH_COMMAND COMMAND sed -i "53a if (NOT \"\${CMAKE_PROJECT_VERSION}\")" CMakeLists.txt COMMAND sed -i "54a set(CMAKE_PROJECT_VERSION \${PROJECT_VERSION})" CMakeLists.txt COMMAND sed -i "55a endif()" CMakeLists.txt)
endif()

ExternalProject_Add(wabt_project
    PREFIX ${CMAKE_SOURCE_DIR}/deps
    DOWNLOAD_NAME wabt_1.0.19.tar.gz
    DOWNLOAD_NO_PROGRESS 1
    URL https://codeload.github.com/WebAssembly/wabt/tar.gz/1.0.19
    URL_HASH SHA256=134f2afc8205d0a3ab89c5f0d424ff3823e9d2769c39d2235aa37eba7abc15ba
    PATCH_COMMAND ${PATCH_COMMAND}
    BUILD_IN_SOURCE 0
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON
    -DBUILD_TESTS=OFF -DBUILD_TOOLS=OFF -DBUILD_LIBWASM=OFF
    INSTALL_COMMAND ""
    LOG_CONFIGURE 1
    LOG_DOWNLOAD 1
    LOG_UPDATE 1
    LOG_BUILD 1
    LOG_INSTALL 1
    BUILD_BYPRODUCTS <BINARY_DIR>/libwabt.a
)

ExternalProject_Get_Property(wabt_project SOURCE_DIR)
ExternalProject_Get_Property(wabt_project BINARY_DIR)
add_library(wabt STATIC IMPORTED GLOBAL)

set(WABT_LIBRARY ${BINARY_DIR}/libwabt.a)
set(WABT_INCLUDE_DIR ${SOURCE_DIR}/ ${BINARY_DIR}/)
file(MAKE_DIRECTORY ${WABT_INCLUDE_DIR})  # Must exist.

set_property(TARGET wabt PROPERTY IMPORTED_LOCATION ${WABT_LIBRARY})
set_property(TARGET wabt PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${WABT_INCLUDE_DIR})

add_dependencies(wabt wabt_project)
unset(SOURCE_DIR)
