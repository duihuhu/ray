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
 * @file doca_compat.h
 * @page compat
 * @defgroup COMPAT Compatibility Management
 *
 * Lib to define compatibility with current version, define experimental Symbols.
 *
 * To set a Symbol (or specifically a function) as experimental:
 *
 * __DOCA_EXPERIMENTAL
 * int func_declare(int param1, int param2);
 *
 * To remove warnings of experimental compile with "-D DOCA_ALLOW_EXPERIMENTAL_API"
 *
 * @{
 */

#ifndef DOCA_COMPAT_H_
#define DOCA_COMPAT_H_

#include "doca_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __linux__

#define DOCA_USED __attribute__((used))

#define DOCA_EXPORTED __attribute__((visibility("default"))) DOCA_USED

#ifndef DOCA_ALLOW_EXPERIMENTAL_API

/**
 * @brief To set a Symbol (or specifically a function) as experimental.
 */
#define __DOCA_EXPERIMENTAL                                                                                            \
	__attribute__((deprecated("Symbol is defined as experimental"), section(".text.experimental"))) DOCA_EXPORTED

#else

#define __DOCA_EXPERIMENTAL __attribute__((section(".text.experimental"))) DOCA_EXPORTED

#endif

#else /* __linux__ */

#define __attribute__(_x_)

#ifdef DOCA_EXPORTS
#define DLL_EXPORT_ATTR dllexport
#else
#define DLL_EXPORT_ATTR dllimport
#endif

#define DOCA_EXPORTED __declspec(DLL_EXPORT_ATTR)

#ifndef DOCA_ALLOW_EXPERIMENTAL_API

/**
 * @brief To set a Symbol (or specifically a function) as experimental.
 */
#define __DOCA_EXPERIMENTAL __declspec(deprecated("Symbol is defined as experimental"), DLL_EXPORT_ATTR)

#else /* DOCA_ALLOW_EXPERIMENTAL_API */

#define __DOCA_EXPERIMENTAL __declspec(DLL_EXPORT_ATTR)

#endif /* DOCA_ALLOW_EXPERIMENTAL_API */
#endif /* __linux__ */

/***************************/
/** Compatibility Helpers **/
/***************************/

#ifdef DOCA_COMPAT_HELPERS

#include <string.h>
#include <stdint.h>

#include <doca_version.h>

#define DOCA_STRUCT_START			uint32_t __doca_api_version
#define DOCA_STRUCT_GET_VERSION(_X_)		(_X_)->__doca_api_version
#define DOCA_STRUCT_CTOR(_X_)			(_X_).__doca_api_version = DOCA_CURRENT_VERSION_NUM
#define DOCA_STRUCT_PTR_CTOR(_X_) 		DOCA_STRUCT_CTOR(*(_X_))
#define DOCA_STRUCT_MEMSET_CTOR(_X_)					\
	do {								\
		memset(&(_X_), 0, sizeof(_X_));				\
		(_X_).__doca_api_version = DOCA_CURRENT_VERSION_NUM;	\
	} while (0)
#define DOCA_STRUCT_PTR_MEMSET_CTOR(_X_)	DOCA_STRUCT_MEMSET_CTOR(*(_X_))
#define DOCA_STRUCT_CTOR_LIST_START		.__doca_api_version = DOCA_CURRENT_VERSION_NUM

#endif /* DOCA_COMPAT_HELPERS */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_COMPAT_H_ */
