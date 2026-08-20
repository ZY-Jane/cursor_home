// TcAssemblerLoadStateFill.h
// Copy into mlir::acuity::tc_assembler (header-only).
//
// Same layering as HAL: start / end peel 31:17 (high:low);
// setField(data, field, value) calls start/end inside.
//
//   #define NN_ADDR_HI  31:17
//   word = setField(word, NN_ADDR_HI, cmdHi);
//   appendLoadState(buf, reg, setField(0, NN_ADDR_HI, cmdHi));
//
// start/end are function-like macros (0?field / 1?field). Do not write
// v.end() in a file that includes this header; the end( token will fire.

#pragma once

#include <cstdint>
#include <vector>

//===----------------------------------------------------------------------===//
// Match HAL: __gcmSTART / __gcmEND / __gcmGETSIZE / __gcmALIGN / __gcmMASK
// / gcmSETFIELD — local names, no HAL include.
//===----------------------------------------------------------------------===//

#define start(reg_field) (0 ? reg_field)
#define end(reg_field) (1 ? reg_field)
#define fieldSize(reg_field) (end(reg_field) - start(reg_field) + 1)
#define fieldAlign(data, reg_field) (((uint32_t)(data)) << start(reg_field))
#define fieldMask(reg_field)                                                   \
  ((uint32_t)((fieldSize(reg_field) == 32)                                     \
                  ? ~0U                                                        \
                  : (~(~0U << fieldSize(reg_field)))))

#define setField(data, field, value)                                           \
  ((((uint32_t)(data)) & ~fieldAlign(fieldMask(field), field)) |               \
   fieldAlign((uint32_t)(value) & fieldMask(field), field))

#define getField(data, field)                                                  \
  ((((uint32_t)(data)) >> start(field)) & fieldMask(field))

#define FE_OPCODE 31:27
#define FE_COUNT 26:16
#define FE_ADDRESS 15:0

namespace mlir {
namespace acuity {
namespace tc_assembler {

enum FeOpcode : uint32_t {
  kFeOpLoadState = 1,
  kFeOpEnd = 2,
  kFeOpNop = 3,
};

constexpr uint32_t kRegPsTriggerNn2 = 0x051d;

inline uint32_t makeLoadStateHeader(uint32_t regAddr, uint32_t count = 1) {
  uint32_t h = 0;
  h = setField(h, FE_OPCODE, kFeOpLoadState);
  h = setField(h, FE_COUNT, count);
  h = setField(h, FE_ADDRESS, regAddr);
  return h;
}

inline uint32_t makeFeEnd() { return setField(0, FE_OPCODE, kFeOpEnd); }

inline void appendWord(std::vector<uint8_t> &buf, uint32_t word) {
  buf.push_back(static_cast<uint8_t>(word));
  buf.push_back(static_cast<uint8_t>(word >> 8));
  buf.push_back(static_cast<uint8_t>(word >> 16));
  buf.push_back(static_cast<uint8_t>(word >> 24));
}

inline void appendWords(std::vector<uint8_t> &buf, const uint32_t *data,
                        uint32_t n) {
  for (uint32_t i = 0; i < n; ++i)
    appendWord(buf, data[i]);
}

inline void appendLoadState(std::vector<uint8_t> &buf, uint32_t regAddr,
                            uint32_t data) {
  appendWord(buf, makeLoadStateHeader(regAddr, /*count=*/1));
  appendWord(buf, data);
}

inline void appendLoadStateN(std::vector<uint8_t> &buf, uint32_t regAddr,
                             const uint32_t *data, uint32_t count) {
  if (count == 0)
    return;
  appendWord(buf, makeLoadStateHeader(regAddr, count));
  appendWords(buf, data, count);
}

inline void appendFeEnd(std::vector<uint8_t> &buf) {
  appendWord(buf, makeFeEnd());
}

} // namespace tc_assembler
} // namespace acuity
} // namespace mlir
