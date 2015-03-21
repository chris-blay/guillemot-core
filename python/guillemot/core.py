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

import roslite
from thermometer import Thermometer


def daemon(thread):
    thread.daemon = True
    return thread


def registrar(**kwargs):
    roslite.Registrar(persistence_filename='/dev/null', **kwargs).run()


def registrar_subscriber(**kwargs):
    roslite.Subscriber(channel='registrar', **kwargs).run()


def thermometer(**kwargs):
    Thermometer(**kwargs).run()


def thermometer_subscriber(**kwargs):
    roslite.Subscriber(channel='thermometer',
                       callback=(lambda self, message:
                                 self.set_value('temp', message)),
                       **kwargs).run()


args = vars(roslite.create_argument_parser('Guillemot Core.').parse_args())
args['context'] = zmq.Context()
daemon(Thread(target=registrar, kwargs=args)).start()
daemon(Thread(target=registrar_subscriber, kwargs=args)).start()
daemon(Thread(target=thermometer, kwargs=args)).start()
daemon(Thread(target=thermometer_subscriber, kwargs=args)).start()
roslite.Atlas(**args).run()
