// NpuCollect.cpp — collectNpuLaunch fills NpuLaunchCollector.

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

LogicalResult mlir::acuity::npu::collectNpuLaunchBlockOps(
    Block &block, NpuLaunchCollector &collector, const HardwareInfo &hwInfo,
    int64_t coreId) {
  for (Operation &op : block) {
    auto triggerOp = dyn_cast<nn::TriggerOp>(&op);
    if (!triggerOp)
      continue;

    // Add FFD/shader/DMA in this same loop (walk order = launch order).

    TriggerParams triggerParams;
    if (failed(fillTriggerParams(triggerParams, triggerOp, hwInfo, coreId)))
      return failure();

    std::vector<uint8_t> loadStateBuf;
    if (failed(emitLoadState(triggerParams, loadStateBuf)))
      return failure();

    NnLaunchEntry nn;
    nn.setOp(&op);
    nn.setLoadStateBuf(std::move(loadStateBuf));
    collector.add(std::move(nn));

    // FFD, when the real op exists:
    // FfdLaunchEntry ffd;
    // ffd.setOp(&op);
    // ffd.setBuf(...);
    // collector.add(std::move(ffd));
  }
  return success();
}

LogicalResult
mlir::acuity::npu::collectNpuLaunch(NpuLaunchCollector &collector,
                                    Operation *dispatchOp,
                                    const HardwareInfo &hwInfo,
                                    int64_t coreId) {
  if (!dispatchOp)
    return failure();
  for (Region &region : dispatchOp->getRegions()) {
    if (failed(collectNpuLaunchBlocks(region, collector, hwInfo, coreId)))
      return failure();
  }
  return success();
}

LogicalResult
mlir::acuity::npu::collectNpuLaunchBlocks(Region &region,
                                          NpuLaunchCollector &collector,
                                          const HardwareInfo &hwInfo,
                                          int64_t coreId) {
  for (Block &block : region) {
    if (failed(collectNpuLaunchBlockOps(block, collector, hwInfo, coreId)))
      return failure();
  }
  return success();
}

LogicalResult
mlir::acuity::npu::serializeNbg(const NpuLaunchCollector &collector,
                                std::vector<uint8_t> &nbgOut) {
  nbgOut.clear();
  // for (size_t i = 0; i < collector.size(); ++i) {
  //   const LaunchPiece &piece = collector.getEntry(i);
  //   switch (getLaunchKind(piece)) {
  //   case LaunchKind::NN:
  //     append(nbgOut, std::get<NnLaunchEntry>(piece).getLoadStateBuf());
  //     break;
  //   case LaunchKind::FFD:
  //     append(nbgOut, std::get<FfdLaunchEntry>(piece).getBuf());
  //     break;
  //   }
  // }
  (void)collector;
  return success();
}
