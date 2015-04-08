// Copyright 2013 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "cvpal/usb_handler.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <string.h>

const uint8_t kPollingInterval = 1;

extern "C" {

const static PROGMEM uint8_t device_descriptor[] = {
  0x12,  // Size of this descriptor, in bytes.
  USBDESCR_DEVICE,  // DEVICE descriptor
  0x10, 0x01,  // 1.10 - current revision of USB specification
  0,  // Device defined at interface level
  0,  // Unused.
  0,  // Unused.
  8,  // Max packet size = 8 bytes.
  USB_CFG_VENDOR_ID,  // 2 bytes 
  USB_CFG_DEVICE_ID,  // 2 bytes 
  USB_CFG_DEVICE_VERSION, // 2 bytes 
  1,  // Manufacturer string index 
  2,  // Product string index 
  3,  // Serial number string index
  1,  // One configuration
};

// http://www.usb.org/developers/devclass_docs/midi10.pdf
const static PROGMEM uint8_t configuration_descriptor[] = {
  // USB configuration descriptor 
  9,  // Size of this descriptor, in bytes.
  USBDESCR_CONFIG,  // CONFIGURATION descriptor
  101, 0,  // Length of the total configuration block, including this descriptor, in bytes.
  2,  // Two interfaces.
  1,  // ID of this configuration.
  3,  // Configuration name string index 
#if USB_CFG_IS_SELF_POWERED
  USBATTR_SELFPOWER,
#else
  USBATTR_BUSPOWER,
#endif
  USB_CFG_MAX_BUS_POWER / 2,  // Max USB current in 2mA units 

  // AC interface descriptor follows inline: 
  9,  // Size of this descriptor, in bytes
  USBDESCR_INTERFACE, // INTERFACE descriptor
  0,  // Index of this interface 
  0,  // Index of this setting
  0,  // 0 endpoints
  1,  // AUDIO 
  1,  // AUDIO_CONTROL 
  0,  // Unused
  0,  // String ID

  // AC Class-Specific descriptor 
  9,  // Size of this descriptor, in bytes
  36,  // CS_INTERFACE
  1,  // HEADER subtype
  0x0, 0x01,  // Revision of class specification - 1.0 
  9, 0,  // Total size of class specific descriptors
  1,  // Number of streaming interfaces
  1,  // MIDIStreaming interface 1 belongs to this AudioControl interface

  // Standard MS Interface Descriptor
  9,  // ize of this descriptor, in bytes
  USBDESCR_INTERFACE, // INTERFACE descriptor
  1,  // Index of this interface 
  0,  // Index of this alternate setting
  2,  // 2 endpoints
  1,  // AUDIO
  3,  // MIDISTREAMING
  0,  // Unused
  0,  // String ID

  // Class-Specific MS interface descriptor 
  7,  // Size of this descriptor, in bytes
  36,  // CS_INTERFACE
  1,  // MS_HEADER subtype
  0x0, 0x01,  // Revision of this class specification
  65, 0,  // Total size of class-specific descriptors

  // MIDI IN Jack Descriptor
  6,  // Size of this descriptor, in bytes 
  36,  // CS_INTERFACE descriptor
  2,  // MIDI_IN_JACK subtype 
  1,  // EMBEDDED
  1,  // ID of this jack 
  0,  // String ID 

  6,  // Size of this descriptor, in bytes 
  36,  // CS_INTERFACE descriptor
  2,  // MIDI_IN_JACK subtype 
  2,  // EXTERNAL
  2,  // ID of this jack 
  0,  // String ID 

  // MIDI OUT Jack Descriptor
  9,  // length of descriptor in bytes 
  36,  // CS_INTERFACE descriptor
  3,  // MIDI_OUT_JACK subtype
  1,  // EMBEDDED
  3,  // ID of this jack 
  1,  // Number of input pins of this jack
  2,  // Source ID
  1,  // Source Pin
  0,  // String ID 

  9,  // Size of this descriptor, in bytes
  36,  // CS_INTERFACE descriptor
  3,  // MIDI_OUT_JACK subtype
  2,  // EXTERNAL
  4,  // ID of this jack
  1,  // Number of input pins of this jack
  1,  // Source ID
  1,  // Source Pin
  0,  // String ID 

  // Interrupt OUT Endpoint Descriptor
  9,  // Size of this descriptor, in bytes
  USBDESCR_ENDPOINT,  // ENDPOINT descriptor
  0x1,  // OUT endpoint 1
  3,  // Interrupt
  8, 0,  // Packet size
  kPollingInterval,  // Polling interval in ms
  0,  // Unused
  0,  // Unused

  // Class-specific OUT Endpoint Descriptor
  5,  // Size of this descriptor, in bytes
  37,  // CS_ENDPOINT descriptor
  1,  // MS_GENERAL subtype
  1,  // Number of embedded MIDI IN jacks
  1,  // ID of the embedded MIDI IN jacks

  // Interrupt IN Endpoint Descriptor
  9,  // Size of this descriptor, in bytes
  USBDESCR_ENDPOINT,  // ENDPOINT descriptor
  0x81,  // IN endpoint 1
  3,  // Interrupt
  8, 0,  // Packet size
  kPollingInterval,  // Polling interval in ms
  0,  // Unused
  0,  // Unused

  // Class-specific IN Endpoint Descriptor
  5,  // Size of this descriptor, in bytes
  37,  // CS_ENDPOINT descriptor 
  1,  // MS_GENERAL subtype
  1,  // Number of embedded MIDI OUT jacks
  3,  // ID of the embedded MIDI OUT jacks
};

uchar usbFunctionDescriptor(usbRequest_t* rq) {
  if (rq->wValue.bytes[1] == USBDESCR_DEVICE) {
    usbMsgPtr = (unsigned short)(device_descriptor);
    return sizeof(device_descriptor);
  } else {
    usbMsgPtr = (unsigned short)(configuration_descriptor);
    return sizeof(configuration_descriptor);
  }
}

uchar usbFunctionSetup(uchar data[8]) {
  usbRequest_t* rq = (usbRequest_t*)((void*)data);
  return 0xff;
}

uchar usbFunctionRead(uchar* data, uchar len) {
  data[0] = 0;
  data[1] = 0;
  data[2] = 0;
  data[3] = 0;
  data[4] = 0;
  data[5] = 0;
  data[6] = 0;

  return 7;
}

uchar usbFunctionWrite(uchar* data, uchar len) {
  return 1;
}

void usbFunctionWriteOut(uchar* data, uchar len) {
  cvpal::UsbHandler::Parse(data, len);
}

}  // extern "C"

namespace cvpal {

/* static */
MidiHandler* UsbHandler::midi_handler_;

/* static */
void UsbHandler::Init(MidiHandler* midi_handler) {
  midi_handler_ = midi_handler;
  usbDeviceDisconnect();
  _delay_ms(200);
  usbDeviceConnect();
  usbInit();
  sei();
}

/* static */
void UsbHandler::Poll() {
  usbPoll();
  if (usbInterruptIsReady()) {
    // We don't emit anything!
  }
}

}  // namespace cvpal
