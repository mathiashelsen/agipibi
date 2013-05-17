# -*- coding: utf-8 -*-
#
#  Copyright 2012,2013 Thibault VINCENT <tibal@reloaded.fr>
#  
#  This file is part of Agipibi.
#
#  Agipibi is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Agipibi is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Agipibi.  If not, see <http://www.gnu.org/licenses/>.
#

import re
from arduino import Arduino, ArduinoError

class AgipibiError(Exception):
    pass

class Agipibi(Arduino):
    CMD_OUT = {
        'PING'      : 0x00,
        'STATUS'    : 0x01,
        'INIT'      : 0x02,
        'LOCKREAD'  : 0x03,
        'CONTROLLER': 0x04,
        'REMOTE'    : 0x05,
        'TALKER'    : 0x06,
        'LISTENER'  : 0x07,
        'UNLISTEN'  : 0x08,
        'UNTALK'    : 0x09,
        'LOCKOUT'   : 0x0a,
        'CLEAR'     : 0x0b,
        'UNLOCK'    : 0x0c,
        'TRIGGER'   : 0x0d,
        'READ'      : 0x0e,
        'WRITE'     : 0x0f,
        'CMD'       : 0x10,
    }
    CMD_IN = {
        'PONG'      : 0x00,
        'CHUNK'     : 0x01,
        'STRING'    : 0x02,
    }
    FLAGS = {
        'BOOLEAN'   : 0x1,
    }

    def interface_ping(self):
        self._write_command('PING')
        try:
            resp = self._read_command(timeout=1)
        except ArduinoError:
            return False
        return resp[0] == 'PONG'

    def gpib_status(self):
        self._write_command('STATUS')
        resp = self._read_command()
        if resp[0] != 'STRING':
            raise AgipibiError('not getting a string in response of'
                                   ' bus status error but: %s' % resp[0])
        pattern = '^E(\d)D(\d)N(\d)n(\d)I(\d)S(\d)A(\d)R(\d)(\d{8})$'
        labels = ('EOI', 'DAV', 'NRFD', 'NDAC', 'IFC', 'SRQ', 'ATN',
                  'REN', 'DIO')
        match = re.match(pattern, self._read_line())
        return dict(zip(labels, match.groups()))

    def gpib_init(self, address=0x00, controller=True):
        self._write_command('INIT')
        self._write(chr(address))
        if controller:
            self._write_command('CONTROLLER')

    def gpib_lock_read(self, state):
        flags = ['BOOLEAN'] if state else [] 
        self._write_command('LOCKREAD', flags=flags)

    def gpib_remote(self, state):
        flags = ['BOOLEAN'] if state else []
        self._write_command('REMOTE', flags=flags)

    def gpib_talker(self, address):
        self._write_command('TALKER')
        self._write(chr(address & 0x1f))

    def gpib_listener(self, address):
        self._write_command('LISTENER')
        self._write(chr(address & 0x1f))

    def gpib_untalk(self):
        self._write_command('UNTALK')

    def gpib_unlisten(self):
        self._write_command('UNLISTEN')

    def gpib_lockout(self):
        self._write_command('LOCKOUT')

    def gpib_clear(self, bus=False):
        flags = ['BOOLEAN'] if bus else []
        self._write_command('CLEAR', flags=flags)

    def gpib_unlock(self):
        self._write_command('UNLOCK')

    def gpib_trigger(self):
        self._write_command('TRIGGER')

    def gpib_read(self):
        data = ''
        # send initial read command
        self._write_command('READ')
        resp = self._read_command()
        # strings are processed at once
        if resp[0] == 'STRING':
            data = self._read_line()
        # chunked streams iteration
        elif resp[0] == 'CHUNK':
            while True:
                size = ord(self._read())
                data += self._read(size=size)
                # transmission is over        
                if 'BOOLEAN' in resp[1]:
                    break
                # ask for a new chunk
                self._write_command('READ')
                resp = self._read_command()
                if resp[0] != 'CHUNK':
                    raise AgipibiError('chunk stream interrupted by'
                                           ' command: %s' % resp[0])
        else:
            raise AgipibiError('expected to get a data transfert but'
                                   ' received command: %s' % resp[0])
        return data

    def gpib_write(self, data):
        for offset in xrange(0, len(data), 255):
            size = min(255, len(data) - offset)
            if size == 255:
                self._write_command('WRITE')
            else:
                self._write_command('WRITE', flags=['BOOLEAN'])
            self._write(chr(size))
            self._write(data[offset : offset + size])
