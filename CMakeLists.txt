cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(simple_ecs)

add_library(simple_ecs INTERFACE)
target_include_directories(
	simple_ecs
	INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(
	simple_ecs
	INTERFACE
	cxx_std_20
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include(CTest)

	add_executable(
		test_simple_ecs
		"${CMAKE_CURRENT_SOURCE_DIR}/test/Tests.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/test/main.cpp"
	)

	target_link_libraries(
		test_simple_ecs
		PRIVATE
		simple_ecs
	)

	enable_testing()
	add_test(
		NAME Simple-ECS_TestSuite
		COMMAND test_simple_ecs
	)
endif()
