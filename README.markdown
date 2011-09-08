### cheaproute: A load-balancing solution for multi-homed residential gateways

The goal of this project is to create an easy-to-use user-space 'router' that
will automatically pick the lowest-latency route for a particular destination.
The most common use case is a home user with multiple independent ISPs.  In my
case, I have both ADSL and cable-modem service, but depending on the location
of the remote host, one of them is usually  faster than the other. 

My current plan is to, for every new socket connection, send a copy of the
initiating packet (SYN in the case of TCP) out all external interfaces.
Whichever interface receives the corresponding ACK first will be the one that
is used for that particular connection. The slower 'sibling connection' will be
reset immediately.

With this simple rule, if one of the links is down, even if just for a
particular route, the system will immediately start using the other link
for all new connections.

At this point, I am still exploring netlink, the user-space packet-queueing
API, and other options for how best to solve this problem. Most of my effort
has been on test tools to aid this exploration, that can evolve into full-blown
functional tests once the project is more mature.

### playbacktun: Easily manufacture packets without a hex editor

This tool is the only part of the project that is currently useful on its own.
Given a file full of UDP, TCP, and ICMP packets serialized in an easy-to-read
JSON format, it will create a TUN interface and send the packets in on that
interface. The user can then play around with routing and iptables rules to see
how netfilter will behave when it receives those packets.

The JSON deserialization code will set all the appropriate checksums in the
packets, so modifying parts of the header is as simple as changing the text in
the file.

    Usage: playbacktun <iface_name> <json_packet_log>

Example JSON packets:

    {
      "ip": {
        "version": 4,
        "tos": 0,
        "id": 1642,
        "flags": ["DF"],
        "fragmentOffset": 0,
        "ttl": 64,
        "protocol": "TCP",
        "source": "192.168.1.125",
        "destination": "72.14.204.147"
      },
      "tcp": {"
        "sourcePort": 39570,
        "destPort": 80,
        "seqNumber": 1628108555,
        "ackNumber": 2245680723,
        "flags": ["ACK","PSH"],
        "windowSize": 115"
      },
      "data": {"type": "text", "data": ["
          "GET / HTTP/1.1\r\n",
          "Accept: */*\r\n",
          "Host: www.google.com\r\n",
          "Connection: Keep-Alive\r\n",
          "\r\n"
        ]
      }
    }

    {
      "ip": {
        "version": 4, "tos": 0, "id": 0, "flags": ["DF"],
        "fragmentOffset": 0, "ttl": 64, "protocol": "ICMP",
        "source": "192.168.6.5", "destination": "192.168.1.20"
      },
      "icmp": {
        "type": "echoRequest", "code": 0,
        "identifier": 27203, "sequenceNumber": 10
      },
      "data": {
        "type": "hex",
        "data": [
          "a4 fe 66 4e 00 00 00 00  6f be 09 00 00 00 00 00",
          "10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f",
          "20 21 22 23 24 25 26 27  28 29 2a 2b 2c 2d 2e 2f",
          "30 31 32 33 34 35 36 37"
        ]
      }
    }

Building
--------

cheaperoute uses CMake for its build system. It should be compiled with GCC 4.4
or later.

Inside the cloned cheaproute directory, create a build directory for cmake:

    $ mkdir build
    $ cd build

Next, build the project:

    $ cmake ..
    $ make

If you like, you can run the unit tests:

    $ make test

Or, for more details:

    $ src/base/cheaproute-base-tests
    $ src/net/cheaproute-net-tests

The applications can be run directly from the build directory:

    # src/playbacktun test_iface ../samples/packet_logs/ping_and_resolve_google.json

Because playbacktun creates a TUN device, you will probably need to run it as
root.
