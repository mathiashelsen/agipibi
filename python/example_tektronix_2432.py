#!/usr/bin/env python
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

'''
Example for the Tektronix 2432 oscilloscope and other models.

Instrument must be set to "T/L" (Talk/Listen) mode, "EOI" message termination,
and have the GPIB address bellow (not 0 which by default collides with the
Arduino controller).

This code shows function arguments having default values, and that should
not be specified most of the time. Please refer to the classes source.
'''

from agipibi import Agipibi, AgipibiError

CIC_ADDRESS=0x00
SCOPE_ADDRESS=0x01

# Open serial port with the Arduino.
ctl = Agipibi(device='/dev/ttyUSB0', debug=False)

# Test communication and µC responsiveness.
if ctl.interface_ping():
	print "Arduino is alive :-)"
else:
	print "No reponse to ping, you should reset the board :-("

# Initialize bus lines and become Controller-In-Charge.
# All lines will be put to HiZ except for NRFD asserted because we gave no
# argument to the function, so we pause the Talker while setting up.
# IFC is pulsed to gain CIC status.
ctl.gpib_init(address=0x00, controller=True)

# Activate 'remote' mode of instruments (not required with this scope).
# It asserts REN untill disabled with False or gpib_init() is called again.
ctl.gpib_remote(True)

# Clear all instruments on the bus.
# Sends DCL when bus=True, reaching all devices. But it would use SDC
# if bus=True and Listeners are set. 
ctl.gpib_clear(bus=True)

# Two functions to set direction of the communication.
def cic_to_scope():
	# Unaddress everyone.
	ctl.gpib_untalk()
	ctl.gpib_unlisten()
	# Set ourself as the Talker.
	ctl.gpib_talker(CIC_ADDRESS)
	# Scope will listen.
	ctl.gpib_listener(SCOPE_ADDRESS)

def scope_to_cic():
	# Unaddress everyone.
	ctl.gpib_untalk()
	ctl.gpib_unlisten()
	# Set scope as the Talker.
	ctl.gpib_talker(SCOPE_ADDRESS)
	# We'll listen for data.
	ctl.gpib_listener(CIC_ADDRESS)


#
# Now let's test some commands
#

print '### get instrument identification'
cic_to_scope()
ctl.gpib_write('ID?')
scope_to_cic()
print ctl.gpib_read()
# Output:
#  ID TEK/2432,V81.1,"29-FEB-88  V1.52 /1.4",TVTRIG

print '### print a two lines message on the scope screen'
cic_to_scope()
ctl.gpib_write('MESS 1:"Hello world",2:"Arduino"')
# On screen:
#  ARDUINO
#  HELLO WORLD

print '### read channel 1 coupling and settings'
cic_to_scope()
ctl.gpib_write('CH1?')
scope_to_cic()
print ctl.gpib_read()
# Output:
#  CH1 VOLTS:1E-1,VARIABLE:0,POSITION:-1.00E-2,COUPLING:GND,FIFTY:OFF,INVERT:OFF

print '### set channel 1 to 0.5 V/div'
cic_to_scope()
ctl.gpib_write('CH1 VOLTS:0.5')

print '### turn CH1 off and CH2 on'
cic_to_scope()
ctl.gpib_write('VMO CH1:OFF,CH2:ON')

print '### acquire the ascii-encoded waveform of CH2 with preamble'
cic_to_scope()
ctl.gpib_write('DAT ENC:ASC,SOU:CH2')
ctl.gpib_write('WAV?')
scope_to_cic()
print ctl.gpib_read()
# Output:
#  WFMPRE WFID:"CH2 DC 200mV 500us NORMAL",NR.PT:1024,PT.OFF:512,PT.FMT:Y,XUNIT:
#  SEC,XINCR:1.000E-5,YMULT:8.000E-3,YOFF:-2.500E-1,YUNIT:V,BN.FMT:RI,ENCDG:ASCI
#  I;CURVE 50,50,49,49,49,50,50,49,50,49,49,49,0,0,0,0,-1,-1,-2,0,0,0,0,0,0,-2,0
#  ,0,0,-1,0,0,-1,-1,0,0,0,0,0,-1,0,-1,0,0,0,0,0,-1,0,0,0,1,-1,0,0,0,0,0,0,0,0,0
#  ,0,-1,0,0,0,0,0,-3,-1,-1,0,0,0,-1,0,-1,0,0,-1,0,0,0,-1,0,0,-1,0,0,-1,0,-1,-1,
#  0,0,1,0,0,-1,-2,0,0,0,0,1,-1,0,0,0,-1,-1,48,50,49,50,49,49,50,49,50,50,49,49,
#  49,49,48,49,50,48,49,49,50,49,50,49,50,49,49,49,49,50,48,50,49,49,49,49,50,49
#  ,49,49,50,50,48,49,49,50,48,50,49,49,49,49,49,50,49,49,50,49,49,50,50,49,50,4
#  8,49,49,49,50,50,49,49,49,49,50,49,50,50,49,49,49,49,50,50,49,49,50,49,49,50,
#  49,49,49,50,49,49,50,50,49,49,50,0,0,0,0,-1,0,-1,-2,0,0,-1,0,0,0,-2,0,0,0,-1,
#  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,0,0,0,-1,0,0,0,1,0,0,0,0,-1,0,-1,0,-1,0,0,
#  -2,0,0,-1,0,0,0,0,-1,0,0,0,0,-1,0,-1,0,-1,0,0,-1,0,0,-2,0,0,0,0,0,0,0,0,0,-1,
#  0,-1,0,-1,0,0,0,0,-1,-1,0,49,49,50,50,50,51,49,49,49,50,50,49,49,50,50,50,49,
#  50,48,50,49,49,49,50,51,49,50,49,48,48,50,48,49,49,49,49,49,48,49,50,49,50,50
#  ,49,50,50,49,50,49,49,50,49,50,50,49,51,49,49,50,49,49,49,49,49,48,48,49,49,4
#  7,49,49,50,49,49,49,49,49,50,49,49,49,49,50,49,49,50,48,50,49,49,49,50,49,50,
#  50,49,49,49,49,49,0,-2,0,0,0,0,-1,0,0,0,0,0,-1,0,-1,0,-1,0,-2,0,0,0,-1,0,0,-1
#  ,-1,0,0,0,0,0,0,-1,-1,0,0,0,0,0,-1,0,0,0,0,0,1,-1,0,0,-1,0,0,0,0,0,0,0,0,-1,0
#  ,0,0,0,0,0,-1,-1,1,0,0,0,-2,-1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-1,-1,0,
#  0,1,0,0,48,49,50,50,49,50,49,49,50,49,49,49,50,49,50,49,49,49,48,49,49,49,49,
#  50,50,49,49,50,49,49,50,49,49,49,49,49,49,49,50,49,49,49,49,49,50,50,50,49,49
#  ,48,49,49,49,49,49,50,49,50,50,50,49,49,49,49,49,49,49,49,49,49,49,49,50,49,5
#  0,49,50,49,49,49,49,50,50,49,50,50,49,49,51,49,49,50,49,49,49,49,49,49,49,50,
#  0,-2,0,-1,0,0,0,0,-1,0,0,0,0,-2,-1,0,-1,0,0,0,0,0,0,0,0,-1,0,0,-1,-2,-1,0,1,0
#  ,0,-1,0,0,0,0,0,0,0,0,0,0,0,0,-1,0,0,0,-2,0,-1,0,0,-1,0,1,0,0,0,0,0,-1,1,0,0,
#  0,0,0,0,0,0,0,0,-1,0,0,0,0,0,-1,-1,0,0,0,-1,0,-1,0,0,0,0,0,0,0,0,-1,49,49,50,
#  50,49,49,49,50,50,49,50,50,49,49,49,49,49,49,50,49,50,50,49,49,49,48,50,49,50
#  ,50,49,49,48,49,49,49,49,49,48,49,50,50,49,49,49,49,50,49,49,50,49,49,49,49,4
#  9,50,49,49,49,50,49,50,50,51,50,49,49,49,50,49,49,49,49,50,50,49,49,49,49,48,
#  50,50,50,49,48,49,50,49,51,49,49,48,50,49,49,49,49,49,49,49,0,0,-1,0,-2,-1,0,
#  0,-1,-1,-1,0,0,-1,-1,0,0,0,-1,0,0,0,0,0,-1,0,0,0,0,-1,-1,0,0,0,-1,0,-1,0,0,0,
#  0,0,0,0,0,0,-1,-1,-1,0,0,0,0,-1,0,0,0,0,0,0,-1,-1,-1,0,0,0,0,0,0,0,0,0,0,0,0,
#  0,-1,-1,0,-1,-2,0,0,0,-1,0,0,0,-1,0,0,-1,-1,-1,-1,0,0,0,0,-1,49,50,50,49,49,5
#  0,49,51,49,49,49,50,50,50,48,49,49,50,49,50,49,50,49,49,49,49,49,50,49,48,48,
#  49,49,50,49,49,49,50,49,50,49,48,50,49,49,50,50,49,50,49,49,50,49,51,50,49,50
#  ,49,50,49,49,50,50,49,49,49,49,49,49,49,50,50,48,50,50,50,49,49,49,49,48,49,4
#  9,50,49,49,50,49,49,49,48,49,49,50,49,50,49,49,50,49,0,-2,0,-1,-1,0,0,0,0,-2,
#  0,0
