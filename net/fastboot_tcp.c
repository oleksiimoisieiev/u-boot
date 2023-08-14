// SPDX-License-Identifier: BSD-2-Clause
/*
 * Copyright (C) 2023 The Android Open Source Project
 */

#include <common.h>
#include <fastboot.h>
#include <net.h>
#include <net6.h>
#include <net/fastboot_tcp.h>
#include <net/tcp.h>
#include <net/tcp6.h>

static char command[FASTBOOT_COMMAND_LEN] = {0};
static char response[FASTBOOT_RESPONSE_LEN] = {0};

static const unsigned short handshake_length = 4;
static const uchar *handshake = "FB01";

static u16 curr_sport;
static u16 curr_dport;
static u32 curr_tcp_seq_num;
static u32 curr_tcp_ack_num;
static unsigned int curr_request_len;
static bool is_ipv6;
static size_t ip_header_size;
static enum fastboot_tcp_state {
	FASTBOOT_CLOSED,
	FASTBOOT_CONNECTED,
	FASTBOOT_DISCONNECTING
} state = FASTBOOT_CLOSED;

static int command_handled_id;
static bool command_handled_success;

static void fastboot_tcp_answer(u8 action, unsigned int len)
{
	const u32 response_seq_num = curr_tcp_ack_num;
	const u32 response_ack_num = curr_tcp_seq_num +
		  (curr_request_len > 0 ? curr_request_len : 1);

#if defined(CONFIG_IPV6)
	if (is_ipv6) {
		net_send_tcp_packet6(len, htons(curr_sport), htons(curr_dport),
				     action, response_seq_num, response_ack_num);
		return;
	}
#endif
	net_send_tcp_packet(len, htons(curr_sport), htons(curr_dport),
			    action, response_seq_num, response_ack_num);
}

static void fastboot_tcp_reset(void)
{
	fastboot_tcp_answer(TCP_RST, 0);
	tcp_set_tcp_state(TCP_CLOSED);
	state = FASTBOOT_CLOSED;
}

static void fastboot_tcp_send_packet(u8 action, const uchar *data, unsigned int len)
{
	uchar *pkt = net_get_async_tx_pkt_buf();

	memset((void *)pkt, 0, PKTSIZE);
	pkt += net_eth_hdr_size() + ip_header_size + TCP_HDR_SIZE + TCP_TSOPT_SIZE + 2;
	memcpy(pkt, data, len);
	fastboot_tcp_answer(action, len);
	memset((void *)pkt, 0, PKTSIZE);
}

static void fastboot_tcp_send_message(const char *message, unsigned int len)
{
	__be64 len_be = __cpu_to_be64(len);
	uchar *pkt = net_get_async_tx_pkt_buf();

	memset((void *)pkt, 0, PKTSIZE);
	pkt += net_eth_hdr_size() + ip_header_size + TCP_HDR_SIZE + TCP_TSOPT_SIZE + 2;
	// Put first 8 bytes as a big endian message length
	memcpy(pkt, &len_be, 8);
	pkt += 8;
	memcpy(pkt, message, len);
	fastboot_tcp_answer(TCP_ACK | TCP_PUSH, len + 8);
	memset((void *)pkt, 0, PKTSIZE);
}

static void fastboot_tcp_handler(uchar *pkt, u16 dport, u16 sport,
				 u32 tcp_seq_num, u32 tcp_ack_num,
				 u8 action, unsigned int len)
{
	int fastboot_command_id;
	u64 command_size;
	u8 tcp_fin = action & TCP_FIN;
	u8 tcp_push = action & TCP_PUSH;

	curr_sport = sport;
	curr_dport = dport;
	curr_tcp_seq_num = tcp_seq_num;
	curr_tcp_ack_num = tcp_ack_num;
	curr_request_len = len;

	switch (state) {
	case FASTBOOT_CLOSED:
		if (tcp_push) {
			if (len != handshake_length ||
			    memcmp(pkt, handshake, handshake_length) != 0) {
				fastboot_tcp_reset();
				break;
			}
			fastboot_tcp_send_packet(TCP_ACK | TCP_PUSH,
						 handshake, handshake_length);
			state = FASTBOOT_CONNECTED;
		}
		break;
	case FASTBOOT_CONNECTED:
		if (tcp_fin) {
			fastboot_tcp_answer(TCP_FIN | TCP_ACK, 0);
			state = FASTBOOT_DISCONNECTING;
			break;
		}
		if (tcp_push) {
			// First 8 bytes is big endian message length
			command_size = __be64_to_cpu(*(u64 *)pkt);
			len -= 8;
			pkt += 8;

			// Only single packet messages are supported ATM
			if (len != command_size) {
				fastboot_tcp_reset();
				break;
			}
			strlcpy(command, pkt, len + 1);
			fastboot_command_id = fastboot_handle_command(command, response);
			fastboot_tcp_send_message(response, strlen(response));

			command_handled_id = fastboot_command_id;
			command_handled_success = strncmp("OKAY", response, 4) == 0;
		}
		break;
	case FASTBOOT_DISCONNECTING:
		if (command_handled_success) {
			fastboot_handle_boot(command_handled_id, command_handled_success);
			command_handled_id = 0;
			command_handled_success = false;
		}

		if (tcp_push)
			state = FASTBOOT_CLOSED;
		break;
	}

	memset(command, 0, FASTBOOT_COMMAND_LEN);
	memset(response, 0, FASTBOOT_RESPONSE_LEN);
	curr_sport = 0;
	curr_dport = 0;
	curr_tcp_seq_num = 0;
	curr_tcp_ack_num = 0;
	curr_request_len = 0;
}

static void fastboot_tcp_handler_ipv4(uchar *pkt, u16 dport,
				      struct in_addr sip, u16 sport,
				      u32 tcp_seq_num, u32 tcp_ack_num,
				      u8 action, unsigned int len)
{
	is_ipv6 = false;
	ip_header_size = IP_HDR_SIZE;
	fastboot_tcp_handler(pkt, dport, sport,
			     tcp_seq_num, tcp_ack_num,
			     action, len);
}

#if defined(CONFIG_IPV6)
static void fastboot_tcp_handler_ipv6(uchar *pkt, u16 dport,
				      struct in6_addr sip, u16 sport,
				      u32 tcp_seq_num, u32 tcp_ack_num,
				      u8 action, unsigned int len)
{
	is_ipv6 = true;
	ip_header_size = IP6_HDR_SIZE;
	fastboot_tcp_handler(pkt, dport, sport,
			     tcp_seq_num, tcp_ack_num,
			     action, len);
}
#endif

void fastboot_tcp_start_server(void)
{
	printf("Using %s device\n", eth_get_name());

	printf("Listening for fastboot command on tcp %pI4\n", &net_ip);
	tcp_set_tcp_handler(fastboot_tcp_handler_ipv4);

#if defined(CONFIG_IPV6)
	printf("Listening for fastboot command on %pI6\n", &net_ip6);
	net_set_tcp_handler6(fastboot_tcp_handler_ipv6);
#endif
}
