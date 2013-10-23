/*
 *  This file is part of Agipibi
 * 
 *  Copyright 2013 Thibault Vincent, Mathias Helsen
 *  Agipibi is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Agipibi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Agipibi.  If not, see <http://www.gnu.org/licenses/>.
 *
 */ 

#ifndef _GPIB_H
#define _GPIB_H

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

typedef enum
{
    PING	= 0x00,
    STATUS	= 0x01,
    INIT	= 0x02,
    LOCKREAD	= 0x03,
    CONTROLLER	= 0x04,
    REMOTE	= 0x05,
    TALKER	= 0x06,
    LISTENER	= 0x07,
    UNLISTEN	= 0x08,
    UNTALK	= 0x09,
    LOCKOUT	= 0x0a,
    CLEAR	= 0x0b,
    UNLOCK	= 0x0c,
    TRIGGER	= 0x0d,
    READ	= 0x0e,
    WRITE	= 0x0f,
    CMD		= 0x10,
    ENGAGE_REQ	= 0x11
} CMD_OUT;

typedef enum
{
    PONG	= 0x00,
    CHUNK	= 0x01,
    STRING	= 0x02,
    REQUEST	= 0x03
} CMD_IN;

typedef enum 
{
    BOOLEAN	= 0x01
} FLAGS;

typedef struct
{
    const char *device;
    int fd;
} port;

typedef struct
{
    const char *device;
    int address;
    int controller;
    port *arduino;
} gpibio;

/* Low level Arduino stuff */

// Initializes a file descriptor to the Arduino running the HPIB interface
port * arduino_init(const char *device);
// Closes the file descriptor
int arduino_close(port *arduino);
// Reads exactly "size" bytes from the Arduino into the buffer
// Returns the number of bytes read, asserts if it fails
int arduino_read(port *arduino, int size, char *buffer);
// Reads a command and possible flags from the Arduino
int arduino_read_command(port *arduino, int *command, int *flags);
// Reads from the Arduino until a \r or \n is found
int arduino_read_line(port *arduino, int size, char *buffer);
// Writes "size" bytes from "buffer" to the Arduino
int arduino_write(port *arduino, int size, char *buffer);
// Writes a command and possible flag to the Arduino
int arduino_write_command(port *arduino, int command, int flags);

/* Higher level GPIB/HPIB interface */
/* 
 * Always call this function first, initializes the Arduino and GPIB interface
 * Set the address of the Arduino and if it's a controller. Returns a pointer
 * to the allocated gpib struct, NULL in case of failure
 */
gpibio * gpib_init(int address, int controller, const char *device);
/*
 * Closes the GPIB interface
 */ 
int gpib_close(gpibio *self);

/* Basic functionallity */
// Pings the Arduino, returns true if success, false (0) if failed
int gpib_ping(gpibio *self);
// Prints the status of the GPIB interface
int gpib_status(gpibio *self);
// Reads from the GPIB, maximum "size" bytes into "buffer"
int gpib_read(gpibio *self, int size, char *buffer);
// Writes "size" bytes from buffer to the GPIB
int gpib_write(gpibio *self, int size, char *buffer);

/* Advanced func, wrappers */
int gpib_lock_read(gpibio *self, int state);
int gpib_remote(gpibio *self, int state);
int gpib_untalk(gpibio *self);
int gpib_unlisten(gpibio *self);
int gpib_lockout(gpibio *self);
int gpib_clear(gpibio *self, int bus);
int gpib_unlock(gpibio *self);	
int gpib_trigger(gpibio *self);
/* More than simple wrappers */
int gpib_talker(gpibio *self, int address);
int gpib_listener(gpibio *self, int address);

#endif
