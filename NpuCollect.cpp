// NpuCollect.cpp — collect identities; LoadStateStore owns loadState bytes.

#include "NpuCollect.h"

using namespace mlir;
using namespace mlir::acuity::npu;

LogicalResult
mlir::acuity::npu::fillTriggerParams(TriggerParams &params,
                                     nn::TriggerOp trigger,
                                     const HardwareInfo &hwInfo,
                                     int64_t coreId) {
  params = TriggerParams();
  params.setCoreId(coreId);
  params.setHwInfo(&hwInfo);

  // TODO: wire to your ODS getters, e.g.:
  // params.setEventId(trigger.getEventId());
  // params.setMultiCoreSync(...);
  // Value cmd = trigger.getCmd();
  // params.setCommandBufferSize(...);
  // params.setCmdOffsetInConst(...);
  // params.setCmdBufferAddr(0);

  (void)trigger;
  return success();
}

LogicalResult LoadStateStore::emit(const TriggerParams &params, size_t &index) {
  std::vector<uint8_t> buf;
  // Bridge to non-MLIR packer, reading via getters:
  // const HardwareInfo *hw = params.getHwInfo();
  // LoadStateInfo packed;
  // packed.coreId = params.getCoreId();
  // packed.cmdBufferAddr = params.getCmdBufferAddr();
  // packed.commandBufferSize = params.getCommandBufferSize();
  // packed.eventId = params.getEventId();
  // packed.multiCoreSync = params.getMultiCoreSync();
  // buf = buildLoadState(packed, hw);
  (void)params;
  index = bufs_.size();
  bufs_.push_back(std::move(buf));
  return success();
}

LogicalResult
mlir::acuity::npu::collectNbgBlockOps(Block &block, NpuCollector &collector,
                                      LoadStateStore &store,
                                      const HardwareInfo &hwInfo,
                                      int64_t coreId) {
  for (Operation &op : block) {
    auto trigger = dyn_cast<nn::TriggerOp>(&op);
    if (!trigger)
      continue;

    TriggerParams params;
    if (failed(fillTriggerParams(params, trigger, hwInfo, coreId)))
      return failure();

    size_t bufIndex = 0;
    if (failed(store.emit(params, bufIndex)))
      return failure();

    collector.add(TriggerNbgEntry(static_cast<int64_t>(bufIndex), &op));
  }
  return success();
}

LogicalResult
mlir::acuity::npu::collectNbgBlocks(Region &region, NpuCollector &collector,
                                    LoadStateStore &store,
                                    const HardwareInfo &hwInfo,
                                    int64_t coreId) {
  for (Block &block : region) {
    if (failed(collectNbgBlockOps(block, collector, store, hwInfo, coreId)))
      return failure();
  }
  return success();
}

LogicalResult
mlir::acuity::npu::serializeNbg(const NpuCollector &collector,
                                const LoadStateStore &store,
                                std::vector<uint8_t> &nbgOut) {
  nbgOut.clear();
  // for (const TriggerNbgEntry &e : collector.getTriggers()) {
  //   ArrayRef<uint8_t> bytes = store.getBuf(e.getOrdinal());
  //   append bytes into nbgOut (copy into the NBG blob)
  // }
  (void)collector;
  (void)store;
  return success();
}
