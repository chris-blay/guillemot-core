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
from distutils.spawn import find_executable
from re import compile
from subprocess import check_output
from time import sleep

import roslite


class Thermometer(roslite.Node):
    '''
    Periodically publishes CPU temperature in Celcius.
    Supports using `sensors` (most common) and `vcgencmd` (Raspberry Pi).
    '''

    def __init__(self, atlas, interface, channel='thermometer', **kwargs):
        super(Thermometer, self).__init__(atlas, interface, **kwargs)
        self._thermometer_publisher = self.publish_to_channel(channel)
        if find_executable('sensors'):
            self._args = ('sensors',)
            self._pattern = compile(r'^CPU Temperature: +\+(\d+\.\d+)')
        elif find_executable('vcgencmd'):
            self._args = ('vcgencmd', 'measure_temp')
            self._pattern = compile(r'^temp=(\d+\.\d+)')
        else:
            raise Exception(
                'Neither `sensors` nor `vcgencmd` are on the PATH.')

    def run(self):
        while True:
            self.put_message(self._thermometer_publisher,
                             self._get_temperature())
            sleep(1)

    def _get_temperature(self):
        for line in check_output(self._args).decode('utf-8').split('\n'):
            match = self._pattern.match(line)
            if match:
                return float(match.group(1))


if __name__ == '__main__':
    parser = roslite.create_argument_parser(
        'Thermometer periodically publishes CPU temperature in Celcius.')
    parser.add_argument('--channel', default='thermometer',
                        help='channel on which messages are published. '
                             'defaults to "thermometer"')
    Thermometer(**vars(parser.parse_args())).run()
