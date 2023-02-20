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
 * @file doca_version.h
 * @page version
 * @defgroup VERSION Version Management
 *
 * Define functions to get the DOCA version, and compare against it.
 *
 * @{
 */

#ifndef DOCA_VERSION_H_
#define DOCA_VERSION_H_

#include <stddef.h>

#include <doca_compat.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Major version number 0-255.
 */
#define DOCA_VER_MAJOR 1
/**
 * @brief Minor version number 0-255.
 */
#define DOCA_VER_MINOR 5
/**
 * @brief Patch version number 0-9999.
 */
#define DOCA_VER_PATCH 1007
/**
 * @brief DOCA Version String.
 */
#define DOCA_VER_STRING "1.5.1007"

/**
 * @brief Macro of version number for comparisons.
 */
#define DOCA_VERSION_NUM(major, minor, patch) ((size_t)((major) << 24 | (minor) << 16 | (patch)))

/**
 * @brief Macro of current version number for comparisons.
 */
#define DOCA_CURRENT_VERSION_NUM DOCA_VERSION_NUM(DOCA_VER_MAJOR, DOCA_VER_MINOR, DOCA_VER_PATCH)

/**
 * @brief Check if the version specified is equal to current.
 */
#define DOCA_VERSION_EQ_CURRENT(major, minor, patch) \
	(DOCA_VERSION_NUM(major, minor, patch) == DOCA_CURRENT_VERSION_NUM)

/**
 * @brief Check if the version specified is less then or equal to current.
 */
#define DOCA_VERSION_LTE_CURRENT(major, minor, patch) \
	(DOCA_VERSION_NUM(major, minor, patch) <= DOCA_CURRENT_VERSION_NUM)

/**
 * @brief Function returning DOCA's (SDK) version string.
 *
 * @return
 * version string, using the format major.minor.patch.
 *
 * @note Represents the SDK version a project was compiled with.
 */
static inline const char *doca_version(void)
{
	return DOCA_VER_STRING;
}

/**
 * @brief Function returning DOCA's (runtime) version string.
 *
 * @return
 * version string, using the format major.minor.patch.
 *
 * @note Represents the runtime version a project is linked against.
 */
__DOCA_EXPERIMENTAL
const char *doca_version_runtime(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_VERSION_H_ */
