cmake_minimum_required(VERSION 3.20)

project(FavoriteWidget VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_executable(favorite_widget WIN32
  src/main.cpp
  src/ui_assets.cpp
  src/utils.cpp
  src/storage.cpp
  src/search.cpp
  src/windows.cpp
)

target_include_directories(favorite_widget PRIVATE src)

target_compile_definitions(favorite_widget PRIVATE
  UNICODE
  _UNICODE
  NOMINMAX
  WIN32_LEAN_AND_MEAN
)

target_link_libraries(favorite_widget PRIVATE
  Threads::Threads
  comctl32
  dwmapi
  gdi32
  gdiplus
  msimg32
  ole32
  shell32
  shlwapi
)

if(MINGW)
  target_link_options(favorite_widget PRIVATE -municode -static-libgcc -static-libstdc++)
endif()

find_program(NSIS_EXECUTABLE
  NAMES makensis
  PATHS
    "C:/Program Files (x86)/NSIS"
    "C:/Program Files/NSIS"
)

install(TARGETS favorite_widget
  RUNTIME DESTINATION .
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/
  DESTINATION assets
)

set(CPACK_PACKAGE_NAME "FavoriteWidget")
set(CPACK_PACKAGE_VENDOR "Favorite Widget")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Windows desktop favorite launcher widget")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Favorite Widget")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_FILE_NAME "FavoriteWidget-${PROJECT_VERSION}-win64")
set(CPACK_NSIS_PACKAGE_NAME "Favorite Widget")
set(CPACK_NSIS_DISPLAY_NAME "Favorite Widget")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MODIFY_PATH OFF)
if(NSIS_EXECUTABLE)
  set(CPACK_NSIS_EXECUTABLE "${NSIS_EXECUTABLE}")
endif()
set(CPACK_GENERATOR "NSIS64;ZIP")

add_custom_command(TARGET favorite_widget POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:favorite_widget>/assets
)

include(CPack)
