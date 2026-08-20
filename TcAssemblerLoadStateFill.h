// TcAssemblerLoadStateFill.h
// Copy into mlir::acuity::tc_assembler (header-only).
//
// HAL uses __gcmSTART / __gcmEND, not start/end: `#define end(` breaks
// string::end() in every TU that includes this header.
//
//   #define NN_ADDR_HI  31:17
//   word = setField(word, NN_ADDR_HI, cmdHi);
//   appendLoadState(buf, reg, setField(0, NN_ADDR_HI, cmdHi));

#pragma once

#include <cstdint>
#include <vector>

#define LS_START(reg_field) (0 ? reg_field)
#define LS_END(reg_field) (1 ? reg_field)
#define fieldSize(reg_field) (LS_END(reg_field) - LS_START(reg_field) + 1)
#define fieldAlign(data, reg_field) (((uint32_t)(data)) << LS_START(reg_field))
#define fieldMask(reg_field)                                                   \
  ((uint32_t)((fieldSize(reg_field) == 32)                                     \
                  ? ~0U                                                        \
                  : (~(~0U << fieldSize(reg_field)))))

#define setField(data, field, value)                                           \
  ((((uint32_t)(data)) & ~fieldAlign(fieldMask(field), field)) |               \
   fieldAlign((uint32_t)(value) & fieldMask(field), field))

#define getField(data, field)                                                  \
  ((((uint32_t)(data)) >> LS_START(field)) & fieldMask(field))

// FE Load State header (high:low). Count is 25:16; bit 26 is FixedPoint.
#define FE_OPCODE 31:27
#define FE_FIXED_POINT 26:26
#define FE_COUNT 25:16
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

inline uint32_t makeLoadStateHeader(uint32_t regAddr, uint32_t count = 1,
                                    uint32_t fixedPoint = 0) {
  uint32_t h = 0;
  h = setField(h, FE_OPCODE, kFeOpLoadState);
  h = setField(h, FE_FIXED_POINT, fixedPoint);
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
                            uint32_t data, uint32_t fixedPoint = 0) {
  appendWord(buf, makeLoadStateHeader(regAddr, /*count=*/1, fixedPoint));
  appendWord(buf, data);
}

inline void appendLoadStateN(std::vector<uint8_t> &buf, uint32_t regAddr,
                             const uint32_t *data, uint32_t count,
                             uint32_t fixedPoint = 0) {
  if (count == 0)
    return;
  appendWord(buf, makeLoadStateHeader(regAddr, count, fixedPoint));
  appendWords(buf, data, count);
}

inline void appendFeEnd(std::vector<uint8_t> &buf) {
  appendWord(buf, makeFeEnd());
}

} // namespace tc_assembler
} // namespace acuity
} // namespace mlir
