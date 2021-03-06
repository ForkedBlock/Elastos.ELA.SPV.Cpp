cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Define helper macro option_with_default
macro(option_with_default OPTION_NAME OPTION_STRING OPTION_DEFAULT)
	if(NOT DEFINED ${OPTION_NAME})
		set(${OPTION_NAME} ${OPTION_DEFAULT})
	endif(NOT DEFINED ${OPTION_NAME})
	option(${OPTION_NAME} "${OPTION_STRING}" ${${OPTION_NAME}})
endmacro(option_with_default OPTION_NAME OPTION_STRING OPTION_DEFAULT)

option_with_default(SPV_FOR_ANDROID "Build spv for android" OFF)
option_with_default(SPV_EXPORT_JNI "SPV export java native interface for android" OFF)

if(SPV_FOR_ANDROID)
	message(STATUS "Build spv for android")

	set(CMAKE_SYSTEM_NAME Android)
	set(CMAKE_ANDROID_STL_TYPE c++_static)
	#set(CMAKE_ANDROID_STL_TYPE c++_shared)

	if(NOT DEFINED ANDROID_NATIVE_API_LEVEL)
		set(CMAKE_SYSTEM_VERSION 23)
	else()
		set(CMAKE_SYSTEM_VERSION ${ANDROID_NATIVE_API_LEVEL})
	endif()

	if("$ENV{ANDROID_NDK}" STREQUAL "")
		message(FATAL_ERROR "Environment variable ANDROID_NDK not define")
	endif()
	set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK})
	set(CMAKE_ANDROID_NDK_TOOLCHAIN_VERSION clang)

	# can be value below: arm64-v8a armeabi-v7a armeabi mips mips64 x86 x86_64
	if(NOT DEFINED CMAKE_ANDROID_ARCH_ABI)
		set(CMAKE_ANDROID_ARCH_ABI armeabi-v7a CACHE STRING "Android ABI (architecture)" FORCE)
	endif()
	set_property(CACHE CMAKE_ANDROID_ARCH_ABI PROPERTY STRINGS arm64-v8a armeabi-v7a armeabi mips mips64 x86 x86_64)
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release Debug RelWithDebInfo MinSizeRel)


project(SPVSDK)


#set(CMAKE_VERBOSE_MAKEFILE ON)

execute_process(
	COMMAND git describe --dirty --always --tags
	COMMAND tr -d "\n"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	OUTPUT_VARIABLE SPVSDK_VERSION_MESSAGE
)

if(NOT DEFINED SPDLOG_LEVEL)
	set(SPDLOG_LEVEL debug CACHE STRING "Android ABI (architecture)" FORCE)
endif()
set_property(CACHE SPDLOG_LEVEL PROPERTY STRINGS trace debug info warning error critical off)

if(SPV_FOR_ANDROID)
	message(STATUS "Android stl type -- ${CMAKE_ANDROID_STL_TYPE}")
	message(STATUS "Android api level -- ${CMAKE_SYSTEM_VERSION}")
	message(STATUS "Android ABI (architecture) -- ${CMAKE_ANDROID_ARCH_ABI}")
endif()

if(UNIX)
	if(APPLE)
		######### MacOSX ###########
		message(STATUS "MacOSX platform detected")
		set(PLATFORM Darwin)
	elseif(ANDROID)
		message(STATUS "Android platform detected")
		set(PLATFORM Android)
	else()
		######### Unix/Linux ###########
		message(STATUS "Unix/Linux platform detected")
		set(PLATFORM Unix)
	endif()
else(UNIX)
	if(WIN32)
		######### Windows ###########
		message(STATUS "Windows system detected")
		add_definitions(-DWNT -DWIN32 -D_WINDOWS)
		set(PLATFORM win)
	else(WIN32)
		message(FATAL_ERROR "Unknown platform")
	endif(WIN32)
endif(UNIX)

# Enable Extra warnings for each compiler
if(SPV_EXTRA_WARNINGS)
	if(MSVC)
		add_definitions("/W4 /wd4100 /wd4206 /wd4127")
	endif(MSVC)
	if(CMAKE_COMPILER_IS_GNUCXX)
		add_definitions("-Wall -Wextra")
	endif(CMAKE_COMPILER_IS_GNUCXX)
	if(CLANG)
		add_definitions("-Weverything")
	endif(CLANG)
	if(INTEL)
		add_definitions("-Wall -Wremarks")
	endif(INTEL)
endif(SPV_EXTRA_WARNINGS)

if(${CMAKE_SIZEOF_VOID_P} MATCHES "8") # It is 64bit, otherwise 32 bit systems match 4
	set(BIT 64)
else (${CMAKE_SIZEOF_VOID_P} MATCHES "8")
	set(BIT 32)
endif(${CMAKE_SIZEOF_VOID_P} MATCHES "8")
message(STATUS "Build ${BIT}bit")

if(NOT DEFINED CMAKE_INSTALL_PREFIX)
	set(CMAKE_INSTALL_PREFIX "/usr/local/" CACHE PATH "Install prefix prepended to target to create install location")
endif(NOT DEFINED CMAKE_INSTALL_PREFIX)

option_with_default(SPV_BUILD_TEST_CASES "Build test cases" OFF)
option_with_default(SPV_BUILD_SAMPLE "Build sample" OFF)
option_with_default(CMAKE_EXPORT_COMPILE_COMMANDS "Export to compile_commands.json" OFF)

option_with_default(SPV_EXTRA_WARNINGS "Enable Maximum Warnings Level" OFF)

set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_RELEASE NDEBUG)
set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_MINSIZEREL NDEBUG)
set_directory_properties(PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO NDEBUG)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-DSPDLOG_DEBUG_ON)
	add_definitions(-DSPDLOG_TRACE_ON)
endif()

set(
	OTHER_INC_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	CACHE INTERNAL "config files" FORCE
)
configure_file(
	${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in
	${CMAKE_CURRENT_BINARY_DIR}/Config.h
)

add_subdirectory(ThirdParty)
add_subdirectory(Core)
add_subdirectory(SDK)
add_subdirectory(Interface)
if(SPV_FOR_ANDROID AND SPV_EXPORT_JNI)
	add_subdirectory(Java)
endif()

include_directories(
	${OTHER_INC_DIRS}
	${ThirdParty_INC_DIRS}
	${SPVCORE_INC_DIRS}
	${SPVSDK_INC_DIRS}
	${SPVJNI_INC_DIRS}
	${INTERFACE_INC_DIRS}
	${FRUIT_INC_DIRS}
)

set(
	SPVSDK_LIB_NAME
	spvsdk
)

set(SPVSDK_OBJECT_TARGET ${SPVSDK_LIB_NAME}_object)
set(SPVSDK_SHARED_TARGET ${SPVSDK_LIB_NAME}_shared CACHE INTERNAL "spv sdk shared target" FORCE)
set(SPVSDK_STATIC_TARGET ${SPVSDK_LIB_NAME}_static CACHE INTERNAL "spv sdk static target" FORCE)

add_library(${SPVSDK_OBJECT_TARGET} OBJECT
	${SPVSDK_SOURCE_FILES}
	${SPVJNI_SOURCE_FILES}
	${SPVCORE_SOURCE_FILES}
)
add_dependencies(${SPVSDK_OBJECT_TARGET} ${Boost_LIBRARIES} ${OpenSSL_LIBRARIES} sqlite bigint fruit)
set_target_properties(${SPVSDK_OBJECT_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(SPVSDK_TARGET_LIST ${SPVSDK_SHARED_TARGET} ${SPVSDK_STATIC_TARGET})

foreach(SPVSDK_TARGET ${SPVSDK_TARGET_LIST})
	if(SPVSDK_TARGET STREQUAL "${SPVSDK_STATIC_TARGET}")
		set(SPVSDK_LIBRARY_TYPE STATIC)
	else()
		set(SPVSDK_LIBRARY_TYPE SHARED)
	endif()

	add_library(${SPVSDK_TARGET} ${SPVSDK_LIBRARY_TYPE} $<TARGET_OBJECTS:${SPVSDK_OBJECT_TARGET}>)

	set_target_properties(${SPVSDK_TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
	set_target_properties(${SPVSDK_TARGET} PROPERTIES OUTPUT_NAME "${SPVSDK_LIB_NAME}")
	target_link_libraries(${SPVSDK_TARGET} dl ${Boost_LIBRARIES} ${OpenSSL_LIBRARIES} sqlite bigint fruit)
	if(ANDROID)
		target_link_libraries(${SPVSDK_TARGET} log atomic)
	endif()
endforeach()

if(SPV_BUILD_TEST_CASES)
	add_subdirectory(Test)
endif()

if(SPV_BUILD_SAMPLE)
	add_subdirectory(Sample)
endif()

file(GLOB INSTALL_HEADER_FILES "Interface/*.h")
install(TARGETS ${SPVSDK_SHARED_TARGET} ${SPVSDK_STATIC_TARGET}
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
)
install(FILES ${INSTALL_HEADER_FILES} DESTINATION include)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Interface/nlohmann/json.hpp DESTINATION include/nlohmann)

