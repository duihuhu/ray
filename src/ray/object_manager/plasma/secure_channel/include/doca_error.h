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
 * @file doca_error.h
 * @page doca error
 * @defgroup ERROR DOCA Error
 * @ingroup DOCACore
 * DOCA Error provides information regarding different errors caused while using the DOCA libraries.
 *
 * @{
 */

#ifndef DOCA_ERROR_H_
#define DOCA_ERROR_H_

#include "doca_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum doca_error {
	DOCA_SUCCESS,
	DOCA_ERROR_UNKNOWN,
	DOCA_ERROR_NOT_PERMITTED,	  /**< Operation not permitted */
	DOCA_ERROR_IN_USE,		  /**< Resource already in use */
	DOCA_ERROR_NOT_SUPPORTED,	  /**< Operation not supported */
	DOCA_ERROR_AGAIN,		  /**< Resource temporarily unavailable, try again */
	DOCA_ERROR_INVALID_VALUE,	  /**< Invalid input */
	DOCA_ERROR_NO_MEMORY,		  /**< Memory allocation failure */
	DOCA_ERROR_INITIALIZATION,	  /**< Resource initialization failure */
	DOCA_ERROR_TIME_OUT,		  /**< Timer expired waiting for resource */
	DOCA_ERROR_SHUTDOWN,		  /**< Shut down in process or completed */
	DOCA_ERROR_CONNECTION_RESET,	  /**< Connection reset by peer */
	DOCA_ERROR_CONNECTION_ABORTED,	  /**< Connection aborted */
	DOCA_ERROR_CONNECTION_INPROGRESS, /**< Connection in progress */
	DOCA_ERROR_NOT_CONNECTED,	  /**< Not Connected */
	DOCA_ERROR_NO_LOCK,		  /**< Unable to acquire required lock */
	DOCA_ERROR_NOT_FOUND,             /**< Resource Not Found */
	DOCA_ERROR_IO_FAILED,		  /**< Input/Output Operation Failed */
	DOCA_ERROR_BAD_STATE,		  /**< Bad State */
	DOCA_ERROR_UNSUPPORTED_VERSION,	  /**< Unsupported version */
	DOCA_ERROR_OPERATING_SYSTEM,	  /**< Operating system call failure */
	DOCA_ERROR_DRIVER,		  /**< DOCA Driver call failure */
	DOCA_ERROR_UNEXPECTED,		  /**< An unexpected scenario was detected */
} doca_error_t;

/**
 * \brief Save the first encountered doca_error_t.
 *
 * Updates the return value variable r to hold the first error that we encountered.
 */
#define DOCA_ERROR_PROPAGATE(r, t) \
do { \
	if (r == DOCA_SUCCESS) \
		r = t; \
} while(0);

/**
 *
 * \brief Returns the string representation of an error code name.
 *
 * Returns a string containing the name of an error code in the enum.
 * If the error code is not recognized, "unrecognized error code" is returned.
 *
 * @param[in] error - Error code to convert to string.
 *
 * @return char* pointer to a NULL-terminated string.
 */
__DOCA_EXPERIMENTAL
const char *doca_get_error_name(doca_error_t error);

/**
 * \brief Returns the description string of an error code.
 *
 *  This function returns the description string of an error code.
 *  If the error code is not recognized, "unrecognized error code" is returned.
 *
 * @param[in] error - Error code to convert to description string.
 *
 * @return char* pointer to a NULL-terminated string.
 */
__DOCA_EXPERIMENTAL
const char *doca_get_error_string(doca_error_t error);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_ERROR_H_ */
