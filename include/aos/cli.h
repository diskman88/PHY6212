/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef AOS_CLI_H
#define AOS_CLI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <yoc/cli.h>

/**
 * This function registers a command with the command-line interface.
 *
 * @param[in]  command  The structure to register one CLI command
 *
 * @return  0 on success, error code otherwise.
 */
int aos_cli_register_command(const struct cli_command *cmd);

/**
 * This function unregisters a command from the command-line interface.
 *
 * @param[in]  command  The structure to unregister one CLI command
 *
 * @return  0 on success,  error code otherwise.
 */
int aos_cli_unregister_command(const struct cli_command *cmd);

/**
 * Register a batch of CLI commands
 * Often, a module will want to register several commands.
 *
 * @param[in]  commands      Pointer to an array of commands.
 * @param[in]  num_commands  Number of commands in the array.
 *
 * @return  0 on success， error code otherwise.
 */
int aos_cli_register_commands(const struct cli_command commands[], int num_commands);

/**
 * Unregister a batch of CLI commands
 *
 * @param[in]  commands      Pointer to an array of commands.
 * @param[in]  num_commands  Number of commands in the array.
 *
 * @return  0 on success, error code otherwise.
 */
int aos_cli_unregister_commands(const struct cli_command commands[], int num_commands);

/**
 * CLI initial function
 * @param void
 * @return  0 on success, error code otherwise
 */
int aos_cli_init(void);

/**
 * Stop the CLI thread and carry out the cleanup
 *
 * @return  0 on success, error code otherwise.
 *
 */
#define aos_cli_stop  cli_service_stop

#define aos_cli_printf printf

#endif /*AOS_CLI_H*/
