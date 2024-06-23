/*
 * This file is part of clr-boot-manager.
 *
 * Copyright © 2016-2018 Intel Corporation
 * Copyright © 2024 Solus Project
 *
 * clr-boot-manager is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "bootman.h"
#include "bootman_private.h"
#include "config.h"
#include "log.h"
#include "nica/files.h"

static bool write_sysconf_file(BootManager *self, const char *filename, const char *contents)
{
        assert(self != NULL);
        assert(self->sysconfig != NULL);

        autofree(FILE) *fp = NULL;
        autofree(char) *path = NULL;
        autofree(char) *dir = NULL;

        dir = string_printf("%s%s", self->sysconfig->prefix, KERNEL_CONF_DIRECTORY);

        if (!nc_mkdir_p(dir, 00755)) {
                LOG_ERROR("Failed to create directory %s: %s", dir, strerror(errno));
                return false;
        }

        path = string_printf("%s%s/%s", self->sysconfig->prefix, KERNEL_CONF_DIRECTORY, filename);

        if (contents == NULL) {
                /* Nothing to be done here. */
                if (!nc_file_exists(path)) {
                        return true;
                }
                if (unlink(path) < 0) {
                        LOG_ERROR("Unable to remove %s: %s", path, strerror(errno));
                        return false;
                }
                return true;
        }

        fp = fopen(path, "w");
        if (!fp) {
                LOG_FATAL("Unable to open %s for writing: %s", path, strerror(errno));
                return false;
        }

        if (fprintf(fp, "%s\n", contents) < 0) {
                LOG_FATAL("Unable to set new timeout: %s", strerror(errno));
                return false;
        }
        return true;
}

bool boot_manager_set_timeout_value(BootManager *self, int timeout)
{
        if (timeout <= 0) {
                return write_sysconf_file(self, "timeout", NULL);
        }

        autofree(char) *timeout_s = string_printf("%d", timeout);

        return write_sysconf_file(self, "timeout", timeout_s);
}

bool boot_manager_set_console_mode(BootManager *self, const char *mode)
{
        return write_sysconf_file(self, "console_mode", mode);
}

static char *read_sysconf_value(BootManager *self, const char *filename)
{
        assert(self != NULL);
        assert(self->sysconfig != NULL);

        autofree(FILE) *fp = NULL;
        autofree(char) *path = NULL;
        autofree(char) *line = NULL;
        size_t size = 0;

        path = string_printf("%s%s/%s", self->sysconfig->prefix, KERNEL_CONF_DIRECTORY, filename);
        if (!nc_file_exists(path)) {
                return NULL;
        }

        fp = fopen(path, "r");
        if (!fp) {
                LOG_FATAL("Unable to open %s for reading: %s", path, strerror(errno));
                return NULL;
        }

        __ssize_t n = getline(&line, &size, fp);
        if (n < 0) {
                LOG_ERROR("Failed to parse config file %s, using defaults", path);
                return NULL;
        }

        line[strcspn(line, "\n")] = '\0';

        return strndup(line, (size_t)n);
}

int boot_manager_get_timeout_value(BootManager *self)
{
        autofree(char) *value = read_sysconf_value(self, "timeout");
        if (value == NULL) {
                return -1;
        }

        int timeout = atoi(value);
        if (timeout <= 0) {
                LOG_ERROR("Failed to parse config file, defaulting to no timeout");
                return -1;
        }

        return timeout;
}

char *boot_manager_get_console_mode(BootManager *self)
{
        return read_sysconf_value(self, "console_mode");
}

/*
 * Editor modelines  -  https://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 expandtab:
 * :indentSize=8:tabSize=8:noTabs=true:
 */
