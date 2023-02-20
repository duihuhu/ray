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
 * @file doca_flow_crypto.h
 * @page doca flow crypto
 * @defgroup FLOW_CRYPTO flow net define
 * DOCA HW offload flow cryptonet structure define. For more details please refer to
 * the user guide on DOCA devzone.
 *
 * @{
 */

#ifndef DOCA_FLOW_CRYPTO_H_
#define DOCA_FLOW_CRYPTO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief doca flow crypto operation protocol type
 */
enum doca_flow_crypto_protocol_type {
	DOCA_FLOW_CRYPTO_PROTOCOL_NONE = 0,
	/**< No security protocol engaged */
	DOCA_FLOW_CRYPTO_PROTOCOL_NISP,
	/**< NISP protocol action */
	DOCA_FLOW_CRYPTO_PROTOCOL_ESP_DECRYPT = 100,
	/**< IPsec ESP protocol decrypt action */
	DOCA_FLOW_CRYPTO_PROTOCOL_ESP_ENCRYPT = 101,
	/**< IPsec ESP protocol encrypt action */
};

/**
 * @brief doca flow crypto operation action type
 */
enum doca_flow_crypto_action_type {
	DOCA_FLOW_CRYPTO_ACTION_NONE = 0,
	/**< No crypto action performed */
	DOCA_FLOW_CRYPTO_ACTION_ENCRYPT,
	/**< Perform encryption */
	DOCA_FLOW_CRYPTO_ACTION_DECRYPT,
	/**< Perform decryption/authentication */
};

/**
 * @brief doca flow crypto operation reformat type
 */
enum doca_flow_crypto_reformat_type {
	DOCA_FLOW_CRYPTO_REFORMAT_NONE = 0,
	/**< No reformat action performed */
	DOCA_FLOW_CRYPTO_REFORMAT_ENCAP,
	/**< Perform encapsulation action */
	DOCA_FLOW_CRYPTO_REFORMAT_DECAP,
	/**< Perform decapsulation action */
};

/**
 * @brief doca flow crypto operation network mode type
 */
enum doca_flow_crypto_net_type {
	DOCA_FLOW_CRYPTO_NET_NONE = 0,
	/**< No network header involved */
	DOCA_FLOW_CRYPTO_NET_TUNNEL,
	/**< Tunnel network header */
	DOCA_FLOW_CRYPTO_NET_TRANSPORT,
	/**< Tramsport network header */
};

/**
 * @brief doca flow crypto operation encapsulation header type
 */
enum doca_flow_crypto_header_type {
	DOCA_FLOW_CRYPTO_HEADER_NONE = 0,
	/**< No network header involved */
	DOCA_FLOW_CRYPTO_HEADER_IPV4,
	/**< IPv4 network header type */
	DOCA_FLOW_CRYPTO_HEADER_IPV6,
	/**< IPv6 network header type */
	DOCA_FLOW_CRYPTO_HEADER_IPV4_UDP,
	/**< IPv4 + UDP network header type */
	DOCA_FLOW_CRYPTO_HEADER_IPV6_UDP,
	/**< IPv6 + UDP network header type */
};

/**
 * @brief doca flow supported crypto action confgiurations
 *
 * "-" - means ignored
 *                        		proto  	rfmt	net	htyp		crypto
 * No crypto action: 			none	-	-	-		-
 *
 * NISP decap only:			nisp	decap	tun	-		none
 * NISP encap only:			nisp	encap	tun	-		none
 * NISP decrypt only:			nisp	none	-	-		decrypt
 * NISP encrypt only:			nisp	none	-	-		encrypt
 * NISP decrypt + decap only:		nisp	decap	tun	-		decrypt
 * NISP encap + encrypt only:		nisp	encap	tun	-		encrypt
 *
 * ESP tunnel decap only:		esp	decap	tun	-		none
 * ESP transport decap only:		esp	decap	tran	4/6/4_udp/6_udp	none
 * ESP tunnel encap only:		esp	encap	tun	-		none
 * ESP transport encap only:		esp	encap	tran	type		none
 * ESP decrypt only:			esp	none	-	-		decrypt
 * ESP encrypt only:			esp	none	-	-		encrypt
 * ESP tunnel decrypt + decap:		esp	decap	tun	-		decrypt
 * ESP transport decrypt + decap:	esp	decap	tran	4/6/4_udp/6_udp	decrypt
 * ESP tunnel encap + encrypt:		esp	encap	tun	-		encrypt
 * ESP transport encap + encrypt:	esp	encap	tran	type		encrypt
 */

#ifdef __cplusplus
} /* extern "C" */
#endif

/** @} */

#endif /* DOCA_FLOW_CRYPTO_H_ */
