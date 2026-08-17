// NpuCollect.h
// Collect-phase definitions for NPU NBG dump (no OpenCL string path).
// Inner loadState emitter can stay MLIR-free and take plain C structs + uint8 buffers.

#ifndef ACUITY_NPU_COLLECT_H
#define ACUITY_NPU_COLLECT_H

#include <cstdint>
#include <vector>

#include "llvm/ADT/SmallVector.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/Operation.h"

// Forward declarations — replace with your real headers.
namespace mlir {
namespace acuity {
struct HardwareInfo;
namespace nn {
class TriggerOp;
} // namespace nn
} // namespace acuity
} // namespace mlir

namespace mlir {
namespace acuity {
namespace npu {

/// Information bag for one trigger's loadState (not the binary buffer itself).
struct TriggerLoadState {
  int64_t coreId = 0;
  const HardwareInfo *hwInfo = nullptr;

  /// Device/logical address; may be 0 until MemAssign/Pack fills it.
  uint64_t cmdBufferAddr = 0;
  /// Optional: offset into constant/cmd pool while addr is still unknown.
  int64_t cmdOffsetInConst = 0;

  uint32_t commandBufferSize = 0;
  int64_t eventId = 0;
  bool multiCoreSync = false;

  // Optional extras for debugging / NBG table:
  // int64_t triggerType = 0;
};

/// One collected trigger: identity + info (+ optional already-emitted bytes).
struct TriggerNbgEntry {
  /// Stable index within this collector (walk order).
  int64_t ordinal = -1;

  /// Valid only during the same collect pass; do not keep after IR mutation.
  Operation *op = nullptr;
  Location loc = UnknownLoc::get(nullptr); // set properly in collect

  TriggerLoadState state;

  /// Filled if you emit during collect; otherwise leave empty and emit at serialize.
  std::vector<uint8_t> loadStateBuf;
};

/// Accumulates all NPU pieces before serialize → NBG blob.
struct NpuCollector {
  llvm::SmallVector<TriggerNbgEntry, 8> triggers;

  // Extend later: cmd constant pool refs, buffer table, dispatch symbol, etc.
  // llvm::SmallVector<CmdPoolEntry, 4> cmdPools;

  void clear() { triggers.clear(); }

  TriggerNbgEntry &addTrigger() {
    triggers.push_back({});
    TriggerNbgEntry &e = triggers.back();
    e.ordinal = static_cast<int64_t>(triggers.size() - 1);
    return e;
  }
};

//===----------------------------------------------------------------------===//
// Collect API (MLIR side) — parallel to codegenBlocks / codegenBlockOps
//===----------------------------------------------------------------------===//

/// Fill TriggerLoadState from a trigger op + hardware info.
LogicalResult fillTriggerLoadState(TriggerLoadState &out, nn::TriggerOp trigger,
                                   const HardwareInfo &hwInfo,
                                   int64_t coreId = 0);

/// Walk a region: for each block, collect NBG-related ops.
LogicalResult collectNbgBlocks(Region &region, NpuCollector &collector,
                               const HardwareInfo &hwInfo, int64_t coreId = 0);

/// Walk one block: TriggerOp → fill state → push into collector.
LogicalResult collectNbgBlockOps(Block &block, NpuCollector &collector,
                                 const HardwareInfo &hwInfo, int64_t coreId = 0);

/// Optional: emit binary loadState into entry.loadStateBuf (or a free vector).
/// Prefer implementing the actual packing in a non-MLIR module that takes
/// a plain C info struct + writes into std::vector / caller buffer.
LogicalResult emitLoadStateToVector(const TriggerLoadState &state,
                                    std::vector<uint8_t> &out);

/// After collect: pack collector into one NBG blob.
LogicalResult serializeNbg(const NpuCollector &collector,
                           std::vector<uint8_t> &nbgOut);

} // namespace npu
} // namespace acuity
} // namespace mlir

#endif // ACUITY_NPU_COLLECT_H
