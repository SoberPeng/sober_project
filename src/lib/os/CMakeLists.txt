cmake_minimum_required(VERSION 2.8)
#########################################################################

# Basic environment setting

if(WIN32)
	SET(BUILD_PROJECT_NAME "libos")
elseif(UNIX)
	SET(BUILD_PROJECT_NAME "os")
endif()

#########################################################################
#INCLUDE_DIRECTORIES(${LIBVZNET_INCLUDE_DIR})
MESSAGE(STATUS "Print the include directores")
get_property(inc_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(inc_dir ${inc_dirs})
  message(STATUS "    include directory='${inc_dir}'")
endforeach()

#########################################################################
#Step 2 : Add library directories
MESSAGE(STATUS "Step 2 : Add library directories")
#LINK_DIRECTORIES(${LIBVZNET_LIBRARY_DIR})
MESSAGE(STATUS "Print the link directores")
get_property(link_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_DIRECTORIES)
foreach(link_dir ${link_dirs})
  message(STATUS "    link directory='${link_dir}'")
endforeach()

#########################################################################

SET(SRC_LIST 
	${CMAKE_CURRENT_SOURCE_DIR}/os_type.h
	${CMAKE_CURRENT_SOURCE_DIR}/os_io.h
	${CMAKE_CURRENT_SOURCE_DIR}/win/os_io.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/os_file.h
	${CMAKE_CURRENT_SOURCE_DIR}/win/os_file.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/os_network.h
	${CMAKE_CURRENT_SOURCE_DIR}/win/os_network.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/os_thread.h
	${CMAKE_CURRENT_SOURCE_DIR}/win/os_thread.cpp
	)

SOURCE_GROUP(os FILES
	${CMAKE_CURRENT_SOURCE_DIR}/os_type.h
	${CMAKE_CURRENT_SOURCE_DIR}/os_io.h
	${CMAKE_CURRENT_SOURCE_DIR}/win/os_io.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/os_file.h
	${CMAKE_CURRENT_SOURCE_DIR}/win/os_file.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/os_network.h
	${CMAKE_CURRENT_SOURCE_DIR}/win/os_network.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/os_thread.h
	${CMAKE_CURRENT_SOURCE_DIR}/win/os_thread.cpp
	)

#########################################################################
#Step 4 : Add PROJECT define 
MESSAGE(STATUS "Step 4 : Add code source")

#########################################################################
#Step 5 : Add executable or library target
MESSAGE(STATUS "Step 5 : Add executable or library target")
#ADD_executable(${BUILD_PROJECT_NAME} ${SRC_LIST})
ADD_LIBRARY(${BUILD_PROJECT_NAME} STATIC ${SRC_LIST})
set_property(TARGET ${BUILD_PROJECT_NAME} PROPERTY FOLDER ${PROJECT_SET_NAME_LIB})
#########################################################################
#Step 6 : link with other library
MESSAGE(STATUS "Step 6 : link with other library")

TARGET_LINK_LIBRARIES(${BUILD_PROJECT_NAME} 
)