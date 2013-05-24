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

#ifndef CVPAL_DAC_H_
#define CVPAL_DAC_H_

#include "avrlib/base.h"

#include <avr/io.h>

namespace cvpal {

class Dac {
 public:
  Dac() { }
  ~Dac() { }
  
  static inline void Init() {
    // Initialize SPI
    DDRA |= _BV(PA7);  // SS
    PORTA |= _BV(PA7);
    DDRA |= _BV(PA5);  // MOSI
    DDRA |= _BV(PA4);  // SCK
  }
  
  static inline void Write(uint16_t a, uint16_t b) {
    PORTA &= ~_BV(PA7);
    SpiSend(0x1000 | a);
    PORTA |= _BV(PA7);

    PORTA &= ~_BV(PA7);
    SpiSend(0x9000 | b);
    PORTA |= _BV(PA7);
  }
  
 private:
  enum UsiFlags {
    TICK = _BV(USIWM0) | _BV(USITC),
    TOCK = _BV(USIWM0) | _BV(USITC) | _BV(USICLK)
  };

  static inline void SpiSend(uint16_t word) {
    USIDR = word >> 8;

    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;

    USIDR = word & 0xff;

    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
    USICR = TICK;
    USICR = TOCK;
  }
  
  DISALLOW_COPY_AND_ASSIGN(Dac);
};

}  // namespace

#endif  // CVPAL_DAC_H_
