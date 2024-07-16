#Cmake file for MenuDemo
#Author: Qi Gao(monkgau@gmail.com)
#Created: 2012/9/17


set(source_files About.cpp debug.cpp main_frame.cpp
 main.cpp MenuNotify.cpp MenuDemo.rc observer_impl_base.hpp resource.h
 SkinSetting.cpp UICrack.cpp UIMenu.cpp stdafx.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})                           

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

add_executable(MenuDemo ${source_files})
add_dependencies(MenuDemo duilib)
target_link_libraries(MenuDemo duilib)
set_target_properties(MenuDemo PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")

if (USE_COPY)
add_custom_command(TARGET MenuDemo POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${PROJECT_BINARY_DIR}/bin/MenuDemo.exe ${PROJECT_SOURCE_DIR}/bin/MenuDemo.exe)
endif()
