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
 * @file doca_mmap.h
 * @page doca mmap
 * @defgroup MMAP DOCA Memory Map
 * @ingroup DOCACore
 * The DOCA memory map provides a centralized repository and orchestration of several memory ranges registration for each device attached to the memory map.
 *
 * @{
 */

#ifndef DOCA_MMAP_H_
#define DOCA_MMAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "doca_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * DOCA Memory Map
 ******************************************************************************/

/**
 * @brief Collection of memory ranges for both local & remote systems.
 *
 * Definitions:
 * Memory Range - virtually contiguous fracture of memory space defined by address and length.
 * Chunk - local system memory range.
 * Remote Chunk - remote system memory range.
 *
 * Limitations:
 * - Memory map should not contain memory ranges from different systems.
 *
 *
 * +---------------------+          +-----------------------------------------+
 * |Memory map           |--------->| chunk 1       +------+  +----------+    |
 * |                     |\         |               | data |  | data     |    |
 * |                     | \        |               +------+  +----------+    |
 * +---------+           |  \       +-----------------------------------------+
 * | dev1    |           |   \
 * +---------+           |    \     +------------------------------------+
 * | dev2    |           |     ---->| chunk 2  +------+    +----------+  |
 * +---------+           |          |          | data |    |  data    |  |
 * |                     |          |          +------+    +----------+  |
 * +---------------------+          +------------------------------------+
 */
struct doca_mmap;

/**
 * @brief Allocates zero size memory map object with default/unset attributes.
 *
 * @details The returned memory map object can be manipulated with
 * doca_mmap_property_set() API.
 *
 * Once all required mmap attributes set it should be reconfigured
 * and adjusted to meet object size setting with doca_mmap_start()
 * See doca_mmap_start for the rest of the details.
 *
 * @param [in] (optional) user_data
 * Identifier provided by user for the newly created doca_mmap. If not NULL,
 * pointed user_data will be set.
 * @param [out] mmap
 * DOCA memory map structure with default/unset attributes.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NO_MEMORY - failed to alloc doca_mmap.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_create(const union doca_data *user_data, struct doca_mmap **mmap);

/**
 * @brief Destroy DOCA Memory Map structure.
 *
 * @details Before calling this function all allocated buffers should be returned back to the mmap.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_PERMITTED - if there is a memory region pointed by one or more `struct doca_buf`,
 *                              or if memory deregistration failed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_destroy(struct doca_mmap *mmap);

/**
 * @brief Start DOCA Memory Map.
 *
 * @details Allows execution of different operations on the mmap, detailed below.
 * On first start verifies & finalizes the mmap object configuration.
 *
 * The following become possible only after start:
 * - Adding a device to the mmap using doca_mmap_dev_add().
 * - Removing a device to the mmap using doca_mmap_dev_rm().
 * - Adding a memory range to the mmap using doca_mmap_populate().
 * - Exporting the mmap using doca_mmap_export().
 * - Mapping doca_buf structures to the memory ranges in the using doca_buf_inventory_buf_by_addr() or
 *   doca_buf_inventory_buf_dup().
 *
 * The following are NOT possible after the first time start is called:
 * - Setting the properties of the mmap using doca_mmap_property_set().
 *
 * @param [in] mmap
 * DOCA memory map structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NO_MEMORY - if memory allocation failed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_start(struct doca_mmap *mmap);

/**
 * @brief Stop DOCA Memory Map.
 *
 * @details Prevents execution of different operations on the mmap.
 * For details see doca_mmap_start().
 *
 * @param [in] mmap
 * DOCA memory map structure.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_stop(struct doca_mmap *mmap);

/**
 * @brief Register DOCA memory map on a given device.
 *
 * @details This operation is not permitted for:
 * - un-started/stopped memory map object.
 * - memory map object that have been exported or created from export.
 *
 * @param [in] mmap
 * DOCA memory map structure.
 * @param [in] dev
 * DOCA Dev instance with appropriate capability.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_PERMITTED - if memory deregistration failed or the operation is not premitted for the given mmap
 * 				(see details in this function description).
 * - DOCA_ERROR_NO_MEMORY - if reached to DOCA_MMAP_MAX_NUM_DEVICES.
 * - DOCA_ERROR_IN_USE - if doca_dev already exists in doca_mmap.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_dev_add(struct doca_mmap *mmap, struct doca_dev *dev);

/**
 * @brief Deregister given device from DOCA memory map.
 *
 * @details This operation is not permitted for:
 * - un-started/stopped memory map object.
 * - memory map object that have been exported or created from export.
 *
 * @param [in] mmap
 * DOCA memory map structure.
 * @param [in] dev
 * DOCA Dev instance that was previously added.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received or doca_dev doesn't exists in doca_mmap.
 * - DOCA_ERROR_NOT_PERMITTED - if memory deregistration failed or the operation is not premitted for the given mmap
 * 				(see details in this function description).
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_dev_rm(struct doca_mmap *mmap, struct doca_dev *dev);

/**
 * @brief Function to be called for each populated memory range on memory map destroy.
 *
 * @param[in] addr
 *   Memory range pointer.
 * @param[in] len
 *   Memory range length.
 * @param[in] opaque
 *   An opaque pointer passed to iterator.
 */
typedef void (doca_mmap_memrange_free_cb_t)(void *addr, size_t len, void *opaque);

/**
 * @brief Add memory range to DOCA memory map.
 *
 * @details  This operation is not permitted for:
 * - un-started/stopped memory map object.
 * - memory map object that have been exported or created from export.
 *
 * @param [in] mmap
 * DOCA memory map structure.
 * @param [in] addr
 * Start address of the memory range to be populated.
 * @param [in] len
 * The size of the memory range in bytes.
 * @param [in] pg_sz
 * Page size alignment of the provided memory range. Must be >= 4096 and a power of 2.
 * @param [in] free_cb
 * Callback function to free the populated memory range on memory map destroy.
 * @param [in] opaque
 * Opaque value to be passed to free_cb once called.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_PERMITTED - if doca_mmap status is invalid for this operation or device registration failed
 *                              or addr and len intersect with an existing chunk.
 * - DOCA_ERROR_NO_MEMORY - if reached to DOCA_MMAP_MAX_NUM_CHUNKS, or memory allocation failed.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_populate(struct doca_mmap *mmap, void *addr, size_t len, size_t pg_sz,
				doca_mmap_memrange_free_cb_t *free_cb, void *opaque);

/**
 * @brief Compose memory map representation for later import with
 * doca_mmap_create_from_export() for one of the devices previously added to
 * the memory map.
 *
 * @details Once this function called on the object it considered as exported.
 *
 * Freeing memory buffer pointed by `*export_desc` is the caller responsibility.
 *
 * This operation is not permitted for:
 * - un-started/stopped memory map object.
 * - memory map object that have been exported or created from export.
 *
 * The following are NOT possible after export:
 * - Setting the properties of the mmap using doca_mmap_property_set().
 * - Adding a device to the mmap using doca_mmap_dev_add().
 * - Removing a device to the mmap using doca_mmap_dev_rm().
 * - Adding a memory range to the mmap using doca_mmap_populate().
 * - Exporting the mmap using doca_mmap_export().
 *
 * @param [in] mmap
 * DOCA memory map structure.
 * @param [in] dev
 * Device previously added to the memory map via doca_mmap_dev_add().
 * Device must have export capability. See doca_devinfo_get_is_mmap_export_supported() in doca_dev.h
 * @param [out] export_desc
 * On successful return should have a pointer to the allocated blob containing serialized representation of the memory
 * map object for the device provided as `dev`.
 * @param [out] export_desc_len
 * Length in bytes of the export_desc.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received or device does not exists in mmap.
 * - DOCA_ERROR_NOT_PERMITTED - the operation is not premitted for the given mmap,
 * 				see details in this function description.
 * The following errors will occur if failed to produce export descriptor:
 * - DOCA_ERROR_NO_MEMORY - if failed to alloc memory for export_desc.
 * - DOCA_ERROR_NOT_SUPPORTED - device missing export capability.
 * - DOCA_ERROR_DRIVER
 *
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_export(struct doca_mmap *mmap, const struct doca_dev *dev, void **export_desc,
			      size_t *export_desc_len);

/**
 * @brief Creates a memory map object representing memory ranges in remote system
 * memory space.
 *
 * @details Once this function called on the object it considered as from_export.
 *
 * The following are NOT possible for the mmap created from export:
 * - Setting the properties of the mmap using doca_mmap_property_set().
 * - Adding a device to the mmap using doca_mmap_dev_add().
 * - Removing a device to the mmap using doca_mmap_dev_rm().
 * - Adding a memory range to the mmap using doca_mmap_populate().
 * - Exporting the mmap using doca_mmap_export().
 *
 * @param [in] (optional) user_data
 * Identifier provided by user for the newly created DOCA memory map.
 * If not NULL, pointed user_data will be set.
 * @param [in] export_desc
 * An export descriptor generated by doca_mmap_export.
 * @param [in] export_desc_len
 * Length in bytes of the export_desc.
 * @param [in] dev
 * A local device connected to the device that resides in the exported mmap.
 * Device must have from export capability. See doca_devinfo_get_is_mmap_from_export_supported() in doca_dev.h
 * @param [out] mmap
 * DOCA memory map granting access to remote memory.
 *
 * @note: The created object not backed by local memory.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received or internal error.
 * The following errors are internal and will occur if failed to produce new mmap from export descriptor:
 * - DOCA_ERROR_NO_MEMORY - if internal memory allocation failed.
 * - DOCA_ERROR_NOT_SUPPORTED - device missing create from export capability.
 * - DOCA_ERROR_NOT_PERMITTED
 * - DOCA_ERROR_DRIVER
 *
 * Limitation: Can only support mmap consisting of a single chunk.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_create_from_export(const union doca_data *user_data, const void *export_desc, size_t export_desc_len,
					  struct doca_dev *dev, struct doca_mmap **mmap);

/*******************************************************************************
 * DOCA Memory Map properties
 ******************************************************************************/

/**
 * @brief Get the user_data of a DOCA Memory Map.
 *
 * @note The user_data that was provided to the mmap upon its creation.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 * @param [out] user_data
 * The user_data of mmap.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_get_user_data(struct doca_mmap *mmap, union doca_data *user_data);

/**
 * @brief Get the max number of chunks to populate in a DOCA Memory Map.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 * @param [out] max_num_chunks
 * The max number of chunks to populate in mmap.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_get_max_num_chunks(struct doca_mmap *mmap, uint32_t *max_num_chunks);

/**
 * @brief Get the max number of devices to add to a DOCA Memory Map.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 * @param [out] max_num_devices
 * The max number of devices that can be added add to mmap.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_get_max_num_devices(struct doca_mmap *mmap, uint32_t *max_num_devices);

/**
 * @brief Get the Total number of `struct doca_buf` objects pointing to the memory in a DOCA Memory Map.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 * @param [out] num_bufs
 * The total number of `struct doca_buf` objects pointing to the memory in mmap.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_get_num_bufs(struct doca_mmap *mmap, uint32_t *num_bufs);

/**
 * @brief Get the flag indicating if a DOCA Memory Map had been exported.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 * @param [out] exported
 * 1 if mmap had been exported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_get_exported(struct doca_mmap *mmap, uint8_t *exported);

/**
 * @brief Get the flag indicating if a DOCA Memory Map had been created from an export.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 * @param [out] from_export
 * 1 if mmap had been created from export, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_get_from_export(struct doca_mmap *mmap, uint8_t *from_export);

/**
 * @brief Set a new max number of chunks to populate in a DOCA Memory Map.
 * Note: once a memory map object has been first started this functionality will not be available.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 * @param [in] max_num_chunks
 * The new max number of chunks to populate in mmap.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_PERMITTED - if trying to set the max number of chunks after first start of the mmap.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_set_max_num_chunks(struct doca_mmap *mmap, uint32_t max_num_chunks);

/**
 * @brief Set a new max number of devices to add to a DOCA Memory Map.
 * Note: once a memory map object has been first started this functionality will not be available.
 *
 * @param [in] mmap
 * The DOCA memory map structure.
 * @param [in] max_num_devices
 * The new max number of devices that can be added add to mmap.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_PERMITTED - if trying to set the max number of devices after first start of the mmap.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_mmap_set_max_num_devices(struct doca_mmap *mmap, uint32_t max_num_devices);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_MMAP_H_ */
