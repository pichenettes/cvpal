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

#include <avr/interrupt.h>
#include <avr/io.h>
#include "avrlib/base.h"

#include "cvpal/dac.h"
#include "cvpal/midi_handler.h"
#include "cvpal/usb_handler.h"

using namespace cvpal;

Dac dac;
UsbHandler usb_handler;
MidiHandler midi_handler;

volatile uint8_t control_clock_tick;

ISR(TIM0_COMPA_vect) {
  // 1kHz clock for timing trigger pulses.
  ++control_clock_tick;
}

void Init() {
  // Initialize Gate outputs.
  DDRA |= _BV(PA6);  // GATE 1
  PORTA &= ~_BV(PA6);
  DDRA |= _BV(PA3);  // GATE 2
  PORTA &= ~_BV(PA3);
  
  dac.Init();
  midi_handler.Init();
  usb_handler.Init(&midi_handler);
  
  // 1kHz timer for timing trigger pulses.
  TCCR0A = _BV(WGM01);
  TCCR0B = 4;
  OCR0A = F_CPU / 256000 - 1;
  TIMSK0 |= _BV(1);
  
  // DCO timer.
  TCCR1B = _BV(WGM13) | 2;
}

int main(void) {
  Init();
  while (1) {
    usb_handler.Poll();
    if (midi_handler.needs_refresh()) {
      midi_handler.Render();
      const State& state = midi_handler.state();

      dac.Write(state.cv[0], state.cv[1]);
      if (state.dco_frequency) {
        // Gate 1 = DCO.
        TCCR1A = _BV(COM1A1);
        ICR1 = state.dco_frequency;
        OCR1A = state.dco_frequency >> 1;
      } else {
        // Gate 1 = digital output.
        TCCR1A = 0;
        if (state.gate[0]) {
          PORTA |= _BV(PA6);
        } else {
          PORTA &= ~_BV(PA6);
        }
      }
      if (state.gate[1]) {
        PORTA |= _BV(PA3);
      } else {
        PORTA &= ~_BV(PA3);
      }
    }
    if (control_clock_tick) {
      --control_clock_tick;
      midi_handler.Tick();
    }
  }
}
