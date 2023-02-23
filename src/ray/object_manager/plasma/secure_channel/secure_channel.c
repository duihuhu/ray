/*
 * Copyright (c) 2021-2022 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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

#include <string.h>

#include "include/doca_argp.h"
#include "include/doca_log.h"

#include "utils/utils.h"

#include "secure_channel_core.h"
// #include "secure_channel.h"
#include "ray/common/id.h"
#include "ray/object_manager/plasma/common.h"
#include "secure_channel_meta_core.h"


#define MAX_TXT_SIZE 4096					/* Maximum size of input text */
#define PCI_ADDR_LEN 8						/* PCI address string length */

struct cc_config {
	char cc_dev_pci_addr[PCI_ADDR_LEN];			/* Comm Channel DOCA device PCI address */
	char cc_dev_rep_pci_addr[PCI_ADDR_LEN];			/* Comm Channel DOCA device representor PCI address */
	char text[MAX_TXT_SIZE];				/* Text to send to Comm Channel server */
};

DOCA_LOG_REGISTER(SECURE_CHANNEL);

using namespace ray;
using namespace plasma;

void InitConnChannel(char **name, struct doca_comm_channel_ep_t **ep, struct doca_comm_channel_addr_t **peer_addr) {
	struct cc_config cfg = {0};
	const char *server_name = "meta_server";
  name = server_name;
	doca_error_t result;
	struct doca_pci_bdf dev_pcie = {0};

	strcpy(cfg.cc_dev_pci_addr, "3b:00.0");


	// result = doca_argp_init("meta_client", &cfg);
	// if (result != DOCA_SUCCESS) {
	// 	DOCA_LOG_ERR("Failed to init ARGP resources: %s", doca_get_error_string(result));
	// 	return EXIT_FAILURE;
	// }

	/* Convert the PCI address into the matching struct */
	result = parse_pci_addr(cfg.cc_dev_pci_addr, &dev_pcie);
	if (result != DOCA_SUCCESS) {
		DOCA_LOG_ERR("Failed to parse the device PCI address: %s", doca_get_error_string(result));
		doca_argp_destroy();
		return EXIT_FAILURE;
	}

  	/* Start the client */
	result = create_comm_channel_client(server_name, &dev_pcie, cfg.text, ep, peer_addr);
	if (result != DOCA_SUCCESS) {
		doca_argp_destroy();
		return EXIT_FAILURE;
	}
  return;
}


/*
 * Secure Channel application main function
 *
 * @argc [in]: command line arguments size
 * @argv [in]: array of command line arguments
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */

void PushMetaToDpu(absl::flat_hash_map<ray::ObjectID, std::unique_ptr<plasma::LocalObject>> *plasma_meta) {
	struct sc_config app_cfg;
	memset(&app_cfg, 0, sizeof(app_cfg));
	struct cc_ctx ctx = {0};
	doca_error_t result;
	int exit_status = EXIT_SUCCESS;

#ifdef DOCA_ARCH_DPU
	app_cfg.mode = SC_MODE_DPU;
#endif
	DOCA_LOG_INFO("AAAAAAA");
	/* Parse cmdline/json arguments */
	result = doca_argp_init("secure_channel", &app_cfg);
	return;
}
// int
// main(int argc, char **argv)
// {
// 	struct sc_config app_cfg = {0};
// 	struct cc_ctx ctx = {0};
// 	doca_error_t result;
// 	int exit_status = EXIT_SUCCESS;

// #ifdef DOCA_ARCH_DPU
// 	app_cfg.mode = SC_MODE_DPU;
// #endif

// 	/* Parse cmdline/json arguments */
// 	result = doca_argp_init("secure_channel", &app_cfg);
// 	if (result != DOCA_SUCCESS) {
// 		DOCA_LOG_ERR("Failed to init ARGP resources: %s", doca_get_error_string(result));
// 		return EXIT_FAILURE;
// 	}
// 	result = register_secure_channel_params();
// 	if (result != DOCA_SUCCESS) {
// 		DOCA_LOG_ERR("Failed to parse register application params: %s", doca_get_error_string(result));
// 		doca_argp_destroy();
// 		return EXIT_FAILURE;
// 	}
// 	result = doca_argp_start(argc, argv);
// 	if (result != DOCA_SUCCESS) {
// 		DOCA_LOG_ERR("Failed to parse application input: %s", doca_get_error_string(result));
// 		doca_argp_destroy();
// 		return EXIT_FAILURE;
// 	}

// 	/* Start Host/DPU endpoint logic */
// 	result = sc_start(&app_cfg, &ctx);
// 	if (result != DOCA_SUCCESS) {
// 		DOCA_LOG_ERR("Failed to initialize endpoint: %s", doca_get_error_string(result));
// 		exit_status =  EXIT_FAILURE;
// 	}

// 	/* ARGP cleanup */
// 	doca_argp_destroy();

// 	return exit_status;
// }
