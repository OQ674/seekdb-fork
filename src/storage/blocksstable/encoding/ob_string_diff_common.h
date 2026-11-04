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

// A short span fits in the union of its first and last machine words.
// The two writes may overlap, but both reproduce the same source bytes. Only
// span bytes are touched; differential positions and row padding are preserved.
// Caller guarantees sizeof(Word) <= count <= 2 * sizeof(Word) and row_cap > 0.
template <typename Word>
OB_INLINE void copy_string_diff_word_span(const char *source, char *buf,
    const int64_t stride, const int64_t row_cap, const int64_t count)
{
  Word first;
  Word last;
  const int64_t width = sizeof(Word);
  MEMCPY(&first, source, width);
  MEMCPY(&last, source + count - width, width);
  for (int64_t row = 0; row < row_cap; ++row) {
    MEMCPY(buf + row * stride, &first, width);
    MEMCPY(buf + row * stride + count - width, &last, width);
  }
}

// Read common-span descriptors once per batch. Preload spans up to 16 bytes
// into local words, avoiding repeated generic memcpy calls for tiny fragments.
// Larger spans retain the normal byte copy. Allocation and format are unchanged.
OB_INLINE void copy_string_diff_common_batch(const ObStringDiffHeader &header,
    char *buf, const int64_t stride, const int64_t row_cap)
{
  if (row_cap > 0) {
    const char *common = header.common_data();
    int64_t offset = 0;
    for (int64_t i = 0; i < header.diff_desc_cnt_; ++i) {
      const int64_t count = header.diff_descs_[i].count_;
      if (0 == header.diff_descs_[i].diff_ && count > 0) {
        char *span_buf = buf + offset;
        if (count > 16) {
          for (int64_t row = 0; row < row_cap; ++row) {
            MEMCPY(span_buf + row * stride, common, count);
          }
        } else if (count >= 8) {
          copy_string_diff_word_span<uint64_t>(common, span_buf, stride, row_cap, count);
        } else if (count >= 4) {
          copy_string_diff_word_span<uint32_t>(common, span_buf, stride, row_cap, count);
        } else if (count >= 2) {
          copy_string_diff_word_span<uint16_t>(common, span_buf, stride, row_cap, count);
        } else {
          const char value = *common;
          for (int64_t row = 0; row < row_cap; ++row) {
            span_buf[row * stride] = value;
          }
        }
        common += count;
      }
      offset += count;
    }
  }
}

} // namespace blocksstable
} // namespace oceanbase
