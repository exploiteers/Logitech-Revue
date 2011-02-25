/* ltt-probe-net.c
 *
 * Loads a function at a marker call site.
 *
 * (C) Copyright 2006 Mathieu Desnoyers <mathieu.desnoyers@polymtl.ca>
 *
 * This file is released under the GPLv2.
 * See the file COPYING for more details.
 */

#include <linux/marker.h>
#include <linux/module.h>
#include <linux/inetdevice.h>
#include <net/sock.h>
#include <ltt/ltt-facility-select-default.h>
#include <ltt/ltt-facility-select-network_ip_interface.h>
#include <ltt/ltt-facility-network.h>
#include <ltt/ltt-facility-socket.h>
#include <ltt/ltt-facility-network_ip_interface.h>

#define NET_SOCKET_SENDMSG_FORMAT "struct socket %p %zu"
void probe_net_socket_sendmsg(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	struct socket *sock;
	size_t size;

	/* Assign args */
	va_start(ap, format);
	sock = va_arg(ap, typeof(sock));
	size = va_arg(ap, typeof(size));

	/* Call tracer */
	trace_socket_sendmsg(sock, sock->sk->sk_family,
		sock->sk->sk_type, sock->sk->sk_protocol, size);
	
	va_end(ap);
}

#define NET_SOCKET_RECVMSG_FORMAT "struct socket %p %zu"
void probe_net_socket_recvmsg(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	struct socket *sock;
	size_t size;

	/* Assign args */
	va_start(ap, format);
	sock = va_arg(ap, typeof(sock));
	size = va_arg(ap, typeof(size));

	/* Call tracer */
	trace_socket_recvmsg(sock, sock->sk->sk_family,
		sock->sk->sk_type, sock->sk->sk_protocol, size);
	
	va_end(ap);
}

#define NET_SOCKET_CREATE_FORMAT "struct socket %p %d"
void probe_net_socket_create(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	struct socket *sock;
	int retval;

	/* Assign args */
	va_start(ap, format);
	sock = va_arg(ap, typeof(sock));
	retval = va_arg(ap, typeof(retval));

	/* Call tracer */
	trace_socket_create(sock, sock->sk->sk_family,
		sock->sk->sk_type, sock->sk->sk_protocol, retval);
	
	va_end(ap);
}

#define NET_SOCKET_CALL_FORMAT "%d %lu"
void probe_net_socket_call(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	int call_number;
	unsigned long first_arg;

	/* Assign args */
	va_start(ap, format);
	call_number = va_arg(ap, typeof(call_number));
	first_arg = va_arg(ap, typeof(first_arg));

	/* Call tracer */
	trace_socket_call(call_number, first_arg);
	
	va_end(ap);
}

#define NET_DEV_XMIT_FORMAT "struct sk_buff %p"
void probe_net_dev_xmit(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	struct sk_buff *skb;

	/* Assign args */
	va_start(ap, format);
	skb = va_arg(ap, typeof(skb));

	/* Call tracer */
	trace_network_packet_out(skb, skb->protocol);
	
	va_end(ap);
}

#define NET_DEV_RECEIVE_FORMAT "struct sk_buff %p"
void probe_net_dev_receive(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	struct sk_buff *skb;

	/* Assign args */
	va_start(ap, format);
	skb = va_arg(ap, typeof(skb));

	/* Call tracer */
	trace_network_packet_in(skb, skb->protocol);
	
	va_end(ap);
}

#define NET_INSERT_IFA_FORMAT "struct in_ifaddr %p"
void probe_net_insert_ifa(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	struct in_ifaddr *ifa;

	/* Assign args */
	va_start(ap, format);
	ifa = va_arg(ap, typeof(ifa));

	/* Call tracer */
	trace_network_ip_interface_dev_up(ifa->ifa_label, ifa->ifa_address);
	
	va_end(ap);
}

#define NET_DEL_IFA_FORMAT "struct in_ifaddr %p"
void probe_net_del_ifa(const char *format, ...)
{
	va_list ap;
	/* Declare args */
	struct in_ifaddr *ifa;

	/* Assign args */
	va_start(ap, format);
	ifa = va_arg(ap, typeof(ifa));

	/* Call tracer */
	trace_network_ip_interface_dev_down(ifa->ifa_label);
	
	va_end(ap);
}



static int __init probe_init(void)
{
	int result;
	result = marker_set_probe("net_socket_sendmsg",
			NET_SOCKET_SENDMSG_FORMAT,
			probe_net_socket_sendmsg);
	if (!result)
		goto cleanup;
	result = marker_set_probe("net_socket_recvmsg",
			NET_SOCKET_RECVMSG_FORMAT,
			probe_net_socket_recvmsg);
	if (!result)
		goto cleanup;
	result = marker_set_probe("net_socket_create",
			NET_SOCKET_CREATE_FORMAT,
			probe_net_socket_create);
	if (!result)
		goto cleanup;
#ifdef __ARCH_WANT_SYS_SOCKETCALL
	result = marker_set_probe("net_socket_call",
			NET_SOCKET_CALL_FORMAT,
			probe_net_socket_call);
	if (!result)
		goto cleanup;
#endif
	result = marker_set_probe("net_dev_xmit",
			NET_DEV_XMIT_FORMAT,
			probe_net_dev_xmit);
	if (!result)
		goto cleanup;
	result = marker_set_probe("net_dev_receive",
			NET_DEV_RECEIVE_FORMAT,
			probe_net_dev_receive);
	if (!result)
		goto cleanup;
	result = marker_set_probe("net_insert_ifa",
			NET_INSERT_IFA_FORMAT,
			probe_net_insert_ifa);
	if (!result)
		goto cleanup;
	result = marker_set_probe("net_del_ifa",
			NET_DEL_IFA_FORMAT,
			probe_net_del_ifa);
	if (!result)
		goto cleanup;

	return 0;

cleanup:
	marker_remove_probe(probe_net_socket_sendmsg);
	marker_remove_probe(probe_net_socket_recvmsg);
	marker_remove_probe(probe_net_socket_create);
	marker_remove_probe(probe_net_socket_call);
	marker_remove_probe(probe_net_dev_xmit);
	marker_remove_probe(probe_net_dev_receive);
	marker_remove_probe(probe_net_insert_ifa);
	marker_remove_probe(probe_net_del_ifa);
	return -EPERM;
}

static void __exit probe_fini(void)
{
	marker_remove_probe(probe_net_socket_sendmsg);
	marker_remove_probe(probe_net_socket_recvmsg);
	marker_remove_probe(probe_net_socket_create);
	marker_remove_probe(probe_net_socket_call);
	marker_remove_probe(probe_net_dev_xmit);
	marker_remove_probe(probe_net_dev_receive);
	marker_remove_probe(probe_net_insert_ifa);
	marker_remove_probe(probe_net_del_ifa);
}

module_init(probe_init);
module_exit(probe_fini);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Net Probe");

