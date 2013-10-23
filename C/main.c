/*
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

/*
 * This is an example program to use a HP 3478A multimeter to read out DC current.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gpib.h"

int main(int argc, char **argv)
{
    // On my system the Arduino is mapped to /dev/ttyACM0, change if needed
    gpibio *gpib = gpib_init( 0x00, 0x01, "/dev/ttyACM0");
    if( gpib == NULL )
    {
	printf("Error opening port\n");
	return -1;
    }
    sleep(1);

    int ret = 0;
    printf("Ping? ");
    ret = gpib_ping(gpib);
    if( ret == 0 )
    {
	printf("Could not establish connection, reset board?\n");
	assert(ret != 0);
    }
    else
    {
	printf("Pong!\n");
    }

    printf("#REMOTE\n");
    gpib_remote( gpib, 1 );
    printf("#CLEAR\n");
    gpib_clear( gpib, 1 );
    printf("#UNTALK\n");
    gpib_untalk( gpib );
    printf("#UNLISTEN\n");
    gpib_unlisten( gpib );
    printf("#TALKER\n");
    gpib_talker( gpib, 0x00 );
    printf("#LISTENER\n");
    gpib_listener( gpib, 0x01 );
	sleep(1);

    char buffer[256];
    int i = 0;
    memset(buffer, 0, 256);
    sprintf(buffer, "F5");
    ret = gpib_write( gpib, 2, buffer);

    gpib_untalk( gpib );
    gpib_unlisten( gpib );
    gpib_talker( gpib, 0x01 );
    gpib_listener( gpib, 0x00 );

    for(i = 0 ; i < 10; i++ )
    {
	ret = 0;
	memset(buffer, 0, 256);
	ret = gpib_read( gpib, 256, buffer );
	printf("Read %d bytes\n", ret);
	printf("%d,%d : %s\n", i, ret, buffer);
    }

    gpib_close( gpib );

    return 0;
}
