#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include "network_utils.h"

#pragma comment(lib, "ws2_32.lib")   // Link Winsock 2 library
#pragma comment(lib, "iphlpapi.lib") // Link IP Helper API library

#define PORT 40000
#define BUF_SIZE 1024
#define KEYWORD "SHUTDOWN_ESP"
#define HEARTBEAT_MSG "HEARTBEAT"
#define HEARTBEAT_INTERVAL_MS 200 // ms

DWORD WINAPI heartbeat_sender(LPVOID arg)
{
  SOCKET sockfd;
  struct sockaddr_in bcast_addr;
  int broadcast = 1;
  char broadcast_ip[16];

  if (get_broadcast_address(broadcast_ip, sizeof(broadcast_ip)) != 0)
  {
    return 1;
  }

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == INVALID_SOCKET)
  {
    return 1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&broadcast, sizeof(broadcast)) < 0)
  {
    closesocket(sockfd);
    return 1;
  }

  memset(&bcast_addr, 0, sizeof(bcast_addr));
  bcast_addr.sin_family = AF_INET;
  bcast_addr.sin_port = htons(PORT);
  bcast_addr.sin_addr.s_addr = inet_addr(broadcast_ip);

  // 获取所有以太网网卡的MAC地址，分别广播（仅限局域网网卡）
  IP_ADAPTER_INFO AdapterInfo[16];
  DWORD buflen = sizeof(AdapterInfo);
  DWORD status = GetAdaptersInfo(AdapterInfo, &buflen);
  if (status == ERROR_SUCCESS)
  {
    PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
    char msg[128];
    while (1)
    {
      pAdapterInfo = AdapterInfo;
      while (pAdapterInfo)
      {
        if (pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET && pAdapterInfo->AddressLength == 6)
        {
          // 排除虚拟网卡
          if (strstr(pAdapterInfo->Description, "VirtualBox") != NULL ||
              strstr(pAdapterInfo->Description, "VMware") != NULL ||
              strstr(pAdapterInfo->Description, "Hyper-V") != NULL ||
              (pAdapterInfo->Address[0] == 0x0A && pAdapterInfo->Address[1] == 0x00 && pAdapterInfo->Address[2] == 0x27)) // VirtualBox MAC
          {
            pAdapterInfo = pAdapterInfo->Next;
            continue;
          }
          // 判断是否为局域网网卡（常见局域网网段：10.x.x.x, 172.16.x.x-172.31.x.x, 192.168.x.x）
          DWORD ip = inet_addr(pAdapterInfo->IpAddressList.IpAddress.String);
          unsigned char *b = (unsigned char *)&ip;
          int is_lan = 0;
          if (b[0] == 10)
            is_lan = 1;
          else if (b[0] == 192 && b[1] == 168)
            is_lan = 1;
          else if (b[0] == 172 && (b[1] >= 16 && b[1] <= 31))
            is_lan = 1;
          if (is_lan)
          {
            snprintf(msg, sizeof(msg), "%s|%02X%02X%02X%02X%02X%02X",
                     HEARTBEAT_MSG,
                     pAdapterInfo->Address[0], pAdapterInfo->Address[1], pAdapterInfo->Address[2],
                     pAdapterInfo->Address[3], pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
            sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&bcast_addr, sizeof(bcast_addr));
          }
        }
        pAdapterInfo = pAdapterInfo->Next;
      }
      Sleep(HEARTBEAT_INTERVAL_MS);
    }
  }
  closesocket(sockfd);
  return 0;
}

void get_all_local_macs(char *macs_str, size_t size)
{
  IP_ADAPTER_INFO AdapterInfo[16];
  DWORD buflen = sizeof(AdapterInfo);
  DWORD status = GetAdaptersInfo(AdapterInfo, &buflen);
  if (status != ERROR_SUCCESS)
  {
    strncpy(macs_str, "00:00:00:00:00:00", size);
    return;
  }
  PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
  macs_str[0] = '\0';
  char one_mac[32];
  while (pAdapterInfo)
  {
    if (pAdapterInfo->Type == MIB_IF_TYPE_ETHERNET && pAdapterInfo->AddressLength == 6)
    {
      // 排除虚拟网卡
      if (strstr(pAdapterInfo->Description, "VirtualBox") != NULL ||
          strstr(pAdapterInfo->Description, "VMware") != NULL ||
          strstr(pAdapterInfo->Description, "Hyper-V") != NULL ||
          (pAdapterInfo->Address[0] == 0x0A && pAdapterInfo->Address[1] == 0x00 && pAdapterInfo->Address[2] == 0x27)) // VirtualBox MAC
      {
        pAdapterInfo = pAdapterInfo->Next;
        continue;
      }
      snprintf(one_mac, sizeof(one_mac), "%02X%02X%02X%02X%02X%02X",
               pAdapterInfo->Address[0], pAdapterInfo->Address[1], pAdapterInfo->Address[2],
               pAdapterInfo->Address[3], pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
      if (macs_str[0] != '\0')
        strncat(macs_str, ",", size - strlen(macs_str) - 1);
      strncat(macs_str, one_mac, size - strlen(macs_str) - 1);
    }
    pAdapterInfo = pAdapterInfo->Next;
  }
  if (macs_str[0] == '\0')
  {
    strncpy(macs_str, "00:00:00:00:00:00", size);
  }
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    return 1;
  }

  SOCKET sockfd;
  struct sockaddr_in servaddr, cliaddr;
  char buffer[BUF_SIZE];
  int len;
  int n;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd == INVALID_SOCKET)
  {
    WSACleanup();
    return 1;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(PORT);
  if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
  {
    closesocket(sockfd);
    WSACleanup();
    return 1;
  }

  HANDLE heartbeat_thread = CreateThread(NULL, 0, heartbeat_sender, NULL, 0, NULL);
  if (heartbeat_thread == NULL)
  {
    closesocket(sockfd);
    WSACleanup();
    return 1;
  }

  char local_macs[256];
  get_all_local_macs(local_macs, sizeof(local_macs));
  while (1)
  {
    len = sizeof(cliaddr);
    n = recvfrom(sockfd, buffer, BUF_SIZE - 1, 0, (struct sockaddr *)&cliaddr, &len);
    if (n < 0)
    {
      continue;
    }
    buffer[n] = '\0';
    if (strstr(buffer, KEYWORD) != NULL)
    {
      char *token = strtok(local_macs, ",");
      while (token)
      {
        if (strstr(buffer, token) != NULL)
        {
          system("shutdown /s /t 0");
          break;
        }
        token = strtok(NULL, ",");
      }
    }
  }
  closesocket(sockfd);
  WSACleanup();
  return 0;
}
