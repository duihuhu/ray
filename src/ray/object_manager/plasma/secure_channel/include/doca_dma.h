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
 * @file doca_dma.h
 * @page DOCA DMA
 * @defgroup DOCADMA DOCA DMA engine
 * DOCA DMA library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_DMA_H_
#define DOCA_DMA_H_

#include <inttypes.h>

#include "doca_buf.h"
#include "doca_compat.h"
#include "doca_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Available jobs for DMA. */
enum doca_dma_job_types {
	DOCA_DMA_JOB_MEMCPY = DOCA_ACTION_DMA_FIRST + 1,
};

/*********************************************************************************************************************
 * DOCA DMA JOB - MEMORY COPY
 *********************************************************************************************************************/

/**
 * A job to be dispatched via the DMA library.
 */
struct doca_dma_job_memcpy {
	struct doca_job base;		 /**< Common job data */
	struct doca_buf *dst_buff;	 /**< Destination data buffer */
	struct doca_buf const *src_buff; /**< Source data buffer */
};

/**
 * Result of a DMA Memcpy job. Will be held inside the doca_event::result field.
 */
struct doca_dma_memcpy_result {
	doca_error_t result; /**< Operation result */
};

/*********************************************************************************************************************
 * DOCA DMA Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA DMA instance.
 */
struct doca_dma;

/**
 * Possible DMA device capabilities.
 */
enum doca_dma_devinfo_caps {
	DOCA_DMA_CAP_NONE = 0,
	DOCA_DMA_CAP_HW_OFFLOAD = 1U << 0, /**< DMA HW offload is supported */
};

/**
 * Create a DOCA DMA instance.
 *
 * @param [out] dma
 * Pointer to pointer to be set to point to the created doca_dma instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - dma argument is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to alloc doca_dma.
 * - DOCA_ERROR_INITIALIZATION - failed to initialise a mutex.
 *
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dma_create(struct doca_dma **dma);

/**
 * @param [in] dma
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_IN_USE - Unable to gain exclusive access to the dma instance.
 * - DOCA_ERROR_IN_USE - One or more work queues are still attached. These must be detached first.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dma_destroy(struct doca_dma *dma);

/**
 * Check if given device is capable of excuting a specific DMA job.
 *
 * @param [in] devinfo
 * The DOCA device information
 *
 * @param [in] job_type
 * DMA job_type available through this device. see enum doca_dma_job_types.
 *
 * @return
 * DOCA_SUCCESS - in case device supports job_type.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support this DMA job.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dma_job_get_supported(struct doca_devinfo *devinfo, enum doca_dma_job_types job_type);

/**
 * Convert doca_dma instance into a generalised context for use with doca core objects.
 *
 * @param [in] dma
 * DMA instance. This must remain valid until after the context is no longer required.
 *
 * @return
 * Non NULL upon success, NULL otherwise.
 */
__DOCA_EXPERIMENTAL
struct doca_ctx *doca_dma_as_ctx(struct doca_dma *dma);

/**
 * Get the maximum supported number of elements in a given DOCA linked-list buffer for DMA job.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @param [out] max_list_num_elem
 * The maximum supported number of elements in a given DOCA linked-list buffer,
 * such that 1 indicates no linked-list buffer support.
 *
 * @return
 * DOCA_SUCCESS - upon success
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dma_get_max_list_buf_num_elem(const struct doca_devinfo *devinfo, uint32_t *max_list_num_elem);

/**
 * Get the maximum supported buffer size for DMA job.
 *
 * @param [in] devinfo
 * The DOCA device information.
 *
 * @param [out] buf_size
 * The maximum supported buffer size in bytes.
 *
 * @return
 * DOCA_SUCCESS - upon success
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dma_get_max_buf_size(const struct doca_devinfo *devinfo, uint64_t *buf_size);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_DMA_H_ */

/** @} */
