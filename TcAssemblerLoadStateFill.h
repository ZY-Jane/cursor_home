// TcAssemblerLoadStateFill.h
// Copy into mlir::acuity::tc_assembler (header-only).
//
// Thin replacement for gcmSETSINGLECTRLSTATE_NEW: only append FE packets
// into a loadState buffer. No reserve / stateDelta / flush tracking.
// vector grows itself.
//
// Packet layout (one Load State):
//   word0 = opcode=1 | count | regAddr   → e.g. 0x0801051d
//   word1.. = register data              → e.g. CmdAddress high 32 bits

#pragma once

#include <cstdint>
#include <vector>

namespace mlir {
namespace acuity {
namespace tc_assembler {

// FE command opcodes (bits 31:27). Same encoding as Vivante/VIP.
enum FeOpcode : uint32_t {
  kFeOpLoadState = 1,
  kFeOpEnd = 2,
  kFeOpNop = 3,
};

// Example: gcregPSTriggerNN2RegAddrs — cmd buffer address bits 47:32.
// Replace with the symbol from your HAL if you prefer.
constexpr uint32_t kRegPsTriggerNn2 = 0x051d;

//===----------------------------------------------------------------------===//
// Bit / header packing
//===----------------------------------------------------------------------===//

/// Put `value` into bits [start, end] of `word` (inclusive, start = low bit).
/// Same role as gcmSETFIELD, without the 1?end : 0?start macro noise.
inline uint32_t setField(uint32_t word, unsigned start, unsigned end,
                         uint32_t value) {
  unsigned width = end - start + 1;
  uint32_t mask = (width >= 32) ? 0xffffffffu : ((1u << width) - 1u);
  return (word & ~(mask << start)) | ((value & mask) << start);
}

inline uint32_t getField(uint32_t word, unsigned start, unsigned end) {
  unsigned width = end - start + 1;
  uint32_t mask = (width >= 32) ? 0xffffffffu : ((1u << width) - 1u);
  return (word >> start) & mask;
}

/// Load State command header: opcode=1, count, 16-bit register address.
inline uint32_t makeLoadStateHeader(uint32_t regAddr, uint32_t count = 1) {
  uint32_t h = 0;
  h = setField(h, 27, 31, kFeOpLoadState);
  h = setField(h, 16, 26, count);
  h = setField(h, 0, 15, regAddr);
  return h;
}

inline uint32_t makeFeEnd() {
  return setField(0, 27, 31, kFeOpEnd);
}

//===----------------------------------------------------------------------===//
// Append little-endian words into vector<uint8_t>
//===----------------------------------------------------------------------===//

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

/// One register: header + data. Like gcmSETSINGLECTRLSTATE_NEW without HAL.
inline void appendLoadState(std::vector<uint8_t> &buf, uint32_t regAddr,
                            uint32_t data) {
  appendWord(buf, makeLoadStateHeader(regAddr, /*count=*/1));
  appendWord(buf, data);
}

/// `count` consecutive registers starting at `regAddr`.
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

// Short wrappers for header ranges like 31:17 (high:low, same as HAL).
// Macros have no namespace; call from anywhere after including this file.
//
//   #define NN_ADDR_HI  31:17          // short alias; do not paste the HAL name
//   uint32_t w = lsSet(0, NN_ADDR_HI, cmdHi);
//   lsPut(buf, kRegPsTriggerNn2, NN_ADDR_HI, cmdHi);
#define lsSet(word, bits, val)                                                 \
  ::mlir::acuity::tc_assembler::setField((word), (0 ? bits), (1 ? bits),       \
                                         (uint32_t)(val))

#define lsPut(buf, reg, bits, val)                                             \
  ::mlir::acuity::tc_assembler::appendLoadState((buf), (reg),                   \
                                               lsSet(0, bits, val))
