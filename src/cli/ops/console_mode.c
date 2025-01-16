/*
 * This file is part of clr-boot-manager.
 *
 * Copyright © 2024 Solus Project
 *
 * clr-boot-manager is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 */

#include <getopt.h>
#include <string.h>

#include "bootman.h"
#include "cli.h"
#include "update.h"

// See `man loader.conf`
static inline bool is_console_mode(const char *str)
{
        char list[][5] = { "", "0", "1", "2", "auto", "max", "keep" };

        for (size_t i = 0; i < sizeof list; i++) {
                if (strcmp(str, list[(int)i]) == 0) {
                        return true;
                }
        }

        return false;
}

bool cbm_command_set_console_mode(int argc, char **argv)
{
        autofree(char) *root = NULL;
        autofree(BootManager) *manager = NULL;
        bool update_efi_vars = false;

        if (!cli_default_args_init(&argc, &argv, &root, NULL, &update_efi_vars)) {
                return false;
        }

        manager = boot_manager_new();
        if (!manager) {
                DECLARE_OOM();
                return false;
        }

        boot_manager_set_update_efi_vars(manager, update_efi_vars);

        /* Use specified root if required */
        if (root) {
                if (!boot_manager_set_prefix(manager, root)) {
                        return false;
                }
        } else {
                /* Default to "/", bail if it doesn't work. */
                if (!boot_manager_set_prefix(manager, "/")) {
                        return false;
                }
        }

        if (argc != 1) {
                fprintf(stderr, "set-console-mode takes one string parameter\n");
                return false;
        }

        char *console_mode = argv[optind];

        if (!is_console_mode(console_mode)) {
                fprintf(stderr,
                        "Please provide a valid value, see `man loader.conf` or use "
                        " to disable.\n");
                return false;
        }

        if (strcmp(console_mode, "") == 0) {
                console_mode = NULL;
        }

        if (!boot_manager_set_console_mode(manager, console_mode)) {
                fprintf(stderr, "Failed to update console mode\n");
                return false;
        }

        if (console_mode == NULL) {
                fprintf(stdout, "Console mode has been removed\n");
        } else {
                fprintf(stdout, "New console mode is: %s\n", console_mode);
        }

        return cbm_command_update_do(manager, root, false);
}

bool cbm_command_get_console_mode(int argc, char **argv)
{
        autofree(char) *root = NULL;
        autofree(BootManager) *manager = NULL;
        autofree(char) *console_mode = NULL;
        bool update_efi_vars = false;

        cli_default_args_init(&argc, &argv, &root, NULL, &update_efi_vars);

        manager = boot_manager_new();
        if (!manager) {
                DECLARE_OOM();
                return false;
        }

        boot_manager_set_update_efi_vars(manager, update_efi_vars);

        /* Use specified root if required */
        if (root) {
                if (!boot_manager_set_prefix(manager, root)) {
                        return false;
                }
        } else {
                /* Default to "/", bail if it doesn't work. */
                if (!boot_manager_set_prefix(manager, "/")) {
                        return false;
                }
        }

        if (argc != 0) {
                fprintf(stderr, "get-console-mode does not take any parameters\n");
                return false;
        }

        console_mode = boot_manager_get_console_mode(manager);
        if (console_mode == NULL) {
                fprintf(stdout, "No console mode is currently configured\n");
        } else {
                fprintf(stdout, "Console mode: %s\n", console_mode);
        }
        return true;
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
