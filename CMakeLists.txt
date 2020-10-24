cmake_minimum_required( VERSION 3.17.4 )

project(DT-CGroup-Tools
	VERSION 1.0.0
	LANGUAGES C)

# installation directory configuration
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)

add_definitions(-D_FILE_OFFSET_BITS=64)

set ( SOURCES main.c )
set ( HEADERS )

add_executable( dt-run-cgroup ${SOURCES} ${HEADERS})
target_link_libraries( dt-run-cgroup )

# installation config
install(TARGETS dt-run-cgroup RUNTIME DESTINATION ${BIN_INSTALL_DIR})
