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

#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "cvpal/midi_handler.h"

using namespace cvpal;

MidiHandler m;

void Trace() {
  const State& state = m.state();
  printf("%d %d %d %d\n", state.cv[0], state.cv[1], state.gate[0], state.gate[1]);
}

void Parse(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  uint8_t buffer[4];
  buffer[0] = a;
  buffer[1] = b;
  buffer[2] = c;
  buffer[3] = d;
  m.Parse(buffer, 4);
  m.Render();
  Trace();
  for (uint8_t i = 0; i < 20; ++i) {
    m.Tick();
    m.Render();
    Trace();
  }
  printf("\n");
}

int main(void) {
  m.Init();
  /*Parse(0x09, 0x94, 60, 127);
  Parse(0x09, 0x94, 62, 100);
  Parse(0x09, 0x94, 64, 127);
  Parse(0x08, 0x84, 62, 127);
  Parse(0x08, 0x84, 64, 127);
  Parse(0x08, 0x84, 60, 127);
  Parse(0x09, 0x94, 60, 127);
  Parse(0x08, 0x84, 60, 127);*/
  Parse(0x09, 0x9b, 36, 20);
  Parse(0x08, 0x8b, 36, 20);
  Parse(0x09, 0x9b, 38, 40);
  Parse(0x09, 0x9b, 36, 127);
}
