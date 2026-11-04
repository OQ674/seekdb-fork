/*
 * Copyright (c) 2025 OceanBase.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include "ob_string_diff_encoder.h"

namespace oceanbase
{
namespace blocksstable
{

// Common spans are identical across rows. Read each descriptor once and copy
// its span to the batch, leaving differential bytes and row padding untouched.
OB_INLINE void copy_string_diff_common_batch(const ObStringDiffHeader &header,
    char *buf, const int64_t stride, const int64_t row_cap)
{
  if (row_cap > 0) {
    const char *common = header.common_data();
    int64_t offset = 0;
    for (int64_t i = 0; i < header.diff_desc_cnt_; ++i) {
      const int64_t count = header.diff_descs_[i].count_;
      if (0 == header.diff_descs_[i].diff_ && count > 0) {
        for (int64_t row = 0; row < row_cap; ++row) {
          MEMCPY(buf + row * stride + offset, common, count);
        }
        common += count;
      }
      offset += count;
    }
  }
}

} // namespace blocksstable
} // namespace oceanbase
