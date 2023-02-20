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
 * @file doca_compress.h
 * @page DOCA COMPRESS
 * @defgroup DOCACOMPRESS DOCA COMPRESS engine
 * DOCA COMPRESS library. For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */
#ifndef DOCA_COMPRESS_H_
#define DOCA_COMPRESS_H_

#include <inttypes.h>

#include <doca_buf.h>
#include <doca_dev.h>
#include <doca_ctx.h>
#include <doca_compat.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA COMPRESS JOBS
 *********************************************************************************************************************/

/** Available jobs for DOCA COMPRESS. */
enum doca_compress_job_types {
	DOCA_COMPRESS_DEFLATE_JOB = DOCA_ACTION_COMPRESS_FIRST + 1,
	DOCA_DECOMPRESS_DEFLATE_JOB,
};

/**
 * Jobs to be dispatched via COMPRESS library.
 */
struct doca_compress_job {
	struct doca_job base;				/**< Common job data. */
	struct doca_buf *dst_buff;			/**< Destination data buffer. */
	struct doca_buf const *src_buff;		/**< Source data buffer. */
	uint64_t *output_chksum;			/**< Output checksum. If it is a compress job the
							  *  checksum calculated is of the src_buf.
							  *  If it is a decompress job the checksum result
							  *  calculated is of the dst_buf.
							  *  When the job processing will end, the output_chksum will
							  *  contain the CRC checksum result in the lower 32bit
							  *  and the Adler checksum result in the upper 32bit. */
};

/*********************************************************************************************************************
 * DOCA COMPRESS Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA COMPRESS instance.
 */
struct doca_compress;

/**
 * Create a DOCA COMPRESS instance.
 *
 * @param [out] compress
 * Pointer to pointer to be set to point to the created doca_compress instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - compress argument is a NULL pointer.
 * - DOCA_ERROR_NO_MEMORY - failed to alloc doca_compress.
 * - DOCA_ERROR_INITIALIZATION - failed to initialize a mutex.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_compress_create(struct doca_compress **compress);

/**
 * Destroy a DOCA COMPRESS instance.
 *
 * @param [in] compress
 * Pointer to instance to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_IN_USE - if unable to gain exclusive access to the compress instance
 *                       or if one or more work queues are still attached. These must be detached first.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_compress_destroy(struct doca_compress *compress);

/**
 * Check if given device is capable for given doca_compress job.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [in] job_type
 * doca_compress job type. See enum doca_compress_job_types.
 *
 * @return
 * DOCA_SUCCESS - in case the job is supported.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - failed to query device capabilities
 *                              or provided devinfo does not support the given doca_compress job.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_compress_job_get_supported(struct doca_devinfo *devinfo, enum doca_compress_job_types job_type);

/**
 * Get maximum buffer size for DOCA COMPRESS job.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [in] job_type
 * doca_compress job type. See enum doca_compress_job_types.
 * @param [out] max_buffer_size
 * The max buffer size for DOCA COMPRESS operation in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - failed to query device capabilities.
 *                              or provided devinfo does not support the given doca_compress job.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_compress_get_max_buffer_size(const struct doca_devinfo *devinfo,
					       enum doca_compress_job_types job_type, uint32_t *max_buffer_size);

/**
 * Get the maximum supported number of elements in DOCA linked-list buffer for compress job.
 *
 * @param [in] devinfo
 * The DOCA device information.
 * @param [out] max_list_num_elem
 * The maximum supported number of elements in DOCA linked-list buffer.
 * The value 1 indicates that only a single element is supported.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_compress_get_max_list_buf_num_elem(const struct doca_devinfo *devinfo, uint32_t *max_list_num_elem);

/**
 * Convert doca_compress instance into a generalised context for use with doca core objects.
 *
 * @param [in] compress
 * COMPRESS instance. This must remain valid until after the context is no longer required.
 *
 * @return
 * Non NULL upon success, NULL otherwise.
 */
__DOCA_EXPERIMENTAL
struct doca_ctx *doca_compress_as_ctx(struct doca_compress *compress);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_COMPRESS_H_ */

/** @} */
