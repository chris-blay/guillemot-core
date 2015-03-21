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
import atexit

from yaml import safe_dump, safe_load

from roslite._roslite import C, create_argument_parser, Node


class Registrar(Node):
    '''
    A key/value store. Provides a service for getting and setting key/value
    pairs and publishes new key/value pairs to a channel for interested Nodes.
    '''

    def __init__(self, atlas, interface, persistence_filename,
                 service='registrar', channel='registrar', **kwargs):
        super(Registrar, self).__init__(atlas, interface, **kwargs)
        self._persistence_filename = persistence_filename
        self._registrar_provider = self.provide_service(service)
        self._registrar_publisher = self.publish_to_channel(channel)
        self._load_store()
        atexit.register(self._save_store)

    def run(self):
        try:
            while True:
                message = self.get_message(self._registrar_provider)
                try:
                    if message[C.REQUEST] == C.SET:
                        key = message[C.KEY]
                        value = message[C.VALUE]
                        self._store[key] = value
                        response = {C.KEY: key, C.VALUE: value}
                        self.put_message(self._registrar_publisher, response)
                    elif message[C.REQUEST] == C.GET:
                        key = message[C.KEY]
                        response = {C.KEY: key, C.VALUE: self._store.get(key)}
                    else:
                        response = None
                except KeyError:
                    response = None
                self.put_message(self._registrar_provider, response)
        finally:
            self._save_store()

    def _load_store(self):
        with open(self._persistence_filename, 'rb') as handle:
            self._store = safe_load(handle.read().decode('UTF-8'))
        if self._store is None:
            self._store = {}

    def _save_store(self):
        with open(self._persistence_filename, 'wb') as handle:
            handle.write(safe_dump(self._store, encoding='utf-8'))


if __name__ == '__main__':
    parser = create_argument_parser('Registrar is a key/value store.')
    parser.add_argument('persistence_filename')
    parser.add_argument('--service', default='registrar',
                        help='service to provide. defaults to "registrar"')
    parser.add_argument('--channel', default='registrar',
                        help='channel to publish to. defaults to "registrar"')
    Registrar(**vars(parser.parse_args())).run()
