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
 * @file doca_dev.h
 * @page doca dev
 * @defgroup DEV DOCA Device
 * @ingroup DOCACore
 * The DOCA device represents an available processing unit backed by the HW or SW implementation.
 *
 * @{
 */

#ifndef DOCA_DEV_H_
#define DOCA_DEV_H_

#include <stdint.h>

#include "doca_types.h"
#include "doca_compat.h"
#include "doca_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Opaque structure representing a local DOCA Device Info instance.
 * This structure is used to query information about the local device.
 */
struct doca_devinfo;
/**
 * @brief Opaque structure representing a representor DOCA Device Info instance.
 * This structure is used to query information about the representor device.
 */
struct doca_devinfo_rep;

/**
 * @brief Opaque structure representing a local DOCA Device instance.
 * This structure is used by libraries for accessing the underlying local device.
 */
struct doca_dev;
/**
 * @brief Opaque structure representing a representor DOCA Device instance.
 * This structure is used by libraries for accessing the underlying representor device.
 */
struct doca_dev_rep;

/**
 * Representor device filter by flavor
 *
 * Multiple options possible but some are mutually exclusive.
 *
 */
enum doca_dev_rep_filter {
	DOCA_DEV_REP_FILTER_ALL		= 0,
	DOCA_DEV_REP_FILTER_NET		= 1 << 1,
	DOCA_DEV_REP_FILTER_VIRTIO_FS	= 1 << 2,
	DOCA_DEV_REP_FILTER_VIRTIO_NET	= 1 << 3,
	DOCA_DEV_REP_FILTER_VIRTIO_BLK	= 1 << 4,
	DOCA_DEV_REP_FILTER_NVME	= 1 << 5,
};

/**
 * @brief Creates list of all available local devices.
 *
 * Lists information about available devices, to start using the device you first have to call doca_dev_open(),
 * while passing an element of this list. List elements become invalid once it has been destroyed.
 *
 * @param [out] dev_list
 * Pointer to array of pointers. Output can then be accessed as follows (*dev_list)[idx].
 * @param [out] nb_devs
 * Number of available local devices.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate enough space.
 * - DOCA_ERROR_NOT_FOUND - failed to get RDMA devices list
 * @note Returned list must be destroyed using doca_devinfo_list_destroy()
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_list_create(struct doca_devinfo ***dev_list, uint32_t *nb_devs);

/**
 * @brief Destroy list of local device info structures.
 *
 * Destroys the list of device information, once the list has been destroyed, all elements become invalid.
 *
 * @param [in] dev_list
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_IN_USE - at least one device in the list is in a corrupted state.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_list_destroy(struct doca_devinfo **dev_list);

/**
 * @brief Create list of available representor devices accessible by dev.
 *
 * Returns all representors managed by the provided device.
 * The provided device must be a local device.
 * The representor may represent a network function attached to the host,
 * or it can represent an emulated function attached to the host.
 *
 * @param [in] dev
 * Local device with access to representors.
 * @param [in] filter
 * Bitmap filter of representor types. See enum doca_dev_rep_filter for more details.
 * @param [out] dev_list_rep
 * Pointer to array of pointers. Output can then be accessed as follows (*dev_list_rep)[idx].
 * @param [out] nb_devs_rep
 * Number of available representor devices.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for list.
 * - DOCA_ERROR_NOT_SUPPORTED - local device does not expose representor devices.
 * @note Returned list must be destroyed using doca_devinfo_rep_list_destroy()
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_list_create(struct doca_dev *dev, int filter,
					  struct doca_devinfo_rep ***dev_list_rep,
					  uint32_t *nb_devs_rep);

/**
 * @brief Destroy list of representor device info structures.
 *
 * Destroy list of representor device information, once the list has been destroyed,
 * all elements of the list are considered invalid.
 *
 * @param [in] dev_list_rep
 * List to be destroyed.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_IN_USE - the doca_dev that created the list is in a corrupted state.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_list_destroy(struct doca_devinfo_rep **dev_list_rep);

/**
 * @brief Initialize local device for use.
 *
 * @param [in] devinfo
 * The devinfo structure of the requested device.
 * @param [out] dev
 * Initialized local doca device instance on success. Valid on success only.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate protection domain for device.
 * - DOCA_ERROR_NOT_CONNECTED - failed to open device.
 * @note In case the same device was previously opened, then the same doca_dev instance is returned.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dev_open(struct doca_devinfo *devinfo, struct doca_dev **dev);

/**
 * @brief Destroy allocated local device instance.
 *
 * @param [in] dev
 * The local doca device instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * - DOCA_ERROR_IN_USE - failed to deallocate device resources.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dev_close(struct doca_dev *dev);

/**
 * @brief Initialize representor device for use.
 *
 * @param [in] devinfo
 * The devinfo structure of the requested device.
 * @param [out] dev_rep
 * Initialized representor doca device instance on success. Valid on success only.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - failed to allocate memory for device.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dev_rep_open(struct doca_devinfo_rep *devinfo, struct doca_dev_rep **dev_rep);

/**
 * @brief Destroy allocated representor device instance.
 *
 * @param [in] dev
 * The representor doca device instance.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_IN_USE - failed to deallocate device resources.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_dev_rep_close(struct doca_dev_rep *dev);

/**
 * @brief Get local device info from device.
 * This should be useful when wanting to query information about device after opening it,
 * and destroying the devinfo list.
 *
 * @param [in] dev
 * The doca device instance.
 *
 * @return
 * The matching doca_devinfo instance in case of success, NULL in case dev is invalid.
 */
__DOCA_EXPERIMENTAL
struct doca_devinfo *doca_dev_as_devinfo(const struct doca_dev *dev);

/**
 * @brief Get representor device info from device.
 * This should be useful when wanting to query information about device after opening it,
 * and destroying the devinfo list.
 *
 * @param [in] dev_rep
 * The representor doca device instance.
 *
 * @return
 * The matching doca_devinfo_rep instance in case of success, NULL in case dev_rep is invalid.
 */
__DOCA_EXPERIMENTAL
struct doca_devinfo_rep *doca_dev_rep_as_devinfo(struct doca_dev_rep *dev_rep);

/*********************************************************************************************************************
 * DOCA Local Device Info Properties
 *********************************************************************************************************************/

#define DOCA_DEVINFO_VUID_SIZE 128
#define DOCA_DEVINFO_IPV4_ADDR_SIZE 4
#define DOCA_DEVINFO_IPV6_ADDR_SIZE 16
#define DOCA_DEVINFO_IFACE_NAME_SIZE 256
#define DOCA_DEVINFO_IBDEV_NAME_SIZE 64

/**
 * @brief Get the PCI address of a DOCA devinfo.
 *
 * @details The BDF of the device - same as the address in lspci.
 * The PCI address type: struct doca_pci_bdf.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] pci_addr
 * The PCI address of devinfo.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NOT_CONNECTED - provided devinfo does not support this property.
 * - DOCA_ERROR_OPERATING_SYSTEM - failed to acquire the PCI address from the OS
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_get_pci_addr(const struct doca_devinfo *devinfo, struct doca_pci_bdf *pci_addr);

/**
 * @brief Get the IPv4 address of a DOCA devinfo.
 *
 * @details The IPv4 address type: uint8_t[DOCA_DEVINFO_IPV4_ADDR_SIZE].
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] ipv4_addr
 * The IPv4 address of devinfo.
 * @param [in] size
 * The size of the input ipv4_addr buffer, must be at least DOCA_DEVINFO_IPV4_ADDR_SIZE
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_OPERATING_SYSTEM - failed to acquire the IPv4 address from the OS
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_get_ipv4_addr(const struct doca_devinfo *devinfo,
					uint8_t *ipv4_addr, uint32_t size);

/**
 * @brief Get the IPv6 address of a DOCA devinfo.
 *
 * @details The IPv6 address type: uint8_t[DOCA_DEVINFO_IPV6_ADDR_SIZE].
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] ipv6_addr
 * The IPv6 address of devinfo.
 * @param [in] size
 * The size of the input ipv6_addr buffer, must be at least DOCA_DEVINFO_IPV6_ADDR_SIZE
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_OPERATING_SYSTEM - failed to acquire the IPv6 address from the OS
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_get_ipv6_addr(const struct doca_devinfo *devinfo,
					uint8_t *ipv6_addr, uint32_t size);
/**
 * @brief Get the name of the ethernet interface of a DOCA devinfo.
 *
 * @details The name of the ethernet interface is the same as it's name in ifconfig.
 * The name of the ethernet interface type: char[DOCA_DEVINFO_IFACE_NAME_SIZE].
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] iface_name
 * The name of the ethernet interface of devinfo.
 * @param [in] size
 * The size of the input iface_name buffer, must be at least DOCA_DEVINFO_IFACE_NAME_SIZE
 * which includes the null terminating byte.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_OPERATING_SYSTEM - failed to acquire the interface name from the OS
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_get_iface_name(const struct doca_devinfo *devinfo,
					 char *iface_name, uint32_t size);

/**
 * @brief Get the name of the IB device represented by a DOCA devinfo.
 *
 * @details The name of the IB device type: char[DOCA_DEVINFO_IBDEV_NAME_SIZE].
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] ibdev_name
 * The name of the IB device represented by devinfo.
 * @param [in] size
 * The size of the input ibdev_name buffer, must be at least DOCA_DEVINFO_IBDEV_NAME_SIZE
 * which includes the null terminating byte.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_NO_MEMORY - no memory (exception thrown).
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_get_ibdev_name(const struct doca_devinfo *devinfo,
					 char *ibdev_name, uint32_t size);

/**
 * @brief Get the hotplug manager capability of a DOCA devinfo.
 *
 * @details The hotplug manager property type: uint8_t*.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] is_hotplug_manager
 * 1 if the hotplug manager capability is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t
doca_devinfo_get_is_hotplug_manager_supported(const struct doca_devinfo *devinfo,
					      uint8_t *is_hotplug_manager);

/**
 * @brief Get the mmap export capability of the device.
 *
 * @details Get uint8_t value defining if the device can be used to export an mmap.
 * See doca_mmap_export() in doca_mmap.h
 * true  - device can be used with the mmap export API.
 * false - export API is guaranteed to faile with DOCA_ERROR_NOT_SUPPORTED.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] export
 * 1 if the mmap export capability is supported, 0 otherwise.
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t
doca_devinfo_get_is_mmap_export_supported(const struct doca_devinfo *devinfo,
					  uint8_t *mmap_export);

/**
 * @brief Get the mmap create from export capability of the device.
 *
 * @details Get uint8_t value defining if the device can be used to create an mmap from an exported mmap.
 * See doca_mmap_create_from_export() in doca_mmap.h
 * true  - device can be used with the mmap create from export API.
 * false - create from export API is guaranteed to fail with DOCA_ERROR_NOT_SUPPORTED.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] from_export
 * 1 if the mmap from export capability is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t
doca_devinfo_get_is_mmap_from_export_supported(const struct doca_devinfo *devinfo,
				     	       uint8_t *from_export);

/**
 * @brief Get the representor devices discovery capability of the device.
 *
 * @details Get uint8_t value defining if the device can be used to create list of representor devices.
 * In case true is returned, then this device supports at least one representor type.
 * See doca_devinfo_rep_list_create().
 * true  - device can be used with the remote list create API with filter DOCA_DEV_REP_FILTER_ALL.
 * false - providing DOCA_DEV_REP_FILTER_ALL is guaranteed to fail with DOCA_ERROR_NOT_SUPPORTED.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] all_supported
 * 1 if the rep list all capability is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_is_list_all_supported(const struct doca_devinfo *devinfo,
							uint8_t *all_supported);

/**
 * @brief Get the remote net discovery capability of the device.
 *
 * @details Get uint8_t value defining if the device can be used to create list of net remote devices.
 * See doca_devinfo_remote_list_create().
 * true  - device can be used with the remote list create API with filter DOCA_DEV_REMOTE_FILTER_NET.
 * false - providing DOCA_DEV_REMOTE_FILTER_NET is guaranteed to fail with DOCA_ERROR_NOT_SUPPORTED.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] net_supported
 * 1 if the rep list net capability is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_is_list_net_supported(const struct doca_devinfo *devinfo,
							uint8_t *net_supported);

/**
 * @brief Get the remote virtio fs discovery capability of the device.
 *
 * @details Get uint8_t value defining if the device can be used to create list of virtio fs remote devices.
 * See doca_devinfo_remote_list_create().
 * true  - device can be used with the remote list create API with filter DOCA_DEV_REMOTE_FILTER_VIRTIO_FS.
 * false - providing DOCA_DEV_REMOTE_FILTER_VIRTIO_FS is guaranteed to fail with DOCA_ERROR_NOT_SUPPORTED.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] virtio_fs_supported
 * 1 if the list virtio fs capability is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_is_list_virtio_fs_supported(const struct doca_devinfo *devinfo,
				       		 	      uint8_t *virtio_fs_supported);

/**
 * @brief Get the remote virtio net discovery capability of the device.
 *
 * @details Get uint8_t value defining if the device can be used to create list of virtio net remote devices.
 * See doca_devinfo_remote_list_create().
 * true  - device can be used with the remote list create API with filter DOCA_DEV_REMOTE_FILTER_VIRTIO_NET.
 * false - providing DOCA_DEV_REMOTE_FILTER_VIRTIO_NET is guaranteed to fail with DOCA_ERROR_NOT_SUPPORTED.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] virtio_net_supported
 * 1 if the list virtio net capability is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_is_list_virtio_net_supported(const struct doca_devinfo *devinfo,
						 	       uint8_t *virtio_net_supported);

/**
 * @brief Get the remote virtio blk discovery capability of the device.
 *
 * @details Get uint8_t value defining if the device can be used to create list of virtio blk remote devices.
 * See doca_devinfo_remote_list_create().
 * true  - device can be used with the remote list create API with filter DOCA_DEV_REMOTE_FILTER_VIRTIO_BLK.
 * false - providing DOCA_DEV_REMOTE_FILTER_VIRTIO_BLK is guaranteed to fail with DOCA_ERROR_NOT_SUPPORTED.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] virtio_blk_supported
 * 1 if the list virtio blk capability is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_is_list_virtio_blk_supported(const struct doca_devinfo *devinfo,
	 						       uint8_t *virtio_blk_supported);

/**
 * @brief Get the remote nvme discovery capability of the device.
 *
 * @details Get uint8_t value defining if the device can be used to create list of nvme remote devices.
 * See doca_devinfo_remote_list_create().
 * true  - device can be used with the remote list create API with filter DOCA_DEV_REMOTE_FILTER_NVME.
 * false - providing DOCA_DEV_REMOTE_FILTER_NVME is guaranteed to fail with DOCA_ERROR_NOT_SUPPORTED.
 *
 * @param [in] devinfo
 * The device to query.
 * @param [out] nvme_supported
 * 1 if the list nvme capability is supported, 0 otherwise.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 * - DOCA_ERROR_DRIVER - failed to query capability support.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_is_list_nvme_supported(const struct doca_devinfo *devinfo,
				  	    		 uint8_t *nvme_supported);

/*********************************************************************************************************************
 * DOCA Representor Device Info Properties
 *********************************************************************************************************************/

#define DOCA_DEVINFO_REP_VUID_SIZE 128

/**
 * @brief Get the Vendor Unique ID of a representor DOCA devinfo.
 *
 * @details The Vendor Unique ID is used as stable ID of a VF/PF.
 * The Vendor Unique ID type: char[DOCA_DEVINFO_VUID_SIZE].
 *
 * @param [in] devinfo_rep
 * The representor device to query.
 * @param [out] rep_vuid
 * The Vendor Unique ID of devinfo_rep.
 * @param [in] size
 * The size of the vuid buffer, including the terminating null byte ('\0').
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_vuid(const struct doca_devinfo_rep *devinfo_rep,
			 	       char *rep_vuid, uint32_t size);

/**
 * @brief Get the PCI address of a DOCA devinfo_rep.
 *
 * @details The PCI address type: struct doca_pci_bdf.
 *
 * @param [in] devinfo_rep
 * The representor of device to query.
 * @param [out] pci_addr
 * The PCI address of the devinfo_rep.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_pci_addr(const struct doca_devinfo_rep *devinfo_rep, struct doca_pci_bdf *pci_addr);

/**
 * @brief Get the PCI function type of a DOCA devinfo_rep.
 *
 * @details The pci function type: enum doca_pci_func_type.
 *
 * @param [in] devinfo_rep
 * The representor of device to query.
 * @param [out] pci_func_type
 * The PCI function type of the devinfo_rep.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - received invalid input.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_devinfo_rep_get_pci_func_type(const struct doca_devinfo_rep *devinfo_rep,
						enum doca_pci_func_type *pci_func_type);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_DEV_H_ */
