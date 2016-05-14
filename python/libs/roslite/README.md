ROSLite
=======

A simple dynamic computational graph framework for robotics,
inspired by [ROS](http://ros.org/).

Background
----------

I like ROS for the most part, but got really frustrated trying to build it
for use on distributions other than Ubuntu Linux. ROS takes an "everything
and the kitchen sink" approach to itself and that just doesn't work when
you're trying to install from source on a Raspberry Pi for the fifth time
after working through all the stupid catkin bugs and then something
still manages to go wrong after several hours and of course it doesn't save
any incremental progress or anything and then after you're done crying you
think to yourself "all I want is something that has publish/subscribe and
client/server with network transparency" and now here we are.

Requirements
------------

ROSLite depends on a few things that aren't in the standard Python library:

- *MessagePack 0.3+*: `msgpack-python` in pip, `python-msgpack` in apt.
- *YAML 3.10+*: `PyYAML` in pip or `python-yaml` in apt.
- *ZeroMQ 14+*: `pyzmq` in pip or `python-zmq` in apt.

Installing
----------

Just make sure this 'roslite' package is in your Python path.

Contents
--------

- *Atlas*: The required Node which handles all channel/service discovery.
- *Registrar*: An optional key/value store Node. Run it if you want.
- *Client*: A basic client Node that can call into a service with a given
  request and then print the result. Mostly meant to be a command line tool.
- *Subscriber*: A basic subscriber Node that subscribes to a channel and then
  prints out messages that it receives. Mostly meant to be a command line tool.
- *Node*: The base class for all ROSLite Nodes. Makes it easy to publish
  or subscribe to a channel, or get and call a service, or provide a service.
  Also has ways to get/set values in `Registrar`. Has a bit of logging as well.
- *create_argument_parser*: A function that helps get all the arguments you
  need to run a Node from the command line.
- *ExecutionInterrupt*: ROSLite treats `SIGKILL` the same way that it treats
  `KeyboardInterrupt` so that `atexit` always works within Nodes. You probably
  don't need to use this directly though, if you catch `KeyboardInterrupt` as
  needed.

ROSLite tries to be pretty flexible with how you can use it. Each of these
can be run as stand-alone Python scripts, or imported into your own Python
code. Subscriber even lets you define your own callback so you can
build some Nodes by creatively constructing it instead of subclassing Node.

Note that ROSLite internally uses `inproc://`, `ipc://`, or `tcp://` for
inter-Node communication so putting more things in the same process reduces
kernel context switches while still allowing for external access when needed.
Debugging can be easier when each is on its own, system resources can be
conserved by having a single Python interpreter run everything in different
threads. You choose!

Completeness
------------

ROSLite is already nearly feature complete since it has such a limited scope.
It's still missing a good "save everything that was published and re-publish
it later" facility but that's about all I can think of. I still have to think
of a witty name for it.

Compatibility
-------------

I'm aiming to use Python 3.2+ as much as possible but note that everything
works perfectly fine in 2.7 as well. And you can even use both in the same
graph so have at it.

See Also
--------
For now, ROSLite is closely tied to Guillemot Core, so look there for
examples about how to use ROSLite. I eventually expect ROSLite to mature
to the point of going into its own git repo but not today.
