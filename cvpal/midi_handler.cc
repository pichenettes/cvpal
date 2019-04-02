// Copyright 2013 Emilie Gillet.
//
// Author: Emilie Gillet (emilie.o.gillet@gmail.com)
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

#include "cvpal/midi_handler.h"

#include <avr/pgmspace.h>

#include "avrlib/op.h"

namespace cvpal {

using namespace avrlib;

static const uint16_t kOctave = 12 << 7;
static const uint16_t kFirstDcoNote = 16 << 7;

/* static */
const MidiHandler::RenderFn MidiHandler::fn_table_[] PROGMEM = {
  &MidiHandler::RenderMonoCvGate,
  &MidiHandler::RenderMonoCvGateWithDco,
  &MidiHandler::RenderDualCvGate,
  &MidiHandler::RenderDualCvGate,
  &MidiHandler::RenderPolyCv,
  &MidiHandler::RenderCcConversion,
  &MidiHandler::RenderMonoCvGateCc,
  &MidiHandler::RenderMonoCvGateCc,
  &MidiHandler::RenderDrumVelocity,
  &MidiHandler::RenderDrumTrigger,
  &MidiHandler::RenderDrumGate,
  &MidiHandler::RenderMonoCvGateWithClock,
  &MidiHandler::RenderMonoCvGateWithClock,
  &MidiHandler::RenderMonoCvGateWithClock,
  &MidiHandler::RenderCalibration,
  &MidiHandler::RenderCalibration,
};

const prog_uint16_t lut_dco_count[] PROGMEM = {
   60675,  60238,  59805,  59374,  58947,  58523,  58102,  57684,
   57269,  56857,  56448,  56042,  55639,  55239,  54841,  54447,
   54055,  53666,  53280,  52897,  52516,  52138,  51763,  51391,
   51021,  50654,  50290,  49928,  49569,  49212,  48858,  48506,
   48157,  47811,  47467,  47125,  46786,  46450,  46116,  45784,
   45455,  45128,  44803,  44481,  44161,  43843,  43527,  43214,
   42903,  42595,  42288,  41984,  41682,  41382,  41084,  40789,
   40495,  40204,  39915,  39628,  39343,  39060,  38779,  38500,
   38223,  37948,  37675,  37404,  37134,  36867,  36602,  36339,
   36077,  35818,  35560,  35304,  35050,  34798,  34548,  34299,
   34052,  33807,  33564,  33323,  33083,  32845,  32609,  32374,
   32141,  31910,  31680,  31452,  31226,  31002,  30779,  30557,
   30337,
};

void MidiHandler::Init() {
  most_recent_channel_ = 0;

  poly_allocator_.Init();
  poly_allocator_.set_size(kNumVoices);  

  calibration_table_[0].Init(0);
  calibration_table_[1].Init(1);
  
  for (uint8_t i = 0; i < kNumDrumChannels; ++i) {
    drum_channel_[i].Stop();
  }
  
  needs_refresh_ = true;
  
  calibrated_note_ = 60;
  Reset();
}

void MidiHandler::Reset() {
  mono_allocator_[0].Clear();
  mono_allocator_[1].Clear();
  for (uint8_t i = 0; i < kNumDrumChannels; ++i) {
    drum_channel_[i].Stop();
  }
  pitch_bend_[0] = 0;
  pitch_bend_[1] = 0;
  clock_counter_ = 0;
  legato_[0] = false;
  legato_[1] = false;
  control_change_[0] = control_change_[1] = 0;
  control_change_[2] = control_change_[3] = 0;
}

void MidiHandler::Parse(const uint8_t* data, uint8_t size) {
  while (size) {
    // uint8_t cable_number = data[0] >> 4;
    uint8_t code_index = data[0] & 0xf;
    uint8_t channel = data[1] & 0xf;
    
    if (code_index != 0x0f && channel != most_recent_channel_) {
      if (!((most_recent_channel_ == 2 && channel == 3) ||
            (most_recent_channel_ == 3 && channel == 2))) {
        Reset();
      }
    }
    
    switch (code_index) {
      case 0x08:
        NoteOff(channel, data[2]);
        most_recent_channel_ = channel;
        break;
      case 0x09:
        NoteOn(channel, data[2], data[3]);
        most_recent_channel_ = channel;
        break;
      case 0x0b:
        ControlChange(channel, data[2], data[3]);
        most_recent_channel_ = channel;
        break;
      case 0x0e:
        PitchBend(channel, data[2], data[3]);
        most_recent_channel_ = channel;
        break;
      case 0x0f:
        RealtimeMessage(data[1]);
        break;
    }
    size -= 4;
    data += 4;
    needs_refresh_ = true;
  }
}

void MidiHandler::RealtimeMessage(uint8_t byte) {
  if (most_recent_channel_ >= 0x0b && most_recent_channel_ <= 0x0d) {
    if (byte == 0xf8) {
      // Clock
      if (clock_counter_ == 0) {
        drum_channel_[0].Trigger(0);
      }
      ++clock_counter_;
      if (most_recent_channel_ == 0x0b) {
        clock_counter_ = 0;
      } else if (most_recent_channel_ == 0x0c && clock_counter_ >= 3) {
        clock_counter_ = 0;
      } else if (most_recent_channel_ == 0x0d && clock_counter_ >= 6) {
        clock_counter_ = 0;
      }
    } else if (byte == 0xfa) {
      // Start
      clock_counter_ = 0;
      drum_channel_[1].Trigger(0);
    }
  }
}

void MidiHandler::NoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
  if (velocity == 0) {
    NoteOff(channel, note);
  } else {
    switch (channel) {
      case 0x00:
      case 0x01:
      case 0x02:
      case 0x03:
      case 0x06:
      case 0x07:
      case 0x0b:
      case 0x0c:
      case 0x0d:
        {
          uint8_t voice = channel == 0x03 ? 1 : 0;
          mono_allocator_[voice].NoteOn(note, velocity);
          if (state_.gate[channel == 0x01 ? 1 : voice] && !legato_[voice]) {
            force_retrigger_[voice] = kRetriggerDuration;
          }
        }
        break;

      case 0x04:
        {
          uint8_t voice = poly_allocator_.NoteOn(note);
          if (state_.gate[voice] && !legato_[voice]) {
            force_retrigger_[voice] = kRetriggerDuration;
          }
          active_note_[voice] = note;
        }
        break;
      
      case 0x08:
      case 0x09:
      case 0x0a:
        {
          if (note == 36) {
            drum_channel_[0].Trigger(velocity);
          } else if (note == 38) {
            drum_channel_[1].Trigger(velocity);
          } else if (note == 40) {
            drum_channel_[2].Trigger(velocity);
          } else if (note == 46) {
            drum_channel_[3].Trigger(velocity);
          }
        }
        break;
      
      case 0x0e:
      case 0x0f:
        {
          uint8_t calibrated_note = 42;
          for (uint8_t i = 1; i <= 8; ++i) {
            if (note == calibrated_note - 1) {
              calibrated_note_ = calibrated_note;
              calibration_table_[channel - 0x0e].Adjust(i, -1);
            } else if (note == calibrated_note + 1) {
              calibrated_note_ = calibrated_note;
              calibration_table_[channel - 0x0e].Adjust(i, +1);
            } else if (note == calibrated_note) {
              calibrated_note_ = calibrated_note;
            }
            calibrated_note += 6;
          }
        }
        break;
    }
  }
}

void MidiHandler::NoteOff(uint8_t channel, uint8_t note) {
  switch (channel) {
    case 0x00:
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x06:
    case 0x07:
    case 0x0b:
    case 0x0c:
    case 0x0d:
      {
        uint8_t voice = channel == 0x03 ? 1 : 0;
        uint8_t top_note = mono_allocator_[voice].most_recent_note().note;
        mono_allocator_[voice].NoteOff(note);
        if (mono_allocator_[voice].size() && 
            mono_allocator_[voice].most_recent_note().note != top_note) {
          force_retrigger_[voice] = !legato_[voice] ? kRetriggerDuration : 0;
        }
      }
      break;
      
    case 0x04:
      {
        uint8_t voice_index = poly_allocator_.NoteOff(note);
        if (voice_index < kNumVoices) {
          active_note_[voice_index] = 0xff;
        }
      }
      break;
      
    case 0x08:
    case 0x09:
    case 0x0a:
      {
        if (note == 36) {
          drum_channel_[0].Stop();
        } else if (note == 38) {
          drum_channel_[1].Stop();
        } else if (note == 40) {
          drum_channel_[2].Stop();
        } else if (note == 46) {
          drum_channel_[3].Stop();
        }
      }
      break;
      
  }
  most_recent_channel_ = channel;
}

void MidiHandler::Tick() {
  if (most_recent_channel_ >= 0x08 && most_recent_channel_ <= 0x0d) {
    for (uint8_t i = 0; i < kNumDrumChannels; ++i) {
      drum_channel_[i].Tick();
    }
    needs_refresh_ = true;
  }
  if (force_retrigger_[0]) {
    --force_retrigger_[0];
    needs_refresh_ = true;
  }
  if (force_retrigger_[1]) {
    --force_retrigger_[1];
    needs_refresh_ = true;
  }
}

void MidiHandler::PitchBend(uint8_t channel, uint8_t lsb, uint8_t msb) {
  int16_t value = (static_cast<uint16_t>(msb) << 7) + lsb;
  value -= 8192;
  uint8_t voice = channel == 0x03 ? 1 : 0;
  pitch_bend_[voice] = value;
}

void MidiHandler::ControlChange(
    uint8_t channel,
    uint8_t number,
    uint8_t value) {
  if (number >= 1 && number <= 4) {
    control_change_[number - 1] = value;
  } else if (channel <= 0x03 || channel >= 0x0b) {
    if (number == 68) {
      uint8_t voice = channel <= 0x02 || channel >= 0x0b ? 0 : 1;
      legato_[voice] = value >= 64;
    }
  }
}

void MidiHandler::Render() {
  RenderFn fn;
  memcpy_P(&fn, fn_table_ + most_recent_channel_, sizeof(RenderFn));
  if (most_recent_channel_ != 1) {
    state_.dco_frequency = 0;
  }
  (this->*fn)();
  needs_refresh_ = false;
}

void MidiHandler::RenderMonoCvGate() {
  if (mono_allocator_[0].size()) {
    int16_t note = mono_allocator_[0].most_recent_note().note;
    state_.cv[0] = NoteToCv(note, pitch_bend_[0], 0);
    state_.cv[1] = mono_allocator_[0].most_recent_note().velocity << 5;
    state_.gate[0] = state_.gate[1] = !force_retrigger_[0];
  } else {
    state_.gate[0] = state_.gate[1] = false;
  }
}

void MidiHandler::RenderMonoCvGateWithClock() {
  RenderMonoCvGate();
  state_.cv[1] = state_.gate[0] ? 4095 : 0;
  state_.gate[0] = drum_channel_[0].trigger();
  state_.gate[1] = drum_channel_[1].trigger();
}

void MidiHandler::RenderMonoCvGateWithDco() {
  if (mono_allocator_[0].size()) {
    int16_t note = mono_allocator_[0].most_recent_note().note;
    state_.cv[0] = NoteToCv(note, pitch_bend_[0], 0);
    state_.cv[1] = mono_allocator_[0].most_recent_note().velocity << 5;
    state_.gate[1] = !force_retrigger_[0];
    
    note <<= 7;
    note += pitch_bend_[0] >> 5;
    
    // Lowest note: E0.
    note -= kFirstDcoNote;
    while (note < 0) {
      note += kOctave;
    }
    uint8_t shifts = 0;
    while (note >= kOctave) {
      note -= kOctave;
      ++shifts;
    }
    uint16_t index_integral = U16ShiftRight4(note);
    uint16_t index_fractional = U8ShiftLeft4(note);
    uint32_t count = pgm_read_word(lut_dco_count + index_integral);
    uint32_t next = pgm_read_word(lut_dco_count + index_integral + 1);
    count -= (count - next) * index_fractional >> 8;
    while (shifts--) {
      count >>= 1;
    }    
    state_.dco_frequency = count;
  } else {
    state_.gate[1] = false;
  }
}

void MidiHandler::RenderDualCvGate() {
  for (uint8_t i = 0; i < kNumVoices; ++i) {
    if (mono_allocator_[i].size()) {
      state_.cv[i] = NoteToCv(
          mono_allocator_[i].most_recent_note().note,
          pitch_bend_[i],
          i);
      state_.gate[i] = !force_retrigger_[i];
    } else {
      state_.gate[i] = false;
    }
  }
}

void MidiHandler::RenderPolyCv() {
  for (uint8_t i = 0; i < kNumVoices; ++i) {
    if (active_note_[i] != 0xff) {
      state_.cv[i] = NoteToCv(active_note_[i], pitch_bend_[0], i);
      state_.gate[i] = !force_retrigger_[i];
    } else {
      state_.gate[i] = false;
    }
  }
}

void MidiHandler::RenderCcConversion() {
  state_.cv[0] = control_change_[0] << 5;
  state_.cv[1] = control_change_[1] << 5;
  state_.gate[0] = control_change_[2] >= 64;
  state_.gate[1] = control_change_[3] >= 64;
}

void MidiHandler::RenderMonoCvGateCc() {
  RenderMonoCvGate();
  state_.cv[1] = control_change_[most_recent_channel_ - 6] << 5;
}

void MidiHandler::RenderDrumVelocity() {
  state_.cv[0] = drum_channel_[0].velocity() << 5;
  state_.cv[1] = drum_channel_[1].velocity() << 5;
  state_.gate[0] = drum_channel_[0].gate();
  state_.gate[1] = drum_channel_[1].gate();
}

void MidiHandler::RenderDrumTrigger() {
  state_.cv[0] = drum_channel_[0].trigger() ? 4095 : 0;
  state_.cv[1] = drum_channel_[1].trigger() ? 4095 : 0;
  state_.gate[0] = drum_channel_[2].trigger();
  state_.gate[1] = drum_channel_[3].trigger();
}

void MidiHandler::RenderDrumGate() {
  state_.cv[0] = drum_channel_[0].gate() ? 4095 : 0;
  state_.cv[1] = drum_channel_[1].gate() ? 4095 : 0;
  state_.gate[0] = drum_channel_[2].gate();
  state_.gate[1] = drum_channel_[3].gate();
}

void MidiHandler::RenderCalibration() {
  state_.gate[0] = true;
  state_.gate[1] = true;
  state_.cv[0] = NoteToCv(calibrated_note_, 0, 0);
  state_.cv[1] = NoteToCv(calibrated_note_, 0, 1);
}

}  // namespace cvpal
