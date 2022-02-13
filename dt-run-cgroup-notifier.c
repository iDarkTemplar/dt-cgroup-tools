/*
 * Copyright (C) 2020-2022 i.Dark_Templar <darktemplar@dark-templar-archives.net>
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
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <linux/limits.h>

int is_populated(const char *filename)
{
	FILE *file;
	int rc = 0;
	char *buffer = NULL;
	size_t buffer_size = 0;
	ssize_t read_size;

	file = fopen(filename, "rt");
	if (file == NULL)
	{
		fprintf(stderr, "Error: failed to open file \"%s\" with error %d: %s\n", filename, errno, strerror(errno));
		return -1;
	}

	while ((read_size = getline(&buffer, &buffer_size, file)) > 0)
	{
		if (strncmp(buffer, "populated ", sizeof("populated ") - 1) == 0)
		{
			rc = (buffer[sizeof("populated ") - 1] == '1');
		}
	}

	fclose(file);

	if (buffer != NULL)
	{
		free(buffer);
	}

	return rc;
}

int main(int argc, char **argv)
{
	int inotify_fd;
	int inotify_watch_fd;
	int rc;
	struct stat statbuf;
	char events_buffer[1024 * (sizeof(struct inotify_event) + NAME_MAX + 1)];
	ssize_t read_size;
	ssize_t index;
	struct inotify_event *event;

	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s cgroup.events\n", (argc > 0) ? argv[0] : "dt-run-cgroup-notifier");
		return -1;
	}

	rc = stat(argv[1], &statbuf);
	if (rc < 0)
	{
		fprintf(stderr, "Error: stat for file \"%s\" failed with error %d: %s\n", argv[1], errno, strerror(errno));
		return -1;
	}

	if (!S_ISREG(statbuf.st_mode))
	{
		fprintf(stderr, "Error: \"%s\" is not a regular file\n", argv[1]);
		return -1;
	}

	inotify_fd = inotify_init();
	if (inotify_fd < 0)
	{
		fprintf(stderr, "Error: inotify_init failed with error %d: %s\n", errno, strerror(errno));
		return -1;
	}

	inotify_watch_fd = inotify_add_watch(inotify_fd, argv[1], IN_MODIFY);
	if (inotify_watch_fd < 0)
	{
		fprintf(stderr, "Error: inotify_add_watch failed with error %d: %s\n", errno, strerror(errno));
		close(inotify_fd);
		return -1;
	}

	rc = is_populated(argv[1]);
	if (rc < 0)
	{
		inotify_rm_watch(inotify_fd, inotify_watch_fd);
		close(inotify_fd);
		return -1;
	}

	while (rc != 0)
	{
		read_size = read(inotify_fd, events_buffer, sizeof(events_buffer));
		if (read_size < 0)
		{
			fprintf(stderr, "Error: read failed with error %d: %s\n", errno, strerror(errno));
			inotify_rm_watch(inotify_fd, inotify_watch_fd);
			close(inotify_fd);
			return -1;
		}

		for (index = 0; index < read_size; )
		{
			event = (struct inotify_event*) &(events_buffer[index]);

			if (event->mask & IN_MODIFY)
			{
				rc = is_populated(argv[1]);
				if (rc < 0)
				{
					inotify_rm_watch(inotify_fd, inotify_watch_fd);
					close(inotify_fd);
					return -1;
				}

				if (rc == 0)
				{
					break;
				}
			}

			index += sizeof(struct inotify_event) + event->len;
		}
	}

	inotify_rm_watch(inotify_fd, inotify_watch_fd);
	close(inotify_fd);

	return 0;
}
