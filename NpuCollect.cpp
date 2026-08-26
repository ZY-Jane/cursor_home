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

LogicalResult mlir::acuity::npu::fillWaitParams(WaitParams &params,
                                                nn::WaitOp wait,
                                                const HardwareInfo &hwInfo,
                                                int64_t coreId) {
  params = WaitParams();
  params.setCoreId(coreId);
  params.setHwInfo(&hwInfo);
  // params.setEventId(wait.getEventId());
  (void)wait;
  return success();
}

LogicalResult
mlir::acuity::npu::emitWaitLoadState(const WaitParams &params,
                                     std::vector<uint8_t> &loadStateBuf) {
  loadStateBuf.clear();
  // loadStateBuf = bytes of BigMma / tc_assembler::genWaitLoadState(params);
  (void)params;
  return success();
}

LogicalResult mlir::acuity::npu::collectNpuLaunchBlockOps(
    Block &block, NpuLaunchCollector &collector, const HardwareInfo &hwInfo,
    int64_t coreId) {
  for (Operation &op : block) {
    if (auto triggerOp = dyn_cast<nn::TriggerOp>(&op)) {
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
      continue;
    }

    if (auto waitOp = dyn_cast<nn::WaitOp>(&op)) {
      WaitParams waitParams;
      if (failed(fillWaitParams(waitParams, waitOp, hwInfo, coreId)))
        return failure();

      std::vector<uint8_t> loadStateBuf;
      if (failed(emitWaitLoadState(waitParams, loadStateBuf)))
        return failure();

      WaitLaunchEntry wait;
      wait.setOp(&op);
      wait.setLoadStateBuf(std::move(loadStateBuf));
      collector.add(std::move(wait));
      continue;
    }

    // FFD / shader / DMA: same loop, same add().
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
  //   case LaunchKind::Wait:
  //     append(nbgOut, std::get<WaitLaunchEntry>(piece).getLoadStateBuf());
  //     break;
  //   }
  // }
  (void)collector;
  return success();
}
