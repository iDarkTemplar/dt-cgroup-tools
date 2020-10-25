cmake_minimum_required( VERSION 3.17.4 )

project(DT-CGroup-Tools
	VERSION 1.0.0
	LANGUAGES C)

# installation directory configuration
set(CONFIG_DIR "/etc" CACHE PATH "Config files directory")
set(EXEC_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} CACHE PATH "Installation prefix for executables and object code libraries" FORCE)
set(BIN_INSTALL_DIR ${EXEC_INSTALL_PREFIX}/bin CACHE PATH "Installation prefix for user executables" FORCE)

find_program(SETCAP_EXECUTABLE NAMES setcap DOC "The setcap executable" REQUIRED)

include(FindPkgConfig)

pkg_search_module(LUA REQUIRED lua lua5.4 lua-5.4 lua54 lua5.3 lua-5.3 lua53 lua5.2 lua-5.2 lua52 lua5.1 lua-5.1 lua51)

add_definitions(-D_FILE_OFFSET_BITS=64)
add_definitions(-DCONFIG_DIR=\"${CONFIG_DIR}\")

add_executable( dt-run-cgroup-helper dt-run-cgroup-helper.c)
target_include_directories( dt-run-cgroup-helper PRIVATE ${LUA_INCLUDE_DIRS} )
target_link_libraries( dt-run-cgroup-helper ${LUA_LIBRARIES} )

add_executable( dt-run-cgroup-notifier dt-run-cgroup-notifier.c)
target_link_libraries( dt-run-cgroup-notifier )

# installation config
install(TARGETS dt-run-cgroup-helper RUNTIME DESTINATION ${BIN_INSTALL_DIR})
install(TARGETS dt-run-cgroup-notifier RUNTIME DESTINATION ${BIN_INSTALL_DIR})
install(FILES "dt-run-cgroup.conf" DESTINATION "${CONFIG_DIR}")

install(
	CODE "execute_process(
		COMMAND
		${SETCAP_EXECUTABLE}
		CAP_SETUID=+ep
		\$ENV{DESTDIR}${BIN_INSTALL_DIR}/dt-run-cgroup-helper)"
)
