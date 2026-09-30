#pragma once

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// libnx exposes IPV6_JOIN_GROUP but omits its request type. SDL3_net builds
// its UDP multicast implementation even though the game uses TCP. Supply the
// BSD request layout here rather than changing the upstream dependency.
#if defined(__SWITCH__) && defined(DYNABLASTER_NEEDS_IPV6_MREQ)
struct ipv6_mreq
{
   struct in6_addr ipv6mr_multiaddr;
   unsigned int ipv6mr_interface;
};
#endif

// Numeric endpoints and wildcard listeners need no DNS service. Resolve them
// locally so single-player loopback works even when DNS/network discovery is
// unavailable, preserving the requested socket type and transport protocol.
static inline int switchGetAddrInfo(const char* host, const char* service,
                                   const struct addrinfo* hints, struct addrinfo** result)
{
   if (!result)
      return EAI_FAIL;
   *result = NULL;
   int family = hints ? hints->ai_family : AF_UNSPEC;
   const int flags = hints ? hints->ai_flags : 0;
   if (family != AF_UNSPEC && family != AF_INET && family != AF_INET6)
      return EAI_FAMILY;
   unsigned int port = 0;
   if (service)
   {
      if (!*service)
         return EAI_SERVICE;
      for (const char* digit = service; *digit; ++digit)
      {
         if (*digit < '0' || *digit > '9')
            return (flags & AI_NUMERICSERV) ? EAI_NONAME : getaddrinfo(host, service, hints, result);
         port = port * 10 + (unsigned int)(*digit - '0');
         if (port > 65535)
            return EAI_SERVICE;
      }
   }
   struct sockaddr_storage address = {0};
   socklen_t address_length;
   struct sockaddr_in* ipv4 = (struct sockaddr_in*)&address;
   struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)&address;
   char unscoped[INET6_ADDRSTRLEN];
   const char* numeric_host = host;
   unsigned int scope = 0;
   if (host && strchr(host, '%'))
   {
      const char* separator = strchr(host, '%');
      const size_t length = (size_t)(separator - host);
      if (length >= sizeof(unscoped) || !separator[1])
         return EAI_NONAME;
      memcpy(unscoped, host, length);
      unscoped[length] = '\0';
      for (const char* digit = separator + 1; *digit; ++digit)
      {
         if (*digit < '0' || *digit > '9' || scope > (0xffffffffu - (unsigned int)(*digit - '0')) / 10)
            return EAI_NONAME;
         scope = scope * 10 + (unsigned int)(*digit - '0');
      }
      numeric_host = unscoped;
   }
   if (!host)
   {
      if (!service)
         return EAI_NONAME;
      if (family == AF_UNSPEC)
         family = AF_INET;
      if (family == AF_INET)
         ipv4->sin_addr.s_addr = htonl((flags & AI_PASSIVE) ? INADDR_ANY : INADDR_LOOPBACK);
      else if (!(flags & AI_PASSIVE))
         ipv6->sin6_addr.s6_addr[15] = 1;
   }
   else if ((family == AF_UNSPEC || family == AF_INET) && inet_pton(AF_INET, host, &ipv4->sin_addr) == 1)
      family = AF_INET;
   else if ((family == AF_UNSPEC || family == AF_INET6) && inet_pton(AF_INET6, numeric_host, &ipv6->sin6_addr) == 1)
      family = AF_INET6;
   else
      return (flags & AI_NUMERICHOST) ? EAI_NONAME : getaddrinfo(host, service, hints, result);
   if (family == AF_INET)
   {
      ipv4->sin_family = AF_INET;
      ipv4->sin_port = htons((unsigned short)port);
      address_length = sizeof(*ipv4);
#ifdef __SWITCH__
      ipv4->sin_len = sizeof(*ipv4);
#endif
   }
   else
   {
      ipv6->sin6_family = AF_INET6;
      ipv6->sin6_port = htons((unsigned short)port);
      ipv6->sin6_scope_id = scope;
      address_length = sizeof(*ipv6);
   }
   // libnx freeaddrinfo frees each node as one allocation, including its address.
   struct switch_addrinfo_node { struct addrinfo info; struct sockaddr_storage address; };
   struct switch_addrinfo_node* node = (struct switch_addrinfo_node*)calloc(1, sizeof(*node));
   if (!node)
      return EAI_MEMORY;
   node->address = address;
   node->info.ai_flags = flags;
   node->info.ai_family = family;
   node->info.ai_socktype = hints && hints->ai_socktype ? hints->ai_socktype : SOCK_STREAM;
   node->info.ai_protocol = hints && hints->ai_protocol ? hints->ai_protocol
                            : node->info.ai_socktype == SOCK_DGRAM ? IPPROTO_UDP : IPPROTO_TCP;
   node->info.ai_addrlen = address_length;
   node->info.ai_addr = (struct sockaddr*)&node->address;
   *result = &node->info;
   return 0;
}

#define getaddrinfo switchGetAddrInfo

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

#include <fcntl.h>
#include <errno.h>

// Ryujinx 1.3.2 reverses F_SETFL's O_NONBLOCK interpretation. Read back the
// state instead of assuming success; real Switch sockets take the first path.
static inline int switchSocketFcntl(int fd, int command, int flags)
{
   int result = fcntl(fd, command, flags);
   if (result < 0 || command != F_SETFL)
      return result;
   int actual = fcntl(fd, F_GETFL, 0);
   if (actual < 0)
      return -1;
   if ((actual & O_NONBLOCK) == (flags & O_NONBLOCK))
      return result;
   result = fcntl(fd, F_SETFL, flags ^ O_NONBLOCK);
   if (result < 0)
      return -1;
   actual = fcntl(fd, F_GETFL, 0);
   if (actual < 0)
      return -1;
   if ((actual & O_NONBLOCK) == (flags & O_NONBLOCK))
      return result;
   errno = EIO;
   return -1;
}

#ifdef __SWITCH__
#define fcntl switchSocketFcntl
#endif
