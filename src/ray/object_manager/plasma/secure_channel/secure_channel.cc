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
#include "dma_copy.h"

#define MAX_TXT_SIZE 4096					/* Maximum size of input text */
#define PCI_ADDR_LEN 8						/* PCI address string length */


DOCA_LOG_REGISTER(SECURE_CHANNEL);

using namespace ray;
using namespace plasma;

struct cc_config {
	char cc_dev_pci_addr[PCI_ADDR_LEN];			/* Comm Channel DOCA device PCI address */
	char cc_dev_rep_pci_addr[PCI_ADDR_LEN];			/* Comm Channel DOCA device representor PCI address */
	char text[MAX_TXT_SIZE];				/* Text to send to Comm Channel server */
};

struct MetaInfo {
  const ray::ObjectID object_id;
  const plasma::Allocation allocation;
  size_t export_desc_len;
  char export_desc[CC_MAX_MSG_SIZE];
  // MetaInfo(){}
  MetaInfo(const ray::ObjectID &object_id, const plasma::Allocation &allocation, size_t export_desc_len=0) :object_id(object_id), allocation(allocation), \
                                                        export_desc_len(export_desc_len){}
};



int InitConnChannel(const char *server_name, struct doca_comm_channel_ep_t **ep, struct doca_comm_channel_addr_t **peer_addr) {
	struct cc_config cfg = {0};
	// const char *server_name = "meta_server";
  // name = server_name;
  // *server_name = "meta_server";
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
	result = create_comm_channel_client(server_name, &dev_pcie, ep,  peer_addr);
	if (result != DOCA_SUCCESS) {
		doca_argp_destroy();
		return EXIT_FAILURE;
	}
  return EXIT_SUCCESS;
}


/*
 * Secure Channel application main function
 *
 * @argc [in]: command line arguments size
 * @argv [in]: array of command line arguments
 * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
 */

int PushMetaToDpu(const char * server_name, struct doca_comm_channel_ep_t *ep, struct doca_comm_channel_addr_t *peer_addr, absl::flat_hash_map<ray::ObjectID, std::unique_ptr<plasma::LocalObject>> *plasma_meta) {
// int PushMetaToDpu(const char * server_name, struct doca_comm_channel_ep_t *ep, struct doca_comm_channel_addr_t *peer_addr) {

  	/* Send hello message */
  doca_error_t result;
  char text[100];
  if (plasma_meta->empty())
    strcpy(text,"hello222");
  else
    strcpy(text,"hello111");

  int client_msg_len = strlen(text) + 1;
  std::cout << "PushMetaToDpu in secure channel" << std::endl;
  // MetaInfo metainfo;
  // size_t mmsg_len = sizeof(MetaInfo);
  /* Make sure peer address is valid */
  // while ((result = doca_comm_channel_peer_addr_update_info(peer_addr)) == DOCA_ERROR_CONNECTION_INPROGRESS) {
  //   // if (end_sample) {
  //   //   result = DOCA_ERROR_UNEXPECTED;
  //   //   break;
  //   // }
  //   usleep(1);
  // }
  if (result != DOCA_SUCCESS) {
    DOCA_LOG_ERR("Failed to validate the connection with the DPU: %s", doca_get_error_string(result));
    return result;
  }
  for (auto &entry : *plasma_meta) {
    // metainfo.object_id =  entry.first;
    // metainfo.allocation =  entry.second->GetAllocation();
    MetaInfo meta_info(entry.first, entry.second->GetAllocation());
    size_t amsg_len = sizeof(meta_info);
    std::cout << "hucc amsg_len " << amsg_len << std::endl;
    // ObjectID object_id = entry.first;
    // int64_t msg_len = sizeof(object_id);

    // const Allocation &allocation = entry.second->GetAllocation();
    // int64_t amsg_len = sizeof(allocation);
    char *export_desc;
    export_desc = RunDmaExport(meta_info.allocation, meta_info.export_desc_len);

    // std::cout << " amsg_len " << amsg_len << " hucc get plasma meta object id " << meta_info.object_id << " allocation information: " << meta_info.allocation.address \
    //   <<   " allocation information size: " << meta_info.allocation.size << " metainfo.export_desc: " << meta_info.export_desc \
    //   << " metainfo.export_desc_len: "<< meta_info.export_desc_len<<std::endl;

    for(int i=0; i<meta_info.export_desc_len; ++i) {
      // meta_info.export_desc[i] = export_desc[i];
      printf("%c", meta_info.export_desc[i]);
    }
    strncpy(meta_info.export_desc, export_desc, meta_info.export_desc_len);
    meta_info.export_desc[meta_info.export_desc_len] = '\0';
    printf("\n");
    printf("export_desc_len: %d\n", meta_info.export_desc_len);
    printf("\n");

    for(int i=0; i<meta_info.export_desc_len; ++i) {
      printf("%c", meta_info.export_desc[i]);
    }
    // std::cout << " allocation information: " << allocation.address << " allocation information size: " << allocation.size << std::endl;
    // result = doca_comm_channel_ep_sendto(ep, &allocation, amsg_len, DOCA_CC_MSG_FLAG_NONE, peer_addr);
    while ((result = doca_comm_channel_ep_sendto(ep, &meta_info, amsg_len, DOCA_CC_MSG_FLAG_NONE, peer_addr)) ==
          DOCA_ERROR_AGAIN) {
      usleep(1);
    }
    if (result != DOCA_SUCCESS) {
      // std::cout<< "Message was not sent: " << doca_get_error_string(result)<<std::endl;
      DOCA_LOG_ERR("Message was not sent: %s", doca_get_error_string(result));
      return EXIT_FAILURE;
    } else if (result == DOCA_SUCCESS) {
      std::cout<< "Message was sent: " << doca_get_error_string(result)<<std::endl;
    }
  }

	// result = doca_comm_channel_ep_sendto(ep, text, client_msg_len, DOCA_CC_MSG_FLAG_NONE, peer_addr);
  // while ((result = doca_comm_channel_ep_sendto(ep, text, client_msg_len, DOCA_CC_MSG_FLAG_NONE, peer_addr)) ==
	//        DOCA_ERROR_AGAIN) {
	// 	usleep(1);
	// }
	// if (result != DOCA_SUCCESS) {
  //   std::cout<< "Message was not sent: " << doca_get_error_string(result)<<std::endl;
	// 	DOCA_LOG_ERR("Message was not sent: %s", doca_get_error_string(result));
  //   return EXIT_FAILURE;
	// } else if (result == DOCA_SUCCESS) {
  //   std::cout<< "Message was sent: " << doca_get_error_string(result)<<std::endl;
  // }
  return EXIT_SUCCESS;
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
