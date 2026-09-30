#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>

// libnx exposes IPV6_JOIN_GROUP but omits its request type. SDL3_net builds
// its UDP multicast implementation even though the game uses TCP. Supply the
// BSD request layout here rather than changing the upstream dependency.
#ifdef __SWITCH__
struct ipv6_mreq
{
   struct in6_addr ipv6mr_multiaddr;
   unsigned int ipv6mr_interface;
};
#endif

// libnx forwards even numeric getnameinfo requests to the DNS service.
// SDL_net only needs numeric strings; handle them locally, which also works
// in emulators that do not implement sfdnsres GetNameInfoRequest.
static inline int switchGetNameInfo(const struct sockaddr* address, socklen_t length,
                                   char* host, socklen_t host_length,
                                   char* service, socklen_t service_length, int flags)
{
   if ((host && !(flags & NI_NUMERICHOST)) || (service && !(flags & NI_NUMERICSERV)))
      return getnameinfo(address, length, host, host_length, service, service_length, flags);
   if (!address)
      return EAI_FAMILY;
   const void* ip;
   unsigned int port;
   unsigned int scope = 0;
   if (address->sa_family == AF_INET && length >= sizeof(struct sockaddr_in))
   {
      const struct sockaddr_in* ipv4 = (const struct sockaddr_in*)address;
      ip = &ipv4->sin_addr;
      port = ntohs(ipv4->sin_port);
   }
   else if (address->sa_family == AF_INET6 && length >= sizeof(struct sockaddr_in6))
   {
      const struct sockaddr_in6* ipv6 = (const struct sockaddr_in6*)address;
      ip = &ipv6->sin6_addr;
      port = ntohs(ipv6->sin6_port);
      scope = ipv6->sin6_scope_id;
   }
   else
      return EAI_FAMILY;
   if (host)
   {
      char numeric[INET6_ADDRSTRLEN];
      if (!inet_ntop(address->sa_family, ip, numeric, sizeof(numeric)))
         return EAI_FAIL;
      const int count = scope ? snprintf(host, host_length, "%s%%%u", numeric, scope)
                              : snprintf(host, host_length, "%s", numeric);
      if (count < 0 || (unsigned int)count >= host_length)
         return EAI_OVERFLOW;
   }
   if (service)
   {
      const int count = snprintf(service, service_length, "%u", port);
      if (count < 0 || (unsigned int)count >= service_length)
         return EAI_OVERFLOW;
   }
   return 0;
}

#define getnameinfo switchGetNameInfo
