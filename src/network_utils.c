#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "iphlpapi.lib")

int get_broadcast_address(char *broadcast_ip, size_t size)
{
  PIP_ADAPTER_INFO adapter_info = NULL;
  ULONG buffer_size = 0;
  DWORD result = GetAdaptersInfo(adapter_info, &buffer_size);

  if (result == ERROR_BUFFER_OVERFLOW)
  {
    adapter_info = (PIP_ADAPTER_INFO)malloc(buffer_size);
    result = GetAdaptersInfo(adapter_info, &buffer_size);
  }

  if (result != ERROR_SUCCESS)
  {
    printf("Failed to get adapter info: %d\n", result);
    free(adapter_info);
    return -1;
  }

  PIP_ADAPTER_INFO adapter = adapter_info;
  while (adapter)
  {
    if (adapter->Type == MIB_IF_TYPE_ETHERNET)
    {
      // Calculate broadcast address
      unsigned long ip = inet_addr(adapter->IpAddressList.IpAddress.String);
      unsigned long mask = inet_addr(adapter->IpAddressList.IpMask.String);
      unsigned long broadcast = ip | ~mask;

      struct in_addr broadcast_addr;
      broadcast_addr.s_addr = broadcast;
      strncpy(broadcast_ip, inet_ntoa(broadcast_addr), size);
      free(adapter_info);
      return 0;
    }
    adapter = adapter->Next;
  }

  free(adapter_info);
  return -1;
}
