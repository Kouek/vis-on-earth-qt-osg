cmake_minimum_required(VERSION 3.18)

project("vis-on-earth-osg-qt" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/execution-charset:utf-8>")
# add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# <dep: Qt>
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# find_package("Qt5" COMPONENTS "Core" "Widgets" "Gui" "Charts" REQUIRED)
find_package("Qt5" COMPONENTS "Core" "Widgets" "Gui" "Charts" REQUIRED)
# </dep: Qt>

# <dep: OSG>
set(OSG_ROOT "<NOT-FOUND>" CACHE PATH "Root of OpenSceneGraph library")
set(OSG_ROOT_DBG "<NOT-FOUND>" CACHE PATH "Root of OpenSceneGraph library (Debug)")
if (${OSG_ROOT} STREQUAL "<NOT-FOUND>")
	message(FATAL_ERROR "OSG_ROOT NOT set!")
endif()
if (${OSG_ROOT_DBG} STREQUAL "<NOT-FOUND>")
	message(FATAL_ERROR "OSG_ROOT_DBG NOT set!")
endif()

list(
	APPEND
	OSG_LIBS
	"osg"
	"osgViewer"
	"osgDB"
	"osgGA"
	"osgText"
	"OpenThreads"
)

message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "CMAKE_CONFIGURATION_TYPES: ${CMAKE_CONFIGURATION_TYPES}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES STREQUAL "Debug")
	set(OSG_INC_DIR "${OSG_ROOT_DBG}/include")
	set(OSG_LIB_DIR "${OSG_ROOT_DBG}/lib")

	foreach(OSG_LIB ${OSG_LIBS})
		list(APPEND NEW_LIBS "${OSG_LIB}d")
	endforeach()
	set(OSG_LIBS ${NEW_LIBS})
else()
	set(OSG_INC_DIR "${OSG_ROOT}/include")
	set(OSG_LIB_DIR "${OSG_ROOT}/lib")
endif()
message(STATUS "OSG_LIBS: ${OSG_LIBS}")
# </dep: OSG>

# <lib: VIS4Earth>
add_compile_definitions(
	VIS4EARTH_SHADER_PREFIX="${CMAKE_CURRENT_LIST_DIR}/vis4earth/shader/"
)
file(GLOB_RECURSE VIS4EARTH_SRCS "${CMAKE_CURRENT_LIST_DIR}/vis4earth/*.cpp")
file(GLOB_RECURSE VIS4EARTH_HEADER_SRCS "${CMAKE_CURRENT_LIST_DIR}/vis4earth/*.h")
file(GLOB_RECURSE VIS4EARTH_UI_SRCS "${CMAKE_CURRENT_LIST_DIR}/vis4earth/*.ui")
add_library(
	"vis4earth"
	${VIS4EARTH_SRCS}
	${VIS4EARTH_HEADER_SRCS}
	${VIS4EARTH_UI_SRCS}
)
target_include_directories(
	"vis4earth"
	PUBLIC
	${OSG_INC_DIR}
	${CMAKE_CURRENT_LIST_DIR}
)
target_link_directories(
	"vis4earth"
	PUBLIC
	${OSG_LIB_DIR}
)
target_link_libraries(
	"vis4earth"
	PUBLIC
	"Qt5::Core"
	"Qt5::Widgets"
	"Qt5::Gui"
	"Qt5::Charts"
	${OSG_LIBS}
)
# </lib: VIS4Earth>

# <app>
set(DATA_PATH_PREFIX "<NOT-FOUND>" CACHE PATH "Prefix of data path")
if (${DATA_PATH_PREFIX} STREQUAL "<NOT-FOUND>")
 	message(FATAL_ERROR "DATA_PATH_PREFIX NOT set!")
endif()
add_compile_definitions(
	DATA_PATH_PREFIX="${DATA_PATH_PREFIX}/"
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/bin")
file(GLOB APP_PATHS "${CMAKE_CURRENT_LIST_DIR}/app/*" LIST_DIR)
foreach(APP_PATH ${APP_PATHS})
	get_filename_component(APP ${APP_PATH} NAME)
	message(STATUS "Found app: ${APP}")

	file(GLOB HEDAER_SRCS "${APP_PATH}/*.h")
	file(GLOB SRCS "${APP_PATH}/*.cpp")
	file(GLOB UI_SRCS "${APP_PATH}/*.ui")
	add_executable(
		${APP}
		${UI_SRCS} # Necessary! For UIC
		${HEDAER_SRCS} # Necessary! For header only target, Qt needs to MOC its headers
		${SRCS}
	)
	target_link_libraries(
		${APP}
		PRIVATE
		"vis4earth"
	)
endforeach()
# </app>

