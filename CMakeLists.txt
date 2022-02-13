cmake_minimum_required( VERSION 3.17.4 )

project(DT-CGroup-Tools
	LANGUAGES C)

include(GNUInstallDirs)

set(CONFIG_DIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}" CACHE PATH "Config files directory")

find_program(SETCAP_EXECUTABLE NAMES setcap DOC "The setcap executable" REQUIRED)

include(FindPkgConfig)

pkg_search_module(LUA REQUIRED lua lua5.4 lua-5.4 lua54 lua5.3 lua-5.3 lua53 lua5.2 lua-5.2 lua52 lua5.1 lua-5.1 lua51)

add_executable( dt-run-cgroup-helper dt-run-cgroup-helper.c)
target_include_directories( dt-run-cgroup-helper PRIVATE ${LUA_INCLUDE_DIRS} )
target_compile_definitions( dt-run-cgroup-helper PRIVATE -DCONFIG_DIR=\"${CONFIG_DIR}\" )
target_link_libraries( dt-run-cgroup-helper ${LUA_LIBRARIES} )

add_executable( dt-run-cgroup-notifier dt-run-cgroup-notifier.c)
target_link_libraries( dt-run-cgroup-notifier )

add_custom_target(scripts
	SOURCES
	dt-run-cgroup
	)

add_custom_target(configs
	SOURCES
	dt-run-cgroup.conf
	)

# installation config
install(TARGETS dt-run-cgroup-helper RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(TARGETS dt-run-cgroup-notifier RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(PROGRAMS dt-run-cgroup DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES dt-run-cgroup.conf DESTINATION ${CONFIG_DIR})

install(
	CODE "execute_process(
		COMMAND
		${SETCAP_EXECUTABLE}
		CAP_SETUID=+ep
		\$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/dt-run-cgroup-helper)"
)
