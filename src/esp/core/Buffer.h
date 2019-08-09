// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include "esp/core/esp.h"

namespace esp {
namespace core {

// Enumeration of data
enum class DataType {
  DT_NONE = 0,
  DT_INT8 = 1,
  DT_UINT8 = 2,
  DT_INT16 = 3,
  DT_UINT16 = 4,
  DT_INT32 = 5,
  DT_UINT32 = 6,
  DT_INT64 = 7,
  DT_UINT64 = 8,
  DT_FLOAT = 9,
  DT_DOUBLE = 10,
};

class Buffer {
 public:
  explicit Buffer(){};
  explicit Buffer(const std::vector<size_t> shape, const DataType dataType) {
    this->shape = shape;
    this->dataType = dataType;
    alloc();
  };
  void clear();
  virtual ~Buffer() { dealloc(); }

 protected:
  void alloc();
  void dealloc();

 public:
  void* data = nullptr;
  size_t totalBytes = 0;
  size_t totalSize = 0;
  DataType dataType = DataType::DT_UINT8;
  std::vector<size_t> shape;

  ESP_SMART_POINTERS(Buffer)
};

}  // namespace core
}  // namespace esp
