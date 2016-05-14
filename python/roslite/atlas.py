#!/usr/bin/python3
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
from collections import namedtuple
from itertools import chain
from os import getpid

import zmq

from roslite._roslite import Base, C, create_argument_parser


_Proxy = namedtuple('_Proxy', (
    'frontend', 'frontend_inproc', 'frontend_ipc', 'frontend_tcp',
    'backend', 'backend_inproc', 'backend_ipc', 'backend_tcp'))


class Atlas(Base):
    '''
    Provides channel/service discovery for roslite Nodes.
    '''

    def __init__(self, atlas, interface, **kwargs):
        super(Atlas, self).__init__(**kwargs)
        self._context = kwargs.get('context')
        if not self._context:
            self._context = zmq.Context()
        self._atlas_provider = self._context.socket(zmq.REP)
        self._atlas_provider.bind(atlas)
        self._interface = interface
        self._pid = getpid()
        self._next_id = 1000
        self._poller = zmq.Poller()
        self._poller.register(self._atlas_provider, zmq.POLLIN)
        self._channels = {}
        self._services = {}

    def run(self):
        while True:
            sockets = tuple(event[0] for event in self._poller.poll())
            if self._atlas_provider in sockets:
                with self.log('Received Atlas request'):
                    message = self.get_message(self._atlas_provider)
                    request = message.get(C.REQUEST)
                    response = None
                    if request == C.LOOKUP_CHANNEL:
                        channel = message.get(C.CHANNEL)
                        if channel:
                            self.log_info('lookup_channel {}'.format(channel))
                            response = self._build_response(
                                message, self._get_channel_proxy(channel),
                                C.PUB, C.SUB)
                        else:
                            self.log_warn('No channel given to lookup')
                    elif request == C.LOOKUP_SERVICE:
                        service = message.get(C.SERVICE)
                        if service:
                            self.log_info('lookup_service {}'.format(service))
                            response = self._build_response(
                                message, self._get_service_proxy(service),
                                C.REQ, C.REP)
                        else:
                            self.log_warn('No service given to lookup')
                    else:
                        self.log_warn('Unknown Atlas request {}'.format(
                            request))
                self.put_message(self._atlas_provider, response)
            for proxy in chain(self._channels.values(),
                               self._services.values()):
                if proxy.frontend in sockets:
                    with self.log('Forwarding message(s) to backend'):
                        try:
                            proxy.backend.send_multipart(
                                proxy.frontend.recv_multipart(), zmq.NOBLOCK)
                        except zmq.error.Again:
                            self.log_info('Backend not available')
                if proxy.backend in sockets:
                    with self.log('Forwarding message(s) to frontend'):
                        proxy.frontend.send_multipart(
                            proxy.backend.recv_multipart())

    def _build_response(self, message, proxy, frontend_type, backend_type):
        if self._interface == message[C.INTERFACE]:
            if self._pid == message[C.PID]:
                return {frontend_type: proxy.frontend_inproc,
                        backend_type: proxy.backend_inproc}
            else:
                return {frontend_type: proxy.frontend_ipc,
                        backend_type: proxy.backend_ipc}
        else:
            return {frontend_type: proxy.frontend_tcp,
                    backend_type: proxy.backend_tcp}

    def _get_channel_proxy(self, channel):
        if channel not in self._channels:
            with self.log('Creating proxy for channel "{}"'.format(channel)):
                self._channels[channel] = self._create_proxy(zmq.XSUB,
                                                             zmq.XPUB)
        return self._channels[channel]

    def _get_service_proxy(self, service):
        if service not in self._services:
            with self.log('Creating proxy for service "{}"'.format(service)):
                self._services[service] = self._create_proxy(zmq.ROUTER,
                                                             zmq.DEALER)
        return self._services[service]

    def _create_proxy(self, frontend_type, backend_type):
        frontend = self._context.socket(frontend_type)
        frontend_id = self._get_id()
        frontend_inproc = self._bind_to_inproc(frontend, frontend_id)
        frontend_ipc = self._bind_to_ipc(frontend, frontend_id)
        frontend_tcp = self._bind_to_tcp(frontend)
        self._poller.register(frontend, zmq.POLLIN)
        self.log_var(frontend_inproc=frontend_inproc,
                     frontend_ipc=frontend_ipc, frontend_tcp=frontend_tcp)
        backend = self._context.socket(backend_type)
        backend_id = self._get_id()
        backend_inproc = self._bind_to_inproc(backend, backend_id)
        backend_ipc = self._bind_to_ipc(backend, backend_id)
        backend_tcp = self._bind_to_tcp(backend)
        self._poller.register(backend, zmq.POLLIN)
        self.log_var(backend_inproc=backend_inproc,
                     backend_ipc=backend_ipc, backend_tcp=backend_tcp)
        return _Proxy(frontend, frontend_inproc, frontend_ipc, frontend_tcp,
                      backend, backend_inproc, backend_ipc, backend_tcp)

    def _get_id(self):
        id = self._next_id
        self._next_id += 1
        return id

    def _bind_to_inproc(self, socket, id):
        address = 'inproc://id{}'.format(id)
        socket.bind(address)
        return address

    def _bind_to_ipc(self, socket, id):
        address = 'ipc:///tmp/atlas{}id{}'.format(self._pid, id)
        socket.bind(address)
        return address

    def _bind_to_tcp(self, socket):
        base_address = 'tcp://{}'.format(self._interface)
        return '{}:{}'.format(base_address,
                              socket.bind_to_random_port(base_address))


if __name__ == '__main__':
    parser = create_argument_parser(
        'Atlas provides channel/service discovery for roslite Nodes.')
    Atlas(**vars(parser.parse_args())).run()
