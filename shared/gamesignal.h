#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

// a signal is a public member, connect(callable) appends a subscriber, operator() invokes every
// subscriber synchronously, in registration order, on the calling thread - no queuing.
//
// connect() returns a Connection token; disconnect(token) removes that one subscriber. Nothing
// auto-disconnects, so anything shorter-lived than the signal it subscribes to must disconnect
// itself, typically from its own destructor.
template <typename... Args>
class Signal
{
public:
   using Slot = std::function<void(Args...)>;
   using Connection = std::size_t;

   Connection connect(Slot slot)
   {
      const Connection id = _next_id++;
      _slots.emplace_back(id, std::move(slot));
      return id;
   }

   void disconnect(Connection id)
   {
      std::erase_if(_slots, [id](const auto& entry) { return entry.first == id; });
   }

   // drops every subscriber
   void disconnectAll()
   {
      _slots.clear();
   }

   void operator()(Args... args) const
   {
      // iterate a copy - a slot disconnecting another subscriber (or itself, via a deferred
      // deletion callback) while this signal is being invoked must not invalidate the loop.
      const auto subscribers = _slots;
      for (const auto& entry : subscribers)
      {
         entry.second(args...);
      }
   }

private:
   Connection _next_id = 0;
   std::vector<std::pair<Connection, Slot>> _slots;
};
