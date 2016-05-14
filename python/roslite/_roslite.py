# coding=utf-8

# Copyright 2015 Christopher Blay <chris.b.blay@gmail.com>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

from __future__ import print_function, unicode_literals
from argparse import Action, ArgumentParser
from contextlib import contextmanager
from os import environ, getpid
import signal
from threading import current_thread

from msgpack import packb, unpackb
import zmq


class ExecutionInterrupt(KeyboardInterrupt):
    pass


def _sigterm_handler(signum, frame):
    raise ExecutionInterrupt()


class _EnvDefault(Action):

    def __init__(self, env_key, **kwargs):
        default = environ.get(env_key)
        kwargs['required'] = not default
        super(_EnvDefault, self).__init__(default=default, **kwargs)

    def __call__(self, parser, namespace, values, option_string):
        setattr(namespace, self.dest, values)


def create_argument_parser(description):
    '''
    Creates an ArgumentParser to help get arguments for running roslite Nodes.
    '''
    parser = ArgumentParser(description=description)
    parser.add_argument('--atlas',
                        action=_EnvDefault, env_key='ROSLITE_ATLAS',
                        help='atlas address to use. should be '
                             'of the form "tcp://<ip-address>:<port>". '
                             'defaults to $ROSLITE_ATLAS')
    parser.add_argument('--interface',
                        action=_EnvDefault, env_key='ROSLITE_INTERFACE',
                        help='interface on which this Node is running. '
                             'should be of the form "<ip-address>". '
                             'defaults to $ROSLITE_INTERFACE')
    parser.add_argument('-v', '--verbose', action='count', default=0,
                        help='enables extra logging')
    return parser


class C(object):
    '''
    Contants used  when communicating with Atlas and Registrar.
    '''

    # Common request keys.
    REQUEST = 1

    # Atlas request keys.
    CHANNEL = 2
    SERVICE = 3
    INTERFACE = 4
    PID = 5

    # Registrar request/response keys.
    KEY = 2
    VALUE = 3

    # Atlas request types.
    LOOKUP_CHANNEL = 1
    LOOKUP_SERVICE = 2

    # Atlas response keys.
    PUB = 1
    SUB = 2
    REQ = 3
    REP = 4

    # Registy request types.
    SET = 1
    GET = 2


class Base(object):
    '''
    Base roslite object which Node and Atlas subclass.
    '''

    _log_depth = 0
    _log_count = 0
    _COLOR_RED = '\033[91m'
    _COLOR_ORANGE = '\033[93m'
    _COLOR_BLUE = '\033[94m'
    _COLOR_NONE = '\033[0m'
    _LEVEL_LABELS = {
        -1: '',
        0: _COLOR_RED + '[E] ' + _COLOR_NONE,
        1: _COLOR_ORANGE + '[W] ' + _COLOR_NONE,
        2: _COLOR_BLUE + '[I] ' + _COLOR_NONE}
    _LEVEL_SHIFT = 1

    def __init__(self, verbose=False, **kwargs):
        self._verbose = verbose
        if current_thread().__class__.__name__ == '_MainThread':
            try:
                signal.signal(signal.SIGTERM, _sigterm_handler)
            except ValueError:
                pass

    @contextmanager
    def log(self, msg, level=2):
        exception_occurred = False
        try:
            if self._verbose >= level - Base._LEVEL_SHIFT:
                self._log_msg('{}…'.format(msg), level=level)
                Base._log_depth += 1
                log_count = Base._log_count
            yield
        except:
            exception_occurred = True
            raise
        finally:
            if self._verbose >= level - Base._LEVEL_SHIFT:
                Base._log_depth -= 1
                if log_count == Base._log_count:
                    print(' ', end='')
                else:
                    print('', flush=False)
                    print('  ' * Base._log_depth, end='')
                print('Error!' if exception_occurred else 'Done!',
                      end='', flush=True)

    def log_var(self, **kwargs):
        level = 2
        if self._verbose >= level - Base._LEVEL_SHIFT:
            for key, value in kwargs.items():
                self._log_msg('{}={}'.format(key, repr(value)), level=level)

    def log_info(self, msg):
        self._log_msg(msg, level=2)

    def log_warn(self, msg):
        self._log_msg(msg, level=1)

    def log_err(self, msg):
        self._log_msg(msg, level=0)
        print('')
        raise Exception(msg)

    def log_out(self, msg):
        self._log_msg(msg, level=-1)

    def _log_msg(self, msg, level):
        if self._verbose >= level - Base._LEVEL_SHIFT:
            level_label = Base._LEVEL_LABELS[level]
            print('', flush=False)
            print('{}{}{}'.format('  ' * Base._log_depth, level_label, msg),
                  end='', flush=True)
            Base._log_count += 1

    def get_message(self, socket, copy=False,
                    encoding='utf-8', use_list=False):
        return unpackb(socket.recv(copy=copy, track=True),
                       encoding=encoding, use_list=use_list)

    def put_message(self, socket, message):
        socket.send(packb(message))

    def get_packed_message(self, socket, copy=False):
        return socket.recv(copy=copy, track=True)

    def put_packed_message(self, socket, message):
        socket.send(message)


class Node(Base):
    '''
    Base class for all regular roslite Nodes which handles
    channel/service discovery with Atlas.
    '''

    def __init__(self, atlas, interface, **kwargs):
        super(Node, self).__init__(**kwargs)
        self._context = kwargs.get('context')
        if not self._context:
            self._context = zmq.Context()
        self._atlas_service = self._context.socket(zmq.REQ)
        self._atlas_service.connect(atlas)
        self._interface = interface
        self._pid = getpid()
        self._channel_cache = {}
        self._service_cache = {}

    def publish_to_channel(self, channel):
        with self.log('Publishing to "{}"'.format(channel)):
            socket = self._context.socket(zmq.PUB)
            address = self._lookup_channel(channel)[C.PUB]
            self.log_var(publish_address=address)
            socket.connect(address)
        return socket

    def subscribe_to_channel(self, channel):
        with self.log('Subscribing to "{}"'.format(channel)):
            socket = self._context.socket(zmq.SUB)
            address = self._lookup_channel(channel)[C.SUB]
            self.log_var(subscribe_address=address)
            socket.connect(address)
            socket.setsockopt_string(zmq.SUBSCRIBE, '')
        return socket

    def _lookup_channel(self, channel):
        if not self._channel_cache.get(channel):
            self.put_message(self._atlas_service,
                             {C.REQUEST: C.LOOKUP_CHANNEL,
                              C.CHANNEL: channel,
                              C.INTERFACE: self._interface,
                              C.PID: self._pid})
            self._channel_cache[channel] = (
                self.get_message(self._atlas_service))
        return self._channel_cache[channel]

    def get_service(self, service, timeout=100):
        with self.log('Getting "{}"'.format(service)):
            socket = self._context.socket(zmq.REQ)
            socket.RCVTIMEO = timeout
            address = self._lookup_service(service)[C.REQ]
            self.log_var(request_address=address)
            socket.connect(address)
        return socket

    def provide_service(self, service):
        with self.log('Providing "{}"'.format(service)):
            socket = self._context.socket(zmq.REP)
            address = self._lookup_service(service)[C.REP]
            self.log_var(reply_address=address)
            socket.connect(address)
        return socket

    def _lookup_service(self, service):
        if not self._service_cache.get(service):
            self.put_message(self._atlas_service,
                             {C.REQUEST: C.LOOKUP_SERVICE,
                              C.SERVICE: service,
                              C.INTERFACE: self._interface,
                              C.PID: self._pid})
            self._service_cache[service] = (
                self.get_message(self._atlas_service))
        return self._service_cache[service]

    def set_value(self, key, value, registrar='registrar'):
        service = self.get_service(registrar)
        self.put_message(service, {C.REQUEST: C.SET,
                                   C.KEY: key,
                                   C.VALUE: value})
        self.get_message(service)

    def get_value(self, key, registrar='registrar'):
        service = self.get_service(registrar)
        self.put_message(service, {C.REQUEST: C.GET, C.KEY: key})
        return self.get_message(service)[C.VALUE]
