// NpuCollect.h
// Collect-phase definitions for NPU NBG dump (no OpenCL string path).
// Uses constructors for TriggerNbgEntry; inner loadState emitter stays MLIR-free.

#pragma once

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
};

/// One collected trigger: identity + info (+ optional already-emitted bytes).
struct TriggerNbgEntry {
  int64_t ordinal = -1;
  Operation *op = nullptr;
  Location loc;
  TriggerLoadState state;
  std::vector<uint8_t> loadStateBuf;

  TriggerNbgEntry() = default;

  /// Construct a table entry for one TriggerOp (state filled by caller or below).
  TriggerNbgEntry(int64_t ordinal, Operation *op, Location loc,
                  TriggerLoadState state)
      : ordinal(ordinal), op(op), loc(loc), state(std::move(state)) {}

  /// Convenience: ordinal + op + already-filled state (loc from op).
  TriggerNbgEntry(int64_t ordinal, Operation *op, TriggerLoadState state)
      : ordinal(ordinal), op(op), loc(op ? op->getLoc() : Location()),
        state(std::move(state)) {}
};

/// Accumulates all NPU pieces before serialize → NBG blob.
struct NpuCollector {
  llvm::SmallVector<TriggerNbgEntry, 8> triggers;

  void clear() { triggers.clear(); }

  /// Next ordinal = current size (walk order).
  int64_t nextOrdinal() const {
    return static_cast<int64_t>(triggers.size());
  }

  void add(TriggerNbgEntry entry) { triggers.push_back(std::move(entry)); }
};

//===----------------------------------------------------------------------===//
// Collect API — parallel to codegenBlocks / codegenBlockOps
//===----------------------------------------------------------------------===//

LogicalResult fillTriggerLoadState(TriggerLoadState &out, nn::TriggerOp trigger,
                                   const HardwareInfo &hwInfo,
                                   int64_t coreId = 0);

LogicalResult collectNbgBlocks(Region &region, NpuCollector &collector,
                               const HardwareInfo &hwInfo, int64_t coreId = 0);

LogicalResult collectNbgBlockOps(Block &block, NpuCollector &collector,
                                 const HardwareInfo &hwInfo, int64_t coreId = 0);

LogicalResult emitLoadStateToVector(const TriggerLoadState &state,
                                    std::vector<uint8_t> &out);

LogicalResult serializeNbg(const NpuCollector &collector,
                           std::vector<uint8_t> &nbgOut);

} // namespace npu
} // namespace acuity
} // namespace mlir
