// Copy this fill sequence into tc_assembler::genNNLoadState.
// All cmd states go into the same loadStateBuf via appendLoadState*.

#include "TcAssemblerLoadStateFill.h"

#include <cstdint>
#include <vector>

namespace mlir {
namespace acuity {
namespace tc_assembler {

// In your tree this is tc_assembler::TriggerParams; here we only take
// the fields genNNLoadState actually encodes, so the snippet compiles.
std::vector<uint32_t> genNNLoadState(uint64_t cmdBufferAddr) {
  std::vector<uint32_t> loadStateBuf;

  // Same packet as HAL:
  //   gcmSETSINGLECTRLSTATE_NEW(..., gcregPSTriggerNN2RegAddrs,
  //       gcmSETFIELD(..., COMMAND_BUFFER_ADDR47_T032, CmdAddress >> 32));
  // Words: 0x0801051d, then cmdBufferAddr[47:32].
  const uint32_t cmdHi = static_cast<uint32_t>(cmdBufferAddr >> 32);

  // Whole 32-bit data word (no sub-field):
  appendLoadState(loadStateBuf, kRegPsTriggerNn2, cmdHi);

  // Sub-field 31:17 — short alias once, then setField like HAL:
  // #define NN_ADDR_HI  31:17
  // appendLoadState(loadStateBuf, kRegPsTriggerNn2,
  //                 setField(0, NN_ADDR_HI, cmdHi));

  // Keep appending into the same buffer:
  // appendLoadState(loadStateBuf, otherReg, otherValue);
  // uint32_t block[] = {v0, v1};
  // appendLoadStateN(loadStateBuf, startReg, block, 2);

  return loadStateBuf;
}

// Codegen side:
//   loadStateBuf = tc_assembler::genNNLoadState(params.getCmdBufferAddr());
// or, once genNNLoadState takes TriggerParams:
//   loadStateBuf = tc_assembler::genNNLoadState(params);

// Parallel packer for the standalone wait op (BigMma: FillWaitLoadState /
// genWaitLoadState). Do not fold this into genNNLoadState.
// std::vector<uint32_t> genWaitLoadState(int64_t eventId) {
//   std::vector<uint32_t> loadStateBuf;
//   appendLoadState(loadStateBuf, kRegWaitEvent, /* fields from eventId */);
//   return loadStateBuf;
// }

} // namespace tc_assembler
} // namespace acuity
} // namespace mlir
