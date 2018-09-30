cmake_minimum_required(VERSION 2.8)
#########################################################################

# Basic environment setting

if(WIN32)
	SET(BUILD_PROJECT_NAME "libfilecache")
elseif(UNIX)
	SET(BUILD_PROJECT_NAME "filecache")
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
#Step 3 : Add code source
IF(WIN32)
	SET(PLATFORM_STR_LIST
		)
ELSEIF(APPLE)
	SET(PLATFORM_STR_LIST
		)
ELSE()
	SET(PLATFORM_STR_LIST
		)
ENDIF()
SET(SRC_LIST 
	${CMAKE_CURRENT_SOURCE_DIR}/base/basedefine.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/diskhelper.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/diskhelper.cpp
	
	# cache
	${CMAKE_CURRENT_SOURCE_DIR}/server/cacheserver.h
	${CMAKE_CURRENT_SOURCE_DIR}/server/cacheserver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/server/cacheservice.h
	${CMAKE_CURRENT_SOURCE_DIR}/server/cacheservice.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/server/cachestanzapool.h
	${CMAKE_CURRENT_SOURCE_DIR}/server/cachestanzapool.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/server/kvdbservice.h
	${CMAKE_CURRENT_SOURCE_DIR}/server/kvdbservice.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/cache/cacheclient.h
	${CMAKE_CURRENT_SOURCE_DIR}/cache/cacheclient.cpp
	
	# kvdb
	${CMAKE_CURRENT_SOURCE_DIR}/kvdb/kvdbclient.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/kvdb/kvdbclient.h
	${CMAKE_CURRENT_SOURCE_DIR}/kvdb/kvdbclient_c.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/kvdb/kvdbclient_c.h
	)

SOURCE_GROUP(base FILES
	${CMAKE_CURRENT_SOURCE_DIR}/base/basedefine.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/diskhelper.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/diskhelper.cpp
	)

SOURCE_GROUP(cache FILES
	${CMAKE_CURRENT_SOURCE_DIR}/cache/cacheclient.h
	${CMAKE_CURRENT_SOURCE_DIR}/cache/cacheclient.cpp
	)
  

SOURCE_GROUP(server FILES
	${CMAKE_CURRENT_SOURCE_DIR}/server/cacheserver.h
	${CMAKE_CURRENT_SOURCE_DIR}/server/cacheserver.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/server/cacheservice.h
	${CMAKE_CURRENT_SOURCE_DIR}/server/cacheservice.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/server/cachestanzapool.h
	${CMAKE_CURRENT_SOURCE_DIR}/server/cachestanzapool.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/server/kvdbservice.h
	${CMAKE_CURRENT_SOURCE_DIR}/server/kvdbservice.cpp
	)
  
SOURCE_GROUP(kvdb FILES
	${CMAKE_CURRENT_SOURCE_DIR}/kvdb/kvdbclient.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/kvdb/kvdbclient.h
	${CMAKE_CURRENT_SOURCE_DIR}/kvdb/kvdbclient_c.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/kvdb/kvdbclient_c.h
	)
#SOURCE_GROUP(network FILES
#	${CMAKE_CURRENT_SOURCE_DIR}/network/networkservice.h
#	${CMAKE_CURRENT_SOURCE_DIR}/network/networkservice.cpp
#	${CMAKE_CURRENT_SOURCE_DIR}/network/asyncsocket.h
#	${CMAKE_CURRENT_SOURCE_DIR}/network/asyncsocket.cpp
#	)
# if (COVERALLS)
	# coveralls_setup(
        # "${CONVERALLS_SRC}"
        # ${COVERALLS_UPLOAD}                 
        # "${PROJECT_SOURCE_DIR}/coveralls-cmake/cmake")
# endif()
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