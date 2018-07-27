/*
 * lwip_demo.c
 *
 *  Created on: 2018��1��3��
 *      Author: Administrator
 *
 *  ����lwIP��raw/callback API���
 */

#include "lwip/ip_addr.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"


//*****************************************************************
// UDP Server

#define UDP_ECHO_PORT	7

/*
 * function: udp_demo_cb
 * parameter: void *arg -
 *            struct udp_pcb *upcb -
 *            struct pbuf *p       - recvive data�����յ������ݰ�
 *            struct ip_addr *addr - remote ip���������ݵ�����ip
 *            u16 port             - remote port���������ݵ�����port
 * description: udp recvive callback function
 *              UDP���ջص�����
 */
static void ICACHE_FLASH_ATTR
udp_demo_cb(void *arg, struct udp_pcb *upcb,
		struct pbuf *p, struct ip_addr* addr, u16 port)
{
	unsigned char* temp = (unsigned char*)addr;
	udp_sendto(upcb, p, addr, port);
	pbuf_free(p);	// �ǵ�Ҫ�ֶ����
}

void ICACHE_FLASH_ATTR
udp_demo_init(void)
{
	struct udp_pcb *upcb;
	upcb = udp_new();
	udp_bind(upcb, IP_ADDR_ANY, UDP_ECHO_PORT);
	udp_recv(upcb, udp_demo_cb, NULL);
	os_printf("udp_demo_init\r\n");
}


//*****************************************************************
// TCP Server

/*
 * function: tcpserver_recv
 * parameter: void *arg -
 *            struct udp_pcb *pcb -
 *            struct pbuf *p -
 *            err_t err -
 * return: err_t
 * description:
 */
static ICACHE_FLASH_ATTR
err_t tcpserver_recv(void *arg, struct tcp_pcb *pcb,
		struct pbuf *p, err_t err)
{
	if(p!=NULL){
		tcp_recved(pcb, p->tot_len);	// ֪ͨ�ں˸��½��մ���
		tcp_write(pcb,p->payload,p->len,1);
		pbuf_free(p);			// �ͷ��������ݰ�
	}else if(err==ERR_OK){		// ����������Ӵ���
		return tcp_close(pcb);	// �ر�����
	}
	return ERR_OK;				// ���ز������
}

/*
 * function: tcpserver_accept
 * parameter: void *arg -
 *            struct udp_pcb *pcb -
 *            err_t err -
 * return: err_t
 * description:
 */
static ICACHE_FLASH_ATTR
err_t tcpserver_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	tcp_recv(pcb, tcpserver_recv);	// �������ע�ắ��tcpserver_recv
	return ERR_OK;
}

/*
 * function: tcpserver_init
 * parameter: void
 * return: void
 * description:
 */
void ICACHE_FLASH_ATTR
tcpserver_init(void)
{
	struct tcp_pcb *pcb;
	pcb = (struct tcp_pcb *)tcp_new();	// ����һ��TCP���ƿ�
	tcp_bind(pcb, IP_ADDR_ANY, 6060);	// ���Ŀ��ƿ�ͱ��ض˿�6060��

	pcb = (struct tcp_pcb *)tcp_listen(pcb);	// ����
	tcp_accept(pcb, tcpserver_accept);	// ע�����ӻص�����tcpserver_accept
	os_printf("tcpserver_init\r\n");
}
