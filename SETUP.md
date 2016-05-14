Setting up Guillemot Core
=========================

All new Guillemot Core code lives in `python`. Other than custom libraries
(the ones I'm too lazy to figure out how to publish on PyPi) inside
`python/libs`, everything is specific to Guillemot Core.

Other stuff not in `python` is legacy Gilligan stuff which may get rewritten,
deleted, etc., and maybe it still works(?) but setting it up is outside the
scope of this document.

Caveat
------

The older Gilligan codebase upon which Gillemot Core is built is heavily
dependent on [ROS](http://ros.org/), and by that I mean *really old* ROS. So it
predates all the new Catkin build system stuff. And nobody ever really fully
understood how it built anything. It's really annoying to deal with.

But the good news is Guillemot Core is aiming for a full switch over to non-ROS
Python 3 (also supporting Python 2 so long as it's not too troublesome) which
should hopefully be more straightforward. But read on to find out...


Requirements
------------

Guillemot Core has all the dependencies of ROSLite,
as well as a few of its own:

- *MessagePack 0.3+*:
  `msgpack-python` in PyPi or `python3-msgpack`/`python-msgpack` in apt.
- *PyUSB 1.0.0b1+*:
  `pyusb` in PyPi or `python3-usb`/`python-usb` in apt.
- *pySerial 3+*:
  `pyserial` in PyPi or `python3-serial`/`python-serial` in apt.
- *Six 1.10+*:
  `six` in PyPi or `python3-six`/`python-six` in apt.
- *YAML 3.10+*:
  `PyYAML` in pip or `python3-yaml`/`python-yaml` in apt.
- *ZeroMQ 14+*:
  `pyzmq` in pip or `python3-zmq`/`python-zmq` in apt.

Configuration
-------------

Since the whole idea behind Guillemot Core is "a bunch of ROSLite nodes working
together" you need to export some environment variables to tell all the separate
nodes (which may be separate processes or all running in one process, depending
on how you start them) how to find each other.

**ROSLITE_ATLAS** Should be in the form of
`tcp://[atlas-ipv4-address]:[atlas-port-number]`

Atlas is the ROSLite node which does all the heavy lifting of discovery and
communication, so all nodes must agree on the same Atlas to see each other, and
exactly one Atlas node must be started at that address for anything to work.

Consider using the external IP address on the machine running Atlas, so that
ROSLite nodes running on other machines can still connect to it.

Also, yes, as far as I know only IPv4 works. Not IPv6, not hostnames. I might
be wrong though, I've just only bothered to use IPv4 so far.

**ROSLITE_INTERFACE** should be in the form of `[your-external-ipv4-address]`

This will match the Atlas IPv4 address on the machine where Atlas is running,
but will be different on other machines in the network with different external
IPv4 addresses. Nodes send this in their communications to Atlas, so that Atlas
can tell if they are running on the same machine, and choose to use a faster
form of underlying connection, like IPC.

**ROSLITE_VERBOSE** is optional, but if you set it to `1` then ROSLite nodes
  using the built-in logging will be more chatty. If you set it to `2` they
  will be the most chatty. It defaults to `0` which is least chatty.

One final note regarding configuration, none of these environment variables
are really needed at all. All ROSLite nodes support taking `--atlas`,
`--interface`, and `--verbose` as command line options, so you could always
directly set these values there. Command line values override environment
variables.

Optional
--------

At this point, you should be able to run any of the executable Python scripts
in the `python` directory. But here are a few suggestions to help you out going
forward:

- Consider adding the `roslite` library to your environment `PATH` since some
  of its executable scripts may come in handy.
- In order for the Thermometer node to work, some kind of sensor executable
  must be available on your path. Currently only `vcgencmd` (should come
  installed by default on Raspbian) and `sensors` (for most non-Raspberry Pi
  cases) are supported. The package name for `sensors` on Debian-based distros
  is `lm-sensors`.
