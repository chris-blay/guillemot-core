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
from sys import stdout
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
    parser.add_argument('-v', '--verbose', action='count',
                        default=int(bool(environ.get('ROSLITE_VERBOSE'))),
                        help='enables extra logging to stdout. '
                             'defaults to $ROSLITE_VERBOSE')
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

    def __init__(self, verbose=False, **kwargs):
        self._verbose = verbose
        if current_thread().__class__.__name__ == '_MainThread':
            try:
                signal.signal(signal.SIGTERM, _sigterm_handler)
            except ValueError:
                pass

    @contextmanager
    def log(self, msg):
        exception_occurred = False
        try:
            if self._verbose:
                self.log_msg('{}…'.format(msg))
                Base._log_depth += 1
                log_count = Base._log_count
                stdout.flush()
            yield
        except:
            exception_occurred = True
            raise
        finally:
            if self._verbose:
                Base._log_depth -= 1
                if log_count == Base._log_count:
                    print(' ', end='')
                else:
                    print('')
                    print('  ' * Base._log_depth, end='')
                print('Error!' if exception_occurred else 'Done!')

    def log_var(self, **kwargs):
        if self._verbose:
            for key, value in kwargs.items():
                self.log_msg('{}={}'.format(key, repr(value)))

    def log_msg(self, msg):
        if self._verbose:
            if Base._log_depth:
                print('')
            print('  ' * Base._log_depth + msg, end='')
            Base._log_count += 1

    def get_message(self, socket, copy=False,
                    encoding='utf-8', use_list=False):
        return unpackb(socket.recv(copy=copy, track=True),
                       encoding=encoding, use_list=use_list)

    def put_message(self, socket, message):
        socket.send(packb(message))


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
        self.put_message(service, {C.REQUEST: C.GET,
                                   C.KEY: key})
        return self.get_message(service)[C.VALUE]
