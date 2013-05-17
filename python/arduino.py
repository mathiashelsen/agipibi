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

import serial

class ArduinoError(Exception):
    pass

class Arduino(object):
    CMD_OUT = {}
    CMD_IN = {}
    FLAGS = {}

    def __init__(self, device='/dev/ttyUSB0', debug=False):
        self._arduino = serial.Serial(port=device, baudrate=115200)
        self._debug = debug

    def _read(self, size=1, timeout=None):
        if timeout is not None:
            self._arduino.timeout = timeout

        received = self._arduino.read(size=size)
        if self._debug:
            print 'R', ' '.join('{0:x}'.format(ord(c)) for c in received)
        
        if timeout is not None:
            self._arduino.timeout = None
        
        if len(received) != size:
            raise ArduinoError('received less bytes than expected'
                               ' from Arduino: %d/%d' % (len(received), size))
        
        return received

    def _read_command(self, timeout=None):
        byte = ord(self._read(timeout=timeout))
        b_command = byte & 0x3f
        b_flags = (byte & 0x60) >> 6

        command = ''
        flags = []
        
        for c_name, c_value in self.CMD_IN.iteritems():
            if b_command == c_value:
                command = c_name
                break
        if not command:
            raise ArduinoError('received an unknown command from'
                               ' Arduino: %x' % b_command)
        
        flags_sum = 0
        for f_name, f_value in self.FLAGS.iteritems():
            if b_flags & f_value:
                flags.append(f_name)
                flags_sum |= f_value
        if b_flags != flags_sum:
            raise ArduinoError('received an unknown flag bit: %x' % b_flags)

        return (command, flags)

    def _read_line(self, timeout=None):
        line = ''
        
        while True:
            char = self._read(timeout=timeout)
            if char == '\n':
                break
            elif char != '\r':
                line += char
        
        return line

    def _write(self, data):
        try:
            if self._debug:
                print 'T', ' '.join('{0:x}'.format(ord(c)) for c in data)
            written = self._arduino.write(data)
        except serial.SerialTimeoutException as err:
            raise ArduinoError('timeout while sending bytes: %s' % err)

        if written != len(data):
            raise ArduinoError('sent less bytes than expected to'
                               ' Arduino: %d/%d' % (len(written), len(data)))

    def _write_command(self, command, flags=None):
        if command not in self.CMD_OUT:
            raise ArduinoError('not sending invalid command: %s' % command)

        flags_value = 0
        if flags is not None:
            for flag in flags:
                if flag in self.FLAGS:
                    flags_value |= self.FLAGS[flag]
                else:
                    raise ArduinoError('unknown flag: %s' % flag)
        
        byte = ((self.CMD_OUT[command] & 0x3f) + (flags_value << 6)) & 0xff
        self._write(chr(byte))
