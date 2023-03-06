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

#pragma once
#include <iostream>
#define CC_EXPORT_DESC_SIZE 500
struct BaseMetaInfo {
  unsigned long address;
  int64_t size;
  ptrdiff_t offset;
  size_t export_desc_len;
  char export_desc[CC_EXPORT_DESC_SIZE] = {0};
  BaseMetaInfo(){}
  BaseMetaInfo(unsigned long address, int64_t size, size_t export_desc_len) {
    this->address = address;
    this->size = size;
    this->export_desc_len =export_desc_len;
  }

};
// int InitConnChannel(const char *name, struct doca_comm_channel_ep_t **ep, struct doca_comm_channel_addr_t **peer_addr);


// /*
//  * Secure Channel application main function
//  *
//  * @argc [in]: command line arguments size
//  * @argv [in]: array of command line arguments
//  * @return: EXIT_SUCCESS on success and EXIT_FAILURE otherwise
//  */

// int PushMetaToDpu(const char * server_name, struct doca_comm_channel_ep_t *ep, struct doca_comm_channel_addr_t *peer_addr, \
//   absl::flat_hash_map<ray::ObjectID, std::unique_ptr<plasma::LocalObject>> *plasma_meta, std::set<std::string> &object_id_set);
// // int PushMetaToDpu(const char * server_name, struct doca_comm_channel_ep_t *ep, struct doca_comm_channel_addr_t *peer_addr);
