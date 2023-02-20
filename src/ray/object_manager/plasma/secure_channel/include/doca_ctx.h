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
 * @file doca_ctx.h
 * @page doca ctx
 * @defgroup CTX DOCA Context
 * @ingroup DOCACore
 * DOCA CTX is the base class of every data-path library in DOCA.
 * It is a specific library/SDK instance object providing abstract data processing functionality.
 * The library exposes events and/or jobs that manipulate data.
 *
 * @{
 */

#ifndef DOCA_CTX_H_
#define DOCA_CTX_H_

#include <stdint.h>

#include "doca_dev.h"
#include "doca_error.h"
#include "doca_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * DOCA Job
 *********************************************************************************************************************/

/**
 * @brief Power 2 single SDK/context action type range.
 */
#define DOCA_ACTION_SDK_RANGE 16

enum doca_action_type {
	DOCA_ACTION_NONE = 0,
	DOCA_ACTION_DMA_FIRST = 1 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_DEVEMU_FIRST = 2 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_DEVEMU_BLOCK_FIRST = 3 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_VIRTIO_FIRST = 4 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_VBLK_FIRST = 5 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_REGEX_FIRST = 6 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_NVME_FIRST = 7 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_APPSHIELD_FIRST = 8 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_SHA_FIRST = 9 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_COMPRESS_FIRST = 10 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_IPSEC_FIRST = 11 << DOCA_ACTION_SDK_RANGE,
	DOCA_ACTION_RMAX_FIRST = 12 << DOCA_ACTION_SDK_RANGE,
};

enum doca_job_flags {
	DOCA_JOB_FLAGS_NONE = 0,
};

/**
 * @brief Job structure describes request arguments for service provided by context.
 *
 * A context of given type may serve one or more request types defined as action type
 * (see definition of enum doca_action_type).
 *
 * DOCA Job layout
 *
 *  SDK job --> +--------------------------+
 *              |     DOCA Job (base)      |
 *              | type                     |
 *              | flags                    |
 *              | ctx                      |
 *              | user data                |
 *              |                          |
 *              +------------+-------------+ <--  job arguments
 *              |                          |      variable size
 *              | arguments                |      SDK specific
 *              |     .                    |      structure
 *              |     .                    |
 *              |     .                    |
 *              |     .                    |
 *              |     .                    |
 *              |     .                    |
 *              |                          |
 *              +------------+-------------+
 */
struct doca_job {
	int type;		   /**< Defines the type of the job. */
	int flags;		   /**< Job submission flags (see `enum doca_job_flags`). */
	struct doca_ctx *ctx;	   /**< Doca CTX targeted by the job. */
	union doca_data user_data; /**< Job identifier provided by user. Will be returned back on completion. */
};

/**
 * @brief Activity completion event.
 *
 * Event structure defines activity completion of:
 * 1. Completion event of submitted job.
 * 2. CTX received event as a result of some external activity.
 */
struct doca_event {
	int type;		    /**< The type of the event originating activity. */
	union doca_data user_data;  /**< Defines the origin of the given event.
				      *  For events originating from submitted jobs,
				      *  this will hold the same user_data provided as part of the job.
				      *  For events originating from external activity,
				      *  refer to the documentation of the specific event type. */
	union doca_data result;	    /**< Event result defined per action type arguments.
				      *  If the result is as small as 64 bit (E.g., status or similar),
				      *  it can be accessed as result.u64.
				      *  Otherwise the data is pointed to by result.ptr,
				      *  where the size is fixed for each action type.*/
};

/*********************************************************************************************************************
 * DOCA Work Queue
 *********************************************************************************************************************/

struct doca_workq;

/**
 * @brief Creates empty DOCA WorkQ object with default attributes.
 *
 * @details The returned WorkQ needs to be added to at least one DOCA CTX.
 * Then the WorkQ can be used to progress jobs and to poll events exposed by the associated CTX.
 *
 * @param [in] depth
 * The maximum number of inflight jobs.
 * @param [out] workq
 * The newly created WorkQ.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - invalid input received.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate WorkQ.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_create(uint32_t depth, struct doca_workq **workq);

/**
 * @brief Destroy a DOCA WorkQ.
 *
 * @details In order to destroy a WorkQ, at first needs to be removed from all DOCA CTXs using it.
 *
 * @param [in] workq
 * The WorkQ to destroy.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - invalid input received.
 * - DOCA_ERROR_IN_USE - WorkQ not removed from one of the doca_ctx.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_destroy(struct doca_workq *workq);

/**
 * @brief Submit a job to a DOCA WorkQ.
 *
 * @details This method is used to submit a job to the WorkQ.
 * The WorkQ should be added to the job->ctx via doca_ctx_workq_add() before job submission.
 * Once a job has been submitted, it can be progressed using doca_workq_progress_retrieve() until the result is ready
 * and retrieved.
 *
 * @param [in] workq
 * The DOCA WorkQ used for progress and retrieval of jobs.
 * @param [in] job
 * The job to submit, the job must be compatible with the WorkQ.
 *
 * @return
 * DOCA_SUCCESS - in case the job was submitted successfully, doca_workq_progress_retrieve() can be called next.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - in case job->ctx is stopped.
 * - DOCA_ERROR_NO_MEMORY - in case the queue is full. See WorkQ depth.
 * - DOCA_ERROR_NOT_FOUND - in case the ctx is not associated to the workQ.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_submit(struct doca_workq *workq, struct doca_job const *job);

/**
 * @brief Arm the WorkQ to receive next completion event.
 *
 * @details This method should be used before waiting on the event handle.
 * The expected flow is as follows:
 * 1. Enable event driven mode using doca_workq_set_event_driven_enable().
 * 2. Get event handle using doca_workq_get_event_handle().
 * 3. Arm the workq.
 * 4. Wait for an event using the event handle. E.g., using epoll_wait().
 * 5. Once the thread wakes up, call doca_workq_event_handle_clear().
 * 6. Call doca_workq_progress_retrieve() until an event is retrieved.
 * 7. Repeat 3.
 *
 * @param [in] workq
 * The WorkQ object to arm.
 *
 * @return
 * - DOCA_SUCCESS - workq has been successfully armed, event handle can be used to wait on events.
 * - DOCA_ERROR_BAD_STATE - event driven mode is not enabled. Try doca_workq_set_event_driven_enable().
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_event_handle_arm(struct doca_workq *workq);

/**
 * @brief Clear triggered events.
 *
 * @details Method used for clearing of events, this method should be called after an event has been received using the
 * event handle. After this is called, the events will no longer be triggered, and the handle can be armed again.
 * See doca_workq_event_handle_arm() for entire flow.
 *
 * @param [in] workq
 * The WorkQ object that received the events.
 * @param [in] handle
 * workq event handle.
 *
 * @return
 * - DOCA_SUCCESS - on successfuly clearing triggered events.
 * - DOCA_ERROR_BAD_STATE - event driven mode is not enabled. Try doca_workq_set_event_driven_enable().
 * - DOCA_ERROR_OPERATING_SYSTEM - a system call has failed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_event_handle_clear(struct doca_workq *workq, doca_event_handle_t handle);

enum doca_workq_retrieve_flags {
	/**< Placeholder for flags argument of doca_workq_progress_retrieve(). */
	DOCA_WORKQ_RETRIEVE_FLAGS_NONE,
};

/**
 * @brief Progress & retrieve single pending event.
 *
 * @details Polling method for progress of submitted jobs and retrieval of events.
 *
 * NOTE: for V1 retrieve supported for single event only.
 *
 * @param [in] workq
 * The WorkQ object to poll for events.
 * @param [out] ev
 * Event structure to be filled in case an event was received.
 * @param [in] flags
 * Flags for progress/retrival operations. A combination of enum doca_workq_retrieve_flags.
 *
 * @return
 * - DOCA_SUCCESS - on successful event retrieval. ev output argument is set.
 * - DOCA_ERROR_AGAIN - no event available (ev output argument not set), try again to make more progress.
 * - DOCA_ERROR_IO_FAILED - the retrieved event is a failure event. The specific error is reported per action type.
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_progress_retrieve(struct doca_workq *workq, struct doca_event *ev, int flags);

/*********************************************************************************************************************
 * DOCA Work Queue properties
 *********************************************************************************************************************/

/**
 * @brief Get the ctx maximum number of contexts allowed within an application.
 *
 * @param [out] max_num_ctx
 * The ctx max number of contexts allowed within an application.
 * @return
 * DOCA_SUCCESS - in case max_num_ctx received the required value properly.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - max_num_ctx is NULL.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_ctx_get_max_num_ctx(uint32_t *max_num_ctx);

/**
 * @brief Get the maximum number of inflight jobs allowed for a DOCA workq.
 *
 * @param [in] workq
 * The DOCA WorkQ.
 * @param [out] depth
 * The maximum number of inflight jobs allowed for workq.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_get_depth(const struct doca_workq *workq, uint32_t *depth);

/**
 * @brief Check if WorkQ event-driven mode is enabled.
 *
 * @details
 * Event-driven mode is not enabled by default.
 * It is possible to enable it by setting this porperty to 1. Using doca_workq_set_event_driven_enable()
 *
 * @param [in] workq
 * The WorkQ to query.
 * @param [out] enabled
 * 0 or 1 indicating if event-driven mode is enabled.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_get_event_driven_enable(struct doca_workq *workq, uint8_t *enabled);

/**
 * @brief Get the event handle for waiting on events.
 *
 * @details
 * Calling this for the first time will enable event-driven mode for the WorkQ.
 * Retrieves the event handle of the WorkQ, the handle does not change throughout the lifecycle of the WorkQ.
 *
 * @param [in] workq
 * The WorkQ to query.
 * @param [out] handle
 * The event handle of the WorkQ.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - event driven mode is not enabled. Try doca_workq_set_event_driven_enable().
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_get_event_handle(struct doca_workq *workq, doca_event_handle_t *handle);

/**
 * @brief Set the maximum number of inflight jobs allowed for a DOCA WorkQ to a given value.
 *
 * @param [in] workq
 * The DOCA WorkQ.
 * @param [in] depth
 * The new maximum number of inflight jobs allowed for workq.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_set_depth(struct doca_workq *workq, uint32_t depth);

/**
 * @brief Enable WorkQ event-driven mode.
 *
 * @details
 * Event-driven mode is not enabled by default.
 * Once enabled, the doca_workq_handle_* APIs can be used in order to wait on events.
 * This mode can only be enabled before adding the WorkQ to any CTX.
 *
 * @param [in] workq
 * The WorkQ to query.
 * @param [in] enable
 * 0 or 1 indicating whether to enable event-driven mode or not.
 *
 * @return
 * DOCA_SUCCESS - in case event driven mode has been set, or is already set to same value.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - workq is still added to at least 1 CTX.
 * - DOCA_ERROR_OPERATING_SYSTEM - a system call has failed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_workq_set_event_driven_enable(struct doca_workq *workq, uint8_t enable);

/*********************************************************************************************************************
 * DOCA Context
 *********************************************************************************************************************/

struct doca_ctx;

/**
 * @brief Add a device to a DOCA CTX.
 *
 * @param [in] ctx
 * The CTX to add the device to.
 * @param [in] dev
 * The device to add.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - On failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - CTX is started.
 * - DOCA_ERROR_IN_USE - the device was already added.
 * - DOCA_ERROR_NOT_SUPPORTED - the provided device is not supported by CTX,
 * 				I.e., the device is not useful for any job, or missing the capabilities.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_ctx_dev_add(struct doca_ctx *ctx, struct doca_dev *dev);

/**
 * @brief Remove a device from a context.
 *
 * @param [in] ctx
 * The CTX to remove the device from. Must already hold the device.
 * @param [in] dev
 * The device to remove.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - On failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - CTX is started.
 * - DOCA_ERROR_NOT_FOUND - the provided device was never added to the ctx or was already removed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_ctx_dev_rm(struct doca_ctx *ctx, struct doca_dev *dev);

/**
 * @brief Finalizes all configurations, and starts the DOCA CTX.
 *
 * @details After starting the CTX, it can't be configured any further.
 * Use doca_ctx_stop in order to reconfigure the CTX.
 *
 * The following become possible only after start:
 * - Adding WorkQ to CTX using doca_ctx_workq_add()
 * - Removing WorkQ from CTX using doca_ctx_workq_rm()
 * - Submitting a job using doca_workq_submit()
 *
 * The following are NOT possible after start and become possible again after calling doca_ctx_stop:
 * - Adding device to CTX using doca_ctx_dev_add()
 * - Removing device from CTX using doca_ctx_dev_rm()
 *
 * @param [in] ctx
 * The DOCA context to start.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - either an invalid input was received or no devices were added to the CTX.
 * - DOCA_ERROR_NOT_SUPPORTED - one of the provided devices is not supported by CTX.
 * - DOCA_ERROR_INITIALIZATION - resource initialization failed (could be due to allocation failure),
 * 				 or the device is in a bad state or another reason caused initialization to fail.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_ctx_start(struct doca_ctx *ctx);

/**
 * @brief Stops the context allowing reconfiguration.
 *
 * @details Once a context has started, it can't be configured any further.
 * This method should be called in case the context needs to be configured after starting.
 * For more details see doca_ctx_start().
 *
 * @param [in] ctx
 * The DOCA context to stop.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - In case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_PERMITTED - either some jobs are still pending or not all WorkQs have been removed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_ctx_stop(struct doca_ctx *ctx);

/**
 * @brief Add a workQ to a context.
 *
 * @details This method adds a WorkQ to a context.
 * Once a WorkQ has been added it will start accepting jobs defined by the CTX & retrieve events from the CTX.
 * The jobs can be progressed using doca_workq_progress_retrieve().
 *
 * @param [in] ctx
 * The library instance that will handle the jobs.
 * @param [in] workq
 * The WorkQ where you want to receive job completions.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - on failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - CTX is not started.
 * - DOCA_ERROR_IN_USE - same WorkQ already added.
 * - DOCA_ERROR_NO_MEMORY - memory allocation failed.
 * - DOCA_ERROR_INITIALIZATION - initialization of WorkQ failed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_ctx_workq_add(struct doca_ctx *ctx, struct doca_workq *workq);

/**
 * @brief Remove a DOCA WorkQ from a DOCA CTX.
 *
 * @details This function can only be used after CTX is started (doca_ctx_start()).
 *
 * @param [in] ctx
 * The library instance containing the WorkQ.
 * @param [in] workq
 * The WorkQ to remove.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - on failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_BAD_STATE - CTX is not started.
 * - DOCA_ERROR_NOT_PERMITTED - some jobs are still pending completion.
 * - DOCA_ERROR_NOT_FOUND - WorkQ does not exist within CTX
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_ctx_workq_rm(struct doca_ctx *ctx, struct doca_workq *workq);

/**
 * @brief Check if CTX supports event driven mode.
 *
 * @details In case the support exists, then this CTX can be added to WorkQ operating in event driven mode.
 *
 * @param [in] ctx
 * The library instance containing the WorkQ.
 * @param [in] event_supported
 * Boolean indicating whether event driven mode is supported.
 *
 * @return
 * DOCA_SUCCESS - In case of success.
 * Error code - on failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_ctx_get_event_driven_supported(struct doca_ctx *ctx, uint8_t *event_supported);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_CTX_H_ */
