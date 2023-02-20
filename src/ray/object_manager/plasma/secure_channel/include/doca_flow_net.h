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
 * @file doca_flow_net.h
 * @page doca flow net
 * @defgroup FLOW_NET flow net define
 * DOCA HW offload flow net structure define. For more details please refer to
 * the user guide on DOCA devzone.
 *
 * @{
 */

#ifndef DOCA_FLOW_NET_H_
#define DOCA_FLOW_NET_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t doca_be16_t; /**< 16-bit big-endian value. */
typedef uint32_t doca_be32_t; /**< 32-bit big-endian value. */
typedef uint64_t doca_be64_t; /**< 64-bit big-endian value. */

#define DOCA_ETHER_ADDR_LEN (6) /**< length of ether add length. */
#define DOCA_PROTO_TCP (6) /**< Transmission Control Protocol. */
#define DOCA_PROTO_UDP (17) /**< User Datagram Protocol. */
#define DOCA_PROTO_GRE (47) /**< Cisco GRE tunnels (rfc 1701,1702). */
#define DOCA_GTPU_PORT (2152) /**< gtpu upd port id. */
#define DOCA_VXLAN_DEFAULT_PORT (4789) /**< default vxlan port id. */
#define DOCA_NISP_DEFAULT_PORT (1000) /**< default nisp/audp port id. */

/** Ethernet frame types */
#define DOCA_ETHER_TYPE_IPV4 (0x0800) /**< IPv4 Protocol. */
#define DOCA_ETHER_TYPE_IPV6 (0x86DD) /**< IPv6 Protocol. */
#define DOCA_ETHER_TYPE_TEB  (0x6558) /**< Transparent Ethernet Bridging. */

/** AUDP header maximal length in dwords */
#define DOCA_FLOW_AUDP_DWORD 6
/** AUDP header maximal length in bytes */
#define DOCA_FLOW_AUDP_HEADER_LEN (DOCA_FLOW_AUDP_DWORD * sizeof(doca_be32_t))

/** NISP header maximal length in dwords */
#define DOCA_FLOW_NISP_DWORD 10
/** NISP header maximal length in bytes */
#define DOCA_FLOW_NISP_HEADER_LEN (DOCA_FLOW_NISP_DWORD * sizeof(doca_be32_t))

/** IPsec ESP header maximal length in bytes */
#define DOCA_FLOW_ESP_HEADER_LEN (2 * sizeof(doca_be32_t))

/** Crypto key maximal length in bytes */
#define DOCA_FLOW_CRYPTO_KEY_LEN_MAX 32

/**
 * NISP/ESP tunnel header may consist of:
 *  - Ethernet addresses
 *  - Ethernet type
 *  - optional VLAN and 802.1Q headers
 *  - IPv4 (with full options) or IPv6 (w/o options)
 *  - optional UDP header
 *  - NISP or ESP header
 */
#define DOCA_FLOW_CRYPTO_REFORMAT_LEN_MAX	\
	(DOCA_ETHER_ADDR_LEN * 2 +	\
	 sizeof(doca_be16_t) +		\
	 sizeof(doca_be16_t) * 2 * 2 +	\
	 sizeof(doca_be32_t) * 15 +	\
	 sizeof(doca_be32_t) * 2 +	\
	 DOCA_FLOW_NISP_HEADER_LEN)

/**
 * @brief doca flow ip address type
 */
enum doca_flow_ip_type {
	DOCA_FLOW_ADDR_NONE = 0,
	/**< ip address is not set */
	DOCA_FLOW_IP4_ADDR = 4,
	/**< ip address is ipv4 */
	DOCA_FLOW_IP6_ADDR = 6,
	/**< ip address is ipv6 */
};

/**
 * @brief doca flow ip address
 */
struct doca_flow_ip_addr {
	uint8_t type;
	/**< ip address type */
	union {
		doca_be32_t ipv4_addr;
		/**< ipv4 address if type is ipv4 */
		doca_be32_t ipv6_addr[4];
		/**< ipv6 address if type is ipv6 */
	};
};

/**
 * @brief doca flow tunnel type
 */
enum doca_flow_tun_type {
	DOCA_FLOW_TUN_NONE = 0,
	/**< tunnel is not set */
	DOCA_FLOW_TUN_VXLAN,
	/**< tunnel is vxlan type */
	DOCA_FLOW_TUN_GTPU,
	/**< tunnel is gtpu type */
	DOCA_FLOW_TUN_GRE,
	/**< tunnel is gre type */
	DOCA_FLOW_TUN_NISP,
	/**< tunnel is nisp type */
	DOCA_FLOW_TUN_AUDP,
	/**< tunnel is nisp type */
	DOCA_FLOW_TUN_ESP,
	/**< tunnel is ipsec esp type */
};

/**
 * @brief doca flow tunnel information
 */
struct doca_flow_tun {
	enum doca_flow_tun_type type;
	/**< tunnel type */
	union {
		struct {
			doca_be32_t vxlan_tun_id;
			/**< vxlan vni(24) + reserved (8). */
		};
		/**< vxlan information if tunnel is vxlan */
		struct {
			doca_be16_t key_present:1;
			/**< gre key is present*/
			doca_be16_t protocol;
			/**< next protocol */
			doca_be32_t gre_key;
			/**< gre key */
		};
		/**< gre information if tunnel is gre */
		struct {
			doca_be32_t gtp_teid;
			/**< gtp teid */
		};
		/**< gtp information if tunnel is gtp */
		struct {
			doca_be32_t nisp_hdr[DOCA_FLOW_NISP_DWORD];
			/**< Opaque nisp tunnel header */
		};
		/**< nisp information if tunnel is nisp*/
		struct {
			doca_be32_t audp_hdr[DOCA_FLOW_AUDP_DWORD];
			/**< Opaque audp tunnel header */
		};
		/**< audp information if tunnel is audp */
		struct {
			doca_be32_t esp_spi;
			/**< ipsec session parameter index */
			doca_be32_t esp_sn;
			/**< ipsec sequence number */
		};
		/**< ipsec esp header information if tunnel is esp */
	};
};

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_NET_H_ */
