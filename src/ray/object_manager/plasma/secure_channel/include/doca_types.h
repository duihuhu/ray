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

/**
 * @file doca_types.h
 * @page doca types
 * @defgroup TYPES DOCA Types
 * @ingroup DOCACore
 * DOCA Types introduces types that are common for many libraries.
 *
 * @{
 */

#ifndef DOCA_TYPES_H_
#define DOCA_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__
	typedef int doca_event_handle_t;  // 'fd' for blocking with epoll/select/poll, event type will be "read ready"
	#define doca_event_invalid_handle -1
#else /* Windows*/
	typedef void* doca_event_handle_t;  // IOCompletionPort.
	#define doca_event_invalid_handle INVALID_HANDLE_VALUE
#endif

union doca_data {
	void *ptr;
	uint64_t u64;
};

/**
 * @brief Specifies the permission level for DOCA buffer
 *
 */
enum doca_access_flags {
	DOCA_ACCESS_LOCAL_READ    = 0,
	DOCA_ACCESS_LOCAL_WRITE   = 1,
	DOCA_ACCESS_REMOTE_WRITE  = (1 << 1),
	DOCA_ACCESS_REMOTE_READ   = (1 << 2),
	DOCA_ACCESS_REMOTE_ATOMIC = (1 << 3),
};

/**
 * @brief Specifies the PCI function type for DOCA representor device
 *
 */
enum doca_pci_func_type {
	DOCA_PCI_FUNC_PF = 0, /* physical function */
	DOCA_PCI_FUNC_VF, /* virtual function */
	DOCA_PCI_FUNC_SF, /* sub function */
};

/**
 * @brief The PCI address of a device - same as the address in lspci
 *
 */
struct doca_pci_bdf {
	#define PCI_FUNCTION_MAX_VALUE 8
	#define PCI_DEVICE_MAX_VALUE 32
	#define PCI_BUS_MAX_VALUE 256
	union {
		uint16_t raw;
		struct {
			uint16_t bus : 8;
			uint16_t device : 5;
			uint16_t function : 3;
		};
	};
};

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_TYPES_H_ */
