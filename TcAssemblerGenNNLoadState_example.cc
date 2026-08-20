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
std::vector<uint8_t> genNNLoadState(uint64_t cmdBufferAddr) {
  std::vector<uint8_t> loadStateBuf;

  // Same packet as HAL:
  //   gcmSETSINGLECTRLSTATE_NEW(..., gcregPSTriggerNN2RegAddrs,
  //       gcmSETFIELD(..., COMMAND_BUFFER_ADDR47_T032, CmdAddress >> 32));
  // Bytes in buffer: 0x0801051d, then cmdBufferAddr[47:32].
  appendLoadState(loadStateBuf, kRegPsTriggerNn2,
                  static_cast<uint32_t>(cmdBufferAddr >> 32));

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

} // namespace tc_assembler
} // namespace acuity
} // namespace mlir
