Setting up Guillemot Core
=========================
 
Caveat
------
The older Gilligan codebase upon which Gillemot Core is built is heavily
dependent on ROS [0], and by that I mean *really old* ROS. So it predates all
the new Catkin build system stuff. And nobody ever really fully understood how
it built anything. And it's really annoying to deal with.

But the good news is Guillemot Core is aiming for a full switch over to non-ROS
Python 3 (also supporting Python 2 so long as it's not too troublesome) which
should hopefully be more straightforward. But read on to find out...

[0] http://ros.org/

Here Goes
---------
All new Guillemot Core code lives in `python`. Other than custom libraries
inside `python/libs` [1], everything is Guillemot-specific code.

There are also a few required external Python library dependencies, which can
be fulfilled in a number of ways, but here are my two recommended methods:

- Assuming the packages are available and up to date, install via Linux
  distribution package manager.
- Install from PyPi into a local Python virtualenv environment.

The specific dependencies are as follows:

- msgpack 0.4+: Known as `python3-msgpack` and/or `python-msgpack` on
  Debian-based distros, or `msgpack-python` on PyPi.
- pyusb 1.0.0b1+: Known as `python3-usb` and/or `python-usb` on Debian-based
  distros, or `pyusb` on PyPi.
- zmq ??: Known as `python3-zmq` and/or `python-zmq` on Debian-based distros,
  or `pyzmq` on PyPi.



[1] The ones I'm too lazy to figure out how to publish on PyPi...
