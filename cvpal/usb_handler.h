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

#ifndef CVPAL_USB_HANDLER_H_
#define CVPAL_USB_HANDLER_H_

#include "avrlib/base.h"

extern "C" {
  #include "usbdrv/usbdrv.h"
};

#include "cvpal/midi_handler.h"

namespace cvpal {

class UsbHandler {
 public:
  UsbHandler() { }
  ~UsbHandler() { }
  
  static void Init(MidiHandler* midi_handler);
  static void Poll();
  static void Parse(const uint8_t* data, uint8_t size) {
    midi_handler_->Parse(data, size);
  }

 private:
  static MidiHandler* midi_handler_;

  DISALLOW_COPY_AND_ASSIGN(UsbHandler);
};

}  // namespace

#endif  // CVPAL_USB_HANDLER_H_
