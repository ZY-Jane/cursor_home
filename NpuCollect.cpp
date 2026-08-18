// NpuCollect.cpp — collect phase: fill params, emit buffer, store buffer.

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

  // TODO: wire to your ODS getters, e.g.:
  // params.setEventId(trigger.getEventId());
  // params.setMultiCoreSync(...);
  // Value cmd = trigger.getCmd();
  // params.setCommandBufferSize(...);
  // params.setCmdOffsetInConst(...);
  // params.setCmdBufferAddr(0);
  // hwInfo is used here to derive sizes / flags, not stored on params.

  (void)trigger;
  (void)hwInfo;
  return success();
}

LogicalResult
mlir::acuity::npu::emitLoadState(const TriggerParams &params,
                                 std::vector<uint8_t> &loadStateBuf) {
  // Bridge to non-MLIR module, reading via getters:
  // LoadStateInfo packed;
  // packed.coreId = params.getCoreId();
  // packed.cmdBufferAddr = params.getCmdBufferAddr();
  // packed.commandBufferSize = params.getCommandBufferSize();
  // packed.eventId = params.getEventId();
  // packed.multiCoreSync = params.getMultiCoreSync();
  // loadStateBuf = buildLoadState(packed);
  (void)params;
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

    TriggerParams params;
    if (failed(fillTriggerParams(params, trigger, hwInfo, coreId)))
      return failure();

    std::vector<uint8_t> loadStateBuf;
    if (failed(emitLoadState(params, loadStateBuf)))
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
