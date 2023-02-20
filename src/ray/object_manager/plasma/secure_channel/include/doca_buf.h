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
 * @file doca_buf.h
 * @page doca buf
 * @defgroup DOCACore Core
 * @defgroup BUF DOCA Buffer
 * @ingroup DOCACore
 * The DOCA Buffer is used for reference data. It holds the information on a memory region that belongs to
 * a DOCA memory map, and its descriptor is allocated from DOCA Buffer Inventory.
 *
 * @{
 */

#ifndef DOCA_BUF_H_
#define DOCA_BUF_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "doca_compat.h"
#include "doca_error.h"

#ifdef __cplusplus
extern "C" {
#endif

struct doca_dev;

/*******************************************************************************
 * DOCA Buffer element
 ******************************************************************************/

/**
 * @brief Opaque structure representing a data buffer, that can be read by registered DOCA devices.
 */
struct doca_buf;

enum doca_buf_extension {
	DOCA_BUF_EXTENSION_NONE = 0,
	DOCA_BUF_EXTENSION_LINKED_LIST	= 1 << 0, /**< Linked list extension */
};

/**
 * @brief Increase the object reference count by 1.
 *
 * @param [in] buf
 * DOCA Buf element.
 * @param [out] refcount
 * The number of references to the object before this operation took place.
 *
 * @return
 * - DOCA_ERROR_NOT_SUPPORTED
 *
 * @note This function is not supported yet.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_refcount_add(struct doca_buf *buf, uint16_t *refcount);

/**
 * @brief Decrease the object reference count by 1, if 0 reached, return the element back to the inventory.
 *
 * @details When refcont 0 reached, all related resources should be released. For example if the element points into
 * some mmap its state will be adjusted accordingly.
 * If DOCA_BUF_EXTENSION_LINKED_LIST is selected the buf must be the head of a list
 *
 * @param [in] buf
 * DOCA Buf element.
 * @param [out] refcount
 * The number of references to the object before this operation took place.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_refcount_rm(struct doca_buf *buf, uint16_t *refcount);

/**
 * @brief Get the reference count of the object.
 *
 * @param [in] buf
 * DOCA Buf element.
 * @param [out] refcount
 * The number of references to the object.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_get_refcount(struct doca_buf *buf, uint16_t *refcount);

/*******************************************************************************
 * DOCA Buffer: Data placement
 *******************************************************************************
 *
 * head   -->            +-------------------+
 *                       |                   |
 *                       |                   |
 *                       |                   |
 *                       |                   |
 *                       |                   |
 * data   -->            +-------------------+
 *                       | data              |
 *                       |                   |
 *                       |                   |
 *                       |                   |
 *                       |                   |
 *                       |                   |
 * data + data_len -->   +-------------------+
 *                       |                   |
 *                       |                   |
 *                       |                   |
 *                       |                   |
 * head + len      -->   +-------------------+
*/

/**
 * @brief Get the buffer's length.
 *
 * @param [in] buf
 * DOCA Buf element.
 * @param [out] len
 * The length of the buffer.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_get_len(struct doca_buf *buf, size_t *len);

/**
 * @brief Get lkey with doca_access_flags access for a DOCA buffer of a DOCA device.
 *
 * @param [in] buf
 * The DOCA buffer to get lkey for.
 *
 * @param [in] dev
 * The DOCA device to get lkey for.
 *
 * @param [in] doca_access_flags
 * LKey access flags (see enum doca_access_flags).
 *
 * @param [out] lkey
 * The returned LKey.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received or if cannot find mkey by the given device.
 * - DOCA_ERROR_NOT_SUPPORTED - if the given access flags is not supported
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_get_lkey(const struct doca_buf *buf, struct doca_dev *dev, uint32_t doca_access_flags, uint32_t *lkey);

/**
 * @brief Get the buffer's head.
 *
 * @param [in] buf
 * DOCA Buf element.
 * @param [out] head
 * The head of the buffer.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_get_head(struct doca_buf *buf, void **head);

/**
 * @brief Get buffer's data length.
 *
 * @param [in] buf
 * DOCA Buf element.
 * @param [out] data_len
 * The data length of the buffer.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_get_data_len(struct doca_buf *buf, size_t *data_len);

/**
 * @brief Get the buffer's data.
 *
 * @param [in] buf
 * DOCA Buf element.
 * @param [out] data
 * The data of the buffer.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_get_data(struct doca_buf *buf, void **data);

/**
 * Set data pointer and data length
 *
 *
 *         +-----------+-----+-----------------+
 * Before  |           |data |                 |
 *         +-----------+-----+-----------------+
 *
 *                 __data_len__
 *                /            \
 *         +-----+--------------+--------------+
 * After   |     |data          |              |
 *         +-----+--------------+--------------+
 *              /
 *            data
 *
 * @param [in] buf
 * DOCA Buf element.
 * @param [in] data
 * Data address.
 * @param [in] data_len
 * Data length.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * doca_error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received
 *                              or if data address and length are outside buffer's limits.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_set_data(struct doca_buf *buf, void *data, size_t data_len);

/*******************************************************************************
 * DOCA Buffer: Linked List extension
 ******************************************************************************/

/**
 * @brief Get next DOCA Buf in linked list.
 *
 * @param [in] buf
 * DOCA Buf element. Buf must have the linked list extension.
 *
 * @param [out] next_buf
 * The next DOCA Buf or null if the 'Linked List' extension of the buffer is disabled.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_list_next(struct doca_buf *buf, struct doca_buf **next_buf);

/**
 * @brief Get last DOCA Buf in linked list.
 *
 * @param [in] buf
 * DOCA Buf element. Buf must have the linked list extension.
 *
 * @param [out] last_buf
 * The last DOCA Buf in the linked list, which may be buf, or buf if the 'Linked List' extension of the buffer
 * is disabled.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_list_last(struct doca_buf *buf, struct doca_buf **last_buf);

/**
 * @brief Check if provided DOCA Buf is the last element in a linked list.
 *
 * @param [in] buf
 * DOCA Buf element. Buf must have the linked list extension.
 *
 * @param [out] is_last
 * If 'Linked List' extension is enabled: true if buf is the last element, false if it is not
 * If 'Linked List' extension is disabled: true
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_list_is_last(const struct doca_buf *buf, bool *is_last);

/**
 * @brief Check if provided DOCA Buf is the first element in a linked list.
 *
 * @param [in] buf
 * DOCA Buf element. Buf must have the linked list extension.
 *
 * @param [out] is_first
 * If 'Linked List' extension is enabled: true if buf is the first element, false if it is not
 * If 'Linked List' extension is disabled: true
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_list_is_first(const struct doca_buf *buf, bool *is_first);

/**
 * @brief Get the number of the elements in list.
 *
 * @param [in] buf
 * DOCA Buf element. Buf must be a head of a list and have the linked list extension.
 *
 * @param [out] num_elements
 * If 'Linked List' extension is enabled: Number of elements in list
 * If 'Linked List' extension is disabled: 1
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_PERMITTED - if the buffer is not a head of a list.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_list_num_elements(const struct doca_buf *buf, uint32_t *num_elements);

/**
 * @brief Append list2 to list1.
 *
 * Before:
 *           +----+  +----+  +----+
 * list1 ->  |1   |->|2   |->|3   |
 *           +----+  +----+  +----+
 *
 *           +----+  +----+
 * list2 ->  |4   |->|5   |
 *           +----+  +----+
 *
 * After:
 *
 *           +----+  +----+  +----+  +----+  +----+
 * list1 ->  |1   |->|2   |->|3   |->|4   |->|5   |
 *           +----+  +----+  +----+  +----+  +----+
 *                                  /
 *                               list2
 * @param [in] list1
 * DOCA Buf representing list1. Buf must be the head of the list and have the linked list extension.
 *
 * @param [in] list2
 * DOCA Buf representing list2. Buf must be the head of the list and have the linked list extension.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_PERMITTED - if one of the buffers is not a head of a list
 *                              or the 'Linked List' extension of one of the buffers is disabled.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_list_chain(struct doca_buf *list1, struct doca_buf *list2);

/**
 * @brief Separate list2 from list1.
 *
 * Before:
 *           +----+  +----+  +----+  +----+  +----+
 * list1 ->  |1   |->|2   |->|3   |->|4   |->|5   |
 *           +----+  +----+  +----+  +----+  +----+
 *                                  /
 *                               list2
 *
 * After:
 *           +----+  +----+  +----+
 * list1 ->  |1   |->|2   |->|3   |
 *           +----+  +----+  +----+
 *
 *           +----+  +----+
 * list2 ->  |4   |->|5   |
 *           +----+  +----+
 *
 * @param [in] list1
 * DOCA Buf representing list1. Must have the linked list extension.
 * @param [in] list2
 * DOCA Buf representing list2, list2 should be contained in list1.
 * list2 must be different from list1 and must have the linked list extension.
 *
 * @return
 * DOCA_SUCCESS - in case of success.
 * Error code - in case of failure:
 * - DOCA_ERROR_INVALID_VALUE - if an invalid input had been received.
 * - DOCA_ERROR_NOT_PERMITTED - if the 'Linked List' extension of one of the buffers is disabled.
 */
__DOCA_EXPERIMENTAL
doca_error_t doca_buf_list_unchain(struct doca_buf *list1, struct doca_buf *list2);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* DOCA_BUF_H_ */
