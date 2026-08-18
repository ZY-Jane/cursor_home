// NpuCollect.cpp — collect phase: fill info, emit buffer, store buffer.

#include "NpuCollect.h"

using namespace mlir;
using namespace mlir::acuity::npu;

LogicalResult
mlir::acuity::npu::fillTriggerLoadState(TriggerLoadState &info,
                                        nn::TriggerOp trigger,
                                        const HardwareInfo &hwInfo,
                                        int64_t coreId) {
  info = TriggerLoadState();
  info.setCoreId(coreId);

  // TODO: wire to your ODS getters, e.g.:
  // info.setEventId(trigger.getEventId());
  // info.setMultiCoreSync(...);
  // Value cmd = trigger.getCmd();
  // info.setCommandBufferSize(...);
  // info.setCmdOffsetInConst(...);
  // info.setCmdBufferAddr(0);
  // hwInfo is used here to derive sizes / flags, not stored on info.

  (void)trigger;
  (void)hwInfo;
  return success();
}

LogicalResult
mlir::acuity::npu::emitLoadState(const TriggerLoadState &info,
                                 std::vector<uint8_t> &loadStateBuf) {
  // Bridge to non-MLIR module, reading via getters:
  // LoadStateInfo packed;
  // packed.coreId = info.getCoreId();
  // packed.cmdBufferAddr = info.getCmdBufferAddr();
  // packed.commandBufferSize = info.getCommandBufferSize();
  // packed.eventId = info.getEventId();
  // packed.multiCoreSync = info.getMultiCoreSync();
  // loadStateBuf = buildLoadState(packed);
  (void)info;
  loadStateBuf.clear();
  return success();
}

LogicalResult
mlir::acuity::npu::collectNbgBlockOps(Block &block, NpuCollector &collector,
                                      const HardwareInfo &hwInfo,
                                      int64_t coreId) {
  for (Operation &op : block) {
    auto trigger = dyn_cast<nn::TriggerOp>(&op);
    if (!trigger)
      continue;

    TriggerLoadState info;
    if (failed(fillTriggerLoadState(info, trigger, hwInfo, coreId)))
      return failure();

    std::vector<uint8_t> loadStateBuf;
    if (failed(emitLoadState(info, loadStateBuf)))
      return failure();

    collector.add(TriggerNbgEntry(collector.nextOrdinal(), &op,
                                  std::move(loadStateBuf)));
  }
  return success();
}

LogicalResult
mlir::acuity::npu::collectNbgBlocks(Region &region, NpuCollector &collector,
                                    const HardwareInfo &hwInfo,
                                    int64_t coreId) {
  for (Block &block : region) {
    if (failed(collectNbgBlockOps(block, collector, hwInfo, coreId)))
      return failure();
  }
  return success();
}

LogicalResult
mlir::acuity::npu::serializeNbg(const NpuCollector &collector,
                                std::vector<uint8_t> &nbgOut) {
  nbgOut.clear();
  // for (const TriggerNbgEntry &e : collector.getTriggers()) {
  //   use e.getOrdinal(), e.getLoadStateBuf()
  // }
  (void)collector;
  return success();
}
