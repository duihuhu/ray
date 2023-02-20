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

/**
 * @file doca_regex.h
 * @page doca RegEx
 * @defgroup RegEx RegEx engine
 * DOCA RegEx library. For more details please refer to the user guide on DOCA
 * devzone.
 *
 * @{
 */

#ifndef DOCA_REGEX_H_
#define DOCA_REGEX_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <doca_buf.h>
#include <doca_compat.h>
#include <doca_ctx.h>
#include <doca_dev.h>
#include <doca_error.h>
#include <doca_regex_mempool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************************************************************
 * Job types
 *********************************************************************************************************************/

/** Available job types for RegEx. */
enum doca_regex_job_types {
	/** Default RegEx search mode */
	DOCA_REGEX_JOB_SEARCH = DOCA_ACTION_REGEX_FIRST + 1,
};

/*********************************************************************************************************************
 * Job flags
 *********************************************************************************************************************/

/** Available job flags for RegEx. */
enum doca_regex_search_job_flags {
	DOCA_REGEX_SEARCH_JOB_FLAG_HIGHEST_PRIORITY_MATCH = 1 << 1,
	DOCA_REGEX_SEARCH_JOB_FLAG_STOP_ON_ANY_MATCH = 1 << 2,
};

/*********************************************************************************************************************
 * DOCA_REGEX_JOB_SEARCH
 *********************************************************************************************************************/

struct doca_regex_search_result;
struct doca_regex_match;

/**
 * Data required to dispatch a job to a RegEx engine.
 */
struct doca_regex_job_search {
	/** Common job data. */
	struct doca_job base;

	/**
	 * IDs which can be used to select which group of rules are used to process this job. Set each value to a non
	 * zero value to enable this feature or 0 to ignore it.
	 */
	uint16_t rule_group_ids[4];

	/** Data for the job. */
	struct doca_buf const *buffer;

	/**
	 * Pointer to where the job response is stored. The caller must ensure this pointer is valid when submitting a
	 * job and it must remain valid until a response for the job has been retrieved from the RegEx engine. This
	 * object will be the returned via the event.result.ptr field.
	 */
	struct doca_regex_search_result *result;

	/**
	 * Set this to 1 to allow a RegEx device to choose to aggregate jobs into batches. Batching can improve
	 * throughput at the cost of latency. Set this to 0 to force this job to begin executing immediately, this will
	 * also force any previously enqueued jobs that have been batched and not yet dispatched to begin processing.
	 * Not all devices will support batching. If a device does not have batching support this flag is ignored.
	 */
	uint8_t allow_batching;
};

/**
 * Result of a RegEx search
 */
struct doca_regex_search_result {
	/** Response flags. A bit masked field for zero or more status flags. See doca_regex_status_flag. */
	uint64_t status_flags;

	/** Total number of detected matches. */
	uint32_t detected_matches;

	/** Total number of returned matches. */
	uint32_t num_matches;

	/** Returned matches. Contains num_matches elements as a linked list. */
	struct doca_regex_match *matches;

	/** Memory pool owning the matches. */
	struct doca_regex_mempool *matches_mempool;
};

/**
 * Description of a RegEx match
 */
struct doca_regex_match {
	/** Allows matches to be linked together for easy management and iteration */
	struct doca_regex_match *next;

	/** Index relative to the start of the job / stream where the match begins */
	uint32_t match_start;

	/** ID of rule used to generate this match. */
	uint32_t rule_id;

	/** Length of matched value. */
	uint32_t length;
};

/**
 * Response status flags
 */
enum doca_regex_status_flag {
	DOCA_REGEX_STATUS_SEARCH_FAILED = 1,
};

/*********************************************************************************************************************
 * DOCA RegEx Context
 *********************************************************************************************************************/

/**
 * Opaque structure representing a DOCA RegEx instance.
 */
struct doca_regex;

/**
 * Create a DOCA RegEx instance.
 *
 * @param [out] regex
 * Pointer to be populated with the address of the newly created RegEx context.
 *
 * @return
 * DOCA_SUCCESS - RegEx instance was created
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_MEMORY - Unable to create required resources.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_create(struct doca_regex **regex);

/**
 * Destroy DOCA RegEx instance.
 *
 * @param [in] regex
 * Instance to be destroyed, MUST NOT BE NULL.
 *
 * @note
 * The context must be stopped via a successful call to doca_ctx_stop before it can be safely destroyed.
 *
 * @return
 * DOCA_SUCCESS - RegEx instance was created
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_destroy(struct doca_regex *regex);

/**
 * Convert RegEx instance into context for use with workQ
 *
 * @param [in] regex
 * The RegEx instance to convert. This must remain valid until after the context is no longer required.
 *
 * @return
 * Non NULL upon success, NULL otherwise.
 */
__DOCA_EXPERIMENTAL
struct doca_ctx *doca_regex_as_ctx(struct doca_regex *regex);

/**
 * Specify the compiled rules data to be used by the hardware RegEx device.
 *
 * @note This property is mutually exclusive with hardware un-compiled rules. This property mandates that a hardware
 * device will be attached before the context is started. If no hardware device will be provided you should not specify
 * hardware rules, or explicitly clear them by setting them to NULL. This will be validated as part of starting the
 * context
 *
 * @note The caller retains ownership of data pointed to by rules_data and is responsible for freeing it when they no
 * longer require it. The engine will make a copy of this data for its own purposes.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] rules_data
 * An opaque blob of rules data which is provided to the hardware device.
 *
 * @param [in] rules_data_size
 * Size of the blob.
 *
 * @return
 * DOCA_SUCCESS - Property was successfully set
 * Error code - in case of failure:
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_LOCK - Unable to gain exclusive control of RegEx instance.
 * DOCA_ERROR_IN_USE - RegEx instance is currently started.
 * DOCA_ERROR_NO_MEMORY - Unable to allocate memory to store a copy of the rules.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_set_hardware_compiled_rules(struct doca_regex *regex, void const *rules_data,
						    size_t rules_data_size);

/**
 * Specify the un-compiled rules data to be used by the hardware RegEx device.
 *
 * @note This property is mutually exclusive with hardware compiled rules. This property mandates that a hardware
 * device will be attached before the context is started. If no hardware device will be provided you should not specify
 * hardware rules, or explicitly clear them by setting them to NULL. This will be validated as part of starting the
 * context
 *
 * @note The caller retains ownership of data pointed to by rules_data and is responsible for freeing it when they no
 * longer require it. The engine will make a copy of this data for its own purposes.
 *
 * @note Rules compilation takes place during context start.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] rules_data
 * An opaque blob of rules data which will be compiled by the engine into compiled rules data which is then provided to
 * the hardware device.
 *
 * @param [in] rules_data_size
 * Size of the blob.
 *
 * @return
 * DOCA_SUCCESS - Property was successfully set
 * Error code - in case of failure:
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_LOCK - Unable to gain exclusive control of RegEx instance.
 * DOCA_ERROR_IN_USE - RegEx instance is currently started.
 * DOCA_ERROR_NO_MEMORY - Unable to allocate memory to store a copy of the rules.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_set_hardware_uncompiled_rules(struct doca_regex *regex, void const *rules_data,
						      size_t rules_data_size);

/**
 * Specify the compiled rules data to be used by the software RegEx device.
 *
 * @note This property is mutually exclusive with software un-compiled rules. This property mandates that a software
 * device will be attached before the context is started. If no software device will be provided you should not specify
 * software rules, or explicitly clear them by setting them to NULL. This will be validated as part of starting the
 * context
 *
 * @note The caller retains ownership of data pointed to by rules_data and is responsible for freeing it when they no
 * longer require it. The engine will make a copy of this data for its own purposes.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] rules_data
 * An opaque blob of rules data which is provided to the software device.
 *
 * @param [in] rules_data_size
 * Size of the blob.
 *
 * @return
 * DOCA_SUCCESS - Property was successfully set
 * Error code - in case of failure:
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_LOCK - Unable to gain exclusive control of RegEx instance.
 * DOCA_ERROR_IN_USE - RegEx instance is currently started.
 * DOCA_ERROR_NO_MEMORY - Unable to allocate memory to store a copy of the rules.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_set_software_compiled_rules(struct doca_regex *regex, void const *rules_data,
						    size_t rules_data_size);

/**
 * Specify the un-compiled rules data to be used by the software RegEx device.
 *
 * @note This property is mutually exclusive with software compiled rules. This property mandates that a software
 * device will be attached before the context is started. If no software device will be provided you should not specify
 * software rules, or explicitly clear them by setting them to NULL. This will be validated as part of starting the
 * context
 *
 * @note The caller retains ownership of data pointed to by rules_data and is responsible for freeing it when they no
 * longer require it. The engine will make a copy of this data for its own purposes.
 *
 * @note Rules compilation takes place during context start.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] rules_data
 * An opaque blob of rules data which will be compiled by the engine into compiled rules data which is then provided to
 * the software device.
 *
 * @param [in] rules_data_size
 * Size of the blob
 *
 * @return
 * DOCA_SUCCESS - Property was successfully set
 * Error code - in case of failure:
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_LOCK - Unable to gain exclusive control of RegEx instance.
 * DOCA_ERROR_IN_USE - RegEx instance is currently started.
 * DOCA_ERROR_NO_MEMORY - Unable to allocate memory to store a copy of the rules.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_set_software_uncompiled_rules(struct doca_regex *regex, void const *rules_data,
						      size_t rules_data_size);

/**
 * Each work queue attached to the RegEx engine gets a pool allocator for matches. Set this value to set the
 * maximum number of matches that can be stored for a given workq.
 *
 * @note The range of valid values for this property depend upon the device in use. This means that acceptance of a
 * value through this API does not ensure the value is acceptable, this will be validated as part of starting the
 * context
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] pool_size
 * Number of items to have available to each workq.
 *
 * @return
 * DOCA_SUCCESS - Property was successfully set
 * Error code - in case of failure:
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_LOCK - Unable to gain exclusive control of RegEx instance.
 * DOCA_ERROR_IN_USE - RegEx instance is currently started.
 *
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_set_workq_matches_memory_pool_size(struct doca_regex *regex, uint32_t pool_size);

/**
 * Set the size of overlap to use when a job exceeds a devices maximum search size. Defaults to 0 (no overlap)
 *
 * When a submitted job is larger than the receiving device can support it must be fragmented. This can cause
 * issues if a match exists but is split across two fragments. To remedy this an overlap size can be set so that
 * these matches may be detected. The overlap defined by this function specifies how many bytes of the previous
 * search fragment will be resent as part of the next search fragment. So for example if a 100 byte job is
 * submitted and a device supported a 32 byte maximum job length then the jobs sent would look as follows:
 *
 * Overlap size   First job   Second Job   Third Job   Fourth job   Fifth Job   Sixth Job
 * 0              [0-31]      [32-63]      [64-95]     [96-99]      ---         ---
 * 8              [0-31]      [24-55]      [42-79]     [72-99]      ---         ---
 * 16             [0-31]      [16-47]      [32-63]     [48-79]      [64-95]     [80-99]
 *
 * This allows the user to select an overlap value which provides enough overlap to detect any match they must
 * find for the lowest cost.
 *
 * @note The range of valid values for this property depend upon the device in use. This means that acceptance of a
 * value through this API does not ensure the value is acceptable, this will be validated as part of starting the
 * context
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] nb_overlap_bytes
 * Number of bytes of overlap to use.
 *
 * @return
 * DOCA_SUCCESS - Property was successfully set
 * Error code - in case of failure:
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_LOCK - Unable to gain exclusive control of RegEx instance.
 * DOCA_ERROR_IN_USE - RegEx instance is currently started.
 *
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_set_huge_job_emulation_overlap_size(struct doca_regex *regex, uint16_t nb_overlap_bytes);

/**
 * Define a threshold for "small jobs". For scenarios where small jobs cause poor performance using the hardware
 * RegEx device these can instead be redirected to the software device. Set this to a value > 0 to enable the
 * feature. Set this value to 0 to disable the feature. Defaults to 0 (disabled)
 *
 * @note This feature requires both a hardware and a software device to be available. The range of valid values for this
 * property depend upon the device in use. This means that acceptance of a value through this API does not ensure the
 * value is acceptable, this will be validated as part of starting the context
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] threshold
 * Threshold job size in bytes.
 *
 * @return
 * DOCA_SUCCESS - Property was successfully set
 * Error code - in case of failure:
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_LOCK - Unable to gain exclusive control of RegEx instance.
 * DOCA_ERROR_IN_USE - RegEx instance is currently started.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_set_small_job_offload_threshold(struct doca_regex *regex, uint16_t threshold);

/**
 * Enable the ability to automatically migrate a job which was executed on the hardware device and subsequently failed
 * to be re-executed on the software device. This is useful if a hardware limitation prevents a job from executing to
 * completion.
 *
 * @note This feature requires both a hardware and a software device to be available. Validated during context start.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] enabled
 * Specify true to enable the feature, false to disable it.
 *
 * @return
 * DOCA_SUCCESS - Property was successfully set
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_LOCK - Unable to gain exclusive control of RegEx instance.
 * DOCA_ERROR_IN_USE - RegEx instance is currently started.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_set_failed_job_fallback_enabled(struct doca_regex *regex, bool enabled);

/**
 * Get the compiled rules data to be used by the hardware RegEx device.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [out] rules_data
 * Value to populate with a pointer to an array of bytes which are a copy of the value currently stored. Caller is
 * assumes ownership of this memory and can choose to release it at any time.
 *
 * @param [out] rules_data_size
 * Size of the array pointed to by rules_data. Only valid when *rules_data != NULL.
 *
 * @return
 * DOCA_SUCCESS - rules_data and rules_data_size are populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_MEMORY - Unable to allocate memory to store a copy of the rules.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_hardware_compiled_rules(struct doca_regex const *regex, void **rules_data,
						    size_t *rules_data_size);

/**
 * Get the un-compiled rules data to be used by the hardware RegEx device.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [out] rules_data
 * Value to populate to hold a pointer to an array of bytes which are a copy of the value currently stored. Caller is
 * assumes ownership of this memory and can choose to release it at any time.
 *
 * @param [out] rules_data_size
 * Size of the array pointed to by rules_data. Only valid when *rules_data != NULL.
 *
 * @return
 * DOCA_SUCCESS - rules_data and rules_data_size are populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_MEMORY - Unable to allocate memory to store a copy of the rules.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_hardware_uncompiled_rules(struct doca_regex const *regex, void **rules_data,
						      size_t *rules_data_size);

/**
 * Get the compiled rules data to be used by the software RegEx device.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [out] rules_data
 * Value to populate with a pointer to an array of bytes which are a copy of the value currently stored. Caller is
 * assumes ownership of this memory and can choose to release it at any time.
 *
 * @param [out] rules_data_size
 * Size of the array pointed to by rules_data. Only valid when *rules_data != NULL.
 *
 * @return
 * DOCA_SUCCESS - rules_data and rules_data_size are populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_MEMORY - Unable to allocate memory to store a copy of the rules.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_software_compiled_rules(struct doca_regex const *regex, void **rules_data,
						    size_t *rules_data_size);

/**
 * Get the un-compiled rules data to be used by the software RegEx device.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [out] rules_data
 * Value to populate with a pointer to an array of bytes which are a copy of the value currently stored. Caller is
 * assumes ownership of this memory and can choose to release it at any time.
 *
 * @param [out] rules_data_size
 * Size of the array pointed to by rules_data. Only valid when *rules_data != NULL.
 *
 * @return
 * DOCA_SUCCESS - rules_data and rules_data_size are populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NO_MEMORY - Unable to allocate memory to store a copy of the rules.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_software_uncompiled_rules(struct doca_regex const *regex, void **rules_data,
						      size_t *rules_data_size);

/**
 * Get the size of work queue pool attached to workq for use with the RegEx engine.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [out] pool_size
 * Number of items that will be available to each workq.
 *
 * @return
 * DOCA_SUCCESS - pool_size is populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_workq_matches_memory_pool_size(struct doca_regex const *regex, uint32_t *pool_size);

/**
 * Get the size of overlap to use when a job exceeds a devices maximum search size.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [out] nb_overlap_bytes
 * Number of bytes of overlap in use.
 *
 * @return
 * DOCA_SUCCESS - nb_overlap_bytes is populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_huge_job_emulation_overlap_size(struct doca_regex const *regex, uint16_t *nb_overlap_bytes);

/**
 * Get the "small jobs" threshold.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] *threshold
 * Threshold job size in bytes.
 *
 * @return
 * DOCA_SUCCESS - threshold is populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_small_job_offload_threshold(struct doca_regex const *regex, uint16_t *threshold);

/**
 * Is the failed job fallback feature enabled.
 *
 * @param [in] regex
 * The RegEx engine.
 *
 * @param [in] enabled
 * Set to true when the feature is enabled, or false if it is disabled.
 *
 * @return
 * DOCA_SUCCESS - enabled is populated
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_failed_job_fallback_enabled(struct doca_regex const *regex, bool *enabled);

/**
 * Determine if a given device is suitable for use with doca_regex.
 *
 * @param [in] devinfo
 * Device to check.
 *
 * @return
 * DOCA_SUCCESS - Device can be used with doca_regex.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NOT_SUPPORTED - Device cannot be used with doca_regex.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_is_supported(struct doca_devinfo const *devinfo);

/**
 * Determine if a given device supports hardware accelerated RegEx searches.
 *
 * @param [in] devinfo
 * Device to check.
 *
 * @return
 * DOCA_SUCCESS - Hardware acceleration is supported.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NOT_SUPPORTED - Hardware acceleration is NOT supported.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_hardware_supported(struct doca_devinfo const *devinfo);

/**
 * Determine if a given device supports software based RegEx searches.
 *
 * @param [in] devinfo
 * Device to check.
 *
 * @return
 * DOCA_SUCCESS - Software support is available.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NOT_SUPPORTED - Software support is NOT available.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_software_supported(struct doca_devinfo const *devinfo);

/**
 * Determine the maximum job size supported by this device.
 *
 * @param [in] devinfo
 * Device to check.
 *
 * @param [out] max_job_len
 * Maximum supported job length.
 *
 * @return
 * DOCA_SUCCESS - max_job_len is populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NOT_SUPPORTED - Device does not support RegEx.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_maximum_job_size(struct doca_devinfo const *devinfo, uint64_t *max_job_len);

/**
 * Determine the maximum job size supported by this device without requiring the huge job emulation feature.
 *
 * @param [in] devinfo
 * Device to check.
 *
 * @param [out] max_job_len
 * Maximum supported job length.
 *
 * @return
 * DOCA_SUCCESS - max_job_len is populated.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NOT_SUPPORTED - Device does not support RegEx.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_get_maximum_non_huge_job_size(struct doca_devinfo const *devinfo, uint64_t *max_job_len);

/**
 * Determine if the given job type is supported for the given device.
 *
 * @param [in] devinfo
 * Device to check.
 *
 * @param [in] job_type
 * Job type to check.
 *
 * @return
 * DOCA_SUCCESS - Job type is supported with the given device.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NOT_SUPPORTED - Job type is NOT supported with the given device.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_job_get_supported(struct doca_devinfo const *devinfo, enum doca_regex_job_types job_type);

/**
 * Determine if 'highest priority' match is supported for the given device when submitting doca_regex_job_search jobs.
 *
 * @param [in] devinfo
 * Device to check.
 *
 * @return
 * DOCA_SUCCESS - Search jobs support highest priority match when using the given device.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NOT_SUPPORTED - highest priority match is NOT supported when using the given device.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_search_job_flag_get_highest_priority_match_supported(struct doca_devinfo const *devinfo);

/**
 * Determine if 'stop on any' match is supported for the given device when submitting doca_regex_job_search jobs.
 *
 * @param [in] devinfo
 * Device to check.
 *
 * @return
 * DOCA_SUCCESS - Search jobs support stop on any match when using the given device.
 * DOCA_ERROR_INVALID_VALUE - received invalid input.
 * DOCA_ERROR_NOT_SUPPORTED - stop on any match is NOT supported when using the given device.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_regex_search_job_flag_get_stop_on_any_match_supported(struct doca_devinfo const *devinfo);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_REGEX_H_ */
