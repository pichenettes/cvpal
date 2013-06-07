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

#include "cvpal/calibration_table.h"

#include <avr/eeprom.h>
#include <avr/pgmspace.h>

namespace cvpal {

void CalibrationTable::Init(uint8_t table_index) {
  table_index_ = table_index;
  
  uint16_t checksum = 0;
  const uint16_t* address = (const uint16_t*)(void*)(table_index_ << 5);
  for (uint16_t i = 0; i < kCalibrationTableSize; ++i) {
    dac_codes_[i] = eeprom_read_word(address);
    checksum += dac_codes_[i];
    ++address;
  }
  if (eeprom_read_word(address) != checksum) {
    // Create a basic calibration table
    uint16_t dac_code = 0;
    for (uint16_t i = 0; i < kCalibrationTableSize; ++i) {
      dac_codes_[i] = dac_code;
      dac_code += 500;
    }
    Save();
  }
}

void CalibrationTable::Save() {
  uint16_t checksum = 0;
  uint16_t* address = (uint16_t*)(void*)(table_index_ << 5);
  for (uint16_t i = 0; i < kCalibrationTableSize; ++i) {
    eeprom_write_word(address, dac_codes_[i]);
    checksum += dac_codes_[i];
    ++address;
  }

  eeprom_write_word(address, checksum);
}

const prog_uint16_t divide_by_3[] PROGMEM = {
      0,    85,   170,   256,   341,   426,   512,   597,   682,
    768,   853,   938,  1024,  1109,  1194,  1280,  1365,  1450,
   1536,  1621,  1706,  1792,  1877,  1962,  2048,  2133,  2218,
   2304,  2389,  2474,  2560,  2645,  2730,  2816,  2901,  2986,
   3072,  3157,  3242,  3328,  3413,  3498,  3584,  3669,  3754,
   3840,  3925,  4010,  4096,  4181,  4266,  4352,  4437,  4522,
   4608,  4693,  4778,  4864,  4949,  5034,  5120,  5205,  5290,
   5376,  5461,  5546,  5632,  5717,  5802,  5888,  5973,  6058,
   6144,  6229,  6314,  6400,  6485,  6570,  6656,  6741,  6826,
   6912,  6997,  7082,  7168,  7253,  7338,  7424,  7509,  7594,
   7680,  7765,  7850,  7936,  8021,  8106,  8192,  8277,  8362,
   8448,  8533,  8618,  8704,  8789,  8874,  8960,  9045,  9130,
   9216,  9301,  9386,  9472,  9557,  9642,  9728,  9813,  9898,
   9984, 10069, 10154, 10240, 10325, 10410, 10496, 10581, 10666,
  10752, 10837, 10922, 11008, 11093, 11178, 11264, 11349, 11434,
  11520, 11605, 11690, 11776, 11861, 11946, 12032, 12117, 12202,
  12288, 12373, 12458, 12544, 12629, 12714, 12800, 12885, 12970,
  13056, 13141, 13226, 13312, 13397, 13482, 13568, 13653, 13738,
  13824, 13909, 13994, 14080, 14165, 14250, 14336, 14421, 14506,
  14592, 14677, 14762, 14848, 14933, 15018, 15104, 15189, 15274,
  15360, 15445, 15530, 15616, 15701, 15786, 15872, 15957, 16042,
  16128, 16213, 16298
};

int16_t CalibrationTable::Calibrate(int16_t note) {
  note -= 36 << 7;
  if (note < 0) {
    note = 0;
  } else if (note > (48 << 7)) {
    note = 48 << 7;
  }
  uint8_t first_index = 0;
  while (note >= 192) {
    ++first_index;
    note -= 192;
  }
  uint32_t fractional_part = pgm_read_word(divide_by_3 + note);
  uint8_t subdivisions = first_index & 3;
  while (subdivisions--) {
    fractional_part += 16384;
  }
  first_index >>= 2;
  
  uint32_t delta = dac_codes_[first_index + 1] - dac_codes_[first_index];
  return dac_codes_[first_index] + ((delta * fractional_part) >> 16);
}

}  // namespace
