# CMake entry point
cmake_minimum_required (VERSION 2.8)
project (voxel)

set( CMAKE_MODULE_PATH "/Users/applekey/Documents/cmakeModules/")
find_package(TBB)


include_directories(
${TBB_INCLUDE_DIRS}
	)

set(ALL_LIBS
${TBB_LIBRARIES}
	)


#file(GLOB HEADERS include/*.h)

set(PROJECT_HEADERS
	)



set(PROJECT_FILES
	main.cpp	 
	)

add_executable(voxel ${PROJECT_FILES} ${PROJECT_HEADERS})
target_link_libraries(	voxel ${ALL_LIBS})