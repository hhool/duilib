# CMake root file for duilib related apps
#Date 2012-09-16
#Author: Qi Gao(monkgau@gmail.com)
#
#  CMake files for duilib
#  file for each app is under the related sub-dir
#

cmake_minimum_required(VERSION 3.0)

project(dui)

message(STATUS," CMake project files for duilib")

option(USE_UNICODE "Use unicode" ON)
option(USE_SHARED "Use shared library" ON)
option(USE_DEMO "Use demo" ON)
option(USE_COPY "Use Copy" OFF)
option(USE_COPY_RESOURCES "Use Copy Resources" OFF)

if(USE_UNICODE)
    # this line is for UNICODE release,which is required by DuiDesigner
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# add each CMake file
add_subdirectory(duilib)
if (USE_DEMO)
    add_subdirectory(360SafeDemo)
    add_subdirectory(FlashDemo)
    add_subdirectory(GameDemo)
    add_subdirectory(ListDemo)
    add_subdirectory(MenuDemo)
    add_subdirectory(QQDemo)
    add_subdirectory(RichListDemo)
endif()
