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
 * @file doca_flow.h
 * @page doca flow
 * @defgroup Flow flow
 * DOCA HW offload flow library. For more details please refer to the user guide
 * on DOCA devzone.
 *
 * @{
 */

#ifndef DOCA_FLOW_H_
#define DOCA_FLOW_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "doca_compat.h"

#include "doca_flow_net.h"
#include "doca_flow_crypto.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief doca flow port struct
 */
struct doca_flow_port;

/**
 * @brief doca flow pipeline struct
 */
struct doca_flow_pipe;

/**
 * @brief doca flow pipeline entry struct
 */
struct doca_flow_pipe_entry;

/**
 * @brief doca flow error type define
 */
enum doca_flow_error_type {
	DOCA_FLOW_ERROR_UNKNOWN,
	/**< Unknown error */
	DOCA_FLOW_ERROR_UNSUPPORTED,
	/**< Operation unsupported */
	DOCA_FLOW_ERROR_INVALID_PARAM,
	/**< Invalid parameter */
	DOCA_FLOW_ERROR_PIPE_BUILD_ITEM,
	/**< Build pipe match items error */
	DOCA_FLOW_ERROR_PIPE_MODIFY_ITEM,
	/**< Modify pipe match items error */
	DOCA_FLOW_ERROR_PIPE_BUILD_ACTION,
	/**< Build pipe actions error */
	DOCA_FLOW_ERROR_PIPE_MODIFY_ACTION,
	/**< Modify pipe actions error */
	DOCA_FLOW_ERROR_PIPE_BUILD_FWD,
	/**< Build pipe fwd error */
	DOCA_FLOW_ERROR_FLOW_CREATE,
	/**< Flow creation error */
	DOCA_FLOW_ERROR_FLOW_DESTROY,
	/**< Flow destroy error */
	DOCA_FLOW_ERROR_OOM,
	/**< Out of memory */
	DOCA_FLOW_ERROR_PORT,
	/**< Port error */
	DOCA_FLOW_ERROR_VERIFY_CONFIG,
	/**< Verification error */
};

/**
 * @brief doca flow error message struct
 */
struct doca_flow_error {
	enum doca_flow_error_type type;
	/**< Cause field and error types */
	const char *message;
	/**< Human-readable error message */
};

/**
 * @brief Shared resource supported types
 */
enum doca_flow_shared_resource_type {
	DOCA_FLOW_SHARED_RESOURCE_METER,
	/**< Shared meter type */
	DOCA_FLOW_SHARED_RESOURCE_COUNT,
	/**< Shared counter type */
	DOCA_FLOW_SHARED_RESOURCE_RSS,
	/**< Shared rss type */
	DOCA_FLOW_SHARED_RESOURCE_CRYPTO,
	/**< Shared crypto action type */
	DOCA_FLOW_SHARED_RESOURCE_MAX,
	/**< Shared max supported types */
};

/**
 * @brief doca flow flags type
 */
enum doca_flow_flags_type {
	DOCA_FLOW_NO_WAIT = 0,
	 /**< entry will not be buffered */
	DOCA_FLOW_WAIT_FOR_BATCH = (1 << 0),
	/**< entry will be buffered */
};

/**
 * @brief doca flow resource quota
 */
struct doca_flow_resources {
	uint32_t nb_counters;
	/**< Number of counters to configure */
	uint32_t nb_meters;
	/**< Number of traffic meters to configure */
};

/**
 * @brief doca flow entry operation
 */
enum doca_flow_entry_op {
	DOCA_FLOW_ENTRY_OP_ADD,
	/**< Add entry */
	DOCA_FLOW_ENTRY_OP_DEL,
	/**< Delete entry */
};

/**
 * @brief doca flow entry status
 */
enum doca_flow_entry_status {
	DOCA_FLOW_ENTRY_STATUS_IN_PROCESS,
	/* The operation is in progress. */
	DOCA_FLOW_ENTRY_STATUS_SUCCESS,
	/* The operation was completed successfully. */
	DOCA_FLOW_ENTRY_STATUS_ERROR,
	/* The operation failed. */
};

/**
 * @brief doca flow entry process callback
 */
typedef void (*doca_flow_entry_process_cb)(struct doca_flow_pipe_entry *entry,
	enum doca_flow_entry_status status,
	enum doca_flow_entry_op op, void *user_ctx);

/**
 * @brief doca flow shared resource unbind callback
 */
typedef void (*doca_flow_shared_resource_unbind_cb)(enum doca_flow_shared_resource_type,
						    uint32_t shared_resource_id,
						    void *bindable_obj);

/**
 * @brief doca flow global configuration
 */
struct doca_flow_cfg {
	uint16_t queues;
	/**< queue id for each offload thread */
	struct doca_flow_resources resource;
	/**< resource quota */
	const char *mode_args;
	/**< set doca flow architecture mode switch, vnf */
	uint32_t nr_shared_resources[DOCA_FLOW_SHARED_RESOURCE_MAX];
	/**< total shared resource per type */
	uint32_t queue_depth;
	/**< Number of pre-configured queue_size, default to 128 */
	doca_flow_entry_process_cb cb;
	/**< callback for entry create/destroy */
	doca_flow_shared_resource_unbind_cb unbind_cb;
	/**< callback for unbinding of a shared resource */
};

/**
 * @brief doca flow port type
 */
enum doca_flow_port_type {
	DOCA_FLOW_PORT_DPDK_BY_ID,
	/**< dpdk port by mapping id */
};

/**
 * @brief doca flow pipe type
 */
enum doca_flow_pipe_type {
	DOCA_FLOW_PIPE_BASIC,
	/**< Flow pipe */
	DOCA_FLOW_PIPE_CONTROL,
	/**< Control pipe */
	DOCA_FLOW_PIPE_LPM,
	/**< longest prefix match (LPM) pipe */
	DOCA_FLOW_PIPE_ORDERED_LIST,
	/**< Ordered list pipe */
};

/**
 * @brief doca flow port configuration
 */
struct doca_flow_port_cfg {
	uint16_t port_id;
	/**< dpdk port id */
	enum doca_flow_port_type type;
	/**< mapping type of port */
	const char *devargs;
	/**< specific per port type cfg */
	uint16_t priv_data_size;
	/**< user private data */
};

/**
 * @brief Mapping to doca flow switch port
 */
#define DOCA_FLOW_SWITCH doca_flow_port_switch_get()

/**
 * Max meta data size in bytes.
 */
#define DOCA_FLOW_META_MAX 20

/**
 * External meta data size in bytes.
 */
#define DOCA_FLOW_META_EXT 12

/**
 * @brief doca flow meta data
 *
 * Meta data known as scratch data can be used to match or modify within pipes.
 * Meta data can be set with value in previous pipes and match in later pipes.
 * User can customize meta data structure as long as overall size doens't exceed limit.
 * To match meta data, mask must be specified when creating pipe.
 * Struct must be aligned to 32 bits.
 * No initial value for Meta data, must match after setting value.
 */
struct doca_flow_meta {
	union {
		uint32_t pkt_meta; /**< Shared with application via packet. */
		struct {
			uint32_t lag_port :2; /**< Bits of LAG member port. */
			uint32_t type :2; /**< 0: traffic 1: SYN 2: RST 3: FIN. */
			uint32_t zone :28; /**< Zone ID for CT processing. */
		} ct;
	};
	uint32_t u32[DOCA_FLOW_META_MAX / 4 - 1]; /**< Programmable user data. */
	uint32_t port_meta; /**< Programmable source vport. */
	uint32_t mark; /**< Mark id. */
	uint8_t nisp_syndrome; /**< NISP decrypt/authentication syndrome. */
	uint8_t ipsec_syndrome; /**< IPsec decrypt/authentication syndrome. */
	uint8_t align[2]; /**< Structure alignment. */
};

/**
 * @brief doca flow match flags
 */
enum doca_flow_match_tcp_flags {
	DOCA_FLOW_MATCH_TCP_FLAG_FIN = (1 << 0),
	/**< match tcp packet with Fin flag */
	DOCA_FLOW_MATCH_TCP_FLAG_SYN = (1 << 1),
	/**< match tcp packet with Syn flag */
	DOCA_FLOW_MATCH_TCP_FLAG_RST = (1 << 2),
	/**< match tcp packet with Rst flag */
	DOCA_FLOW_MATCH_TCP_FLAG_PSH = (1 << 3),
	/**< match tcp packet with Psh flag */
	DOCA_FLOW_MATCH_TCP_FLAG_ACK = (1 << 4),
	/**< match tcp packet with Ack flag */
	DOCA_FLOW_MATCH_TCP_FLAG_URG = (1 << 5),
	/**< match tcp packet with Urg flag */
	DOCA_FLOW_MATCH_TCP_FLAG_ECE = (1 << 6),
	/**< match tcp packet with Urg flag */
	DOCA_FLOW_MATCH_TCP_FLAG_CWR = (1 << 7),
	/**< match tcp packet with Urg flag */
};

/**
 * @brief doca flow matcher information
 */
struct doca_flow_match {
	uint32_t flags;
	/**< match items which are no value */
	struct doca_flow_meta meta;
	/**< Programmable meta data. */
	uint8_t out_src_mac[DOCA_ETHER_ADDR_LEN];
	/**< outer source mac address */
	uint8_t out_dst_mac[DOCA_ETHER_ADDR_LEN];
	/**< outer destination mac address */
	doca_be16_t out_eth_type;
	/**< outer Ethernet layer type */
	doca_be16_t out_vlan_tci;
	/**< outer vlan tci */
	struct doca_flow_ip_addr out_src_ip;
	/**< outer source ip address */
	struct doca_flow_ip_addr out_dst_ip;
	/**< outer destination ip address */
	uint8_t out_l4_type;
	/**< outer layer 4 protocol type */
	uint8_t out_tcp_flags;
	/**< outer tcp flags */
	doca_be16_t out_src_port;
	/**< outer layer 4 source port */
	doca_be16_t out_dst_port;
	/**< outer layer 4 destination port */
	struct doca_flow_tun tun;
	/**< tunnel info */
	uint8_t in_src_mac[DOCA_ETHER_ADDR_LEN];
	/**< inner source mac address */
	uint8_t in_dst_mac[DOCA_ETHER_ADDR_LEN];
	/**< inner destination mac address */
	doca_be16_t in_eth_type;
	/**< inner Ethernet layer type */
	doca_be16_t in_vlan_tci;
	/**< inner vlan tci */
	struct doca_flow_ip_addr in_src_ip;
	/**< inner source ip address if tunnel is used */
	struct doca_flow_ip_addr in_dst_ip;
	/**< inner destination ip address if tunnel is used */
	uint8_t in_l4_type;
	/**< inner layer 4 protocol type if tunnel is used */
	uint8_t in_tcp_flags;
	/**< inner tcp flags */
	doca_be16_t in_src_port;
	/**< inner layer 4 source port if tunnel is used */
	doca_be16_t in_dst_port;
	/**< inner layer 4 destination port if tunnel is used */
};

/**
 * @brief doca flow encap data information
 */
struct doca_flow_encap_action {
	uint8_t src_mac[DOCA_ETHER_ADDR_LEN];
	/**< source mac address */
	uint8_t dst_mac[DOCA_ETHER_ADDR_LEN];
	/**< destination mac address */
	doca_be16_t vlan_tci;
	/**< vlan tci */
	struct doca_flow_ip_addr src_ip;
	/**< source ip address */
	struct doca_flow_ip_addr dst_ip;
	/**< destination ip address */
	struct doca_flow_tun tun;
	/**< tunnel info */
};

/**
 * @brief doca flow actions information
 */
struct doca_flow_actions {
	uint8_t action_idx;
	/**< index according to place provided on creation */
	uint32_t flags;
	/**< action flags */
	bool decap;
	/**< when true, will do decap */
	struct doca_flow_meta meta;
	/**< modify meta data, pipe action as mask */
	uint8_t mod_src_mac[DOCA_ETHER_ADDR_LEN];
	/**< modify source mac address */
	uint8_t mod_dst_mac[DOCA_ETHER_ADDR_LEN];
	/**< modify VLAN ID */
	doca_be16_t mod_vlan_id;
	/**< modify destination mac address */
	struct doca_flow_ip_addr mod_src_ip;
	/**< modify source ip address */
	struct doca_flow_ip_addr mod_dst_ip;
	/**< modify destination ip address */
	uint8_t ttl;
	/**< modify(ADD) TTL value */
	doca_be16_t mod_src_port;
	/**< modify layer 4 source port */
	doca_be16_t mod_dst_port;
	/**< modify layer 4 destination port */
	bool has_encap;
	/**< when true, will do encap */
	struct doca_flow_encap_action encap;
	/**< encap data information */
	struct {
		enum doca_flow_crypto_protocol_type proto_type;
		/**< Crypto shared action type */
		uint32_t crypto_id;
		/**< Crypto shared action id */
	} security;
	/**< security shared action */
};

/**
 * @brief forwarding action type
 */
enum doca_flow_fwd_type {
	DOCA_FLOW_FWD_NONE = 0,
	/**< No forward action be set */
	DOCA_FLOW_FWD_RSS,
	/**< Forwards packets to rss */
	DOCA_FLOW_FWD_PORT,
	/**< Forwards packets to one port */
	DOCA_FLOW_FWD_PIPE,
	/**< Forwards packets to another pipe */
	DOCA_FLOW_FWD_DROP,
	/**< Drops packets */
	DOCA_FLOW_FWD_ORDERED_LIST_PIPE,
	/**< Forwards packet to a specific entry in an ordered list pipe. */
};

/**
 * @brief rss offload types
 */
enum doca_rss_type {
	DOCA_FLOW_RSS_IP = (1 << 0),
	/**< rss by ip head */
	DOCA_FLOW_RSS_UDP = (1 << 1),
	/**< rss by udp head */
	DOCA_FLOW_RSS_TCP = (1 << 2),
	/**< rss by tcp head */
};

/**
 * @brief forwarding configuration
 */
struct doca_flow_fwd {
	enum doca_flow_fwd_type type;
	/**< indicate the forwarding type */
	union {
		struct {
			uint32_t rss_flags;
			/**< rss offload types */
			uint16_t *rss_queues;
			/**< rss queues array */
			int num_of_queues;
			/**< number of queues */
			uint32_t shared_rss_id;
			/**< shared rss id, only for pipe's fwd is NULL */
		};
		/**< rss configuration information */
		struct {
			uint16_t port_id;
			/**< destination port id */
		};
		/**< port configuration information */
		struct {
			struct doca_flow_pipe *next_pipe;
			/**< next pipe pointer */
		};
		/**< next pipe configuration information */
		struct {
			/** Ordered list pipe to select an entry from. */
			struct doca_flow_pipe *pipe;
			/** Index of the ordered list pipe entry. */
			uint32_t idx;
		} ordered_list_pipe;
		/**< next ordered list pipe configuration */
	};
};


/**
 * @brief doca flow rss resource configuration
 */
struct doca_flow_resource_rss_cfg {
	uint32_t flags;
	/**< rss offload types */
	uint16_t *queues_array;
	/**< rss queues array */
	int nr_queues;
	/**< number of queues */
};

/**
 * @brief doca flow meter resource configuration
 */
struct doca_flow_resource_meter_cfg {
	uint64_t cir;
	/**< Committed Information Rate (bytes/second). */
	uint64_t cbs;
	/**< Committed Burst Size (bytes). */
};

/**
 * @brief doca flow crypro resource configuration
 */
struct doca_flow_resource_crypto_cfg {
	enum doca_flow_crypto_protocol_type proto_type;
	/**< packet reformat action */
	enum doca_flow_crypto_action_type action_type;
	/**< crypto action */
	enum doca_flow_crypto_reformat_type reformat_type;
	/**< packet reformat action */
	enum doca_flow_crypto_net_type net_type;
	/**< packet network mode type */
	enum doca_flow_crypto_header_type header_type;
	/**< packet header type */
	uint16_t reformat_data_sz;
	/**< reformat header length in bytes */
	uint8_t reformat_data[DOCA_FLOW_CRYPTO_REFORMAT_LEN_MAX];
	/**< reformat header buffer */
	union {
		struct {
			uint16_t key_sz;
			/**< key size in bytes */
			uint8_t key[DOCA_FLOW_CRYPTO_KEY_LEN_MAX];
			/**< Crypto key buffer */
		};
		void* security_ctx;
		/**< crypto object handle */
	};
	struct doca_flow_fwd fwd;
	/**< Crypto action continuation */
};

/**
 * @brief doca flow shared resource configuration
 */
struct doca_flow_shared_resource_cfg {
	union {
		struct doca_flow_resource_meter_cfg meter_cfg;
		struct doca_flow_resource_rss_cfg rss_cfg;
		struct doca_flow_resource_crypto_cfg crypto_cfg;
	};
};

/**
 * @brief doca monitor action flags
 */
enum {
	DOCA_FLOW_MONITOR_NONE = 0,
	/**< No monitor action be set */
	DOCA_FLOW_MONITOR_METER = (1 << 1),
	/**< set monitor with meter action */
	DOCA_FLOW_MONITOR_COUNT = (1 << 2),
	/**< set monitor with counter action */
	DOCA_FLOW_MONITOR_AGING = (1 << 3),
	/**< set monitor with aging action */
};

/**
 * @brief doca monitor action configuration
 */
struct doca_flow_monitor {
	uint8_t flags;
	/**< indicate which actions be included */
	struct {
		uint64_t cir;
		/**< Committed Information Rate (bytes/second). */
		uint64_t cbs;
		/**< Committed Burst Size (bytes). */
	};
	/**< meter action configuration */
	uint32_t shared_meter_id;
	/**< shared meter id */
	uint32_t shared_counter_id;
	/**< shared counter id */
	uint32_t aging;
	/**< aging time in seconds.*/
	uint64_t user_data;
	/**< aging user data input.*/
};

/**
 * @brief action type enumeration
 */
enum doca_flow_action_type {
	DOCA_FLOW_ACTION_AUTO = 0, /* Derived from pipe actions. */
	DOCA_FLOW_ACTION_CONSTANT, /* Pipe action is constant, masked by description. */
	DOCA_FLOW_ACTION_SET, /* Set value from entry action, masked by description. */
	DOCA_FLOW_ACTION_ADD, /* Add field value from pipe actions or flow entry. */
	DOCA_FLOW_ACTION_COPY, /* Copy field to another field. */
	DOCA_FLOW_ACTION_MAX, /* End of action type list. */
};

/**
 * @brief extended modification action
 */
struct doca_flow_action_field {
	/**
	 * Field address of pipe match to decide field type and byte offset.
	 */
	void *address;
	/**
	 * If address is not NULL, bit offset within the field from the address.
	 * Otherwise, bit offset from the start of context field.
	 */
	uint32_t offset;
};

/**
 * @brief action description
 */
struct doca_flow_action_desc {
	enum doca_flow_action_type type; /**< type */
	union {
		union { /* Mask value of modify action type CONST and SET, host order for meta, BE otherwise. */
			uint32_t u32;
			uint64_t u64;
			uint8_t u8[16];
		} mask;
		struct {
			struct doca_flow_action_field src; /* Source info to copy from. */
			struct doca_flow_action_field dst; /* Or destination info to copy to. */
			uint32_t width; /* Bit width to copy */
		} copy;
		struct {
			struct doca_flow_action_field dst; /* destination info. */
			uint32_t width; /* Bit width to add */
		} add;
	};
};

/**
 * @brief Metadata action description per field
 */
struct doca_flow_action_descs_meta {
	struct doca_flow_action_desc pkt_meta; /**< action description of pkt_meta. */
	struct doca_flow_action_desc u32[DOCA_FLOW_META_MAX / 4 - 1]; /**< action description of meta. */
};

/**
 * @brief action descriptions
 */
struct doca_flow_action_descs {
	struct doca_flow_action_descs_meta meta;     /**< action description of meta data. */
	struct doca_flow_action_desc src_mac;        /**< action description of source MAC. */
	struct doca_flow_action_desc dst_mac;        /**< action description of destination MAC. */
	struct doca_flow_action_desc eth_type;       /**< action description of ether type. */
	struct doca_flow_action_desc vlan_id;        /**< action description of VLAN ID. */
	struct doca_flow_action_desc src_ip;         /**< action description of source IP. */
	struct doca_flow_action_desc dst_ip;         /**< action description of destination IP. */
	struct doca_flow_action_desc ttl;            /**< action description of IPv4 TTL. */
	struct doca_flow_action_desc src_port;       /**< action description of source L4 port. */
	struct doca_flow_action_desc dst_port;       /**< action description of destination L4 port. */
	struct doca_flow_action_desc tunnel;         /**< action description of tunnel. */
};

/** Type of an ordered list element. */
enum doca_flow_ordered_list_element_type {
	/**
	 * Ordered list element is struct doca_flow_actions,
	 * the next element is struct doca_flow_action_descs
	 * associated with the current element.
	 */
	DOCA_FLOW_ORDERED_LIST_ELEMENT_ACTIONS,
	/**
	 * Ordered list element is struct doca_flow_action_descs.
	 * If the previous element type is ACTIONS, the current element is associated with it.
	 * Otherwise the current element is ordered w.r.t. the previous one.
	 */
	DOCA_FLOW_ORDERED_LIST_ELEMENT_ACTION_DESCS,
	/**
	 * Ordered list element is struct doca_flow_monitor.
	 */
	DOCA_FLOW_ORDERED_LIST_ELEMENT_MONITOR,
};

/** Ordered list configuration. */
struct doca_flow_ordered_list {
	/**
	 * List index among the lists of the pipe.
	 * At pipe creation, it must match the list position in the array of lists.
	 * At entry insertion, it determines which list to use.
	 */
	uint32_t idx;
	/** Number of elements in the list. */
	uint32_t size;
	/** An array of DOCA flow structure pointers, depending on types. */
	const void **elements;
	/** Types of DOCA Flow structures each of the elements is pointing to. */
	enum doca_flow_ordered_list_element_type *types;
};

/**
 * @brief pipe attributes
 */
struct doca_flow_pipe_attr {
	const char *name;
	/**< name for the pipeline */
	enum doca_flow_pipe_type type;
	/**< type of pipe. enum doca_flow_pipe_type */
	bool is_root;
	/**< pipeline is root or not. If true it means the pipe is a root pipe executed on packet arrival. */
	uint32_t nb_flows;
	/**< maximum number of flow rules, default is 8k if not set */
	uint8_t nb_actions;
	/**< maximum number of doca flow action array, default is 1 if not set */
	uint8_t nb_ordered_lists;
	/**< number of ordered lists in the array, default 0, mutually exclusive with nb_actions */
};

/**
 * @brief pipeline configuration
 */
struct doca_flow_pipe_cfg {
	struct doca_flow_pipe_attr attr;
	/**< attributes of pipe */
	struct doca_flow_port *port;
	/**< port for the pipeline */
	struct doca_flow_match *match;
	/**< matcher for the pipeline */
	struct doca_flow_match *match_mask;
	/**< match mask for the pipeline */
	struct doca_flow_actions **actions;
	/**< actions array for the pipeline */
	struct doca_flow_action_descs **action_descs;
	/**< action array descriptions */
	struct doca_flow_monitor *monitor;
	/**< monitor for the pipeline */
	struct doca_flow_ordered_list **ordered_lists;
	/**< array of ordered list types */
};

/**
 * @brief flow query result
 */
struct doca_flow_query {
	uint64_t total_bytes;
	/**< total bytes hit this flow */
	uint64_t total_pkts;
	/**< total packets hit this flow */
};

/**
 * @brief flow shared resources query result
 */
struct doca_flow_shared_resource_result {
	union {
		struct doca_flow_query counter;
	};
};

/**
 * @brief aged flow query callback context
 */
struct doca_flow_aged_query {
	uint64_t user_data;
	/**< The user input context, otherwise the doca_flow_pipe_entry pointer */
};

/**
 * @brief Initialize the doca flow.
 *
 * This is the global initialization function for doca flow. It
 * initializes all resources used by doca flow.
 *
 * Must be invoked first before any other function in this API.
 * this is a one time call, used for doca flow initialization and
 * global configurations.
 *
 * @param cfg
 * Port configuration, see doca_flow_cfg for details.
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * 0 on success, a negative errno value otherwise and error is set.
 */
__DOCA_EXPERIMENTAL
int
doca_flow_init(const struct doca_flow_cfg *cfg,
	       struct doca_flow_error *error);

/**
 * @brief Destroy the doca flow.
 *
 * Release all the resources used by doca flow.
 *
 * Must be invoked at the end of the application, before it exits.
 */
__DOCA_EXPERIMENTAL
void
doca_flow_destroy(void);

/**
 * @brief Start a doca port.
 *
 * Start a port with the given configuration. Will create one port in
 * the doca flow layer, allocate all resources used by this port, and
 * create the default offload flows including jump and default RSS for
 * traffic.
 *
 * @param cfg
 * Port configuration, see doca_flow_cfg for details.
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * Port handler on success, NULL otherwise and error is set.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_port *
doca_flow_port_start(const struct doca_flow_port_cfg *cfg,
		     struct doca_flow_error *error);

/**
 * @brief Stop a doca port.
 *
 * Stop the port, disable the traffic.
 *
 * @param port
 * Port struct.
 * @return
 * 0 on success, negative on failure.
 */
__DOCA_EXPERIMENTAL
int
doca_flow_port_stop(struct doca_flow_port *port);

/**
 * @brief pair two doca flow ports.
 *
 * This API should be used to pair two doca ports. This pair should be the
 * same as the actual physical layer paired information. Those two pair
 * ports have no order, a port cannot be paired with itself.
 *
 * In this API, default behavior will be handled according to each modes.
 * In VNF mode, pair information will be translated to queue action to
 * redirect packets to it's pair port. In SWITCH and REMOTE_VNF mode,
 * default rules will be created to redirect packets between 2 pair ports.
 *
 * @param port
 * Pointer to doca flow port.
 * @param pair_port
 * Pointer to the pair port.
 * @return
 * 0 on success, negative on failure.
 */

__DOCA_EXPERIMENTAL
int
doca_flow_port_pair(struct doca_flow_port *port, struct doca_flow_port *pair_port);

/**
 * @brief Get pointer of user private data.
 *
 * User can manage specific data structure in port structure.
 * The size of the data structure is given on port configuration.
 * See doca_flow_cfg for more details.
 *
 * @param port
 * Port struct.
 * @return
 * Private data head pointer.
 */
__DOCA_EXPERIMENTAL
uint8_t*
doca_flow_port_priv_data(struct doca_flow_port *port);

/**
 * @brief Configure a single shared resource.
 *
 * This API can be used by bounded and unbounded resources.
 *
 * @param type
 * Shared resource type.
 * @param id
 * Shared resource id.
 * @param cfg
 * Pointer to a shared resource configuration.
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * 0 on success, negative on failure.
 */
__DOCA_EXPERIMENTAL
int
doca_flow_shared_resource_cfg(enum doca_flow_shared_resource_type type, uint32_t id,
			      struct doca_flow_shared_resource_cfg *cfg,
			      struct doca_flow_error *error);

/**
 * @brief Binds a bulk of shared resources to a bindable object.
 *
 * Binds a bulk of shared resources from the same type to a bindable object.
 * Currently the bindable objects are ports and pipes.
 *
 * @param type
 * Shared resource type.
 * @param res_array
 * Array of shared resource IDs.
 * @param res_array_len
 * Shared resource IDs array length.
 * @param bindable_obj
 * Pointer to an allowed bindable object, use NULL to bind globally.
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * 0 on success, negative on failure.
 */
__DOCA_EXPERIMENTAL
int
doca_flow_shared_resources_bind(enum doca_flow_shared_resource_type type, uint32_t *res_array,
				uint32_t res_array_len, void *bindable_obj,
				struct doca_flow_error *error);

/**
 * @brief Extract information about shared counter
 *
 * Query an array of shared objects of a specific type.
 *
 * @param type
 * Shared object type.
 * @param res_array
 * Array of shared objects IDs to query.
 * @param query_results_array
 * Data array retrieved by the query.
 * @param array_len
 * Number of objects and their query results in their arrays (same number).
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * 0 on success, negative on failure.
 */
__DOCA_EXPERIMENTAL
int
doca_flow_shared_resources_query(enum doca_flow_shared_resource_type type,
				 uint32_t *res_array,
				 struct doca_flow_shared_resource_result *query_results_array,
				 uint32_t array_len,
				 struct doca_flow_error *error);

/**
 * @brief Create one new pipe.
 *
 * Create new pipeline to match and offload specific packets, the pipe
 * configuration includes the following components:
 *
 *     match: Match one packet by inner or outer fields.
 *     match_mask: The mask for the matched items.
 *     actions: Includes the modify specific packets fields, Encap and
 *                  Decap actions.
 *     monitor: Includes Count, Age, and Meter actions.
 *     fwd: The destination of the matched action, include RSS, Hairpin,
 *             Port, and Drop actions.
 *
 * This API will create the pipe, but would not start the HW offload.
 *
 * @param cfg
 * Pipe configuration.
 * @param fwd
 * Fwd configuration for the pipe.
 * @param fwd_miss
 * Fwd_miss configuration for the pipe. NULL for no fwd_miss.
 * When creating a pipe if there is a miss and fwd_miss configured,
 * packet steering should jump to it.
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * Pipe handler on success, NULL otherwise and error is set.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_pipe *
doca_flow_pipe_create(const struct doca_flow_pipe_cfg *cfg,
		const struct doca_flow_fwd *fwd,
		const struct doca_flow_fwd *fwd_miss,
		struct doca_flow_error *error);

/**
 * @brief Add one new entry to a pipe.
 *
 * When a packet matches a single pipe, will start HW offload. The pipe only
 * defines which fields to match. When offloading, we need detailed information
 * from packets, or we need to set some specific actions that the pipe did not
 * define. The parameters include:
 *
 *    match: The packet detail fields according to the pipe definition.
 *    actions: The real actions according to the pipe definition.
 *    monitor: Defines the monitor actions if the pipe did not define it.
 *    fwd: Define the forward action if the pipe did not define it.
 *
 * This API will do the actual HW offload, with the information from the fields
 * of the input packets.
 *
 * @param pipe_queue
 * Queue identifier.
 * @param pipe
 * Pointer to pipe.
 * @param match
 * Pointer to match, indicate specific packet match information.
 * @param actions
 * Pointer to modify actions, indicate specific modify information.
 * @param monitor
 * Pointer to monitor actions.
 * @param fwd
 * Pointer to fwd actions.
 * @param flags
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_flags_type.
 * @param usr_ctx
 * Pointer to user context.
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * Pipe entry handler on success, NULL otherwise and error is set.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_pipe_entry*
doca_flow_pipe_add_entry(uint16_t pipe_queue,
			struct doca_flow_pipe *pipe,
			const struct doca_flow_match *match,
			const struct doca_flow_actions *actions,
			const struct doca_flow_monitor *monitor,
			const struct doca_flow_fwd *fwd,
			uint32_t flags,
			void *usr_ctx,
			struct doca_flow_error *error);

/**
 * @brief Add one new entry to a control pipe.
 *
 * Refer to doca_flow_pipe_add_entry.
 *
 * @param pipe_queue
 * Queue identifier.
 * @param priority
 * Priority value.
 * @param pipe
 * Pointer to pipe.
 * @param match
 * Pointer to match, indicate specific packet match information.
 * @param match_mask
 * Pointer to match mask information.
 * @param actions
 * Pointer to modify actions, indicate specific modify information.
 * @param action_descs
 * action descriptions
 * @param monitor
 * Pointer to monitor actions.
 * @param fwd
 * Pointer to fwd actions.
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * Pipe entry handler on success, NULL otherwise and error is set.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_pipe_entry*
doca_flow_pipe_control_add_entry(uint16_t pipe_queue,
			uint32_t priority,
			struct doca_flow_pipe *pipe,
			const struct doca_flow_match *match,
			const struct doca_flow_match *match_mask,
			const struct doca_flow_actions *actions,
			const struct doca_flow_action_descs *action_descs,
			const struct doca_flow_monitor *monitor,
			const struct doca_flow_fwd *fwd,
			struct doca_flow_error *error);

/**
 * @brief Add one new entry to a lpm pipe.
 *
 * This API will populate the lpm entries
 *
 * @param pipe_queue
 * Queue identifier.
 * @param pipe
 * Pointer to pipe.
 * @param match
 * Pointer to match, indicate specific packet match information.
 * @param match_mask
 * Pointer to match mask information.
 * @param actions
 * Pointer to modify actions, indicate specific modify information.
 * @param monitor
 * Pointer to monitor actions.
 * @param fwd
 * Pointer to fwd actions.
 * @param flag
 * Flow entry will be pushed to hw immediately or not. enum doca_flow_flags_type.
 * @param usr_ctx
 * Pointer to user context.
 * @param error
 * Output error, set doca_flow_error for details.
 * @return
 * Pipe entry handler on success, NULL otherwise and error is set.
 */
__DOCA_EXPERIMENTAL
struct doca_flow_pipe_entry*
doca_flow_pipe_lpm_add_entry(uint16_t pipe_queue,
			 struct doca_flow_pipe *pipe,
			 const struct doca_flow_match *match,
			 const struct doca_flow_match *match_mask,
			 const struct doca_flow_actions *actions,
			 const struct doca_flow_monitor *monitor,
			 const struct doca_flow_fwd *fwd,
			 const enum doca_flow_flags_type flag,
			 void *usr_ctx,
			 struct doca_flow_error *error);

/**
 * @brief Free one pipe entry.
 *
 * This API will free the pipe entry and cancel HW offload. The
 * Application receives the entry pointer upon creation and if can
 * call this function when there is no more need for this offload.
 * For example, if the entry aged, use this API to free it.
 *
 * @param pipe_queue
 * Queue identifier.
 * @param entry
 * The pipe entry to be removed.
 * @param usr_ctx
 * The pointer to user context.
 * @return
 * 0 on success, negative on failure.
 */
__DOCA_EXPERIMENTAL
int
doca_flow_pipe_rm_entry(uint16_t pipe_queue, void *usr_ctx,
			struct doca_flow_pipe_entry *entry);

/**
 * @brief Destroy one pipe
 *
 * Destroy the pipe, and the pipe entries that match this pipe.
 *
 * @param pipe
 * Pointer to pipe.
 */
__DOCA_EXPERIMENTAL
void
doca_flow_pipe_destroy(struct doca_flow_pipe *pipe);

/**
 * @brief Flush pipes of one port
 *
 * Destroy all pipes and all pipe entries belonging to the port.
 *
 * @param port
 * Pointer to doca flow port.
 */
__DOCA_EXPERIMENTAL
void
doca_flow_port_pipes_flush(struct doca_flow_port *port);

/**
 * @brief Destroy a doca port.
 *
 * Destroy the doca port, free all resources of the port.
 *
 * @param port
 * Pointer to doca flow port.
 */
__DOCA_EXPERIMENTAL
void
doca_flow_port_destroy(struct doca_flow_port *port);

/**
 * @brief Dump pipe of one port
 *
 * Dump all pipes information belong to this port.
 *
 * @param port
 * Pointer to doca flow port.
 * @param f
 * The output file of the pipe information.
 */
__DOCA_EXPERIMENTAL
void
doca_flow_port_pipes_dump(struct doca_flow_port *port, FILE *f);

/**
 * @brief Dump pipe information
 *
 * @param pipe
 * Pointer to doca flow pipe.
 * @param f
 * The output file of the pipe information.
 */
 __DOCA_EXPERIMENTAL
void
doca_flow_pipe_dump(struct doca_flow_pipe *pipe, FILE *f);


/**
 * @brief Extract information about specific entry
 *
 * Query the packet statistics about specific pipe entry
 *
 * @param entry
 * The pipe entry toe query.
 * @param query_stats
 * Data retrieved by the query.
 * @return
 * 0 on success, negative on failure.
 */
__DOCA_EXPERIMENTAL
int
doca_flow_query(struct doca_flow_pipe_entry *entry,
		struct doca_flow_query *query_stats);

/**
 * @brief Handle aging of flows in queue.
 *
 * Go over all flows and release aged flows from being
 * tracked. The entries array will be filled with aged flows.
 *
 * Since the number of flows can be very large, it can take
 * a significant amount of time to go over all flows so this
 * function is limited by time quota, which means it might
 * return without handling all flows which requires the user
 * to call it again. Once a full cycle is done this function will
 * return -1.
 *
 * @param port
 * Port to handle aging
 * @param queue
 * Queue identifier.
 * @param quota
 * Max time quota in micro seconds for this function to handle aging.
 * @param entries
 * User input entries array for the aged flows.
 * @param len
 * User input length of entries array.
 * @return
 * > 0 the number of aged flows filled in entries array.
 * 0 no aged entries in current call.
 * -1 full cycle done.
 */
__DOCA_EXPERIMENTAL
int
doca_flow_aging_handle(struct doca_flow_port *port, uint16_t queue,
			uint64_t quota, struct doca_flow_aged_query *entries, int len);

/**
 * @brief Process entries in queue.
 *
 * The application must invoke this function in order to complete
 * the flow rule offloading and to receive the flow rule operation status.
 *
 * @param port
 * Port
 * @param pipe_queue
 * Queue identifier.
 * @param timeout
 * Max time in micro seconds for this function to process entries.
 * Process once if timeout is 0
 * @param max_processed_entries
 * Flow entries number to process
 * If it is 0, it will proceed until timeout.
 * @return
 * > 0: the number of entries processed
 * 0:   no entries are processed
 * negative value: failure
 */
__DOCA_EXPERIMENTAL
int
doca_flow_entries_process(struct doca_flow_port *port,
		uint16_t pipe_queue, uint64_t timeout,
		uint32_t max_processed_entries);

/**
 * @brief Get entry's status
 *
 * @param entry
 * pipe entry
 * @return
 * entry's status
 */
__DOCA_EXPERIMENTAL
enum doca_flow_entry_status
doca_flow_pipe_entry_get_status(struct doca_flow_pipe_entry *entry);

/**
 * @brief Get doca flow switch port
 *
 * The application could use this function to get the doca switch port, then
 * create pipes and pipe entries on this port.
 *
 */
__DOCA_EXPERIMENTAL
struct doca_flow_port *
doca_flow_port_switch_get(void);

/**
 * Add an entry to the ordered list pipe.
 *
 * @param pipe_queue
 * Queue identifier.
 * @param pipe
 * Pipe handle.
 * @param idx
 * Unique entry index. It is the user's responsibility to ensure uniqueness.
 * @param ordered_list
 * Ordered list with pointers to struct doca_flow_actions and struct doca_flow_monitor
 * at the same indices as they were at the pipe creation time.
 * If the configuration contained an element of struct doca_flow_action_descs,
 * the corresponding array element is ignored and can be NULL.
 * @param fwd
 * Entry forward configuration.
 * @param flags
 * Entry insertion flags.
 * @param user_ctx
 * Opaque context for the completion callback.
 * @param[out] error
 * Receives immediate error info.
 * @return struct doca_flow_pipe_entry *
 * The entry inserted.
 */
struct doca_flow_pipe_entry *
doca_flow_pipe_ordered_list_add_entry(uint16_t pipe_queue,
		struct doca_flow_pipe *pipe,
		uint32_t idx,
		const struct doca_flow_ordered_list *ordered_list,
		const struct doca_flow_fwd *fwd,
		enum doca_flow_flags_type flags,
		void *user_ctx,
		struct doca_flow_error *error);

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_H_ */
