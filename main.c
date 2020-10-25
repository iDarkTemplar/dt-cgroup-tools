/*
 * Copyright (C) 2020 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of DT CGroup Tools.
 *
 * DT CGroup Tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DT CGroup Tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DT CGroup Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <pwd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifndef CONFIG_DIR
#error CONFIG_DIR is not defined
#endif /* CONFIG_DIR */

#define config_filename CONFIG_DIR "/dt-run-cgroup.conf"

int is_path_allowed(const char *path)
{
	struct passwd *pw_info;
	lua_State *lua;
	int rc;

	pw_info = getpwuid(geteuid());
	if (pw_info == NULL)
	{
		fprintf(stderr, "Error: failed to obtain username\n");
		return -1;
	}

	lua = luaL_newstate();
	if (lua == NULL)
	{
		fprintf(stderr, "Error: failed to initialize lua\n");
		return -1;
	}

	luaL_openlibs(lua);

	rc = luaL_dofile(lua, config_filename);
	if (rc != LUA_OK)
	{
		fprintf(stderr, "Error: failed to load config file %s, lua error %d\n", config_filename, rc);
		lua_close(lua);
		return -1;
	}

	lua_settop(lua, 0);

	lua_getglobal(lua, "is_directory_allowed");
	if (!lua_isfunction(lua, -1))
	{
		fprintf(stderr, "Error: lua failed to find \"is_directory_allowed\" function\n");
		lua_close(lua);
		return -1;
	}

	lua_pushstring(lua, pw_info->pw_name);
	lua_pushinteger(lua, pw_info->pw_uid);
	lua_pushstring(lua, path);

	rc = lua_pcall(lua, 3, 1, 0);
	if (rc != LUA_OK)
	{
		fprintf(stderr, "Error: lua returned error %d while checking path \"%s\"\n", rc, path);

		for (rc = 1; rc <= lua_gettop(lua); ++rc)
		{
			fprintf(stderr, "Error: lua error message: %s\n", lua_tostring(lua, rc));
		}

		lua_close(lua);
		return -1;
	}

	rc = lua_isboolean(lua, 1) && lua_toboolean(lua, 1);

	lua_close(lua);

	return rc;
}

int main(int argc, char **argv)
{
	int rc;

	if (argc < 3)
	{
		fprintf(stderr, "USAGE: %s cgroup_dir executable [args ...]\n", argv[0]);
		return -1;
	}

	rc = is_path_allowed(argv[1]);
	if (rc == 0)
	{
		fprintf(stderr, "Error: path \"%s\" is not allowed for current user\n", argv[1]);
		return -1;
	}
	else if (rc < 0)
	{
		return -1;
	}

	return 0;
}
