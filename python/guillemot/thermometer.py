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

    _SOURCES = (
        (('vcgencmd', 'measure_temp'), r'^temp=(?P<temp>\d+\.\d+)'),
        (('sensors',),
         r'^(Physical id 0|CPU Temperature): +\+(?P<temp>\d+\.\d+)'),
    )

    def __init__(self, atlas, interface, channel='thermometer', **kwargs):
        super(Thermometer, self).__init__(atlas, interface, **kwargs)
        self._thermometer_publisher = self.publish_to_channel(channel)
        for args, pattern in Thermometer._SOURCES:
            if find_executable(args[0]):
                self._args = args
                self._pattern = compile(pattern)
                return
        self.log_err('No configured temperature sources are on the PATH.')

    def run(self):
        while True:
            temp = self._get_temperature()
            if temp is not None:
                self.put_message(self._thermometer_publisher, temp)
            else:
                self.log_warn('Unable to get temperature!')
            sleep(1)

    def _get_temperature(self):
        for line in check_output(self._args).decode('utf-8').split('\n'):
            match = self._pattern.match(line)
            if match:
                return float(match.group('temp'))


if __name__ == '__main__':
    parser = roslite.create_argument_parser(
        'Thermometer periodically publishes CPU temperature in Celcius.')
    parser.add_argument('--channel', default='thermometer',
                        help='channel on which messages are published. '
                             'defaults to "thermometer"')
    Thermometer(**vars(parser.parse_args())).run()
