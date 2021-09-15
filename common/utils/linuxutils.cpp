#include "linuxutils.h"
#include <sys/utsname.h>
#include <netinet/in.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

namespace  {

const int BUFSIZE = 8192;

struct route_info {
    struct in_addr dstAddr;
    struct in_addr srcAddr;
    struct in_addr gateWay;
    char ifName[IF_NAMESIZE];
};

int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
    struct nlmsghdr *nlHdr;
    int readLen = 0, msgLen = 0;

    do {
        // Recieve response from the kernel
        if ((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0) {
            return -1;
        }

        nlHdr = (struct nlmsghdr *) bufPtr;

        // Check if the header is valid
        if ((NLMSG_OK(nlHdr, readLen) == 0)
            || (nlHdr->nlmsg_type == NLMSG_ERROR)) {
            return -1;
        }

        // Check if the its the last message
        if (nlHdr->nlmsg_type == NLMSG_DONE) {
            break;
        } else {
        // Else move the pointer to buffer appropriately
            bufPtr += readLen;
            msgLen += readLen;
        }

        // Check if its a multi part message
        if ((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0) {
           // return if its not
            break;
        }
    } while ((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));

    return msgLen;
}

void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo, char *gateway)
{
    struct rtmsg *rtMsg;
    struct rtattr *rtAttr;
    int rtLen;

    rtMsg = (struct rtmsg *) NLMSG_DATA(nlHdr);

    // If the route is not for AF_INET or does not belong to main routing table then return.
    if ((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
        return;

    // get the rtattr field
    rtAttr = (struct rtattr *) RTM_RTA(rtMsg);
    rtLen = RTM_PAYLOAD(nlHdr);
    for (; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
        switch (rtAttr->rta_type) {
        case RTA_OIF:
            if_indextoname(*(int *) RTA_DATA(rtAttr), rtInfo->ifName);
            break;
        case RTA_GATEWAY:
            rtInfo->gateWay.s_addr= *(u_int *) RTA_DATA(rtAttr);
            break;
        case RTA_PREFSRC:
            rtInfo->srcAddr.s_addr= *(u_int *) RTA_DATA(rtAttr);
            break;
        case RTA_DST:
            rtInfo->dstAddr .s_addr= *(u_int *) RTA_DATA(rtAttr);
            break;
        }
    }

    if (rtInfo->dstAddr.s_addr == 0)
        sprintf(gateway, (char *) inet_ntoa(rtInfo->gateWay));

    return;
}
}


QString LinuxUtils::getOsVersionString()
{
    struct utsname unameData;
    if (uname(&unameData) == 0)
    {
        return QString(unameData.sysname) + " " + QString(unameData.version);
    }
    else
    {
        return "Can't detect OS Linux version";
    }
}

// taken from https://stackoverflow.com/questions/3288065/getting-gateway-to-use-for-a-given-ip-in-ansi-c
void LinuxUtils::getDefaultRoute(QString &outGatewayIp, QString &outInterfaceName)
{
    struct nlmsghdr *nlMsg;
    struct rtmsg *rtMsg;
    struct route_info *rtInfo;
    char msgBuf[BUFSIZE];

    int sock, len, msgSeq = 0;

    if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
        return;

    memset(msgBuf, 0, BUFSIZE);

    // point the header and the msg structure pointers into the buffer
    nlMsg = (struct nlmsghdr *) msgBuf;
    rtMsg = (struct rtmsg *) NLMSG_DATA(nlMsg);

    // Fill in the nlmsg header
    nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));  // Length of message.
    nlMsg->nlmsg_type = RTM_GETROUTE;   // Get the routes from kernel routing table .

    nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;    // The message is a request for dump.
    nlMsg->nlmsg_seq = msgSeq++;    // Sequence of the message packet.
    nlMsg->nlmsg_pid = getpid();    // PID of process sending the request.

    // Send the request
    if (send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
        return;
    }

    // Read the response
    if ((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0)
    {
        return;
    }

    // Parse the response
    struct route_info ri;
    char gateway[255] = "\0";
    memset(&ri, 0, sizeof(struct route_info));
    parseRoutes(nlMsg, &ri, gateway);

    close(sock);

    outGatewayIp = QString::fromStdString(gateway);
    outInterfaceName = QString::fromStdString(ri.ifName);
}


