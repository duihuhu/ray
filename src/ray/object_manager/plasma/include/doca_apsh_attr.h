/*
 * Copyright (c) 2021 NVIDIA CORPORATION & AFFILIATES, ALL RIGHTS RESERVED.
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
 * @file doca_apsh_attr.h
 * @page apsh_attr
 * @defgroup DOCA_APSH_ATTR App Shield Attributes
 *
 * DOCA App Shield attributes to query with get functions, see doca_apsh.h
 *
 * @{
 */

#ifndef _DOCA_APSH_ATTR__H_
#define _DOCA_APSH_ATTR__H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief system os types
 */
enum doca_apsh_system_os
{
	DOCA_APSH_SYSTEM_LINUX,	     				/**< linux */
	DOCA_APSH_SYSTEM_WINDOWS,    				/**< windows */
};

/**
 * @brief doca app shield configuration attributes
 */
enum doca_apsh_system_config_attr {
	DOCA_APSH_OS_SYMBOL_MAP,				/**< os symbol map path */
	DOCA_APSH_MEM_REGION,					/**< memory region path */
	DOCA_APSH_KPGD_FILE,					/**< kpgd file path */
	DOCA_APSH_VHCA_ID,					/**< vhca id */
	DOCA_APSH_OS_TYPE,					/**< os type */
	DOCA_APSH_HASHTEST_LIMIT,				/**< limit of vm areas to attest */
	DOCA_APSH_MODULES_LIMIT,				/**< limit of modules number */
	DOCA_APSH_PROCESS_LIMIT,				/**< limit of processes number */
	DOCA_APSH_THREADS_LIMIT,				/**< limit of threads number */
	DOCA_APSH_LDRMODULES_LIMIT,				/**< limit of ldrmodules number on windows */
	DOCA_APSH_LIBS_LIMIT,					/**< limit of libs number */
	DOCA_APSH_VADS_LIMIT,					/**< limit of vads number */
	DOCA_APSH_WINDOWS_ENVARS_LIMIT,				/**< length limit of envars for windows */
	DOCA_APSH_HANDLES_LIMIT,				/**< limit of handles number on windows */
	DOCA_APSH_STRING_LIMIT					/**< length limit of apsh_read_str */
};

/** @brief dma dev name */
typedef struct doca_dev *DOCA_APSH_DMA_DEV_TYPE;
/** @brief regex dev name */
typedef char *DOCA_APSH_REGEX_DEV_TYPE;
/** @brief os symbol map path */
typedef char *DOCA_APSH_OS_SYMBOL_MAP_TYPE;
/** @brief memory region path */
typedef char *DOCA_APSH_MEM_REGION_TYPE;
/** @brief kpgd file path */
typedef char *DOCA_APSH_KPGD_FILE_TYPE;
/** @brief vhca id */
typedef struct doca_dev_rep *DOCA_APSH_VHCA_ID_TYPE;
/** @brief os type */
typedef enum doca_apsh_system_os DOCA_APSH_OS_TYPE_TYPE;
/** @brief limit of vm areas to attest */
typedef int DOCA_APSH_HASHTEST_LIMIT_TYPE;
/** @brief llimit of modules number */
typedef int DOCA_APSH_MODULES_LIMIT_TYPE;
/** @brief limit of processes number */
typedef int DOCA_APSH_PROCESS_LIMIT_TYPE;
/** @brief limit of threads number */
typedef int DOCA_APSH_THREADS_LIMIT_TYPE;
/** @brief limit of libs number */
typedef int DOCA_APSH_LIBS_LIMIT_TYPE;
/** @brief limit of vads number */
typedef int DOCA_APSH_VADS_LIMIT_TYPE;
/** @brief length limit of envars for windows */
typedef int DOCA_APSH_WINDOWS_ENVARS_LIMIT_TYPE;
/** @brief length limit of apsh_read_str */
typedef int DOCA_APSH_STRING_LIMIT_TYPE;

/**
 * @brief doca app shield process attributes
 */
enum doca_apsh_process_attr{
	DOCA_APSH_PROCESS_PID, 					/**< process id */
	DOCA_APSH_PROCESS_PPID,					/**< process parent id */
	DOCA_APSH_PROCESS_COMM,					/**< process executable name */
	DOCA_APSH_PROCESS_STATE,				/**< process state */
	DOCA_APSH_PROCESS_CPU_TIME,				/**< process cpu time [ps]*/
	DOCA_APSH_PROCESS_WINDOWS_OFFSET = 1000,		/**< process offset */
	DOCA_APSH_PROCESS_WINDOWS_THREADS,			/**< process thread count */
	DOCA_APSH_PROCESS_LINUX_GID = 2000,			/**< process group id */
	DOCA_APSH_PROCESS_LINUX_UID,				/**< process user id */
};

/** @brief process pid type */
typedef unsigned int DOCA_APSH_PROCESS_PID_TYPE;
/** @brief process pid type */
typedef unsigned int DOCA_APSH_PROCESS_PPID_TYPE;
/** @brief process comm type */
typedef const char* DOCA_APSH_PROCESS_COMM_TYPE;
/** @brief process state type */
typedef long DOCA_APSH_PROCESS_STATE_TYPE;
/** @brief process cpu time type */
typedef uint64_t DOCA_APSH_PROCESS_CPU_TIME_TYPE;
/** @brief process offset type */
typedef uint64_t DOCA_APSH_PROCESS_WINDOWS_OFFSET_TYPE;
/** @brief process threads type */
typedef int DOCA_APSH_PROCESS_WINDOWS_THREADS_TYPE;
/** @brief process gid type */
typedef unsigned int DOCA_APSH_PROCESS_LINUX_GID_TYPE;
/** @brief process uid type */
typedef unsigned int DOCA_APSH_PROCESS_LINUX_UID_TYPE;

/**
 * @brief doca app shield thread attributes
 */
enum doca_apsh_thread_attr{
	DOCA_APSH_THREAD_PID,					/**< thread process id */
	DOCA_APSH_THREAD_TID,					/**< thread id */
	DOCA_APSH_THREAD_STATE,					/**< thread state */
	DOCA_APSH_THREAD_WINDOWS_WAIT_REASON = 1000,		/**< thread wait reason */
	DOCA_APSH_THREAD_WINDOWS_OFFSET,			/**< thread offset */
	DOCA_APSH_THREAD_LINUX_PROC_NAME = 2000,		/**< thread process name */
	DOCA_APSH_THREAD_LINUX_THREAD_NAME,			/**< thread name */
};

/** @brief thread pid type */
typedef unsigned int DOCA_APSH_THREAD_PID_TYPE;
/** @brief thread tid type */
typedef unsigned int DOCA_APSH_THREAD_TID_TYPE;
/** @brief thread state type */
typedef long DOCA_APSH_THREAD_STATE_TYPE;
/** @brief thread wait reason type */
typedef unsigned char DOCA_APSH_THREAD_WINDOWS_WAIT_REASON_TYPE;
/** @brief thread offset type */
typedef uint64_t DOCA_APSH_THREAD_WINDOWS_OFFSET_TYPE;
/** @brief thread proc name type */
typedef const char* DOCA_APSH_THREAD_LINUX_PROC_NAME_TYPE;
/** @brief thread thread name type */
typedef const char* DOCA_APSH_THREAD_LINUX_THREAD_NAME_TYPE;

/**
 * @brief doca app shield lib attributes
 */
enum doca_apsh_lib_attr{
	DOCA_APSH_LIB_PID,					/**< lib pid */
	DOCA_APSH_LIB_COMM,					/**< lib name */
	DOCA_APSH_LIB_LIBRARY_PATH,     			/**< lib loaded library path */
	DOCA_APSH_LIB_WINDOWS_FULL_DLL_NAME = 1000,		/**< lib full dll name */
	DOCA_APSH_LIB_WINDOWS_SIZE_OFIMAGE,			/**< lib size of image */
	DOCA_APSH_LIB_LINUX_LOAD_ADRESS = 2000,			/**< lib load address */
};

/** @brief lib pid type */
typedef unsigned int DOCA_APSH_LIB_PID_TYPE;
/** @brief lib comm type */
typedef const char* DOCA_APSH_LIB_COMM_TYPE;
/** @brief lib loaded library path type */
typedef const char* DOCA_APSH_LIB_LIBRARY_PATH_TYPE;
/** @brief lib full dll name type */
typedef const char* DOCA_APSH_LIB_WINDOWS_FULL_DLL_NAME_TYPE;
/** @brief lib size ofimage type */
typedef unsigned long DOCA_APSH_LIB_WINDOWS_SIZE_OFIMAGE_TYPE;
/** @brief lib load adress type */
typedef uint64_t DOCA_APSH_LIB_LINUX_LOAD_ADRESS_TYPE;

/**
 * @brief doca app shield virtual address descriptor attributes
 */
enum doca_apsh_vad_attr{
	DOCA_APSH_VMA_PID,					/**< vma process id */
	DOCA_APSH_VMA_OFFSET,					/**< vma offset */
	DOCA_APSH_VMA_PROTECTION,				/**< vma protection */
	DOCA_APSH_VMA_VM_START,					/**< vma vm start */
	DOCA_APSH_VMA_VM_END,					/**< vma vm end */
	DOCA_APSH_VMA_PROCESS_NAME,				/**< vma process name */
	DOCA_APSH_VMA_FILE_PATH,				/**< vma file path */
	DOCA_APSH_VMA_WINDOWS_COMMIT_CHARGE = 1000,		/**< vma commit charge */
	DOCA_APSH_VMA_WINDOWS_PRIVATE_MEMORY,			/**< vma private memory */
};

/** @brief vma pid type */
typedef unsigned int DOCA_APSH_VMA_PID_TYPE;
/** @brief vma offset type */
typedef uint64_t DOCA_APSH_VMA_OFFSET_TYPE;
/** @brief vma protection type */
typedef const char* DOCA_APSH_VMA_PROTECTION_TYPE;
/** @brief vma vm start type */
typedef uint64_t DOCA_APSH_VMA_VM_START_TYPE;
/** @brief vma vm end type */
typedef uint64_t DOCA_APSH_VMA_VM_END_TYPE;
/** @brief vma file path type */
typedef const char* DOCA_APSH_VMA_PROCESS_NAME_TYPE;
/** @brief vma file path type */
typedef const char* DOCA_APSH_VMA_FILE_PATH_TYPE;
/** @brief vma commit charge type */
typedef int DOCA_APSH_VMA_WINDOWS_COMMIT_CHARGE_TYPE;
/** @brief vma private memory type */
typedef int DOCA_APSH_VMA_WINDOWS_PRIVATE_MEMORY_TYPE;


/**
 * @brief doca app shield attestation attributes
 */
enum doca_apsh_attestation_attr{
	DOCA_APSH_ATTESTATION_PID,				/**< attestation process id */
	DOCA_APSH_ATTESTATION_COMM,				/**< attestation process name */
	DOCA_APSH_ATTESTATION_PATH_OF_MEMORY_AREA,		/**< attestation path of memory area */
	DOCA_APSH_ATTESTATION_PROTECTION,			/**< attestation protection */
	DOCA_APSH_ATTESTATION_START_ADDRESS,			/**< attestation start address */
	DOCA_APSH_ATTESTATION_END_ADDRESS,			/**< attestation end address */
	DOCA_APSH_ATTESTATION_PAGES_NUMBER,			/**< attestation process pages count in binary file */
	DOCA_APSH_ATTESTATION_PAGES_PRESENT,			/**< attestation pages present in memory */
	DOCA_APSH_ATTESTATION_MATCHING_HASHES,			/**< attestation pages hash match count from pages in memory */
	DOCA_APSH_ATTESTATION_HASH_DATA_IS_PRESENT,		/**< attestation hash data is present */
};

/** @brief attestation pid type */
typedef unsigned int DOCA_APSH_ATTESTATION_PID_TYPE;
/** @brief attestation comm type */
typedef const char* DOCA_APSH_ATTESTATION_COMM_TYPE;
/** @brief attestation path of memory area type */
typedef const char* DOCA_APSH_ATTESTATION_PATH_OF_MEMORY_AREA_TYPE;
/** @brief attestation protection type */
typedef const char* DOCA_APSH_ATTESTATION_PROTECTION_TYPE;
/** @brief attestation start address type */
typedef uint64_t DOCA_APSH_ATTESTATION_START_ADDRESS_TYPE;
/** @brief attestation end address type */
typedef uint64_t DOCA_APSH_ATTESTATION_END_ADDRESS_TYPE;
/** @brief attestation pages number type */
typedef int DOCA_APSH_ATTESTATION_PAGES_NUMBER_TYPE;
/** @brief attestation pages present type */
typedef int DOCA_APSH_ATTESTATION_PAGES_PRESENT_TYPE;
/** @brief attestation matching hashes type */
typedef int DOCA_APSH_ATTESTATION_MATCHING_HASHES_TYPE;
/** @brief attestation hash data is present type */
typedef bool DOCA_APSH_ATTESTATION_HASH_DATA_IS_PRESENT_TYPE;

/**
 * @brief doca app shield module attributes
 */
enum doca_apsh_module_attr{
	DOCA_APSH_MODULES_OFFSET,				/**< module offset */
	DOCA_APSH_MODULES_NAME,					/**< module name */
	DOCA_APSH_MODULES_SIZE,					/**< module size */
};

/** @brief module offset type */
typedef uint64_t DOCA_APSH_MODULES_OFFSET_TYPE;
/** @brief module name type */
typedef const char* DOCA_APSH_MODULES_NAME_TYPE;
/** @brief module size type */
typedef uint32_t DOCA_APSH_MODULES_SIZE_TYPE;

/**
 * @brief doca app shield privileges attributes
 * windows privilege list can be found on:
 * https://docs.microsoft.com/en-us/windows/win32/secauthz/privilege-constants
 */
enum doca_apsh_privilege_attr {
	DOCA_APSH_PRIVILEGES_PID,				/**< privilege process pid */
	DOCA_APSH_PRIVILEGES_COMM,				/**< privilege process name */
	DOCA_APSH_PRIVILEGES_NAME,				/**< privilege name, for example: SeTcbPrivilege */
	DOCA_APSH_PRIVILEGES_IS_ON,				/**< is the privilege turned on or off.
								   * For Windows this is the outcome of
								   * get(PRESENT) && (get(ENABLED) || get(DEFAULT)) */
	DOCA_APSH_PRIVILEGES_WINDOWS_PRESENT = 1000,		/**< privilege present flag */
	DOCA_APSH_PRIVILEGES_WINDOWS_ENABLED,			/**< privilege enabled flag */
	DOCA_APSH_PRIVILEGES_WINDOWS_DEFAULT,			/**< privilege enabledbydefault flag */
};

/** @brief privilege process pid */
typedef unsigned int DOCA_APSH_PRIVILEGES_PID_TYPE;
/** @brief privilege process name */
typedef const char* DOCA_APSH_PRIVILEGES_COMM_TYPE;
/** @brief privilege name type */
typedef const char* DOCA_APSH_PRIVILEGES_NAME_TYPE;
/** @brief privilege is on type */
typedef bool DOCA_APSH_PRIVILEGES_IS_ON_TYPE;
/** @brief privilege windows present type */
typedef bool DOCA_APSH_PRIVILEGES_WINDOWS_PRESENT_TYPE;
/** @brief privilege windows enabled type */
typedef bool DOCA_APSH_PRIVILEGES_WINDOWS_ENABLED_TYPE;
/** @brief privilege windows enabled by default type */
typedef bool DOCA_APSH_PRIVILEGES_WINDOWS_DEFAULT_TYPE;

/**
* @brief doca app shield envars attributes
*/
enum doca_apsh_envar_attr {
	DOCA_APSH_ENVARS_PID,					/**< envars pid */
	DOCA_APSH_ENVARS_COMM,					/**< envars process name */
	DOCA_APSH_ENVARS_VARIABLE,				/**< envars variable */
	DOCA_APSH_ENVARS_VALUE,					/**< envars value */
	DOCA_APSH_ENVARS_WINDOWS_BLOCK = 1000,			/**< envars windows environment block address */
};

/** @brief envars pid type */
typedef unsigned int DOCA_APSH_ENVARS_PID_TYPE;
/** @brief envars comm type */
typedef const char* DOCA_APSH_ENVARS_COMM_TYPE;
/** @brief envars variable type */
typedef const char* DOCA_APSH_ENVARS_VARIABLE_TYPE;
/** @brief envars value type */
typedef const char* DOCA_APSH_ENVARS_VALUE_TYPE;
/** @brief envars windows block address type */
typedef uint64_t DOCA_APSH_ENVARS_WINDOWS_BLOCK_TYPE;

/**
* @brief doca app shield LDR-Modules attributes
*/
enum doca_apsh_ldrmodule_attr {
	DOCA_APSH_LDRMODULE_PID,				/**< ldrmodule process pid */
	DOCA_APSH_LDRMODULE_COMM,				/**< ldrmodule process name */
	DOCA_APSH_LDRMODULE_BASE_ADDRESS,			/**< ldrmodule base address */
	DOCA_APSH_LDRMODULE_LIBRARY_PATH,			/**< ldrmodule loaded library path */
	DOCA_APSH_LDRMODULE_WINDOWS_BASE_DLL_NAME = 1000,	/**< ldrmodule full dll name */
	DOCA_APSH_LDRMODULE_WINDOWS_SIZE_OF_IMAGE,		/**< ldrmodule size of image */
	DOCA_APSH_LDRMODULE_WINDOWS_INLOAD,			/**< ldrmodule appear in inload list */
	DOCA_APSH_LDRMODULE_WINDOWS_INMEM,			/**< ldrmodule appear in inmem list */
	DOCA_APSH_LDRMODULE_WINDOWS_ININIT,			/**< ldrmodule appear in ininit list */
};

/** @brief ldrmodule pid type */
typedef unsigned int DOCA_APSH_LDRMODULE_PID_TYPE;
/** @brief ldrmodule comm type */
typedef const char* DOCA_APSH_LDRMODULE_COMM_TYPE;
/** @brief ldrmodule base adress type */
typedef uint64_t DOCA_APSH_LDRMODULE_BASE_ADDRESS_TYPE;
/** @brief ldrmodule library path type */
typedef const char* DOCA_APSH_LDRMODULE_LIBRARY_PATH_TYPE;
/** @brief ldrmodule windows BASE dll name type */
typedef const char* DOCA_APSH_LDRMODULE_WINDOWS_BASE_DLL_NAME_TYPE;
/** @brief ldrmodule size of image type */
typedef unsigned long DOCA_APSH_LDRMODULE_WINDOWS_SIZE_OF_IMAGE_TYPE;
/** @brief ldrmodule inload type */
typedef bool DOCA_APSH_LDRMODULE_WINDOWS_INLOAD_TYPE;
/** @brief ldrmodule inmem type */
typedef bool DOCA_APSH_LDRMODULE_WINDOWS_INMEM_TYPE;
/** @brief ldrmodule ininit type */
typedef bool DOCA_APSH_LDRMODULE_WINDOWS_ININIT_TYPE;

/**
 * @brief doca app shield handle attributes
 */
enum doca_apsh_handle_attr {
	DOCA_APSH_HANDLE_PID,					/**< handle process id */
	DOCA_APSH_HANDLE_COMM,					/**< handle process name */
	DOCA_APSH_HANDLE_VALUE,					/**< handle value */
	DOCA_APSH_HANDLE_TABLE_ENTRY,				/**< handle table entry */
	DOCA_APSH_HANDLE_TYPE,					/**< handle type */
	DOCA_APSH_HANDLE_ACCESS,				/**< handle access */
	DOCA_APSH_HANDLE_NAME,					/**< handle name */
};

/** @brief handle pid type */
typedef unsigned int DOCA_APSH_HANDLE_PID_TYPE;
/** @brief handle comm type */
typedef const char* DOCA_APSH_HANDLE_COMM_TYPE;
/** @brief handle value type */
typedef uint64_t DOCA_APSH_HANDLE_VALUE_TYPE;
/** @brief handle table entry type */
typedef uint64_t DOCA_APSH_HANDLE_TABLE_ENTRY_TYPE;
/** @brief handle type type */
typedef const char* DOCA_APSH_HANDLE_TYPE_TYPE;
/** @brief handle access type */
typedef uint64_t DOCA_APSH_HANDLE_ACCESS_TYPE;
/** @brief handle name type */
typedef const char* DOCA_APSH_HANDLE_NAME_TYPE;

#ifdef __cplusplus
}
#endif

/** @} */

#endif
