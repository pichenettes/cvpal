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

#ifndef CVPAL_CALIBRATION_TABLE_H_
#define CVPAL_CALIBRATION_TABLE_H_

#include "avrlib/base.h"

namespace cvpal {

const uint8_t kCalibrationTableSize = 9;

class CalibrationTable {
 public:
  CalibrationTable() { }
  ~CalibrationTable() { }

  void Init(uint8_t table_index);
  int16_t Calibrate(int16_t note);
  void Adjust(uint8_t index, int8_t delta) {
    int16_t dac_code = dac_codes_[index];
    dac_code += delta;
    if (dac_code < 0) {
      dac_code = 0;
    } else if (dac_code > 4095) {
      dac_code = 4095;
    }
    dac_codes_[index] = dac_code;
    Save();
  }

 private:
  uint16_t ComputeChecksum();
  void Save();
   
  uint8_t table_index_;
  uint16_t dac_codes_[kCalibrationTableSize];

  DISALLOW_COPY_AND_ASSIGN(CalibrationTable);
};

}  // namespace

#endif  // CVPAL_CALIBRATION_TABLE_H_
