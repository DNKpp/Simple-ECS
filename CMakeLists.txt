cmake_minimum_required(VERSION 3.13 FATAL_ERROR)

include(CMakeDependentOption)

project(simple_ecs)

find_package(Doxygen)
CMAKE_DEPENDENT_OPTION(SIMPLE_ECS_GENERATE_DOCS "Create and install the HTML based API documentation (requires Doxygen)" ON
	"DOXYGEN_FOUND" ON
)

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

if (SIMPLE_ECS_GENERATE_DOCS)
	# check if Doxygen is installed
	find_package(Doxygen)
	if (DOXYGEN_FOUND)
		# set input and output files
		set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
		set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

		# request to configure the file
		configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)

		#SET(DOXYGEN_GENERATE_LATEX NO)

		# note the option ALL which allows to build the docs together with the application
		add_custom_target(doc_doxygen ALL
			COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			COMMENT "Generating API documentation with Doxygen"
			VERBATIM
		)

		install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html DESTINATION share/doc)
	else (DOXYGEN_FOUND)
	  message("Doxygen need to be installed to generate the doxygen documentation")
	endif (DOXYGEN_FOUND)
else ()
	message("Documentation generation skipped.")
endif()

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
