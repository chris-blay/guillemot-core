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
from time import sleep

from yaml import safe_load

from roslite._roslite import create_argument_parser, Node


class Client(Node):
    '''
    Sends a request to a service and prints out the response.
    '''

    def __init__(self, atlas, interface, service, request,
                 number=1, wait=1, **kwargs):
        super(Client, self).__init__(atlas, interface, **kwargs)
        self._service = self.get_service(service)
        self._request = safe_load(request)
        self._number = number if number > 0 else float('infinity')
        self._wait = wait

    def run(self):
        while self._number:
            self.put_message(self._service, self._request)
            self.log_out(repr(self.get_message(self._service)))
            self._number -= 1
            if self._number:
                sleep(self._wait)


if __name__ == '__main__':
    parser = create_argument_parser(
        'Client sends a request to a service and prints out the response.')
    parser.add_argument('service', help='service to use')
    parser.add_argument('request', help='request to send, formatted in YAML')
    parser.add_argument('-n', '--number', default=1, type=int,
                        help='number of times to make the request. '
                             'specify <= 0 for infinite repetitions')
    parser.add_argument('-w', '--wait', default=1, type=float,
                        help='number of seconds to wait between requests')
    Client(**vars(parser.parse_args())).run()
