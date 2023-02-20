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
 * @file doca_sha.h
 * @page doca sha
 * @defgroup sha engine
 * DOCA SHA library. For more details please refer to the user guide on DOCA
 * devzone.
 *
 * @{
 */

#ifndef DOCA_SHA_H_
#define DOCA_SHA_H_

#include <doca_compat.h>
#include <doca_buf.h>
#include <doca_ctx.h>
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA SHA JOB TYPES
 *********************************************************************************************************************/

/**
 * Doca sha action type enums, used to specify sha job types.
 */
enum doca_sha_job_type {
	DOCA_SHA_JOB_SHA1 = DOCA_ACTION_SHA_FIRST + 1,
	DOCA_SHA_JOB_SHA256,
	DOCA_SHA_JOB_SHA512,
	DOCA_SHA_JOB_SHA1_PARTIAL,
	DOCA_SHA_JOB_SHA256_PARTIAL,
	DOCA_SHA_JOB_SHA512_PARTIAL,
};

/*********************************************************************************************************************
 * DOCA SHA JOB defines
 *********************************************************************************************************************/

/**
 * User response buffer length should be >= the following specified length, depending on the SHA function.
 * Both SHA1_PARTIAL and SHA1 need 20bytes.
 * Both SHA256_PARTIAL and SHA256 need 32bytes.
 * Both SHA512_PARTIAL and SHA512 need 64bytes.
 */
#define DOCA_SHA1_BYTE_COUNT       20			    /**< SHA1 calculation hex-format result length. */

#define DOCA_SHA256_BYTE_COUNT     32			    /**< SHA256 calculation hex-format result length. */

#define DOCA_SHA512_BYTE_COUNT     64			    /**< SHA512 calculation hex-format result length. */

/**
 * Flag enum for SHA job.
 */
enum doca_sha_job_flags {
	DOCA_SHA_JOB_FLAGS_NONE = 0,		 /**< Default flag for all SHA job. */
	DOCA_SHA_JOB_FLAGS_SHA_PARTIAL_FINAL	 /**< Only useful for doca_sha_partial_job. */
};

/**
 * DOCA SHA job definition.
 * -- "struct doca_sha_job" is used for one-shot SHA calculation.
 * -- Its typical usage is:
 *    -- construct a job: struct doca_sha_job job = {
 *                                   .base.type = DOCA_SHA_JOB_SHA1,
 *                                   .req_buf   = user_req_buf,
 *                                   .resp_buf  = user_resp_buf,
 *                                   .flags     = DOCA_SHA_JOB_FLAGS_NONE
 *                        };
 *    -- submit job:      doca_workq_submit(workq, &job.base);
 *    -- retrieve event:  doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *
 * -- For doca_workq_submit() return code:
 *    -- DOCA_SUCCESS:
 *       -- The job is submitted successfully. It also means: this submitted source data cannot be freely
 *          manipulated until its response is received.
 *    -- DOCA_ERROR_INVALID_VALUE:
 *       -- Some of the job attribute members use illegal value.
 *          for example, response buffer length is < 20bytes for SHA1; request buffer length == 0, and the job
 *          type attribute is not supported.
 *    -- DOCA_ERROR_NO_MEMORY:
 *       -- The job resouce is exhausted for now, we need to call progress_retrieve() first to receive response
 *          and free job resource, then call job_submit() to try again to submit the same job.
 *    -- DOCA_ERROR_BAD_STATE:
 *       -- sha_ctx is corrupted now, need reset.
 *
 * -- For doca_workq_progress_retrieve() return code:
 *    -- DOCA_SUCCESS:
 *       -- we get a response from SHA engine.
 *          user can utilise doca_job's user_data field to setup special data to correlate the returned event
 *          and the corresponding job.
 *    -- DOCA_ERROR_AGAIN:
 *       -- In order to get a response, we need to call progress_retrieve() again.
 *    -- DOCA_ERROR_IO_FAILED:
 *       -- abnormal occurs in the SHA engine hardware queue, sha_ctx and workq need to be re-initialized.
 *    -- DOCA_ERROR_INVALID_VALUE:
 *       -- received invalid input.
 */
struct doca_sha_job {
	struct doca_job base;		 /**< Opaque structure. */
	struct doca_buf *req_buf;	 /**< User request.
					   *  SHA engine accessible buffer pointed to the user input data.
					   *  The req_buf can be a linked_list doca_buf, so that a chained multiple bufs
					   *  can be used as valid request.
					   */
	struct doca_buf *resp_buf;	 /**< User response.
					   *  The response byte count can be decided by DOCA_SHAXXX_BYTE_COUNT macro.
					   *  The chained doca_buf is descouraged to be used as a response.
					   *  Although resp_buf can be a linked_list doca_buf, no submission failure,
					   *  but only the head element of the chained buf is used for now, because the
					   *  SHA output is no more than 64bytes.
					   */
	uint64_t flags;			 /**< SHA job flags.
					   *  For the last segment of a doca_sha_partial_job, use
					   *  DOCA_SHA_JOB_FLAGS_SHA_PARTIAL_FINAL.
					   *  Otherwise, use DOCA_SHA_JOB_FLAGS_NONE.
					   */
};

/**
 * Opaque structure representing a doca sha_partial session object.
 */
struct doca_sha_partial_session;

/**
 * DOCA SHA_PARTIAL job definition.
 * -- "struct doca_sha_partial_job" is used for stateful SHA calculation.
 * -- Its typical usage for a job composed of 3 segments is:
 *    -- get a session handle:  doca_sha_partial_session *session;
 *                              doca_sha_partial_session_create(ctx, workq, &session);
 *    -- construct the 1st job: struct doca_sha_partial_job job = {
 *                                   .sha_job.base.type        = DOCA_SHA_JOB_SHA1_PARTIAL,
 *                                   .sha_job.req_buf          = user_req_buf_of_1st_segment,
 *                                   .sha_job.resp_buf         = user_resp_buf,
 *                                   .sha_job.flags            = DOCA_SHA_JOB_FLAGS_NONE,
 *                                   .session                  = session,
 *                              };
 *    -- submit 1st segment:    doca_workq_submit(workq, &job.sha_job.base);
 *    -- retrieve 1st event:    doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *       The purpose of this call is to make sure the 1st_segment processing is finished before we can continue to
 *       send the next segment, because it is necessary to sequentially process all segment for generating correct
 *       SHA result. And the "user_resp_buf" at this moment contains garbage values.
 *    -- after the DOCA_SUCCESS event of the 1st segment is received, we can continue to submit 2nd segment:
 *    -- construct the 2nd job: struct doca_sha_partial_job job = {
 *                                   .sha_job.base.type        = DOCA_SHA_JOB_SHA1_PARTIAL,
 *                                   .sha_job.req_buf          = user_req_buf_of_2nd_segment,
 *                                   .sha_job.resp_buf         = user_resp_buf,
 *                                   .sha_job.flags            = DOCA_SHA_JOB_FLAGS_NONE,
 *                                   .session                  = session,
 *                              };
 *    -- submit 2nd segment:    doca_workq_submit(workq, &job.sha_job.base);
 *    -- retrieve 2nd event:    doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *       The purpose of this call is also to make sure the 2nd_segment processing is finished.
 *       And the "user_resp_buf" at this moment still contains garbage values.
 *    -- after the DOCA_SUCCESS event of the 2nd segment is received, we can continue to submit 3rd/final segment:
 *    -- construct the 3rd job: struct doca_sha_partial_job job = {
 *                                   .sha_job.base.type        = DOCA_SHA_JOB_SHA1_PARTIAL,
 *                                   .sha_job.req_buf          = user_req_buf_of_3rd_segment,
 *                                   .sha_job.resp_buf         = user_resp_buf,
 *                                   .sha_job.flags            = DOCA_SHA_JOB_FLAGS_SHA_PARTIAL_FINAL,
 *                                   .session                  = session,
 *                              };
 *    -- submit 3rd segment:    doca_workq_submit(workq, &job.sha_job.base);
 *    -- retrieve 3rd event:    doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *    -- After the DOCA_SUCCESS event of the 3rd segment is received, the whole job processing is done. We can get
 *       the expected SHA result from "user_resp_buf".
 *    -- release session:       doca_sha_partial_session_destroy(ctx, workq, session);
 *    -- During the whole process, please make sure to use the same "session" handle.
 *    -- And for the last segment, the "DOCA_SHA_JOB_FLAGS_SHA_PARTIAL_FINAL" flag must be set.
 *
 * -- For doca_workq_submit() return code:
 *    -- DOCA_SUCCESS:
 *       -- The job is submitted successfully. It also means: this submitted source data cannot be freely
 *          manipulated until its response is received.
 *    -- DOCA_ERROR_INVALID_VALUE:
 *       -- Some of the job attribute members use illegal value.
 *          for example, response buffer length is < 20bytes for SHA1; request buffer length == 0, and the job
 *          type attribute is not supported.
 *    -- DOCA_ERROR_NO_MEMORY:
 *       -- The job resouce is exhausted for now, we need to call progress_retrieve() first to receive response
 *          and free job resource, then call job_submit() to try again to submit the same job.
 *    -- DOCA_ERROR_BAD_STATE:
 *       -- sha_ctx is corrupted now, need reset.
 *
 * -- For doca_workq_progress_retrieve() return code:
 *    -- DOCA_SUCCESS:
 *       -- we get a response from SHA engine.
 *          user can utilise doca_job's user_data field to setup special data to correlate the returned event
 *          and the corresponding job.
 *    -- DOCA_ERROR_AGAIN:
 *       -- In order to get a response, we need to call progress_retrieve() again.
 *    -- DOCA_ERROR_IO_FAILED:
 *       -- abnormal occurs in the SHA engine hardware queue, sha_ctx and workq need to be re-initialized.
 *    -- DOCA_ERROR_INVALID_VALUE:
 *       -- received invalid input.
 *
 * Note:
 * -- sha_partial_job session requirement:
 *    -- make sure the same doca_sha_partial_session used for all segments of a whole job.
 *    -- before 1st segment submission, call doca_sha_partial_session_create() to grab a session handle.
 *    -- from the 1st to the last segment submission, always reuse the same session handle.
 *    -- after the last segment processing, to prevent a session resource leak, the user must explicitly call
 *       doca_sha_partial_session_destroy() to release this session handle.
 *    -- The doca_sha_partial_session_destroy() is provided to let user to free session handle at his will.
 *    -- If a session handle is released before the whole stateful SHA is finished, or if different handles
 *       are used for a stateful SHA, the job submission may fail due to job validity check failure; even the job
 *       submission successes, and the engine is not stalled, a wrong SHA result is expected.
 *    -- The "session" resource is limited, it is user's responsibility to make sure all allocated "session" handles
 *       are released.
 *    -- If "DOCA_SHA_JOB_FLAGS_SHA_PARTIAL_FINAL" is not properly set, the engine will not be stalled,
 *       but a wrong SHA result is expected.
 *
 * -- sha_partial_job segment length requirement:
 *    -- only the last segment allows seg-byte-count != multiple-of-64 for sha1 and sha256.
 *       For example, for the above example code, the 1st and 2nd segment byte length must be multiple of 64.
 *    -- only the last segment allows seg-byte-count != multiple-of-128 for sha512.
 *    -- If the above requirement is not met, job_submission will fail.
 */
struct doca_sha_partial_job {
	struct doca_sha_job sha_job;		 /**< A basic sha_job. */
	struct doca_sha_partial_session *session;/**< An opaque structure for user.
						   *  Used to maintain state for stateful SHA calculation.
						   */
};


/*********************************************************************************************************************
 * DOCA SHA Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a doca sha instance.
 */
struct doca_sha;

/**
 * Create a DOCA SHA instance.
 *
 * @param [out] ctx
 * Instance pointer to be created, MUST NOT BE NULL.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - not enough memory for allocation.
 * - DOCA_ERROR_NOT_SUPPORTED - the required engine is not supported.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_create(struct doca_sha **ctx);

/**
 * Destroy DOCA SHA instance.
 *
 * @param [in] ctx
 * Instance to be destroyed, MUST NOT BE NULL.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_IN_USE - the required engine resource is not released yet. Please call doca_ctx_stop().
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_destroy(struct doca_sha *ctx);

/**
 * Check if given device is capable of excuting a specific SHA job.
 *
 * @param [in] devinfo
 * The DOCA device information
 *
 * @param [in] job_type
 * SHA job_type available through this device. see enum doca_sha_job_type.
 *
 * @return
 * DOCA_SUCCESS - in case device supports job_type.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - provided devinfo does not support this SHA job.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_job_get_supported(const struct doca_devinfo *devinfo, enum doca_sha_job_type job_type);

/**
 * Get the maximum supported number of elements in a given DOCA linked-list buffer for SHA job.
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
 * - DOCA_ERROR_NOT_SUPPORTED - failed to query device capabilities.
 *                              or provided devinfo does not support the given doca_sha job.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_get_max_list_buf_num_elem(const struct doca_devinfo *devinfo, uint32_t *max_list_num_elem);

/**
 * Get maximum source buffer size for DOCA SHA job.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [out] max_buffer_size
 * The max buffer size for DOCA SHA operation in bytes.
 *
 * @return
 * DOCA_SUCCESS - upon success
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - in case of invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - failed to query device capabilities.
 *                              or provided devinfo does not support the given doca_sha job.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_get_max_src_buffer_size(const struct doca_devinfo *devinfo, uint64_t *max_buffer_size);

/**
 * Get minimum result buffer size for DOCA SHA job.
 *
 * @param [in] devinfo
 * The DOCA device information
 * @param [in] job_type
 * doca_sha job type. See enum doca_sha_job_type.
 * @param [out] min_buffer_size
 * The min buffer size for DOCA SHA operation in bytes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - failed to query device capabilities.
 *                              or provided devinfo does not support the given doca_sha job.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_get_min_dst_buffer_size(const struct doca_devinfo *devinfo,
					       enum doca_sha_job_type job_type, uint32_t *min_buffer_size);

/**
 * Get DOCA SHA operating mode: hardware or openssl_fallback.
 * First to use doca_sha_job_get_supported() to decide whether the devinfo support doca_sha engine.
 * Second to use doca_sha_get_hardware_supported() to decide whether this doca_sha engine is hardware_based
 * or openssl_fallback_based.
 *
 * @param [in] devinfo
 * The DOCA device information
 *
 * @return
 * DOCA_SUCCESS - in case of success, it is a hardware_based engine.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_SUPPORTED - failed to query device capabilities.
 *                              or provided devinfo does not support the given doca_sha job.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_get_hardware_supported(const struct doca_devinfo *devinfo);

/**
 * Convert sha instance into context for use with workQ
 *
 * @param [in] ctx
 * SHA instance. This must remain valid until after the context is no longer required.
 *
 * @return
 * Non NULL - doca_ctx object on success.
 * Error:
 * - NULL.
 */
__DOCA_EXPERIMENTAL
struct doca_ctx *doca_sha_as_ctx(struct doca_sha *ctx);

/**
 * Get a session object used for sha_partial calculation.
 * For the same sha_partial job, user need to keep using the same sha_partial_session.
 * A session object can only be used for the same sha_ctx and workq.
 * It cannot be shared between different sha_ctx or different workq.
 *
 * @param [in] ctx
 * SHA instance.
 * @param [in] workq
 * Workq instance.
 *
 * @param [out] session
 * Instance pointer to be created, MUST NOT BE NULL.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - the resource is exhausted for now, please wait the existing jobs to finish or
 *                          call doca_sha_partial_session_destroy() to release the allocated session objects.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_partial_session_create(struct doca_sha *ctx,
					     struct doca_workq *workq,
					     struct doca_sha_partial_session **session);

/**
 * Release the session object used for sha_partial calculation.
 * Please make sure the session to be released belonging to the specific sha_ctx and workq.
 *
 * @param [in] ctx
 * SHA instance.
 * @param [in] workq
 * Workq instance.
 * @param [in] session
 * A sha_partial_session object to be released.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_partial_session_destroy(struct doca_sha *ctx,
					      struct doca_workq *workq,
					      struct doca_sha_partial_session *session);

/**
 * Copy the sha_partial session state from "from_session" to "to_session".
 * This is useful if large amounts of data to be sha-calculated which share the same header segments,
 * and only differ in the tail last few bytes.
 *    For example, we have two msgs: msg_0 = {header, tail_0}, msg_1 = {header, tail_1},
 *    both of them need to be processed in sha_partial job.
 *
 *    when without session_copy functionality:
 *        we will do sha_calculation for msg_0 as:
 *            doca_sha_partial_session_create(ctx, workq, &session_0);
 *            doca_workq_submit(workq, &job_0_for_header);
 *            doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *            doca_workq_submit(workq, &job_0_for_tail_0);
 *            doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *            doca_sha_partial_session_destroy(ctx, workq, session_0);
 *        then, we will do sha_calculation for msg_1 as:
 *            doca_sha_partial_session_create(ctx, workq, &session_1);
 *            doca_workq_submit(workq, &job_1_for_header);
 *            doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *            doca_workq_submit(workq, &job_1_for_tail_1);
 *            doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *            doca_sha_partial_session_destroy(ctx, workq, session_1);
 *
 *    Obviously, the same data "header" is calculated twice!
 *
 *    when utilising the session_copy functionality,
 *        we will do sha_calculation for msg_0 as:
 *            doca_sha_partial_session_create(ctx, workq, &session_0);
 *            doca_workq_submit(workq, &job_0_for_header);
 *            doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *        do a session_copy:
 *            doca_sha_partial_session_create(ctx, workq, &session_1);
 *            doca_sha_partial_session_copy(ctx, workq, session_0, session_1);
 *        continue to finish msg_0:
 *            doca_workq_submit(workq, &job_0_for_tail_0);
 *            doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *            doca_sha_partial_session_destroy(ctx, workq, session_0);
 *        continue to finish msg_1:
 *            doca_workq_submit(workq, &job_1_for_tail_1);
 *            doca_workq_progress_retrieve(workq, &event, DOCA_WORKQ_RETRIEVE_FLAGS_NONE);
 *            doca_sha_partial_session_destroy(ctx, workq, session_1);
 *
 *
 * @param [in] ctx
 * SHA instance.
 * @param [in] workq
 * Workq instance.
 * @param [in] from_session
 * The source sha_partial_session object to be copied.
 *
 * @param [out] to_session
 * The dest sha_partial_session object.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_UNEXPECTED - received corrupted input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_sha_partial_session_copy(struct doca_sha *ctx,
					   struct doca_workq *workq,
					   const struct doca_sha_partial_session *from_session,
					   struct doca_sha_partial_session *to_session);

/** @} */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DOCA_SHA_H_ */
