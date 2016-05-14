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
from threading import Thread

import zmq

import nano_bridge
from libs import roslite
import thermometer


NODES = (
    (nano_bridge.NanoBridge, 'nano_bridge'),
    (roslite.Registrar, 'registrar'),
    (thermometer.Thermometer, 'thermometer'),
)

parser = roslite.create_argument_parser(
    'Guillemot Core runs many different roslite nodes in a single process.')
parser.add_argument('--persistence_filename', default='/dev/null')
args = vars(parser.parse_args())
args['context'] = zmq.Context()
for node, name in NODES:
    target = lambda **kwargs: node(**kwargs).run()
    thread = Thread(target=target, name=name, kwargs=args)
    thread.daemon = True
    thread.start()
roslite.Atlas(**args).run()
