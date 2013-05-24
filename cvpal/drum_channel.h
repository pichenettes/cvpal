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

#ifndef CVPAL_DRUM_CHANNEL_H_
#define CVPAL_DRUM_CHANNEL_H_

#include "avrlib/base.h"
#include "cvpal/calibration_table.h"
#include "cvpal/note_stack.h"
#include "cvpal/voice_allocator.h"

namespace cvpal {

const uint16_t kDrumTriggerDuration = 10;

class DrumChannel {
 public:
  DrumChannel() { }
  ~DrumChannel() { }
  
  inline void Trigger(uint8_t velocity) {
    counter_ = 0;
    velocity_ = velocity;
  }
  
  inline void Tick() {
    if (counter_ != 0xffff) {
      ++counter_;
    }
  }
  
  inline void Stop() {
    counter_ = 0xffff;
  }
  
  inline bool trigger() const { return counter_ < kDrumTriggerDuration; }
  inline bool gate() const { return counter_ != 0xffff; }
  inline uint8_t velocity() const { return velocity_; }
  
 private:
  uint8_t velocity_;
  uint16_t counter_;
  
  DISALLOW_COPY_AND_ASSIGN(DrumChannel);
};

}  // namespace

#endif  // CVPAL_DRUM_CHANNEL_H_