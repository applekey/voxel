# CMake entry point
cmake_minimum_required (VERSION 2.8)
project (voxel)

set( CMAKE_MODULE_PATH "/Users/applekey/Documents/cmakeModules/")
find_package(TBB)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLFW REQUIRED)


include_directories(
${TBB_INCLUDE_DIRS}
${OPENGL_INCLUDE_DIRS}
${GLEW_INCLUDE_DIRS}
${GLFW_INCLUDE_DIRS}
	)

set(ALL_LIBS
${TBB_LIBRARIES}
${OPENGL_LIBRARY}
${GLEW_LIBRARY}
${GLFW_LIBRARY}
	)


file(GLOB HEADERS *.h)

set(PROJECT_HEADERS
${HEADERS}
	)



set(PROJECT_FILES
	main.cpp	 
	)

add_executable(voxel ${PROJECT_FILES} ${PROJECT_HEADERS})
target_link_libraries(	voxel ${ALL_LIBS})