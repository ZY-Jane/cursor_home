// NpuCollect.cpp — emitLoadState fills loadStateBuf by reference.

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

LogicalResult
mlir::acuity::npu::emitLoadState(const TriggerParams &params,
                                 std::vector<uint8_t> &loadStateBuf) {
  loadStateBuf.clear();
  // Internal module writes into loadStateBuf (reference, same object as caller).
  // const HardwareInfo *hw = params.getHwInfo();
  // loadStateBuf = buildLoadState(..., hw);
  (void)params;
  return success();
}

LogicalResult
mlir::acuity::npu::collectNpuBlockOps(Block &block, NpuDescCollector &collector,
                                      const HardwareInfo &hwInfo,
                                      int64_t coreId) {
  for (Operation &op : block) {
    auto triggerOp = dyn_cast<nn::TriggerOp>(&op);
    if (!triggerOp)
      continue;

    // Later: also collect cmdBuffer / coefData from other ops into the collector.

    TriggerParams triggerParams;
    if (failed(fillTriggerParams(triggerParams, triggerOp, hwInfo, coreId)))
      return failure();

    std::vector<uint8_t> loadStateBuf;
    if (failed(emitLoadState(triggerParams, loadStateBuf)))
      return failure();

    collector.add(TriggerEntry(collector.nextOrdinal(), &op,
                                  std::move(loadStateBuf)));
  }
  return success();
}

LogicalResult
mlir::acuity::npu::collectNpu(NpuDescCollector &collector, Operation *dispatchOp,
                              const HardwareInfo &hwInfo, int64_t coreId) {
  if (!dispatchOp)
    return failure();
  for (Region &region : dispatchOp->getRegions()) {
    if (failed(collectNpuBlocks(region, collector, hwInfo, coreId)))
      return failure();
  }
  return success();
}

LogicalResult
mlir::acuity::npu::collectNpuBlocks(Region &region, NpuDescCollector &collector,
                                    const HardwareInfo &hwInfo,
                                    int64_t coreId) {
  for (Block &block : region) {
    if (failed(collectNpuBlockOps(block, collector, hwInfo, coreId)))
      return failure();
  }
  return success();
}

LogicalResult
mlir::acuity::npu::serializeNbg(const NpuDescCollector &collector,
                                std::vector<uint8_t> &nbgOut) {
  nbgOut.clear();
  // for (const TriggerEntry &e : collector.getTriggers()) {
  //   use e.getOrdinal(), e.getLoadStateBuf()
  // }
  (void)collector;
  return success();
}
