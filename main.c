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
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifndef CONFIG_DIR
#error CONFIG_DIR is not defined
#endif /* CONFIG_DIR */

#define config_filename CONFIG_DIR "/dt-run-cgroup.conf"

int lua_getuserinfo(lua_State *L)
{
	struct passwd *pw_info;

	pw_info = getpwuid(geteuid());
	if (pw_info == NULL)
	{
		lua_pushstring(L, "failed to obtain information about current user");
		lua_error(L);
		return 0;
	}

	lua_pushstring(L, pw_info->pw_name);
	lua_pushinteger(L, pw_info->pw_uid);
	lua_pushinteger(L, pw_info->pw_gid);

	return 3;
}

int lua_is_accessible(lua_State *L)
{
	struct stat statbuf;
	uid_t user_uid;
	gid_t user_gid;
	int rc;

	if ((lua_gettop(L) < 1) || (!lua_isstring(L, -3)) || (!lua_isinteger(L, -2)) || (!lua_isinteger(L, -1)))
	{
		lua_pushstring(L, "function requires 3 arguments");
		lua_error(L);
		return 0;
	}

	rc = stat(lua_tostring(L, -3), &statbuf);
	if (rc < 0)
	{
		lua_pushstring(L, "failed to obtain information about file");
		lua_error(L);
		return 0;
	}

	user_uid = lua_tointeger(L, -2);
	user_gid = lua_tointeger(L, -1);

	if (user_uid == statbuf.st_uid)
	{
		lua_pushboolean(L, statbuf.st_mode & S_IWUSR);
	}
	else if (user_gid == statbuf.st_gid)
	{
		lua_pushboolean(L, statbuf.st_mode & S_IWGRP);
	}
	else
	{
		lua_pushboolean(L, statbuf.st_mode & S_IWOTH);
	}

	return 1;
}

int is_path_allowed(const char *path)
{
	lua_State *lua;
	int rc;

	lua = luaL_newstate();
	if (lua == NULL)
	{
		fprintf(stderr, "Error: failed to initialize lua\n");
		return -1;
	}

	luaL_openlibs(lua);

	lua_register(lua, "getuserinfo", &lua_getuserinfo);
	lua_register(lua, "is_accessible", &lua_is_accessible);

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

	lua_pushstring(lua, path);

	rc = lua_pcall(lua, 1, 1, 0);
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
	char cgroup_file[PATH_MAX] = { 0 };
	FILE *cgroup_handle;
	uid_t original_uid;

	if (argc < 3)
	{
		fprintf(stderr, "USAGE: %s cgroup_dir executable [args ...]\n", argv[0]);
		return -1;
	}

	if (strlen(argv[1]) + sizeof("/cgroup.procs") - 1 >= PATH_MAX)
	{
		fprintf(stderr, "Error: path \"%s/cgroup.procs\" is too long\n", argv[1]);
		return -1;
	}

	if (snprintf(cgroup_file, PATH_MAX, "%s/cgroup.procs", argv[1]) < 0)
	{
		fprintf(stderr, "Error: failed to process cgroup path\n");
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

	original_uid = geteuid();
	if (seteuid(0) < 0)
	{
		fprintf(stderr, "Error: seteuid failed with error %d, %s\n", errno, strerror(errno));
		return -1;
	}

	cgroup_handle = fopen(cgroup_file, "at");
	if (cgroup_handle == NULL)
	{
		fprintf(stderr, "Error: failed to open file \"%s\" with error %d, %s\n", cgroup_file, errno, strerror(errno));
		return -1;
	}

	if (fprintf(cgroup_handle, "%llu\n", (unsigned long long) getpid()) < 0)
	{
		fprintf(stderr, "Error: failed to write file \"%s\" with error %d, %s\n", cgroup_file, errno, strerror(errno));
		fclose(cgroup_handle);
		return -1;
	}

	fclose(cgroup_handle);

	if (seteuid(original_uid) < 0)
	{
		fprintf(stderr, "Error: seteuid failed with error %d, %s\n", errno, strerror(errno));
		return -1;
	}

	execvp(argv[2], argv + 2);

	// exec failed
	fprintf(stderr, "Error: exec failed with error %d, %s\n", errno, strerror(errno));
	return -1;
}
