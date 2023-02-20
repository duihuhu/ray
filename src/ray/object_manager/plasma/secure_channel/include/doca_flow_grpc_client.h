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
 * @file doca_flow_grpc_client.h
 * @page doca flow grpc client
 * @defgroup GRPC Flow
 * DOCA flow grpc API to run remote HW offload with flow library.
 * For more details please refer to the user guide on DOCA devzone.
 *
 * @{
 */

#ifndef DOCA_FLOW_GRPC_CLIENT_H_
#define DOCA_FLOW_GRPC_CLIENT_H_

#include <doca_flow.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief General DOCA Flow response struct
 */
struct doca_flow_grpc_response {
	bool success;
	/**< in case of success should be true */
	struct doca_flow_error error;
	/**< Otherwise, this field contains the error information */
	/* in case of success one of the following may be used or more */
	uint64_t pipe_id;
	/**< pipe id */
	uint64_t entry_id;
	/**< entry id */
	uint64_t switch_port_id;
	/**< switch port id */
	int aging_res;
	/**< return value from handle aging */
	uint64_t nb_entries_processed;
	/**< return value from entries process */
	enum doca_flow_entry_status entry_status;
	/**< return value of entry get status */
};

/**
 * @brief pipeline configuration wrapper
 */
struct doca_flow_grpc_pipe_cfg {
	struct doca_flow_pipe_cfg *cfg;
	/**< doca_flow_pipe_cfg struct */
	uint16_t port_id;
	/**< port id */
};

/**
 * @brief forwarding configuration wrapper
 */
struct doca_flow_grpc_fwd {
	struct doca_flow_fwd *fwd;
	/**< doca flow fwd struct */
	uint64_t next_pipe_id;
	/**< next pipe id */
};

/**
 * @brief doca flow grpc bindable object types
 */
enum doca_flow_grpc_bindable_obj_type {
	DOCA_FLOW_GRPC_BIND_TYPE_PIPE,
	/**< bind resource to a pipe */
	DOCA_FLOW_GRPC_BIND_TYPE_PORT,
	/**< bind resource to a port */
	DOCA_FLOW_GRPC_BIND_TYPE_NULL,
	/**< bind resource globally */
};

/**
 * @brief bindable object configuration
 */
struct doca_flow_grpc_bindable_obj {
	enum doca_flow_grpc_bindable_obj_type type;
	/**< bindable object type */
	union {
		uint32_t port_id;
		/**< port id if type is port */
		uint64_t pipe_id;
		/**< pipe id if type is pipe */
	};
};

/**
 * @brief Initialize a channel to DOCA flow grpc server.
 *
 * Must be invoked first before any other function in this API.
 * this is a one time call, used for grpc channel initialization.
 *
 * @param grpc_address
 * String representing the service ip, i.e. "127.0.0.1" or "192.168.100.3:5050".
 * If no port is provided, it will use the service default port.
 */
__DOCA_EXPERIMENTAL
void doca_flow_grpc_client_create(const char *grpc_address);

/**
 * @brief RPC call for doca_flow_init().
 *
 * @param cfg
 * Program configuration, see doca_flow_cfg for details.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_init(const struct doca_flow_cfg *cfg);

/**
 * @brief RPC call for doca_flow_port_start().
 *
 * @param cfg
 * Port configuration, see doca_flow_port_cfg for details.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_port_start(const struct doca_flow_port_cfg *cfg);

/**
 * @brief RPC call for doca_flow_port_stop().
 *
 * @param port_id
 * Port ID.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_port_stop(uint16_t port_id);

/**
 * @brief RPC call for doca_flow_port_pair().
 *
 * @param port_id
 * port ID.
 * @param pair_port_id
 * pair port ID.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_port_pair(uint16_t port_id, uint16_t pair_port_id);

/**
 * @brief RPC call for doca_flow_shared_resource_cfg().
 *
 * @param type
 * Shared resource type.
 * @param id
 * Shared resource id.
 * @param cfg
 * Pointer to a shared resource configuration.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_shared_resource_cfg(enum doca_flow_shared_resource_type type,
		uint32_t id, struct doca_flow_shared_resource_cfg *cfg);

/**
 * @brief RPC call for doca_flow_shared_resources_bind().
 *
 * @param type
 * Shared resource type.
 * @param res_array
 * Array of shared resource IDs.
 * @param res_array_len
 * Shared resource IDs array length.
 * @param bindable_obj_id
 * Pointer to a bindable object ID.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_shared_resources_bind(enum doca_flow_shared_resource_type type,
		uint32_t *res_array, uint32_t res_array_len, struct doca_flow_grpc_bindable_obj *bindable_obj_id);

/**
 * @brief RPC call for doca_flow_shared_resources_query().
 *
 * @param type
 * Shared object type.
 * @param res_array
 * Array of shared objects IDs to query.
 * @param query_results_array
 * Data array retrieved by the query.
 * @param array_len
 * Number of objects and their query results in their arrays (same number).
 * @return
 * 0 on success, negative on failure.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_shared_resources_query(enum doca_flow_shared_resource_type type,
		uint32_t *res_array, struct doca_flow_shared_resource_result *query_results_array, uint32_t array_len);

/**
 * @brief RPC call for doca_flow_pipe_create().
 *
 * @param cfg
 * Pipe configuration, see doca_flow_grpc_pipe_cfg for details.
 * @param fwd
 * Fwd configuration for the pipe.
 * @param fwd_miss
 * Fwd_miss configuration for the pipe. NULL for no fwd_miss.
 * When creating a pipe if there is a miss and fwd_miss configured,
 * packet steering should jump to it.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_pipe_create(const struct doca_flow_grpc_pipe_cfg *cfg,
		const struct doca_flow_grpc_fwd *fwd, const struct doca_flow_grpc_fwd *fwd_miss);

/**
 * @brief RPC call for doca_flow_pipe_add_entry().
 *
 * @param pipe_queue
 * Queue identifier.
 * @param pipe_id
 * Pipe ID.
 * @param match
 * Pointer to match, indicate specific packet match information.
 * @param actions
 * Pointer to modify actions, indicate specific modify information.
 * @param monitor
 * Pointer to monitor actions.
 * @param client_fwd
 * Pointer to fwd actions.
 * @param flags
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_flags_type.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_pipe_add_entry(uint16_t pipe_queue,
		uint64_t pipe_id, const struct doca_flow_match *match, const struct doca_flow_actions *actions,
		const struct doca_flow_monitor *monitor, const struct doca_flow_grpc_fwd *client_fwd, uint32_t flags);

/**
 * @brief RPC call for doca_flow_pipe_control_add_entry().
 *
 * @param pipe_queue
 * Queue identifier.
 * @param priority
 * Priority value..
 * @param pipe_id
 * Pipe ID.
 * @param match
 * Pointer to match, indicate specific packet match information.
 * @param match_mask
 * Pointer to match mask information.
 * @param client_fwd
 * Pointer to fwd actions.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_pipe_control_add_entry(uint16_t pipe_queue, uint8_t priority,
		uint64_t pipe_id, const struct doca_flow_match *match,
		const struct doca_flow_match *match_mask, const struct doca_flow_grpc_fwd *client_fwd);

/**
 * @brief RPC call for doca_flow_pipe_lpm_add_entry().
 *
 * @param pipe_queue
 * Queue identifier.
 * @param pipe_id
 * Pipe ID.
 * @param match
 * Pointer to match, indicate specific packet match information.
 * @param match_mask
 * Pointer to match mask information.
 * @param actions
 * Pointer to modify actions, indicate specific modify information.
 * @param monitor
 * Pointer to monitor actions.
 * @param client_fwd
 * Pointer to fwd actions.
 * @param flag
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_flags_type.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response
doca_flow_grpc_pipe_lpm_add_entry(uint16_t pipe_queue, uint64_t pipe_id, const struct doca_flow_match *match,
		const struct doca_flow_match *match_mask, const struct doca_flow_actions *actions,
		const struct doca_flow_monitor *monitor, const struct doca_flow_grpc_fwd *client_fwd,
		const enum doca_flow_flags_type flag);

/**
 * @brief RPC call for doca_flow_grpc_pipe_rm_entry().
 *
 * @param pipe_queue
 * Queue identifier.
 * @param entry_id
 * The entry ID to be removed.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_pipe_rm_entry(uint16_t pipe_queue, uint64_t entry_id);

/**
 * @brief RPC call for doca_flow_port_pipes_flush().
 *
 * @param port_id
 * Port ID.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_port_pipes_flush(uint16_t port_id);

/**
 * @brief RPC call for doca_flow_port_pipes_dump().
 *
 * @param port_id
 * Port ID.
 * @param f
 * The output file of the pipe information.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_port_pipes_dump(uint16_t port_id, FILE *f);

/**
 * @brief RPC call for doca_flow_pipe_destroy().
 *
 * @param pipe_id
 * Pipe ID.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_pipe_destroy(uint64_t pipe_id);

/**
 * @brief RPC call for doca_flow_port_destroy().
 *
 * @param port_id
 * Port ID.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_port_destroy(uint16_t port_id);

/**
 * @brief RPC call for doca_flow_query().
 *
 * @param entry_id
 * The pipe entry ID to query.
 * @param query_stats
 * Data retrieved by the query.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_query(uint64_t entry_id, struct doca_flow_query *query_stats);


/**
 * @brief RPC call for doca_flow_aging_handle().
 *
 * @param port_id
 * Port id to handle aging
 * @param queue
 * Queue identifier.
 * @param quota
 * Max time quota in micro seconds for this function to handle aging.
 * @param entries_id
 * User input entries array for the aged flows.
 * @param len
 * User input length of entries array.
 * @return
 * doca_flow_grpc_response.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_aging_handle(uint16_t port_id, uint16_t queue,
		uint64_t quota, uint64_t *entries_id, int len);

/**
 * @brief RPC call for doca_flow_grpc_entries_process().
 *
 * @param port_id
 * Port ID
 * @param pipe_queue
 * Queue identifier.
 * @param timeout
 * Max time in micro seconds for this function to process entries.
 * Process once if timeout is 0
 * @param max_processed_entries
 * Flow entries number to process
 * If it is 0, it will proceed until timeout.
 * @return
 * doca_flow_grpc_response
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_entries_process(uint16_t port_id,	uint16_t pipe_queue,
		uint64_t timeout, uint32_t max_processed_entries);

/**
 * @brief RPC call for doca_flow_pipe_entry_get_status()
 *
 * @param entry_id
 * pipe entry ID
 * @return
 * doca_flow_grpc_response
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_pipe_entry_get_status(uint64_t entry_id);

/**
 * @brief RPC call for doca_flow_port_switch_get()
 *
 * @return
 * doca_flow_grpc_response
 *
 */
__DOCA_EXPERIMENTAL
struct doca_flow_grpc_response doca_flow_grpc_port_switch_get(void);

/**
 * @brief RPC call for doca_flow_destroy().
 */
__DOCA_EXPERIMENTAL
void doca_flow_grpc_destroy(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_GRPC_CLIENT_H_ */
