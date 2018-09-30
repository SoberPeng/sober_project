cmake_minimum_required(VERSION 2.8)
#########################################################################

# Basic environment setting

if(WIN32)
	SET(BUILD_PROJECT_NAME "libeventservice")
elseif(UNIX)
	SET(BUILD_PROJECT_NAME "eventservice")
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
		${CMAKE_CURRENT_SOURCE_DIR}/base/win32.h
		${CMAKE_CURRENT_SOURCE_DIR}/base/win32.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/base/win32socketinit.h
		${CMAKE_CURRENT_SOURCE_DIR}/base/win32socketinit.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/base/winping.h
		${CMAKE_CURRENT_SOURCE_DIR}/base/winping.cpp
		)
ELSEIF(APPLE)
	SET(PLATFORM_STR_LIST
		${CMAKE_CURRENT_SOURCE_DIR}/base/maccocoathreadhelper.h
		${CMAKE_CURRENT_SOURCE_DIR}/base/maccocoathreadhelper.mm
		${CMAKE_CURRENT_SOURCE_DIR}/base/scoped_autorelease_pool.h
		${CMAKE_CURRENT_SOURCE_DIR}/base/scoped_autorelease_pool.mm
		)
ELSE()
	SET(PLATFORM_STR_LIST
		)
ENDIF()
SET(SRC_LIST 
	${CMAKE_CURRENT_SOURCE_DIR}/base/asyncfile.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/asyncfile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/base64.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/base64.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/basictypes.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/buffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/bytebuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/bytebuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/common.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/common.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/constructormagic.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/criticalsection.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/helpmethods.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/helpmethods.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/ipaddress.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/ipaddress.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/logging.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/logging.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/noncopyable.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/nethelpers.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/nethelpers.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/scoped_ptr.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/scoped_ref_ptr.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/sha1.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/sha1.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/shared_ptr.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/signalthread.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/signalthread.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/sigslot.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/socket.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/socketaddress.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/socketaddress.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/socketfactory.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/socketserver.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/stream.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/stream.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/stringencode.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/stringencode.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/stringutils.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/stringutils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/timeutils.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/timeutils.cpp
	${PLATFORM_STR_LIST}
	
	# event
	${CMAKE_CURRENT_SOURCE_DIR}/event/thread.h
	${CMAKE_CURRENT_SOURCE_DIR}/event/thread.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/messagequeue.h
	${CMAKE_CURRENT_SOURCE_DIR}/event/messagequeue.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/messagehandler.h
	${CMAKE_CURRENT_SOURCE_DIR}/event/messagehandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/signalevent.h
	${CMAKE_CURRENT_SOURCE_DIR}/event/signalevent.cpp
	
  
	${CMAKE_CURRENT_SOURCE_DIR}/net/eventservice.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/eventservice.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/databuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/databuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/asynchttpsocket.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/asynchttpsocket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/asyncpacketsocket.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/asyncpacketsocket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/networkservice.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/networkservice.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/networktinterface.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/networktinterface.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/networktinterfaceimpl.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/networktinterfaceimpl.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/networkudpimpl.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/networkudpimpl.cpp

  
	${CMAKE_CURRENT_SOURCE_DIR}/http/http_parser.h
	${CMAKE_CURRENT_SOURCE_DIR}/http/http_parser.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/reply.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/reply.h
	${CMAKE_CURRENT_SOURCE_DIR}/http/mime_types.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/mime_types.h
	${CMAKE_CURRENT_SOURCE_DIR}/http/handlermanager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/handlermanager.h
	${CMAKE_CURRENT_SOURCE_DIR}/http/file_handler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/file_handler.h

	# ${CMAKE_CURRENT_SOURCE_DIR}/network/networkservice.h
	# ${CMAKE_CURRENT_SOURCE_DIR}/network/networkservice.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/dp/dpclient.h
	${CMAKE_CURRENT_SOURCE_DIR}/dp/dpclient.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/dp/dpserver.h
	${CMAKE_CURRENT_SOURCE_DIR}/dp/dpserver.cpp

	${CMAKE_CURRENT_SOURCE_DIR}/mem/membuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/mem/membuffer.cpp
  
	${CMAKE_CURRENT_SOURCE_DIR}/tls/tls.h
	${CMAKE_CURRENT_SOURCE_DIR}/tls/tls.cpp
	)

SOURCE_GROUP(base FILES
	${CMAKE_CURRENT_SOURCE_DIR}/base/asyncfile.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/asyncfile.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/asyncsocket.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/asyncsocket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/base64.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/base64.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/basictypes.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/buffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/bytebuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/bytebuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/common.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/common.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/constructormagic.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/criticalsection.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/helpmethods.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/helpmethods.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/ipaddress.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/ipaddress.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/logging.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/logging.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/noncopyable.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/nethelpers.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/nethelpers.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/scoped_ptr.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/scoped_ref_ptr.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/sha1.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/sha1.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/shared_ptr.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/signalthread.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/signalthread.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/sigslot.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/socket.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/socketaddress.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/socketaddress.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/socketfactory.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/socketserver.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/stream.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/stream.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/stringencode.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/stringencode.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/stringutils.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/stringutils.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/base/timeutils.h
	${CMAKE_CURRENT_SOURCE_DIR}/base/timeutils.cpp
	${PLATFORM_STR_LIST}
	)

SOURCE_GROUP(event FILES
	${CMAKE_CURRENT_SOURCE_DIR}/event/thread.h
	${CMAKE_CURRENT_SOURCE_DIR}/event/thread.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/messagequeue.h
	${CMAKE_CURRENT_SOURCE_DIR}/event/messagequeue.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/messagehandler.h
	${CMAKE_CURRENT_SOURCE_DIR}/event/messagehandler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/event/signalevent.h
	${CMAKE_CURRENT_SOURCE_DIR}/event/signalevent.cpp
	)
  
SOURCE_GROUP(net FILES
	${CMAKE_CURRENT_SOURCE_DIR}/net/eventservice.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/eventservice.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/databuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/databuffer.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/asynchttpsocket.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/asynchttpsocket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/asyncpacketsocket.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/asyncpacketsocket.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/networkservice.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/networkservice.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/networktinterface.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/networktinterface.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/networktinterfaceimpl.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/networktinterfaceimpl.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/net/networkudpimpl.h
	${CMAKE_CURRENT_SOURCE_DIR}/net/networkudpimpl.cpp
	)

SOURCE_GROUP(http FILES
	${CMAKE_CURRENT_SOURCE_DIR}/http/http_parser.h
	${CMAKE_CURRENT_SOURCE_DIR}/http/http_parser.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/reply.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/reply.h
	${CMAKE_CURRENT_SOURCE_DIR}/http/mime_types.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/mime_types.h
	${CMAKE_CURRENT_SOURCE_DIR}/http/handlermanager.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/handlermanager.h
	${CMAKE_CURRENT_SOURCE_DIR}/http/file_handler.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/http/file_handler.h
	)
  
SOURCE_GROUP(dp FILES
	${CMAKE_CURRENT_SOURCE_DIR}/dp/dpclient.h
	${CMAKE_CURRENT_SOURCE_DIR}/dp/dpclient.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/dp/dpserver.h
	${CMAKE_CURRENT_SOURCE_DIR}/dp/dpserver.cpp
	)
  
SOURCE_GROUP(mem FILES
	${CMAKE_CURRENT_SOURCE_DIR}/mem/membuffer.h
	${CMAKE_CURRENT_SOURCE_DIR}/mem/membuffer.cpp
	)

SOURCE_GROUP(tls FILES
	${CMAKE_CURRENT_SOURCE_DIR}/tls/tls.h
	${CMAKE_CURRENT_SOURCE_DIR}/tls/tls.cpp
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