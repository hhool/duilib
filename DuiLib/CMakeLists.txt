# cmake file for duilib
#Author Qi Gao(monkgau@gmail.com)
#Created: 2012/09/16

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} Root_src)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Control Control_src)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Core Core_src)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Layout Layout_src)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/Utils Utils_src)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Control)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Core)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Layout)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Utils)

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
add_library(duilib SHARED ${Control_src} ${Core_src} ${Layout_src} ${Utils_src} ${Root_src})

add_definitions(-DUILIB_EXPORTS)
target_link_libraries(duilib comctl32)
set_target_properties(duilib PROPERTIES OUTPUT_NAME "duilib")
if (USE_COPY)
add_custom_command(TARGET duilib POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${PROJECT_BINARY_DIR}/lib/duilib.dll ${PROJECT_SOURCE_DIR}/bin/duilib.dll)
endif()

message(STATUS "USE_COPY_RESOURCES: " ${USE_COPY_RESOURCES})
# set build tpye
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if (USE_COPY_RESOURCES)
    add_custom_command(TARGET duilib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/bin/Button ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/Button)
    add_custom_command(TARGET duilib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/bin/Icon ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/Icon)
    add_custom_command(TARGET duilib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/bin/skin ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/skin)
    # copy *.xml files to bin directory
    set(MY_RESOURCE_FILES
      bg.bmp
      button_down.bmp
      button_nor.bmp
      button_over.bmp
      fg.bmp
      LeftWithFill.png
      ScrCapture.xml
      scrollbar.bmp
      scrollbar.xml
      test1.xml
      thumb.bmp
      voice_rect.png
      winbk.bmp
    )
  foreach(file ${MY_RESOURCE_FILES}) 
    add_custom_command(TARGET duilib POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
      ${PROJECT_SOURCE_DIR}/bin/${file} ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE}/${file})
  endforeach()
endif()
