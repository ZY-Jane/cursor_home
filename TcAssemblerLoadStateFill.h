// TcAssemblerLoadStateFill.h
// Copy into BigMma loadStateFill.h (only included by loadStateFill.cpp).
//
// 31:17 ranges MUST stay macros (LS_START / LS_END). Do not turn setField
// into a C++ function that takes one integer "field" — 31:27 cannot be a
// function argument, and you will get a header like 0x00000428 (address only).
//
// Buffer is vector<uint32_t>: one FE command word per element.
// Debug: p /x buf[0]  →  0x0801051d

#pragma once

#include <cstdint>
#include <vector>

namespace ls_detail {

constexpr uint32_t packField(uint32_t word, unsigned lo, unsigned hi,
                             uint32_t value) {
  unsigned width = hi - lo + 1;
  uint32_t m = (width >= 32) ? 0xffffffffu : ((1u << width) - 1u);
  return (word & ~(m << lo)) | ((value & m) << lo);
}

constexpr uint32_t unpackField(uint32_t word, unsigned lo, unsigned hi) {
  unsigned width = hi - lo + 1;
  uint32_t m = (width >= 32) ? 0xffffffffu : ((1u << width) - 1u);
  return (word >> lo) & m;
}

} // namespace ls_detail

#define LS_START(reg_field) (0 ? reg_field)
#define LS_END(reg_field) (1 ? reg_field)

#define setField(data, field, value)                                           \
  (::ls_detail::packField((uint32_t)(data), (unsigned)(LS_START(field)),       \
                          (unsigned)(LS_END(field)), (uint32_t)(value)))

#define getField(data, field)                                                  \
  (::ls_detail::unpackField((uint32_t)(data), (unsigned)(LS_START(field)),      \
                            (unsigned)(LS_END(field))))

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

constexpr uint32_t makeLoadStateHeader(uint32_t regAddr, uint32_t count = 1,
                                       uint32_t fixedPoint = 0) {
  uint32_t h = 0;
  h = setField(h, FE_OPCODE, kFeOpLoadState);
  h = setField(h, FE_FIXED_POINT, fixedPoint);
  h = setField(h, FE_COUNT, count);
  h = setField(h, FE_ADDRESS, regAddr);
  return h;
}

static_assert(makeLoadStateHeader(0x051d, 1, 0) == 0x0801051du,
              "Load State header packing is broken");
static_assert(makeLoadStateHeader(0x0428, 1, 0) == 0x08010428u,
              "opcode bits missing — setField must stay a macro");

inline uint32_t makeFeEnd() { return setField(0, FE_OPCODE, kFeOpEnd); }

inline void appendWord(std::vector<uint32_t> &buf, uint32_t word) {
  buf.push_back(word);
}

inline void appendWords(std::vector<uint32_t> &buf, const uint32_t *data,
                        uint32_t n) {
  buf.insert(buf.end(), data, data + n);
}

inline void appendLoadState(std::vector<uint32_t> &buf, uint32_t regAddr,
                            uint32_t data, uint32_t fixedPoint = 0) {
  appendWord(buf, makeLoadStateHeader(regAddr, /*count=*/1, fixedPoint));
  appendWord(buf, data);
}

inline void appendLoadStateN(std::vector<uint32_t> &buf, uint32_t regAddr,
                             const uint32_t *data, uint32_t count,
                             uint32_t fixedPoint = 0) {
  if (count == 0)
    return;
  appendWord(buf, makeLoadStateHeader(regAddr, count, fixedPoint));
  appendWords(buf, data, count);
}

inline void appendFeEnd(std::vector<uint32_t> &buf) {
  appendWord(buf, makeFeEnd());
}

} // namespace tc_assembler
} // namespace acuity
} // namespace mlir
