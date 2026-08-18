// NpuCollect.h
// Collect-phase definitions for NPU NBG dump (C++ classes, private members).

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/Operation.h"
#include "mlir/Support/LogicalResult.h"

// Forward declarations — replace with your real headers.
namespace mlir {
namespace acuity {
class HardwareInfo;
namespace nn {
class TriggerOp;
} // namespace nn
} // namespace acuity
} // namespace mlir

namespace mlir {
namespace acuity {
namespace npu {

/// Information for one trigger's loadState (not the binary buffer itself).
class TriggerLoadState {
public:
  TriggerLoadState() = default;

  TriggerLoadState(int64_t coreId, const HardwareInfo *hwInfo,
                   uint64_t cmdBufferAddr, int64_t cmdOffsetInConst,
                   uint32_t commandBufferSize, int64_t eventId,
                   bool multiCoreSync)
      : coreId_(coreId), hwInfo_(hwInfo), cmdBufferAddr_(cmdBufferAddr),
        cmdOffsetInConst_(cmdOffsetInConst),
        commandBufferSize_(commandBufferSize), eventId_(eventId),
        multiCoreSync_(multiCoreSync) {}

  int64_t getCoreId() const { return coreId_; }
  void setCoreId(int64_t v) { coreId_ = v; }

  const HardwareInfo *getHwInfo() const { return hwInfo_; }
  void setHwInfo(const HardwareInfo *v) { hwInfo_ = v; }

  uint64_t getCmdBufferAddr() const { return cmdBufferAddr_; }
  void setCmdBufferAddr(uint64_t v) { cmdBufferAddr_ = v; }

  int64_t getCmdOffsetInConst() const { return cmdOffsetInConst_; }
  void setCmdOffsetInConst(int64_t v) { cmdOffsetInConst_ = v; }

  uint32_t getCommandBufferSize() const { return commandBufferSize_; }
  void setCommandBufferSize(uint32_t v) { commandBufferSize_ = v; }

  int64_t getEventId() const { return eventId_; }
  void setEventId(int64_t v) { eventId_ = v; }

  bool getMultiCoreSync() const { return multiCoreSync_; }
  void setMultiCoreSync(bool v) { multiCoreSync_ = v; }

private:
  int64_t coreId_ = 0;
  const HardwareInfo *hwInfo_ = nullptr;
  uint64_t cmdBufferAddr_ = 0;
  int64_t cmdOffsetInConst_ = 0;
  uint32_t commandBufferSize_ = 0;
  int64_t eventId_ = 0;
  bool multiCoreSync_ = false;
};

/// One collected trigger: identity + info (+ optional already-emitted bytes).
class TriggerNbgEntry {
public:
  TriggerNbgEntry() = default;

  TriggerNbgEntry(int64_t ordinal, Operation *op, Location loc,
                  TriggerLoadState state)
      : ordinal_(ordinal), op_(op), loc_(loc), state_(std::move(state)) {}

  /// Convenience: take loc from op (op must be non-null).
  TriggerNbgEntry(int64_t ordinal, Operation *op, TriggerLoadState state)
      : TriggerNbgEntry(ordinal, op, op->getLoc(), std::move(state)) {}

  int64_t getOrdinal() const { return ordinal_; }
  void setOrdinal(int64_t v) { ordinal_ = v; }

  Operation *getOp() const { return op_; }
  void setOp(Operation *v) { op_ = v; }

  Location getLoc() const { return loc_; }
  void setLoc(Location v) { loc_ = v; }

  const TriggerLoadState &getState() const { return state_; }
  TriggerLoadState &getState() { return state_; }
  void setState(TriggerLoadState v) { state_ = std::move(v); }

  const std::vector<uint8_t> &getLoadStateBuf() const { return loadStateBuf_; }
  std::vector<uint8_t> &getLoadStateBuf() { return loadStateBuf_; }
  void setLoadStateBuf(std::vector<uint8_t> v) {
    loadStateBuf_ = std::move(v);
  }

private:
  int64_t ordinal_ = -1;
  Operation *op_ = nullptr;
  Location loc_;
  TriggerLoadState state_;
  std::vector<uint8_t> loadStateBuf_;
};

/// Accumulates all NPU pieces before serialize → NBG blob.
class NpuCollector {
public:
  NpuCollector() = default;

  void clear() { triggers_.clear(); }

  int64_t nextOrdinal() const {
    return static_cast<int64_t>(triggers_.size());
  }

  void add(TriggerNbgEntry entry) { triggers_.push_back(std::move(entry)); }

  size_t size() const { return triggers_.size(); }
  bool empty() const { return triggers_.empty(); }

  ArrayRef<TriggerNbgEntry> getTriggers() const { return triggers_; }
  MutableArrayRef<TriggerNbgEntry> getTriggers() { return triggers_; }

  TriggerNbgEntry &getTrigger(size_t i) { return triggers_[i]; }
  const TriggerNbgEntry &getTrigger(size_t i) const { return triggers_[i]; }

  TriggerNbgEntry &back() { return triggers_.back(); }
  const TriggerNbgEntry &back() const { return triggers_.back(); }

private:
  llvm::SmallVector<TriggerNbgEntry, 8> triggers_;
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
