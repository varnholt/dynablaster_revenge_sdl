#pragma once

#include <memory>

#include <SDL3_net/SDL_net.h>

// owning SDL_net handles
using NetStreamSocketHandle = std::unique_ptr<NET_StreamSocket, decltype(&NET_DestroyStreamSocket)>;
using NetAddressHandle = std::unique_ptr<NET_Address, decltype(&NET_UnrefAddress)>;
using NetServerHandle = std::unique_ptr<NET_Server, decltype(&NET_DestroyServer)>;
