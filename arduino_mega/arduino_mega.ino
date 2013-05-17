/*
 *  Copyright 2013 Thibault VINCENT <tibal@reloaded.fr>
 *
 *  This file is part of Agipibi.
 *
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
 */

/* configurable pin mapping to Arduino Mega */
#define DIO1  22  /* GPIB 1  : I/O data bit 1 */
#define DIO2  23  /* GPIB 2  : I/O data bit 2 */
#define DIO3  24  /* GPIB 3  : I/O data bit 3 */
#define DIO4  25  /* GPIB 4  : I/O data bit 4 */
#define DIO5  26  /* GPIB 13 : I/O data bit 5 */
#define DIO6  27  /* GPIB 14 : I/O data bit 6 */
#define DIO7  28  /* GPIB 15 : I/O data bit 7 */
#define DIO8  29  /* GPIB 16 : I/O data bit 8 */
#define EOI   30  /* GPIB 5  : End Or Identify */
#define DAV   31  /* GPIB 6  : Data Valid */
#define NRFD  32  /* GPIB 7  : Not Ready For Data */
#define NDAC  33  /* GPIB 8  : Not Data Accepted */
#define IFC   34  /* GPIB 9  : Interface Clear */
#define ATN   35  /* GPIB 11 : Attention */
#define REN   36  /* GPIB 17 : Remote Enable */
#define SRQ    2  /* GPIB 10 : Service Request
                     This Arduino pin must have interrupt */
#define SRQ_i  0  /* Interrupt number for the SRQ pin (see Arduino doc) */
#define LED   13  /* Busy signal (board is waiting), 13 is the on-board LED */

/* serial commands */
/* ~ from computer ~ */
#define IN_PING       0x00
#define IN_STATUS     0x01
#define IN_INIT       0x02
#define IN_LOCKREAD   0x03
#define IN_CONTROLLER 0x04
#define IN_REMOTE     0x05
#define IN_TALKER     0x06
#define IN_LISTENER   0x07
#define IN_UNLISTEN   0x08
#define IN_UNTALK     0x09
#define IN_LOCKOUT    0x0a
#define IN_CLEAR      0x0b
#define IN_UNLOCK     0x0c
#define IN_TRIGGER    0x0d
#define IN_READ       0x0e
#define IN_WRITE      0x0f
#define IN_CMD        0x10
#define IN_ENGAGE_REQ 0x11
/* ~ to computer ~ */
#define OUT_PONG      0x00
#define OUT_CHUNK     0x01
#define OUT_STRING    0x02
#define OUT_REQUEST   0x03
#define OUT_TIMEOUT   0x04
/* ~ flags ~ */
#define FLG_BOOLEAN   0x01

/* GPIB commands */
/* ~ bus control ~ */
#define G_TAD 0x40
#define G_LAD 0x20
#define G_UNL 0x3f
#define G_UNT 0x5f
/* ~ unaddressed ~ */
#define G_LLO 0x11
#define G_DCL 0x14
#define G_PPU 0x15
#define G_SPE 0x18
#define G_SPD 0x19
/* ~ addressed ~ */
#define G_GTL 0x01
#define G_SDC 0x04
#define G_PPC 0x05
#define G_GET 0x08
#define G_TCT 0x09

/* serial command/flag manipulation 
   MSB f f c c c c c c LSB
       ___ ___________
        |   |
        |   \_ command
        |
        \_____ flags
  */
#define add_flag(byt, flag) (byte)(byt | (flag << 6))
#define get_cmd(byt)        (byte)(byt & 0x3f)
#define get_flags(byt)      (byte)((byt & 0x60) >> 6)
#define has_flag(byt, flag) ((byt & (flag << 6)) > 0)

/* open collector emulation with no external resistor */
#define set_bit(pin) \
                      pinMode(pin, OUTPUT); \
                      digitalWrite(pin, LOW);
#define clear_bit(pin) \
                      pinMode(pin, INPUT_PULLUP);
#define wait_set(pin) \
                      pinMode(pin, INPUT_PULLUP); \
                      while (digitalRead(pin) == HIGH) {;}
#define wait_clear(pin) \
                      pinMode(pin, INPUT_PULLUP); \
                      while (digitalRead(pin) == LOW) {;}

/* talker/listener state machine */
/* WARNING: these are not configuration variables, you must use
   !!!!!!!  serial commands to adjust parameters */
byte self_address = 0x00;
boolean self_is_listener = false;
byte talker_address = 0x00;
boolean talker_is_set = false;
byte listeners[31];
byte listener_count = 0;
boolean int_srq_engaged = false;
boolean int_dat_engaged = false;

/* misc */
#define BUFFER_SIZE 255

/* Arduino initialization */
void setup() {
  Serial.begin(115200);
  gpib_bus_init(false);
  pinMode(LED, OUTPUT);  /* internal LED */
}

/* Arduino main loop */
void loop() {
  unsigned int n;
  boolean boo;
  byte cmd, arg, buf[BUFFER_SIZE];
  
  while (true) {
    int_pause(false);
    if (Serial.available() > 0) {
      int_pause(true);
      /* read a command */
      cmd = Serial.read();
      /* process command */
      switch(get_cmd(cmd)) {
                
        /* to detect board lockup, just reply */
        case IN_PING:
          Serial.write((byte)OUT_PONG);
          break;
        
        /* send bus state (handshake, control, and data lines)
           this may break the state machine (send INIT and reset equipment) */
        case IN_STATUS:  
          gpib_status();
          break;
        
        /* re-init bus lines */
        case IN_INIT:
          /* read my new address */
          while (Serial.available() < 1) {;}
          self_address = Serial.read() & 0x1f;
          /* if flag is set, lock read handshake */
          gpib_bus_init(has_flag(cmd, FLG_BOOLEAN));
          break;
         
        /* lock or release read ack lines, preventing Talker to write */
        case IN_LOCKREAD:
          gpib_bus_lockread(has_flag(cmd, FLG_BOOLEAN));
          break;
        
        /* reclaim controller-in-charge autority */
        case IN_CONTROLLER:
          gpib_bus_controller();
          break;

        /* invoke bus-wide remote mode (may lock instrument front panels) */ 
        case IN_REMOTE:
          /* flag controls activation */
          gpib_bus_remote(has_flag(cmd, FLG_BOOLEAN));
          break;
  
        /* make a device become Talker */
        case IN_TALKER:
          /* read target address */
          while (Serial.available() < 1) {;}
          gpib_cmd_bus_talker(Serial.read() & 0x1f);
          break;
  
        /* make a device become Listener */
        case IN_LISTENER:
          /* read target address */
          while (Serial.available() < 1) { ; }
          gpib_cmd_bus_listener(Serial.read() & 0x1f);
          break;
          
        /* inactivate Talkers */
        case IN_UNTALK:
          gpib_cmd_bus_untalk();
          break;
        
        /* inactivate Listeners */
        case IN_UNLISTEN:
          gpib_cmd_bus_unlisten();
          break;

        /* disable front panel controls of instruments */
        case IN_LOCKOUT:
          gpib_cmd_unaddr_lockout();
          break;
        
        /* make one or many instruments go back to default condition */
        case IN_CLEAR:
          /* flag triggers bus-wide DCL */
          if (has_flag(cmd, FLG_BOOLEAN)) {
            gpib_cmd_unaddr_clear();
          } else {
            gpib_cmd_addr_clear();
          }
          break;
        
        /* enable front panel of Listener instruments */
        case IN_UNLOCK:
          gpib_cmd_unaddr_lockout();
          break;
          
        /* trigger simultaneous action on Listener instruments */
        case IN_TRIGGER:
          gpib_cmd_addr_trigger();
          break;
          
        /* get bytes from bus */
        case IN_READ:
          /* fill our buffer with GPIB data */
          n = 0;
          do {
            boo = gpib_read(&(buf[n++]));
          } while(boo && n < BUFFER_SIZE);
          /* send flag if GPIB got EOI */
          if (!boo) {
            Serial.write(add_flag(OUT_CHUNK, FLG_BOOLEAN));
          } else {
            Serial.write(OUT_CHUNK);
          }
          /* tell buffer size */
          Serial.write((byte)n);
          /* send buffer to computer */
          Serial.write(buf, n);
          break;
        
        /* send data bytes */
        case IN_WRITE:
          /* get chunk size */
          while (Serial.available() < 1) {;}
          n = Serial.read();
          /* send everything on the bus */
          for (int i = 0; i < n; i++) {
            while (Serial.available() < 1) {;}
            /* EOI is set if we're at the end of chunk, and flag was set
               or we would expect another chunk after this one */
            gpib_write(Serial.read(), (i == n-1) && has_flag(cmd, FLG_BOOLEAN));
          }
          break;
        
        /* send a GPIB command */
        case IN_CMD:
          while (Serial.available() < 1) {;}
          gpib_cmd(Serial.read());
          break;

        /* re-engage SRQ interruption reporting */
        case IN_ENGAGE_REQ:
          int_set_srq(has_flag(cmd, FLG_BOOLEAN));
          break;
      }
    }
  }
}

/* add an address to listeners if not present
   return: true if we added it */
boolean _listeners_add(byte address) {
  boolean found = false;
  for (int i = 0; i < listener_count; i++) {
    if (listeners[i] == address) {
      found = true;
      break;
    }
  }
  if (!found) {
    listeners[listener_count++] = address;
  }
  return !found;
}

/* remove an address from listeners list */
boolean _listeners_remove(byte address) {
  boolean found = false;
  for (int i = 1; i < listener_count; i++) {
    if (listeners[i - 1] == address) {
      if (!found) {
        found = true;
      }
      listeners[i - 1] = listeners[i];
      listeners[i] = address;
    }
  }
  if (found) {
    listener_count--;
  }  
  return found;
}

/* toggle on-board busy LED */
void _led_busy(boolean state) {
    digitalWrite(LED, state);
}

/* pause/resume interrupts */
void int_pause(boolean state) {
  /* pause */
  if (state) {
    /* SRQ */
    if (int_srq_engaged) {
      detachInterrupt(SRQ_i);
    }
  }
  /* resume */
  else {
    /* SRQ */
    if (int_srq_engaged) {
      int_set_srq(true);
    }
  }
}

/* engage/disengage SRQ interruption */
void int_set_srq(boolean state) {
  int_srq_engaged = state;
  pinMode(SRQ, INPUT_PULLUP);
  if (state) {
    attachInterrupt(SRQ_i, _int_cb_srq, LOW);
  } else {
    detachInterrupt(SRQ_i);
  }
}

/* callback to handle SRQ interruption */
void _int_cb_srq() {
  /* disable the interruption */
  int_set_srq(false);
  /* signal the event */
  Serial.write(OUT_REQUEST);
}

/* init GPIB bus lines */
void gpib_bus_init(boolean lock) {
  set_bit(NRFD);
  pio_reset();
  clear_bit(EOI);
  clear_bit(DAV);
  clear_bit(IFC);
  clear_bit(SRQ);
  clear_bit(ATN);
  clear_bit(REN);
  /* prevent bus writing at init time */
  gpib_bus_lockread(true);
}

/* lock handshake lines to prevent writting, this is mandatory when
   we become Listener. In case others are listening too, they could
   trigger race conditions by clearing NDAC quickly */
void gpib_bus_lockread(boolean lock) {
  if (lock) {
    set_bit(NRFD);
  } else {
    clear_bit(NRFD);
  }
  clear_bit(NDAC); 
}

/* become the system Controller-In-Charge */
void gpib_bus_controller() {
  set_bit(IFC);
  delayMicroseconds(100);
  clear_bit(IFC);
}

/* invoke/release remote mode of instruments */
void gpib_bus_remote(boolean assert) {
  if (assert) {
    set_bit(REN);
  } else {
    clear_bit(REN);
  }
}

/* TAD(Talker Address): set instrument to become Talker */
void gpib_cmd_bus_talker(byte address) {
  /* there can be only one Talker */
  if (talker_is_set) {
    gpib_cmd_bus_untalk();
  }
  /* if we switch from Listener to Talker */
  if (self_is_listener && address == self_address) {
    /* do not lock reads anymore */
    gpib_bus_lockread(false);
    self_is_listener = false;
  }
  /* if found, get this address off Listener list (us or others) */
  _listeners_remove(address);
  /* */
  gpib_cmd(address | G_TAD);
  talker_is_set = true;
  talker_address = address;
}

/* LAD(Listener Address): make (one more) instrument become Listener */
void gpib_cmd_bus_listener(byte address) {
  /* it's a switch from Talker to Listener */
  if (talker_is_set && talker_address == address) {
    gpib_cmd_bus_untalk();
  }
  /* add to Listeners list */
  if (_listeners_add(address)) {
    if (address == self_address) {
      self_is_listener = true;
    }
    gpib_cmd(address | G_LAD);
  }
}

/* UNT(Untalk): inactivate talkers */
void gpib_cmd_bus_untalk() {
  talker_is_set = false;
  gpib_cmd(G_UNT);
}

/* UNL(Unlisten): inactivate listeners */
void gpib_cmd_bus_unlisten() {
  listener_count = 0;
  self_is_listener = false;
  gpib_cmd(G_UNL);
}

/* [unaddressed] LLO(Local Lock Out): disable front panel controls of instruments */
void gpib_cmd_unaddr_lockout() {
  gpib_cmd(G_LLO);
}

/* [unaddressed] DCL(Device Clear): reset all instruments to "power on" conditions */
void gpib_cmd_unaddr_clear() {
  talker_is_set = false;
  listener_count = 0;
  self_is_listener = false;
  gpib_cmd(G_DCL);
}

/* [addressed] GTL(Go To Local): enable front panel controls of instrument */
void gpib_cmd_addr_local() {
  gpib_cmd(G_GTL);
}

/* [addressed] SDC(Selective DCL): initiate instrument to power-on conditions */
void gpib_cmd_addr_clear() {
  /* when clearing ourself */
  if (talker_is_set && talker_address == self_address) {
    gpib_cmd_bus_untalk();
  } else if (self_is_listener && _listeners_remove(self_address)) {
    self_is_listener = false;
  }
  gpib_cmd(G_SDC);
}

/* [addressed] GET(Group Execute Trigger): trigger simultaneous action on instruments */
void gpib_cmd_addr_trigger() {
  gpib_cmd(G_GET); 
}

/* send a command (write with ATN) */
void gpib_cmd(byte command) {
  _gpib_write(command, true, false);
}

/* send a data byte (write with EOI if is_end is true) */
void gpib_write(byte data, boolean is_end) {
  _gpib_write(data, false, is_end); 
}

/* write a byte to bus */
void _gpib_write(byte data, boolean is_command, boolean is_end) {
  /* get attention for commands */
  if (is_command) {
    set_bit(ATN);
  } else {
    clear_bit(ATN);
  }
  /* release some line */
  clear_bit(EOI);
  clear_bit(DAV);
  clear_bit(NRFD);
  /* FIXME timeout */
  _led_busy(true);
  /* wait for consistency, one instrument to initiate reading sequence */
  wait_set(NDAC);
  /* wait for *all* listening instruments readiness */
  wait_clear(NRFD);
  /* write byte */
  pio_write(data);
  /* signal transmission of last byte */
  if (is_end && !is_command) {
    set_bit(EOI);
  }
  /* validate bus data */
  set_bit(DAV);
  /* wait for *all* listeners to complete read */
  wait_clear(NDAC);
  /* FIXME timeout */
  _led_busy(false);
  /* bus data not valid anymore */
  clear_bit(DAV);
  /* reset data lines */
  pio_reset();
  /* clear attention line */
  if (is_command) {
    clear_bit(ATN);
  }
  /* end of transmission not signaled anymore */
  if (is_end && !is_command) {
    clear_bit(EOI);
  }
}

/* read a byte from GPIB
 * return: true if EOI is set
 */
boolean gpib_read(byte* b) {
  boolean eoi;
  /* handshake : we're ready to receive data */
  set_bit(NDAC);
  clear_bit(NRFD);
  /* FIXME timeout */
  _led_busy(true);
  /* wait for valid data */
  wait_set(DAV);
  /* handshake : not ready for a new read */
  set_bit(NRFD);
  /* read data */
  *b = pio_read();
  /* was it the last byte of transmission */
  eoi = digitalRead(EOI);
  /* handshake : read accepted */
  clear_bit(NDAC);
  /* wait for data release */
  wait_clear(DAV);
  /* FIXME timeout */
  _led_busy(false);
  /* handshake : get ready for next byte */
  set_bit(NDAC);
  return eoi;
}

/* reset GPIB data lines to HiZ */
void pio_reset() {
  pinMode(DIO1, INPUT_PULLUP);
  pinMode(DIO2, INPUT_PULLUP);
  pinMode(DIO3, INPUT_PULLUP);
  pinMode(DIO4, INPUT_PULLUP);
  pinMode(DIO5, INPUT_PULLUP);
  pinMode(DIO6, INPUT_PULLUP);
  pinMode(DIO7, INPUT_PULLUP);
  pinMode(DIO8, INPUT_PULLUP);
}

/* get byte from GPIB data lines */
byte pio_read() {
  byte b = 0;
  pio_reset();
  bitWrite(b, 0, !digitalRead(DIO1));
  bitWrite(b, 1, !digitalRead(DIO2));
  bitWrite(b, 2, !digitalRead(DIO3));
  bitWrite(b, 3, !digitalRead(DIO4));
  bitWrite(b, 4, !digitalRead(DIO5));
  bitWrite(b, 5, !digitalRead(DIO6));
  bitWrite(b, 6, !digitalRead(DIO7));
  bitWrite(b, 7, !digitalRead(DIO8));
  return b;
}

/* set byte on GPIB data lines */
void pio_write(byte b) {
  pinMode(DIO1, OUTPUT); digitalWrite(DIO1, bitRead(~b, 0));
  pinMode(DIO2, OUTPUT); digitalWrite(DIO2, bitRead(~b, 1));
  pinMode(DIO3, OUTPUT); digitalWrite(DIO3, bitRead(~b, 2));
  pinMode(DIO4, OUTPUT); digitalWrite(DIO4, bitRead(~b, 3));
  pinMode(DIO5, OUTPUT); digitalWrite(DIO5, bitRead(~b, 4));
  pinMode(DIO6, OUTPUT); digitalWrite(DIO6, bitRead(~b, 5));
  pinMode(DIO7, OUTPUT); digitalWrite(DIO7, bitRead(~b, 6));
  pinMode(DIO8, OUTPUT); digitalWrite(DIO8, bitRead(~b, 7));
}

/* send GPIB bus state back to computer */
void gpib_status() {
  Serial.write(OUT_STRING);
  pinMode(EOI, INPUT_PULLUP); Serial.print("E"); Serial.print(digitalRead(EOI));
  pinMode(DAV, INPUT_PULLUP); Serial.print("D"); Serial.print(digitalRead(DAV));
  pinMode(NRFD,INPUT_PULLUP); Serial.print("N"); Serial.print(digitalRead(NRFD));
  pinMode(NDAC,INPUT_PULLUP); Serial.print("n"); Serial.print(digitalRead(NDAC));
  pinMode(IFC, INPUT_PULLUP); Serial.print("I"); Serial.print(digitalRead(IFC));
  pinMode(SRQ, INPUT_PULLUP); Serial.print("S"); Serial.print(digitalRead(SRQ));
  pinMode(ATN, INPUT_PULLUP); Serial.print("A"); Serial.print(digitalRead(ATN));
  pinMode(REN, INPUT_PULLUP); Serial.print("R"); Serial.print(digitalRead(REN));
  pio_reset();
  Serial.print(digitalRead(DIO8));
  Serial.print(digitalRead(DIO7));
  Serial.print(digitalRead(DIO6));
  Serial.print(digitalRead(DIO5));
  Serial.print(digitalRead(DIO4));
  Serial.print(digitalRead(DIO3));
  Serial.print(digitalRead(DIO2));
  Serial.println(digitalRead(DIO1));
}
