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
from collections import deque
from glob import glob
from time import sleep

from libs import roslite
from serial import Serial, SerialException


class NanoBridge(roslite.Node):
    '''
    Handles all communication to/from an Arduino Nano.
    '''

    _WARN_NANO_NOT_AVAILABLE = ('Unable to open a /dev/ttyUSB. '
                                'There are either none or more than one.')
    _WARN_PRESSURE_SENSOR_NON_INTEGER = (
        'Presure sensor sent a non-integer value!?')
    _WARN_PRESSURE_SENSOR_NOT_POWERED = 'Pressure sensor is not powered!'
    _WARN_PRESSURE_SENSOR_NOT_CONNECTED = 'Pressure sensor is not connected!'
    _PRESSURE_THRESHOLD = 100

    def __init__(self, atlas, interface,
                 pressure_channel='pressure', **kwargs):
        super(NanoBridge, self).__init__(atlas, interface, **kwargs)
        self._pressure_publisher = self.publish_to_channel(pressure_channel)
        self._pressure_values = deque(maxlen=10)

    def run(self):
        while True:
            try:
                tty = self._get_tty()
                while True:
                    try:
                        value = int(tty.readline())
                    except ValueError:
                        self.log_warn(
                            NanoBridge._WARN_PRESSURE_SENSOR_NON_INTEGER)
                        sleep(1)
                        continue
                    if value <= 0 or value >= 1023:
                        self.log_warn(
                            NanoBridge._WARN_PRESSURE_SENSOR_NOT_POWERED)
                        continue
                    values = self._pressure_values
                    values.append(value)
                    threshold = NanoBridge._PRESSURE_THRESHOLD
                    if max(values) - min(values) > threshold:
                        self.log_warn(
                            NanoBridge._WARN_PRESSURE_SENSOR_NOT_CONNECTED)
                    else:
                        self.put_message(self._pressure_publisher,
                                         sum(values) / len(values))
            except SerialException:
                self.log_warn(NanoBridge._WARN_NANO_NOT_AVAILABLE)
                sleep(1)

    def _get_tty(self):
        filenames = glob('/dev/ttyUSB*')
        if not filenames or len(filenames) > 1:
            raise SerialException()
        return Serial(filenames[0])


if __name__ == '__main__':
    parser = roslite.create_argument_parser(
        'NanoBridge handles all communication to/from an Arduino Nano.')
    parser.add_argument('--pressure', default='pressure',
                        help='channel on which pressure is published. '
                             'defaults to "pressure"')
    NanoBridge(**vars(parser.parse_args())).run()
