# ft_irc — Channel Memory & Routing Fix

## Problem

Clients could register and connect successfully, but channel communication was completely broken:

1. **Isolation** — Messages sent in a channel never reached other clients in the same channel.
2. **Silent Kick** — Clients were dropped from channel context the moment they sent a message or performed an action.

## Root Causes

### 1. Decoupled Memory Allocation (Split Reality Bug)

`commandDispatcher::handleJoin` was creating `new Channel()` objects and storing them **only in its own private vectors** (`_vChannelNames`, `_vChannels`). These were never registered with the `Server` object.

Meanwhile, `handlePrivmsg` looked up channels via `server.getChannelByName()`, which was a **stub that always returned NULL**. The Server class had **zero channel storage** — no map, no vector, nothing.

Result: Two clients joining `#foo` would each get a different `Channel*` allocated in the dispatcher's scope. They were functionally talking to two different realities of the same channel name.

### 2. Inverted Lifecycle Ownership

`handleJoin` never called `chan->addClient(&client)`. Even if the channel object existed, no client was ever added to it. When `handlePrivmsg` checked `chan->hasClient(&client)`, it always returned false, causing a "442 You're not on that channel" error — the client was effectively evicted.

Additionally, line 189 used `return` instead of `continue`, which exited the entire function on duplicate channel detection instead of just skipping to the next channel in a multi-channel JOIN.

### 3. Broadcast Format Hardcoding

`Channel::broadcast()` always prepended `:nick PRIVMSG #channel :` to every message, including JOIN notifications. This broke IRC protocol for non-PRIVMSG broadcasts.

## Solution

### `parsing/Client.hpp` — Server gains channel ownership

```cpp
class Server{
private:
    std::map<std::string, Channel*> _channels;  // NEW: single source of truth
public:
    Channel* getOrCreateChannel(const std::string& name);  // NEW
    // ... existing methods unchanged
};
```

### `parsing/Client.cpp` — Server methods implemented

**`getOrCreateChannel`** — Find existing channel or create a new one in the Server's map:
```cpp
Channel* Server::getOrCreateChannel(const std::string& name)
{
    std::map<std::string, Channel*>::iterator it = _channels.find(name);
    if (it != _channels.end())
        return it->second;
    Channel *chan = new Channel();
    chan->setName(name);
    _channels[name] = chan;
    return chan;
}
```

**`getChannelByName`** — Actually searches the map instead of returning NULL.

**`processChannelJoin`** — Handles the full join lifecycle:
1. Find or create the channel in `Server::_channels`
2. Run `canJoin()` checks (key, limit, invite-only)
3. Call `addClient()` to add the client to the channel
4. Promote to operator if channel is empty (first user)
5. Send JOIN confirmation to the joining client
6. Broadcast JOIN to existing channel members

### `parsing/cmdDispatcher.hpp` — Dispatcher stripped of state

Removed private members:
```diff
 class commandDispatcher{
-private:
-    std::vector<std::string> _vChannelNames;
-    std::vector<Channel *> _vChannels;
-public:
+public:
```

The dispatcher no longer owns any channel data. It is purely functional.

### `parsing/cmdDispatcher.cpp` — handleJoin simplified

Before: 60 lines of channel allocation, duplicate detection, and broken broadcasting.

After: Pure parsing that delegates to `server.processChannelJoin()`:
```cpp
void commandDispatcher::handleJoin(Client &client, const Command &cmd, Server &server)
{
    // ... validate registration, parse params, split by comma ...
    for (size_t i = 0; i < channels.size(); i++)
    {
        // ... validate channel prefix (# or &) ...
        server.processChannelJoin(client, channelName, key);
    }
}
```

**`handlePrivmsg`** — Now builds the full IRC PRIVMSG line and sends it through `Channel::broadcast()`. Since `getChannelByName` actually works, channel messages are delivered.

### `channel/channel.cpp` — Broadcast fixed

**`broadcast`** is now a raw forwarder — it sends whatever string the caller provides:
```cpp
void Channel::broadcast(std::string message, Client *sender)
{
    for (std::vector<Client*>::iterator it = _vClient.begin(); it != _vClient.end(); it++)
    {
        if ((*it)->getFd() != sender->getFd())
            send((*it)->getFd(), message.c_str(), message.length(), 0);
    }
}
```

All callers (`kickClient`, `ChangeTopic`, `removeClient`, `inviteToChannel`) now format their own complete IRC protocol lines before calling `broadcast`.

## Architecture After Fix

```
Dispatcher Layer (stateless)          Server Engine (single source of truth)
┌─────────────────────────┐          ┌──────────────────────────────┐
│ handleJoin:             │          │ _channels: map<string, Chan*>│
│   parse channel names   │────────> │   "#foo" -> Channel* (shared)│
│   parse keys            │  delegate│   "#bar" -> Channel* (shared)│
│   validate prefix       │          │                              │
│                         │          │ processChannelJoin():        │
│ handlePrivmsg:          │          │   getOrCreateChannel()       │
│   parse target          │────────> │   canJoin() check            │
│   parse message         │  lookup  │   addClient()                │
│   format IRC line       │          │   broadcast JOIN             │
│   send via channel      │<──────── │                              │
└─────────────────────────┘          └──────────────────────────────┘
```

## Build

```bash
make re    # clean rebuild
./ircserv <port> <password>
```

All source compiles under `-Wall -Wextra -Werror -std=c++98` with zero warnings.
