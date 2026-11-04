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
#include "storage/blocksstable/encoding/ob_string_diff_common.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <new>
#include <random>
#include <vector>

using namespace oceanbase::blocksstable;

int main()
{
  std::mt19937_64 random(20261104);
  const int64_t boundary_rows[] = {0, 1, 2, 3, 127, 128, 129, 1024};
  for (int round = 0; round < 8192; ++round) {
    int desc_count = random() % 17;
    if (round == 0) desc_count = 0;
    if (round == 1 || round == 2) desc_count = 1;
    if (round == 3) desc_count = 2;
    const int map_size = round % 2 == 0 ? 0 : 16;
    std::vector<char> storage(sizeof(ObStringDiffHeader) + desc_count + map_size + 129);
    auto *header = new (storage.data()) ObStringDiffHeader();
    header->diff_desc_cnt_ = desc_count;
    header->hex_char_array_size_ = map_size;
    int remaining = 128;
    int length = 0;
    int common_size = 0;
    for (int i = 0; i < desc_count; ++i) {
      auto &desc = header->diff_descs_[i];
      desc.diff_ = random() % 2;
      int count = random() % (std::min(127, remaining) + 1);
      if (round == 1) count = 1;
      if (round == 2) count = 127;
      if (round == 3) count = i == 0 ? 127 : 1;
      desc.count_ = count;
      remaining -= count;
      length += count;
      if (!desc.diff_) common_size += count;
    }
    header->string_size_ = length;
    for (int i = 0; i < common_size; ++i) header->common_data()[i] = static_cast<char>(random());
    const int64_t rows = round < 8 ? boundary_rows[round] : random() % 130;
    const int64_t stride = std::max(128, length) + random() % 33;
    std::vector<char> expected(rows * stride + 64, static_cast<char>(0xA5));
    std::vector<char> actual = expected;
    // Original production implementation is the reference. Compare every byte,
    // including differential positions, padding and both outer guard regions.
    for (int64_t row = 0; row < rows; ++row) {
      header->copy_string(ObStringDiffHeader::LeftToRight(), std::logical_not<uint8_t>(),
          header->common_data(), expected.data() + 32 + row * stride);
    }
    copy_string_diff_common_batch(*header, actual.data() + 32, stride, rows);
    copy_string_diff_common_batch(*header, nullptr, stride, 0);
    if (actual != expected) {
      std::fprintf(stderr, "common batch mismatch round=%d length=%d rows=%ld stride=%ld\n",
          round, length, rows, stride);
      std::abort();
    }
  }
  std::puts("PASS 8192 common-fragment batch layouts with full-buffer and guard comparison");
}
