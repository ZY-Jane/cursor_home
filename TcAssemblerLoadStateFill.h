// TcAssemblerLoadStateFill.h
// Copy into mlir::acuity::tc_assembler (header-only).
//
// Bit ranges are high:low like HAL (e.g. 31:17). lsStart / lsEnd peel those
// apart; lsSet / makeLoadStateHeader / lsPut call them — not setField.
//
// Packet layout (one Load State):
//   word0 = opcode=1 | count | regAddr   → e.g. 0x0801051d
//   word1.. = register data

#pragma once

#include <cstdint>
#include <vector>

//===----------------------------------------------------------------------===//
// 31:17 → start / end  (same trick as __gcmSTART / __gcmEND)
//===----------------------------------------------------------------------===//

#define lsStart(bits) (0 ? bits)
#define lsEnd(bits) (1 ? bits)
#define lsSize(bits) (lsEnd(bits) - lsStart(bits) + 1)
#define lsMask(bits)                                                           \
  ((uint32_t)((lsSize(bits) == 32) ? ~0U : (~(~0U << lsSize(bits)))))
#define lsAlign(data, bits) (((uint32_t)(data)) << lsStart(bits))

/// Insert `val` into `bits` of `word`. Uses lsStart/lsEnd; does not call setField.
#define lsSet(word, bits, val)                                                 \
  (((uint32_t)(word) & ~lsAlign(lsMask(bits), bits)) |                         \
   lsAlign((uint32_t)(val) & lsMask(bits), bits))

#define lsGet(word, bits) ((((uint32_t)(word)) >> lsStart(bits)) & lsMask(bits))

// FE Load State header fields (high:low).
#define LS_FE_OPCODE 31:27
#define LS_FE_COUNT 26:16
#define LS_FE_ADDRESS 15:0

namespace mlir {
namespace acuity {
namespace tc_assembler {

enum FeOpcode : uint32_t {
  kFeOpLoadState = 1,
  kFeOpEnd = 2,
  kFeOpNop = 3,
};

constexpr uint32_t kRegPsTriggerNn2 = 0x051d;

/// Load State command header: opcode=1, count, 16-bit register address.
inline uint32_t makeLoadStateHeader(uint32_t regAddr, uint32_t count = 1) {
  uint32_t h = 0;
  h = lsSet(h, LS_FE_OPCODE, kFeOpLoadState);
  h = lsSet(h, LS_FE_COUNT, count);
  h = lsSet(h, LS_FE_ADDRESS, regAddr);
  return h;
}

inline uint32_t makeFeEnd() { return lsSet(0, LS_FE_OPCODE, kFeOpEnd); }

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

// One register write whose data word has a 31:17-style field.
//   #define NN_ADDR_HI  31:17
//   lsPut(buf, kRegPsTriggerNn2, NN_ADDR_HI, cmdHi);
#define lsPut(buf, reg, bits, val)                                             \
  ::mlir::acuity::tc_assembler::appendLoadState((buf), (reg),                   \
                                               lsSet(0, bits, val))
