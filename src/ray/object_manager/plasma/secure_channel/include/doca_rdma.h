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
 * @file doca_rdma.h
 * @page doca rdma
 * @defgroup DOCA_RDMA DOCA RDMA
 * @ingroup DOCACore
 *
 * DOCA RDMA bridge.
 *
 * @{
 */

#ifndef DOCA_RDMA_H_
#define DOCA_RDMA_H_

#ifdef __linux__
	#include <infiniband/verbs.h>
#else // Windows
	#define UM_MAX_ERRNO 999
	#define UM_ENOMEM 12
	#define __DEVX_IFC_H__

	typedef uint16_t __be16;
	typedef uint32_t __be32;
	typedef uint64_t __be64;
	typedef uint16_t u16;
	typedef uint32_t u32;
	typedef uint64_t u64;

	#include <windows.h>
	#include <mlx5verbs.h>
#endif // __linux__

#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_dev;

/**
 * @brief Get the protection domain associated with a DOCA device.
 *
 * @param [in] dev
 * DOCA device to get the pd from.
 * @param [out] pd
 * The protection-domain associated with the given DOCA device.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 * - DOCA_ERROR_BAD_STATE - in case the device's pd is not valid (bad state)
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dev_get_pd(const struct doca_dev *dev, struct ibv_pd **pd);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_RDMA_H_ */
