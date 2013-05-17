Agipibi : An Arduino to GPIB Interface
======================================
!(documentation/arduino_and_cable.jpg?raw=true)

Description
-----------

Agipibi is a cheap GPIB interface based on the Arduino Mega board or higher
models. It connects scientific instruments using IEEE 488 buses to a computer,
or the micro-controller itself if you integrate it in your own code.
This project goal is to provide the best open source implementation of a GPIB
controller on prototyping platforms. 

Features
--------

  - system controller role
  - act as talker or listener in full address space
  - multiple concurrent listeners
  - switch instruments to remote mode
  - (un)lock front panels
  - fast reading (Arduino is buffering chunks of data)
  - group trigger
  - timeout handling when reading bus

Work-in-progress:

  - SRQ request interruption and serial polling
  - more examples for Tektronix scopes
  - bridge to LabVIEW with a Python script (TCP server)

Usage
-----

This interface was designed for Arduino Mega 1280, to use other boards you'll
have to edit the pin mapping at the top of arduino_mega.ino sketch. Be careful
about the SRQ line that should have an interrupt capable output.

  1. Connect your GPIB bus/instrument to the board with no additional
     component. You'll find examples in the documentation directory.

  2. Build and upload the 'arduino_mega.ino' sketch in Arduino IDE.

  3. Use the Python module 'agipibi.py' to begin tests. Examples are provided.
     It's best to start in a scenario that doesn't require a controller or
     bi-directional communication. At first I put my scope in Talker mode and
     had it transmit a waveform using menus only. The following code would be
     enough in this case.

```python
from agipibi import Agipibi
dev = Agipibi()
if dev.interface_ping():
    dev.gpib_init(controller=False)
    waveform = dev.gpib_read() # press the 'Transmit' scope button
    print 'Received %d bytes' % len(waveform)
else:
    print 'Arduino is not responding'
```

Authors
-------

  - Thibault VINCENT <tibal@reloaded.fr>

Thanks
------

Agipibi was inspired by similar projects:

  - A japanese blogger article
    http://bananawani-mc.blogspot.fr/2010/09/arduinogpib.html

  - GPL code from Dennis Dingeldein
    http://www.spurtikus.de/basteln/gpib_en.html
  
  - USB to GPIB Controller by LPVO
    http://lpvo.fe.uni-lj.si/en/research/electronics/
  
  - GPIB USB Adapter by Steven Casagrande
    https://github.com/Galvant/gpibusb-firmware
