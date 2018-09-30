#F:\code\osc\aliyun_opensearch\aliyun_opensearch\builds\vs2013>cmake ../.. -DCMAK
#E_BUILD_TYPE=DEBUG -DBUILD_CURL_TESTS=OFF -DBUILD_CURL_EXE=OFF -DJSONCPP_WITH_TE
#STS=OFF -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF
#  
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(proxy_server)
set_property(GLOBAL PROPERTY USE_FOLDERS On)

MESSAGE(STATUS "01*********************************************************")

INCLUDE(cmake/global_environment_settings.cmake)
# This Include Will setting
# include path
# include library path
# link library

IF(WIN32)
	IF(MSVC12)
		INCLUDE(cmake/win_vs2013_environment_settings.cmake)
	ELSEIF(MSVC14)
		INCLUDE(cmake/win_vs2017_environment_settings.cmake)
	ENDIF()
ELSEIF(UNIX)
	#ADD_COMPILE_OPTIONS(-fpermissive)
	IF(CMAKE_BUILD_PLATFORM MATCHES HISIV300)
		MESSAGE("CMAKE_BUILD_PLATFORM MATCHES HISI")
		INCLUDE(cmake/hisi_v300_environment_settings.cmake)
	ELSEIF(CMAKE_BUILD_PLATFORM MATCHES HISIV500)
		MESSAGE("CMAKE_BUILD_PLATFORM MATCHES HISIV500")
		INCLUDE(cmake/hisi_v500_environment_settings.cmake)
	ELSEIF(CMAKE_BUILD_PLATFORM MATCHES HIMIX100)
		MESSAGE("CMAKE_BUILD_PLATFORM MATCHES LITEOS")
		INCLUDE(cmake/himix100_environment_settings.cmake)
	ELSEIF(CMAKE_BUILD_PLATFORM MATCHES UBUNTU64)
		MESSAGE("CMAKE_BUILD_PLATFORM MATCHES UBUNTU64")
		INCLUDE(cmake/ubuntu64_environment_settings.cmake)
	ENDIF()
ENDIF()

MESSAGE(STATUS "02*********************************************************")
INCLUDE_DIRECTORIES(${VZPROJECT_INCLUDE_DIR})
LINK_DIRECTORIES(${VZPROJECT_LIBRARY_DIR})

# add_compile_options(-fpermissive)
include_regular_expression("^([^p]|p[^r]|pr[^e]|pre[^c]|prec[^o]|preco[^m]|precom[^p])")

ADD_SUBDIRECTORY(src/lib/os)
ADD_SUBDIRECTORY(src/lib/eventservice)
ADD_SUBDIRECTORY(src/lib/filecache)
ADD_SUBDIRECTORY(src/lib/app)

# Third part
ADD_SUBDIRECTORY(src/third_part/json)
ADD_SUBDIRECTORY(src/third_part/encode)

# TEST
#ADD_SUBDIRECTORY(src/test/thead_test)
#ADD_SUBDIRECTORY(src/test/tcpserver)
#ADD_SUBDIRECTORY(src/test/ns_tcpserver)
#ADD_SUBDIRECTORY(src/test/tcpclient)
#ADD_SUBDIRECTORY(src/test/httpserver)
#ADD_SUBDIRECTORY(src/test/select_test)
#ADD_SUBDIRECTORY(src/test/smart_test)
#ADD_SUBDIRECTORY(src/test/multiple_http_server)
#ADD_SUBDIRECTORY(src/test/udpserver)
#ADD_SUBDIRECTORY(src/test/udpclient)
#ADD_SUBDIRECTORY(src/test/multicast_server)
#ADD_SUBDIRECTORY(src/test/packet_client)
#ADD_SUBDIRECTORY(src/test/packet_server)
#ADD_SUBDIRECTORY(src/test/dptest)
#ADD_SUBDIRECTORY(src/test/membuffertest)
#ADD_SUBDIRECTORY(src/test/signalevent_test)
#ADD_SUBDIRECTORY(src/test/appstarup)
#ADD_SUBDIRECTORY(src/test/filecache_test)
#ADD_SUBDIRECTORY(src/test/encode_test)

ADD_SUBDIRECTORY(cmake)
MESSAGE(STATUS "06*********************************************************")
