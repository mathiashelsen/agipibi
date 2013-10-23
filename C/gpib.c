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

#include "gpib.h"

gpibio * gpib_init(int address, int controller, const char *device)
{
    gpibio *self = NULL;
    self = malloc ( sizeof( gpibio ) );
    self->address = address;
    self->controller = controller;
    self->arduino = NULL;
    self->arduino = arduino_init( device );
    if(self->arduino == NULL)
    {
	return NULL;
    }
    self->device = device;

    arduino_write_command( self->arduino, INIT, 0 );
    char buffer = address;
    arduino_write( self->arduino, 1, &buffer);
    if( controller )
    {
	arduino_write_command( self->arduino, CONTROLLER, 0 );
    }
    
    return self;
}

int gpib_close(gpibio *self)
{
    arduino_close(self->arduino);
    free(self);

    return 0;
}

int gpib_ping(gpibio *self)
{
    arduino_write_command( self->arduino, PING, 0 );
    int command = 0, flags = 0;    
    arduino_read_command( self->arduino, &command, &flags );
    return ( command == PONG );
}


int gpib_status(gpibio *self)
{
    arduino_write_command( self->arduino, STATUS, 0 );
    int command = 0, flags = 0;
    arduino_read_command( self->arduino, &command, &flags);
    if ( command == STRING )
    {
	char buffer[256];
	memset(buffer, 0, 256);
	arduino_read_line( self->arduino, 256, buffer );
    }

    return 0;
}

int gpib_read(gpibio *self, int size, char *buffer)
{
    int ret;
    int command = 0, flags = 0;
    ret = arduino_write_command( self->arduino, READ, 0 );
    ret = arduino_read_command( self->arduino, &command, &flags );

    if( command == STRING )
    {
	ret = arduino_read_line( self->arduino, size, buffer );
	return ret;
    }
    else if( command == CHUNK )
    {
	char s = 0, offset = 0;
	ret = arduino_read( self->arduino, 1, &s );
	ret = arduino_read( self->arduino, s, &(buffer[(int)offset]) );
	offset += s;
	
	while( flags != BOOLEAN && command == CHUNK )
	{
	    ret = arduino_write_command( self->arduino, READ, 0 );
	    ret = arduino_read_command( self->arduino, &command, &flags );
	    ret = arduino_read( self->arduino, 1, &s );
	    ret = arduino_read( self->arduino, s, &(buffer[(int)offset]) );
	    offset += s;
	}
	return (int)(offset + s);	
    }
    return ret;
}



int gpib_write(gpibio *self, int size, char *buffer)
{
    int offset, ret = 0;
    for( offset = 0; offset < size; offset += 255 )
    {
	int s = 255 > (size-offset) ? (size-offset) : 255;
	if( s == 255 )
	{
	    ret = arduino_write_command( self->arduino, WRITE, 0);
	}
	else
	{
	    ret = arduino_write_command( self->arduino, WRITE, BOOLEAN );
	}
	char tmp = (char) s;
	ret = arduino_write( self->arduino, 1, &tmp );

	ret = arduino_write( self->arduino, s, buffer+offset );
    }
    return ret;
}


int gpib_lock_read(gpibio *self, int state) { return arduino_write_command( self->arduino, LOCKREAD, state ? BOOLEAN : 0 ); }
int gpib_remote(gpibio *self, int state)    { return arduino_write_command( self->arduino, REMOTE, state ? BOOLEAN : 0 ); }
int gpib_untalk(gpibio *self)		    { return arduino_write_command( self->arduino, UNTALK, 0 ); }
int gpib_unlisten(gpibio *self)		    { return arduino_write_command( self->arduino, UNLISTEN, 0 ); }
int gpib_lockout(gpibio *self)		    { return arduino_write_command( self->arduino, LOCKOUT, 0 ); }
int gpib_clear(gpibio *self, int bus)	    { return arduino_write_command( self->arduino, CLEAR, bus ? BOOLEAN : 0 ); }
int gpib_unlock(gpibio *self)		    { return arduino_write_command( self->arduino, UNLOCK, 0 ); }
int gpib_trigger(gpibio *self)		    { return arduino_write_command( self->arduino, TRIGGER, 0); }

int gpib_talker(gpibio *self, int address)
{
    arduino_write_command( self->arduino, TALKER, 0 );
    char buffer = (char) address;
    return arduino_write( self->arduino, 1, &buffer);
}

int gpib_listener(gpibio *self, int address)
{
    arduino_write_command( self->arduino, LISTENER, 0 );
    char buffer = (char) address;
    return arduino_write( self->arduino, 1, &buffer);
}

port * arduino_init(const char *device)
{
    port *arduino = malloc(sizeof(port));

    arduino->fd = open(device, O_RDWR);
    if( arduino->fd == -1 )
    {
	return NULL;
    }

    return arduino;
}

int arduino_close(port *arduino)
{
    close(arduino->fd);
    free(arduino);

    return 0;
}

int arduino_read(port *arduino, int size, char *buffer)
{
    // readsize = number of bytes read
    int readsize = 0;
    while( readsize != size)
    {
	// Try to read as many bytes as possible
	int retval = read( arduino->fd, buffer+readsize, (size_t) ( size-readsize ) );
	if( retval < 0 )
	{
	    // In case something went wrong, it makes no sense to continue operation
	    printf("%s\n", strerror(errno) );
	    assert(retval >= 0);
	}
	// Increase the number of bytes read
	readsize += retval;
    }
    // Return the number of bytes read
    return readsize;
}

int arduino_read_command(port *arduino, int *command, int *flags)
{
    char byte = 0;
    int ret = 0;
    ret = arduino_read(arduino, 1, &byte);
    if ( ret > 0 )
    {
	*command = byte & 0x3f;
	*flags	= (byte & 0x60) >> 6;
    }

    return ret;
}

int arduino_read_line(port *arduino, int size, char *buffer)
{
    int i = 0;
    char tmp = 0, l = 0;
    do
    {
	l = arduino_read(arduino, 1, &tmp);
	if( tmp != '\n' && tmp != '\r' && l == 1)
	{
	    buffer[i] = tmp;
	    i++;
	}
    }while( tmp != '\n' && i < size );

    return i;
}

int arduino_write(port *arduino, int size, char *buffer)
{   
    // This will keep track of how many bytes have been written
    int writesize = 0;
    while(size != writesize)
    {
	// Try to write as many bytes as possible
	int retval = write(arduino->fd, buffer+writesize, (size_t)( size-writesize));
	if( retval < 0 )
	{
	    // If we couldn't write anything, crash
	    printf("%s\n", strerror(errno));
	    assert(retval >= 0);
	}
	// Increase the number of bytes read
	writesize += retval; 
    }
    // Return the number of bytes read from the Arduino
    return writesize;
}

int arduino_write_command(port *arduino, int command, int flags)
{
    char byte = 0;
    byte = ((command & 0x3f) |  (flags << 6)) & 0xff;
    return arduino_write(arduino, 1, &byte);
}
