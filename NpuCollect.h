// NpuCollect.h
// Collect-phase definitions for NPU NBG dump (C++ classes, private members).
//
// TriggerParams   = fields extracted from IR/hw (input to emit).
// loadStateBuf    = filled by emitLoadState via reference, then moved onto entry.
// TriggerEntry    = one collected trigger (identity + loadStateBuf).
// serializeNbg    = later: write NBG bytes from collected info.

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/MutableArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "mlir/IR/Block.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Region.h"
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

/// Parameters extracted from one TriggerOp + hw, used only to emit loadState.
/// hwInfo points at Pass-owned HardwareInfo; must outlive emitLoadState.
class TriggerParams {
public:
  TriggerParams() = default;

  TriggerParams(int64_t coreId, const HardwareInfo *hwInfo,
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

/// One collected trigger: identity + loadState bytes filled by emitLoadState.
class TriggerEntry {
public:
  TriggerEntry() = default;

  TriggerEntry(int64_t ordinal, Operation *op, Location loc,
                  std::vector<uint8_t> loadStateBuf)
      : ordinal_(ordinal), op_(op), loc_(loc),
        loadStateBuf_(std::move(loadStateBuf)) {}

  /// Convenience: take loc from op (op must be non-null).
  TriggerEntry(int64_t ordinal, Operation *op,
                  std::vector<uint8_t> loadStateBuf)
      : TriggerEntry(ordinal, op, op->getLoc(), std::move(loadStateBuf)) {}

  int64_t getOrdinal() const { return ordinal_; }
  void setOrdinal(int64_t v) { ordinal_ = v; }

  Operation *getOp() const { return op_; }
  void setOp(Operation *v) { op_ = v; }

  Location getLoc() const { return loc_; }
  void setLoc(Location v) { loc_ = v; }

  ArrayRef<uint8_t> getLoadStateBuf() const { return loadStateBuf_; }
  MutableArrayRef<uint8_t> getLoadStateBuf() { return loadStateBuf_; }
  void setLoadStateBuf(std::vector<uint8_t> v) {
    loadStateBuf_ = std::move(v);
  }

private:
  int64_t ordinal_ = -1;
  Operation *op_ = nullptr;
  Location loc_;
  std::vector<uint8_t> loadStateBuf_;
};

/// Accumulates collected trigger info; NBG blob is written later by serializeNbg.
class NpuCollector {
public:
  NpuCollector() = default;

  void clear() { triggers_.clear(); }

  int64_t nextOrdinal() const {
    return static_cast<int64_t>(triggers_.size());
  }

  void add(TriggerEntry entry) { triggers_.push_back(std::move(entry)); }

  size_t size() const { return triggers_.size(); }
  bool empty() const { return triggers_.empty(); }

  ArrayRef<TriggerEntry> getTriggers() const { return triggers_; }
  MutableArrayRef<TriggerEntry> getTriggers() { return triggers_; }

  TriggerEntry &getTrigger(size_t i) { return triggers_[i]; }
  const TriggerEntry &getTrigger(size_t i) const { return triggers_[i]; }

  TriggerEntry &back() { return triggers_.back(); }
  const TriggerEntry &back() const { return triggers_.back(); }

private:
  llvm::SmallVector<TriggerEntry, 8> triggers_;
};

//===----------------------------------------------------------------------===//
// Collect API — parallel to codegenBlocks / codegenBlockOps
//===----------------------------------------------------------------------===//

/// Read IR + hw into TriggerParams. Does not write loadState bytes.
LogicalResult fillTriggerParams(TriggerParams &params, nn::TriggerOp trigger,
                                const HardwareInfo &hwInfo,
                                int64_t coreId = 0);

/// Pack loadState into the caller's vector (out-param, filled in place).
LogicalResult emitLoadState(const TriggerParams &params,
                            std::vector<uint8_t> &loadStateBuf);

/// Walk a dispatch: collectNpu → collectNpuBlocks → collectNpuBlockOps.
/// Fills NpuCollector from IR (loadState now; cmdBuffer / coefData later).
/// Does not write the NBG blob.
LogicalResult collectNpu(NpuCollector &collector, Operation *dispatchOp,
                         const HardwareInfo &hwInfo, int64_t coreId = 0);

LogicalResult collectNpuBlocks(Region &region, NpuCollector &collector,
                               const HardwareInfo &hwInfo, int64_t coreId = 0);

LogicalResult collectNpuBlockOps(Block &block, NpuCollector &collector,
                                 const HardwareInfo &hwInfo,
                                 int64_t coreId = 0);

/// Later step: pack collected info into an NBG buffer.
LogicalResult serializeNbg(const NpuCollector &collector,
                           std::vector<uint8_t> &nbgOut);

} // namespace npu
} // namespace acuity
} // namespace mlir
