/*
 * Copyright (c) 2022 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
 *
 * This software product is a proprietary product of NVIDIA CORPORATION &
 * AFFILIATES (the "Company") and all right, title, and interest in and to the
 * software product, including all associated intellectual property rights, are
 * and shall remain exclusively with the Company.
 *
 * This software product is governed by the End User License Agreement
 * provided with the software product.
 *
 */
#ifndef SECURE_CHANNEL_META_CORE_H
#define SECURE_CHANNEL_META_CORE_H
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include "include/doca_comm_channel.h"
#include "include/doca_log.h"

#include "ray/object_manager/plasma/secure_channel/samples/common.h"
#include "secure_channel.h"
#define MAX_MSG_SIZE 4080		/* Comm Channel maximum message size */
#define CC_MAX_QUEUE_SIZE 10		/* Maximum amount of message in queue */

// DOCA_LOG_REGISTER(CC_CLIENT);

static bool end_sample;		/* Shared variable to allow for a proper shutdown */

/*
 * Signal handler
 *
 * @signum [in]: Signal number to handle
 */
static void
signal_handler(int signum);
/*
 * Run DOCA Comm Channel client sample
 *
 * @server_name [in]: Server Name
 * @dev_pci_addr [in]: PCI address for device
 * @text [in]: Client message
 * @return: DOCA_SUCCESS on success and DOCA_ERROR otherwise
 */
doca_error_t
create_comm_channel_client(const char *server_name, struct doca_pci_bdf *dev_pci_addr, const char *text, struct doca_comm_channel_ep_t *ep, struct doca_comm_channel_addr_t *addr);
#endif