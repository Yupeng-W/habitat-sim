// Copyright (c) Facebook, Inc. and its affiliates.
// This source code is licensed under the MIT license found in the
// LICENSE file in the root directory of this source tree.

#include "Buffer.h"

namespace esp {
namespace core {

size_t getDataTypeByteSize(DataType dt) {
  switch (dt) {
    case DataType::DT_INT8:
    case DataType::DT_UINT8:
      return 1;
    case DataType::DT_INT16:
    case DataType::DT_UINT16:
      return 2;
    case DataType::DT_INT32:
    case DataType::DT_UINT32:
      return 4;
    case DataType::DT_INT64:
    case DataType::DT_UINT64:
      return 8;
    case DataType::DT_FLOAT:
      return sizeof(float);
    case DataType::DT_DOUBLE:
      return sizeof(double);
    default:
      return 0;
  }
}

void Buffer::clear() {
  if (this->data != nullptr) {
    memset(this->data, 0, this->totalBytes);
  }
}

void Buffer::alloc() {
  size_t size = 1;
  for (size_t i = 0; i < this->shape.size(); i++) {
    size *= this->shape[i];
  }
  if (size != this->totalSize) {
    dealloc();
    this->totalSize = size;
    this->totalBytes = size * getDataTypeByteSize(this->dataType);
    if (this->totalBytes > 0) {
      this->data = malloc(this->totalBytes);
    }
  }
}

void Buffer::dealloc() {
  if (this->data != nullptr) {
    free(this->data);
    this->data = nullptr;
    this->totalSize = 0;
    this->totalBytes = 0;
  }
}

}  // namespace core
}  // namespace esp
