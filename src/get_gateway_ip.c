// COPYRIGHT NOTICE:
// Copy-pasted from and full credit to: 
// https://www.linuxquestions.org/questions/linux-networking-3/howto-find-gateway-address-through-code-397078/#post2023303

#include <asm/types.h>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define BUFSIZE 8192

struct route_info{
	struct in_addr dstAddr;
	struct in_addr srcAddr;
	struct in_addr gateWay;
	char ifName[IF_NAMESIZE];
};

ssize_t readNlSock(int sockFd, char **bufPtr, size_t *bufSize,
		unsigned int seqNum, unsigned int pId){
	struct nlmsghdr *nlHdr;
	ssize_t readLen = 0;
	size_t msgLen = 0;
	char *buffer = *bufPtr;

	do{
		/* Keep one BUFSIZE chunk free so each recv() can append the next netlink block. */
		while((*bufSize - msgLen) < BUFSIZE) {
			size_t newSize = *bufSize + BUFSIZE;
			char *newBuffer = (char *)realloc(buffer, newSize);

			if(newBuffer == NULL) {
				perror("realloc");
				return -1;
			}

			buffer = newBuffer;
			*bufPtr = newBuffer;
			*bufSize = newSize;
		}

		/* Recieve response from the kernel */
		if((readLen = recv(sockFd, buffer + msgLen, *bufSize - msgLen, 0)) < 0){
			perror("SOCK READ: ");
			return -1;
		}

		nlHdr = (struct nlmsghdr *)(buffer + msgLen);

		/* Check if the header is valid */
		if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
		{
			perror("Error in recieved packet");
			return -1;
		}

		/* Check if the its the last message */
		if(nlHdr->nlmsg_type == NLMSG_DONE) {
			break;
		}
		else{
			msgLen += (size_t)readLen;
		}

		/* Check if its a multi part message */
		if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
			/* return if its not */
			break;
		}
	} while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
	return msgLen;
}

/* For parsing the route info returned */
void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo,
		unsigned char *gateway_ip, char *net_interface)
{
	// FIXME: Check if gateway is IPv6 and abort accordingly

	struct rtmsg *rtMsg;
	struct rtattr *rtAttr;
	int rtLen;

	rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);

	/* If the route is not for AF_INET or does not belong to main routing table
	   then return. */
	if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
		return;

	/* get the rtattr field */
	rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
	rtLen = RTM_PAYLOAD(nlHdr);
	for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen)){
		switch(rtAttr->rta_type) {
			case RTA_OIF:
				if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
				break;
			case RTA_GATEWAY:
				memcpy(&rtInfo->gateWay, RTA_DATA(rtAttr), sizeof(rtInfo->gateWay));
				break;
			case RTA_PREFSRC:
				memcpy(&rtInfo->srcAddr, RTA_DATA(rtAttr), sizeof(rtInfo->srcAddr));
				break;
			case RTA_DST:
				memcpy(&rtInfo->dstAddr, RTA_DATA(rtAttr), sizeof(rtInfo->dstAddr));
				break;
		}
	}

	// check that it ain't the default OS gateway
	if (strstr((char *)inet_ntoa(rtInfo->dstAddr), "0.0.0.0")) {
		// the network interface must match
		if(strcmp(net_interface, rtInfo->ifName))
			return;

		*((unsigned int*)gateway_ip) = rtInfo->gateWay.s_addr;
	}

	return;
}

int get_gateway_ip(unsigned char *gateway_ip, char *net_interface)
{
	struct nlmsghdr *nlMsg;
	struct route_info *rtInfo;
	char *msgBuf;
	size_t msgBufSize = BUFSIZE;
	size_t len = 0;
	ssize_t readLen;

	int sock;
	unsigned int msgSeq = 0;

	/* Create Socket */
	if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
		perror("Socket Creation: ");

	msgBuf = (char *)malloc(msgBufSize);
	if(msgBuf == NULL) {
		perror("malloc");
		if(sock >= 0)
			close(sock);
		return -1;
	}

	/* Initialize the buffer */
	memset(msgBuf, 0, msgBufSize);

	/* point the header and the msg structure pointers into the buffer */
	nlMsg = (struct nlmsghdr *)msgBuf;

	/* Fill in the nlmsg header*/
	nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
	nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

	nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
	nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
	nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

	/* Send the request */
	if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0){
		printf("Write To Socket Failed...\n");
		free(msgBuf);
		close(sock);
		return -1;
	}

	/* Read the response */
	if((readLen = readNlSock(sock, &msgBuf, &msgBufSize, msgSeq, getpid())) < 0) {
		printf("Read From Socket Failed...\n");
		free(msgBuf);
		close(sock);
		return -1;
	}
	len = (size_t)readLen;

	nlMsg = (struct nlmsghdr *)msgBuf;

	/* Parse and print the response */
	rtInfo = (struct route_info *)malloc(sizeof(struct route_info));

	/* THIS IS THE NETTSTAT -RL code I commented out the printing here and in parse routes */
	for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len)){
		memset(rtInfo, 0, sizeof(struct route_info));
		parseRoutes(nlMsg, rtInfo, gateway_ip, net_interface);
	}
	free(rtInfo);
	free(msgBuf);
	close(sock);

	return 0;
}
