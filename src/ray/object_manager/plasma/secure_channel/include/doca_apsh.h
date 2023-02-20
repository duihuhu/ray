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
 * @file doca_apsh.h
 * @page apsh
 * @defgroup DOCA_APSH App Shield
 *
 * DOCA App Shield library let you to monitor operation system that resides on the host.
 * This is done with the DPU DMA capabilities and the regex engine.
 * Please follow the programmer guide for system configurations.
 *
 * @{
 */

#ifndef _DOCA_APSH__H_
#define _DOCA_APSH__H_

#include "doca_compat.h"
#include "doca_apsh_attr.h"
#include "doca_dev.h"
#include <doca_error.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief apsh handler
 */
struct doca_apsh_ctx;

/**
 * @brief system handler
 *
 * host bare metal/vm ...
 */
struct doca_apsh_system;

/**
 * @brief system module
 *
 * for example kernel module for linux
 */
struct doca_apsh_module;

/**
 * @brief system process
 *
 * hold the process information such as pid, cmd ...
 */
struct doca_apsh_process;

/**
 * @brief system process thread
 *
 * hold the thread process information
 */
struct doca_apsh_thread;

/**
 * @brief system process libs
 *
 * hold the process loadable libraries (DLL's or SO's)
 */
struct doca_apsh_lib;

/**
 * @brief system process virtual address descriptors
 */
struct doca_apsh_vad;

/**
 * @brief system process attestation
 *
 * contain data regarding attestation - process intrusion detection
 */
struct doca_apsh_attestation;

/**
 * @brief system process privilege
 *
 * hold the privilege process information
 */
struct doca_apsh_privilege;

/**
 * @brief system process envar
 *
 * hold the privilege process information
 */
struct doca_apsh_envar;

/**
 * @brief system process ldrmodule
 *
 * hold the ldrmodule process information
 */
struct doca_apsh_ldrmodule;

/**
 * @brief system process handle
 *
 * hold the handle process information
 */
struct doca_apsh_handle;


/**
 * @brief Create a new apsh handler
 *
 * Allocate memory and init the opaque struct for apsh handler.
 * Before using the system handler use doca_apsh_start
 *
 * @return
 *   apsh context required for creating system handler, NULL on failure
 */
__DOCA_EXPERIMENTAL
struct doca_apsh_ctx *doca_apsh_create(void);

/**
 * @brief Start apsh handler
 *
 * Start apsh handler and init connection to devices.
 * Need to set apsh params with setter functions before starting the system.
 * Mandatory setters: doca_apsh_dma_dev_set.
 * Other setters can be query automatically but will take time.
 *
 * @param ctx
 *   App Shield handler
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_start(struct doca_apsh_ctx *ctx);

/**
 * @brief Free the APSH memory and close connections.
 *
 * @param ctx
 *   apsh context to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_destroy(struct doca_apsh_ctx *ctx);

/**
 * @brief Set apsh dma device
 *
 * This is a Mandatory setter
 *
 * @param ctx
 *   apsh handler
 * @param dma_dev
 *   doca device with dma capabilities, please refer to doca_dev.h
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc new buffer for dma_dev_name.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_dma_dev_set(struct doca_apsh_ctx *ctx, struct doca_dev *dma_dev);

/**
 * @brief Set apsh regex device
 *
 * This is not a Mandatory setter
 *
 * @note currently supported only for windows systems.
 *
 * @param ctx
 *   apsh handler
 * @param regex_dev
 *   doca device with the capabilities of regex
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_regex_dev_set(struct doca_apsh_ctx *ctx, struct doca_dev *regex_dev);

/**
 * @brief Create a new system handler
 *
 * Allocate memory and init the opaque struct for system handler.
 * Before using the system handler use doca_apsh_system_start
 *
 * @param ctx
 *   apsh handler
 * @return
 *   returns system pointer, NULL on failure
 */
__DOCA_EXPERIMENTAL
struct doca_apsh_system *doca_apsh_system_create(struct doca_apsh_ctx *ctx);

/**
 * @brief Destroy system handler
 *
 * This will not destroy process/module/libs ...
 *
 * @param system
 *   system context to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_system_destroy(struct doca_apsh_system *system);

/**
 * @brief Start system handler
 *
 * Start system handler and init connection to the system.
 * Need to set system params with setter functions before starting the system.
 * Mandatory setters: os_symbol_map, mem_region, dev.
 * Other setters can be query automatically but will take time.
 *
 * @param system
 *   system handler
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if app-shield system initialization has failed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_system_start(struct doca_apsh_system *system);

/**
 * @brief Set system os symbol map
 *
 * This is a Mandatory setter
 *
 * @param system
 *   system handler
 * @param system_os_symbol_map_path
 *   the os memory map data, unique per os build
 *   please note that changing linux kernel (adding/removing modules) will change the map
 *   should be created by running the doca_system_os_symbol_map tool on the system os
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc new buffer for system_os_symbol_map_path.
 *   - DOCA_ERROR_BAD_STATE - if system was already started.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_sys_os_symbol_map_set(struct doca_apsh_system *system,
					    const char *system_os_symbol_map_path);

/**
 * @brief Set system allowed memory regions
 *
 * This is a Mandatory setter
 *
 * @param system
 *   system handler
 * @param system_mem_region_path
 *   path to json file containing the memory regions of the devices
 * 	 The memory regions are unique per system, would not change on reboot or between different
 *   devices of the same system. note that adding/removing device from the host can change the
 *   regions. The json can be created by running the doca_system_mem_region tool on the system.
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc new buffer for system_os_symbol_map_path.
 *   - DOCA_ERROR_BAD_STATE - if system was already started.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_sys_mem_region_set(struct doca_apsh_system *system, const char *system_mem_region_path);

/**
 * @brief Set system device
 *
 * This is a Mandatory setter
 *
 * @param system
 *   system handler
 * @param dev
 *   the device that is connected to the system to be queried.
 *   for example a vf that is connected to a vm or pf that is connected to the bare-metal.
 *   doca representor device from dma device configured in doca_apsh_dma_dev_set.
 *   to query the right device please refer to doca_dev.h for full options.
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_BAD_STATE - if system was already started.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_sys_dev_set(struct doca_apsh_system *system, struct doca_dev_rep *dev);

/**
 * @brief Set system os type
 *
 * This is a must setter
 *
 * @param system
 *   system handler
 * @param os_type
 *   system os type - windows/linux
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_NOT_SUPPORTED - if unsupported OS type had been received.
 *   - DOCA_ERROR_BAD_STATE - if system was already started.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_sys_os_type_set(struct doca_apsh_system *system, enum doca_apsh_system_os os_type);

/**
 * @brief Set system kpgd file
 *
 * This is not a must setter
 *
 * @param system
 *   system handler
 * @param system_kpgd_path
 *   the path to kpgd file
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_NOT_SUPPORTED - if unsupported OS type had been received.
 *   - DOCA_ERROR_BAD_STATE - if system was already started.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_sys_kpgd_file_set(struct doca_apsh_system *system, const char *system_kpgd_file_path);

/**
 * @brief Shadow function - configure attribute value for a system
 *
 * Do not use this function, recommended to use doca_apsh_sys_config
 *
 * @param system
 *   system handler
 * @param attr
 *    Attribute to set in the system
 * @param value
 *    the value to set
 * @return
*   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_NOT_SUPPORTED - if attr was OS type and an unsupported OS type had been received.
 *   - DOCA_ERROR_NO_MEMORY - if memory allocation failed.
 *   - DOCA_ERROR_BAD_STATE - if system is already started.
 */
__DOCA_EXPERIMENTAL
doca_error_t __doca_apsh_sys_config(struct doca_apsh_system *system, enum doca_apsh_system_config_attr attr, void *value);

/**
 * @brief configure attribute value for a system, such as: hashtest limit, symbols map ...
 *
 *
 * @param system
 *   system handler
 * @param attr
 *    Attribute to set in the system
 * @param value
 *    the value to set
 * @return
*   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_NOT_SUPPORTED - if attr was OS type and an unsupported OS type had been received.
 *   - DOCA_ERROR_NO_MEMORY - if memory allocation failed.
 *   - DOCA_ERROR_BAD_STATE - if system is already started.
 */

#define doca_apsh_sys_config(system, attr, value) (__doca_apsh_sys_config(system, attr, (void *)((uintptr_t)value)))

/**
 * @brief Get array of current modules installed on the system
 *
 * This function is multithreaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return array is snapshot, this is not dynamic array, need to free it.
 *
 * @param system
 *   System handler
 * @param modules
 *   Array of module opaque pointers of the systems
 * @param modules_size
 *   Output param, will contain size of modules array on success.
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if modules list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to modules array.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_modules_get(struct doca_apsh_system *system, struct doca_apsh_module ***modules,
				 int *modules_size);

/**
 * @brief Destroys a modules array
 *
 * @param modules
 * 	 Array of module opaque pointers of the systems to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_module_free(struct doca_apsh_module **modules);

/**
 * @brief Shadow function - get attribute value for a module
 *
 * Do not use this function, recommended to use doca_apsh_mod_info_get
 *
 * @param module
 *   single module handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_module_info_get(struct doca_apsh_module *module, enum doca_apsh_module_attr attr);

/**
 * @brief Get attribute value for a module
 *
 * Get the requested info from module handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param module
 *   single module handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_module_info_get(module, attr) ((attr##_TYPE)(uintptr_t)__doca_apsh_module_info_get(module, attr))

/**
 * @brief Get array of current processes running on the system
 *
 * This function is multithreaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return array is snapshot, this is not dynamic array, need to free it.
 *
 * @param system
 *   System handler
 * @param processes
 *   Array of process opaque pointers of the systems
 * @param processes_size
 *   Output param, will contain size of processes array on success.
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if processes list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to processes array.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_processes_get(struct doca_apsh_system *system, struct doca_apsh_process ***processes,
				    int *processes_size);

/**
 * @brief Destroys a process context
 *
 * @param processes
 * 	 Array of process opaque pointers of the systems to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_processes_free(struct doca_apsh_process **processes);

/**
 * @brief Shadow function - get attribute value for a process
 *
 * Do not use this function, recommended to use doca_apsh_process_info_get
 *
 * @param process
 *   single process handler
 * @param attr
 *   Attribute to get the info on the process
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_process_info_get(struct doca_apsh_process *process,
                     enum doca_apsh_process_attr attr);

/**
 * @brief Get attribute value for a process
 *
 * Get the requested info from process handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param process
 *   single process handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_process_info_get(process, attr) ((attr##_TYPE)(uintptr_t)__doca_apsh_process_info_get(process, attr))

/**
 * @brief Get array of current process loadable libraries
 *
 * This function is multithreaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return array is snapshot, this is not dynamic array, need to free it.
 *
 * @param process
 *   Process handler
 * @param libs
 *   Array of libs opaque pointers of the process.
 *   in case process doesn't point to any libs, will return NULL.
 * @param libs_size
 *   Output param, will contain size of libs array on success.
 * @return
 *   DOCA_SUCCESS - in case of success (including the case libs_size is zero).
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if libs list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to libs array.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_libs_get(struct doca_apsh_process *process, struct doca_apsh_lib ***libs, int *libs_size);

/**
 * @brief Destroys a libs context
 *
 * @param libs
 *	Array of libs opaque pointers of the process to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_libs_free(struct doca_apsh_lib **libs);

/**
 * @brief Shadow function - get attribute value for a lib
 *
 * Do not use this function, recommended to use doca_apsh_lib_info_get
 *
 * @param lib
 *   single lib handler
 * @param attr
 *   Attribute to get the info on the lib
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_lib_info_get(struct doca_apsh_lib *lib, enum doca_apsh_lib_attr attr);

/**
 * @brief Get attribute value for a lib
 *
 * Get the requested info from lib handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param lib
 *   single lib handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_lib_info_get(lib, attr) ((attr##_TYPE)(uintptr_t)__doca_apsh_lib_info_get(lib, attr))

/**
 * @brief Get array of current process threads
 *
 * This function is multithreaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return array is snapshot, this is not dynamic array, need to free it.
 *
 * @param process
 *   Process handler
 * @param threads
 *   Array of threads opaque pointers of the process.
 *   in case process doesn't have any threads, will return NULL.
 * @param threads_size
 *   Output param, will contain size of threads array on success.
 * @return
 *   DOCA_SUCCESS - in case of success (including the case threads_size is zero).
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if threads list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to threads array.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_threads_get(struct doca_apsh_process *process, struct doca_apsh_thread ***threads,
				  int *threads_size);

/**
 * @brief Destroys a threads context
 *
 * @param threads
 * 	 Array of threads opaque pointers of the process to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_threads_free(struct doca_apsh_thread **threads);

/**
 * @brief Shadow function - get attribute value for a thread
 *
 * Do not use this function, recommended to use doca_apsh_thread_info_get
 *
 * @param thread
 *   single thread handler
 * @param attr
 *   Attribute to get the info on the thread
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_thread_info_get(struct doca_apsh_thread *thread, enum doca_apsh_thread_attr attr);

/**
 * @brief Get attribute value for a thread
 *
 * Get the requested info from thread handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param thread
 *   single thread handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_thread_info_get(thread, attr)                                                     \
    ((attr##_TYPE)__doca_apsh_thread_info_get(thread, attr))

/**
 * @brief Get array of current process vads - virtual address descriptor
 *
 * This function is multithreaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return array is snapshot, this is not dynamic array, need to free it.
 *
 * @param process
 *   Process handler
 * @param vads
 *   Array of vads opaque pointers of the process.
 *   in case process doesn't point to any vads, will return NULL.
 * @param vads_size
 *   Output param, will contain size of vads array on success.
 * @return
 *   DOCA_SUCCESS - in case of success (including the case vads_size is zero).
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if modules list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to modules array.
 *   - DOCA_ERROR_NOT_FOUND - if process hasn't been found.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_vads_get(struct doca_apsh_process *process, struct doca_apsh_vad ***vads, int *vads_size);

/**
 * @brief Destroys a vads context
 *
 * @param vads
 * 	 Array of vads opaque pointers of the process to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_vads_free(struct doca_apsh_vad **vads);

/**
 * @brief Shadow function - get attribute value for a vad
 *
 * Do not use this function, recommended to use doca_apsh_vad_info_get
 *
 * @param vad
 *   single vad handler
 * @param attr
 *   Attribute to get the info on the vad
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_vad_info_get(struct doca_apsh_vad *vad, enum doca_apsh_vad_attr attr);

/**
 * @brief Get attribute value for a vad
 *
 * Get the requested info from vad handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param vad
 *   single vad handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_vad_info_get(vad, attr) ((attr##_TYPE)(uintptr_t)__doca_apsh_vad_info_get(vad, attr))

/**
 * @brief Get current process attestation
 *
 * This function is multithreaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return is snapshot, this is not dynamic, need to free it.
 *
 * @param process
 *   Process handler
 * @param exec_hash_map_path
 *   path to file containing the hash calculations of the executable and dlls/libs of the process
 *   note that changing the process code or any libs can effect this.
 *   The file can be created by running the doca_exec_hash_build_map tool on the system.
 * @param attestation
 *   Attestation opaque pointers of the process
 * @param attestation_size
 *   Output param, will contain size of attestation array on success.
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if modules list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to modules array.
 *   - DOCA_ERROR_NOT_FOUND - if process hasn't been found.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_attestation_get(struct doca_apsh_process *process, const char *exec_hash_map_path,
				      struct doca_apsh_attestation ***attestation, int *attestation_size);

/**
 * @brief Destroys a attestation context
 *
 * @param attestation
 * 	 Attestation opaque pointer of the process to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_attestation_free(struct doca_apsh_attestation **attestation);

/**
 * @brief Shadow function - get attribute value for a attestation
 *
 * Do not use this function, recommended to use doca_apsh_attestation_info_get
 *
 * @param attestation
 *   single attestation handler
 * @param attr
 *   Attribute to get the info on the attestation
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_attst_info_get(struct doca_apsh_attestation *attestation,
                      enum doca_apsh_attestation_attr attr);

/**
 * @brief Get attribute value for a attestation
 *
 * Get the requested info from attestation handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param attestation
 *   single attestation handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_attst_info_get(attestation, attr)                                                 \
	((attr##_TYPE)(uintptr_t)__doca_apsh_attst_info_get(attestation, attr))

/**
 * @brief refresh single attestation handler of a process with new snapshot
 *
 * This function is multithreaded compatible with different system context,
 * Refresh the snapshot of the handler.
 * Recommended to query all wanted information before refreshing.
 *
 * @param attestation
 *   single attestation handler to refresh
 * @param attestation_size
 *   Output param, will contain size of attestation array on success.
 * @return
 *   DOCA_SUCCESS - in case of success.
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if modules list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to modules array.
 *   - DOCA_ERROR_NOT_FOUND - if process hasn't been found.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_apsh_attst_refresh(struct doca_apsh_attestation ***attestation, int *attestation_size);

/**
 * @brief Get array of current process privileges
 *
 * This function is multi-threaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return array is snapshot, this is not dynamic array, need to free it.
 *
 * @note currently supported only for windows systems.
 *
 * @param process
 *   Process handler
 * @param privileges
 *   Array of privileges opaque pointers of the process.
 *   in case process doesn't have any privileges, will return NULL.
 * @param privileges_size
 *   Output param, will contain size of privileges array on success.
 * @return
 *   DOCA_SUCCESS - in case of success (including the case privileges_size is zero).
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if privileges list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to privileges array.
 *   - DOCA_ERROR_NOT_SUPPORTED - in case of unsupported system os.
 */
__DOCA_EXPERIMENTAL
doca_error_t
doca_apsh_privileges_get(struct doca_apsh_process *process, struct doca_apsh_privilege ***privileges, int *privileges_size);

/**
 * @brief Destroys a privileges context
 *
 * @param privileges
 * 	 Array of privileges opaque pointers of the process to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_privileges_free(struct doca_apsh_privilege **privileges);

/**
 * @brief Shadow function - get attribute value for a privilege
 *
 * Do not use this function, recommended to use doca_apsh_privilege_info_get
 *
 * @param privilege
 *   single privilege handler
 * @param attr
 *   Attribute to get the info on the privilege
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_privilege_info_get(struct doca_apsh_privilege *privilege, enum doca_apsh_privilege_attr attr);

/**
 * @brief Get attribute value for a privilege
 *
 * Get the requested info from privilege handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param privilege
 *   single privilege handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_privilege_info_get(privilege, attr)                                                     \
    ((attr##_TYPE)__doca_apsh_privilege_info_get(privilege, attr))

/**
 * @brief Get array of current process environment variables
 *
 * This function is multi-threaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return array is snapshot, the function allocates this array, use doca_apsh_envars_free to free it.
 *
 * @note currently supported only for windows systems.
 *
 * @param process
 *   Process handler
 * @param envars
 *   Array of environment variables opaque pointers of the process.
 *   in case process doesn't have any envars, will return NULL.
 * @param envars_size
 *   Output param, will contain size of envars array on success.
 * @return
 *   DOCA_SUCCESS - in case of success (including the case envars_size is zero).
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if envars list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to envars array.
 *   - DOCA_ERROR_NOT_SUPPORTED - in case of unsupported system os.
 */
__DOCA_EXPERIMENTAL
doca_error_t
doca_apsh_envars_get(struct doca_apsh_process *process, struct doca_apsh_envar ***envars, int *envars_size);

/**
 * @brief Destroys a envars context
 *
 * @param envars
 * 	 Array of envars opaque pointers of the process to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_envars_free(struct doca_apsh_envar **envars);

/**
 * @brief Shadow function - get attribute value for an environment variable
 *
 * Do not use this function, recommended to use doca_apsh_envar_info_get
 *
 * @param envar
 *   single envar handler
 * @param attr
 *   Attribute to get the info on the envar
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_envar_info_get(struct doca_apsh_envar *envar, enum doca_apsh_envar_attr attr);

/**
 * @brief Get attribute value for an environment variable
 *
 * Get the requested info from envar handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param envar
 *   single envar handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_envar_info_get(envar, attr)                                                     \
    ((attr##_TYPE)__doca_apsh_envar_info_get(envar, attr))

/**
* @brief Get array of current process modules
*
 * This function is multi-threaded compatible with different system context,
* meaning do not call this function simultaneously with the same system context.
* The return array is snapshot, this is not dynamic array, need to free it.
*
 * @note currently supported only for windows systems.
*
* @param process
*   Process handler
* @param ldrmodules
*   Array of ldrmodules opaque pointers of the process.
*   in case process doesn't have any modules, will return NULL.
* @param ldrmodules_size
*   Output param, will contain size of ldrmodules array on success.
* @return
*   DOCA_SUCCESS - in case of success (including the case ldrmodules_size is zero).
*   doca_error code - in case of failure:
*   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
*   - DOCA_ERROR_INITIALIZATION - if ldrmodules list initialization failed.
*   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to ldrmodules array.
*   - DOCA_ERROR_NOT_SUPPORTED - in case of unsupported system os.
*/
__DOCA_EXPERIMENTAL
doca_error_t
doca_apsh_ldrmodules_get(struct doca_apsh_process *process, struct doca_apsh_ldrmodule ***ldrmodules, int *ldrmodules_size);

/**
* @brief Destroys a ldrmodules context
*
* @param ldrmodules
*      Array of ldrmodules opaque pointers of the process to destroy
*/
__DOCA_EXPERIMENTAL
void doca_apsh_ldrmodules_free(struct doca_apsh_ldrmodule **ldrmodules);

/**
* @brief Shadow function - get attribute value for a modules
*
* Do not use this function, recommended to use doca_apsh_ldrmodule_info_get
*
* @param ldrmodule
*   single ldrmodule handler
* @param attr
*   Attribute to get the info on the module
* @return
*   return the info requested, need to cast
*/
__DOCA_EXPERIMENTAL
const void *__doca_apsh_ldrmodule_info_get(struct doca_apsh_ldrmodule *ldrmodule, enum doca_apsh_ldrmodule_attr attr);

/**
* @brief Get attribute value for a ldrmodule
*
* Get the requested info from ldrmodule handler.
* The info is right to the snapshot (at the get function moment)
* full list (type and descriptions) can be found in doca_apsh_attr.h
*
* @param ldrmodule
*   single ldrmodule handler
* @param attr
*   Attribute to get the info on the module
* @return
*   return the info requested (with casting)
*/
#define doca_apsh_ldrmodule_info_get(ldrmodule, attr)                                                     \
    ((attr##_TYPE)__doca_apsh_ldrmodule_info_get(ldrmodule, attr))

/**
 * @brief Get array of current process handles
 *
 * This function is multi-threaded compatible with different system context,
 * meaning do not call this function simultaneously with the same system context.
 * The return array is snapshot, this is not dynamic array, need to free it.
 *
 * @note currently supported only for windows systems.
 *
 * @param process
 *   Process handler
 * @param handles
 *   Array of handles opaque pointers of the process.
 *   in case process doesn't have any handles, will return NULL.
 * @param handles_size
 *   Output param, will contain size of handles array on success.
 * @return
 *   DOCA_SUCCESS - in case of success (including the case handles_size is zero).
 *   doca_error code - in case of failure:
 *   - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 *   - DOCA_ERROR_INITIALIZATION - if handles list initialization failed.
 *   - DOCA_ERROR_NO_MEMORY - if cannot alloc memory to handles array.
 *   - DOCA_ERROR_NOT_SUPPORTED - in case of unsupported system os.
 */
__DOCA_EXPERIMENTAL
doca_error_t
doca_apsh_handles_get(struct doca_apsh_process *process, struct doca_apsh_handle ***handles, int *handles_size);

/**
 * @brief Destroys a handles context
 *
 * @param handles
 * 	 Array of handles opaque pointers of the process to destroy
 */
__DOCA_EXPERIMENTAL
void doca_apsh_handles_free(struct doca_apsh_handle **handles);

/**
 * @brief Shadow function - get attribute value for a handle
 *
 * Do not use this function, recommended to use doca_apsh_handle_info_get
 *
 * @param handle
 *   single handle handler
 * @param attr
 *   Attribute to get the info on the handle
 * @return
 *   return the info requested, need to cast
 */
__DOCA_EXPERIMENTAL
const void *__doca_apsh_handle_info_get(struct doca_apsh_handle *handle, enum doca_apsh_handle_attr attr);

/**
 * @brief Get attribute value for a handle
 *
 * Get the requested info from handle handler.
 * The info is right to the snapshot (at the get function moment)
 * full list (type and descriptions) can be found in doca_apsh_attr.h
 *
 * @param handle
 *   single handle handler
 * @param attr
 *   Attribute to get the info on the module
 * @return
 *   return the info requested (with casting)
 */
#define doca_apsh_handle_info_get(handle, attr)                                                     \
    ((attr##_TYPE)__doca_apsh_handle_info_get(handle, attr))

#ifdef __cplusplus
}
#endif

/** @} */

#endif
