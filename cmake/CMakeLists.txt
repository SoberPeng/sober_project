cmake_minimum_required(VERSION 2.8)

SET(BUILD_PROJECT_NAME "vzbase_cmake")

SET(SRC_LIST 
	${CMAKE_CURRENT_SOURCE_DIR}/global_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/himix100_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/hisi_v300_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/hisi_v500_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/ubuntu64_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/win_vs2013_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/win_vs2017_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/cmake_main.cpp
	)
SOURCE_GROUP(${BUILD_PROJECT_NAME} FILES
	${CMAKE_CURRENT_SOURCE_DIR}/global_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/himix100_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/hisi_v300_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/hisi_v500_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/ubuntu64_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/win_vs2013_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/win_vs2017_environment_settings.cmake
	${CMAKE_CURRENT_SOURCE_DIR}/cmake_main.cpp
)

ADD_executable(${BUILD_PROJECT_NAME} ${SRC_LIST})
