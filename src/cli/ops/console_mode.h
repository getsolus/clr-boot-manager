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

#pragma once

#include "cli.h"

/**
 * Set the console mode to be used by the bootloader
 *
 * Set the default console mode to be used when using the "update" command.
 * This value will be used when next configuring the bootloader, and is used
 * to configure the console mode.
 *
 * See `console-mode` in `man loader.conf` for possible values.
 *
 * @param argc Must be 1.
 * @param argv Single argument matching a valid console mode.
 *
 * @return boolean indicating success or failure.
 */
bool cbm_command_set_console_mode(int argc, char **argv);

/**
 * Get the console mode to be used by the bootloader
 *
 * Get the default console mode to be used when using the "update" command.
 * This value will be used when next configuring the bootloader, and is used
 * to configure the console mode.
 *
 * See `console-mode` in `man loader.conf` for possible values.
 *
 * @param argc Must be 0.
 * @param argv None.
 *
 * @return boolean indicating success or failure.
 */
bool cbm_command_get_console_mode(int argc, char **argv);

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
