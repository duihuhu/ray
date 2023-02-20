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
 * @file doca_dpdk.h
 * @page doca dpdk
 * @defgroup DOCA_DPDK DOCA DPDK
 * @ingroup DOCACore
 *
 * DOCA API for integration with DPDK.
 *
 * @{
 */

#ifndef DOCA_DPDK_H_
#define DOCA_DPDK_H_

#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

struct doca_dev;

/**
 * @brief Attach a DPDK port specified by DOCA device.
 *
 * Thread unsafe API.
 *
 * It's the user responsibility to set the DPDK EAL initialization to
 * skip probing the PCI device associated with the given DOCA device
 * to prevent EAL from using it.
 *
 * No initialization is done for the probed PDPK port and the port is not started.
 *
 *
 * @param [in] dev
 * DOCA device to attach PDK port for.
 * @param [in] devargs
 * DPDK devargs style - must NOT contains the device's PCI address ([domain:]bus:devid.func).
 * @param [out] port_id
 * The attached DPDK port identifier.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 * - DOCA_ERROR_DRIVER - in case of DPDK error during DPDK port attach.
 * - DOCA_ERROR_NO_MEMORY - in case of memory allocation failure.
 * TODO: complete error documentation
 */

__DOCA_EXPERIMENTAL
doca_error_t doca_dpdk_port_probe(struct doca_dev *dev, const char *devargs);

/**
 * @brief Return the DOCA device associated with a DPDK port.
 *
 * @param [in] port_id
 * The DPDK port identifier to get the associated DOCA device for.
 * @param [out] dev
 * The DPDK DOCA device associated with the given DPDK port identifier.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 * - DOCA_ERROR_NOT_FOUND - in case there is no such DPDK port associated with a DOCA device.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dpdk_port_as_dev(uint16_t port_id, struct doca_dev **dev);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DPDK_H_ */
