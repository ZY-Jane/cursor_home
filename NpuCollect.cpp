// NpuCollect.cpp — collect phase with private-member C++ classes.

#include "NpuCollect.h"

using namespace mlir;
using namespace mlir::acuity::npu;

LogicalResult
mlir::acuity::npu::fillTriggerLoadState(TriggerLoadState &out,
                                        nn::TriggerOp trigger,
                                        const HardwareInfo &hwInfo,
                                        int64_t coreId) {
  out = TriggerLoadState();
  out.setCoreId(coreId);
  out.setHwInfo(&hwInfo);

  // TODO: wire to your ODS getters, e.g.:
  // out.setEventId(trigger.getEventId());
  // out.setMultiCoreSync(...);
  // Value cmd = trigger.getCmd();
  // out.setCommandBufferSize(...);
  // out.setCmdOffsetInConst(...);
  // out.setCmdBufferAddr(0);

  (void)trigger;
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

    TriggerLoadState state;
    if (failed(fillTriggerLoadState(state, trigger, hwInfo, coreId)))
      return failure();

    collector.add(
        TriggerNbgEntry(collector.nextOrdinal(), &op, std::move(state)));

    // Optional early emit:
    // if (failed(emitLoadStateToVector(collector.back().getState(),
    //                                  collector.back().getLoadStateBuf())))
    //   return failure();
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
mlir::acuity::npu::emitLoadStateToVector(const TriggerLoadState &state,
                                         std::vector<uint8_t> &out) {
  // Bridge to non-MLIR module, reading via getters:
  // LoadStateInfo info;
  // info.coreId = state.getCoreId();
  // ...
  // out = buildLoadState(info);
  (void)state;
  out.clear();
  return success();
}

LogicalResult
mlir::acuity::npu::serializeNbg(const NpuCollector &collector,
                                std::vector<uint8_t> &nbgOut) {
  nbgOut.clear();
  // for (const TriggerNbgEntry &e : collector.getTriggers()) {
  //   use e.getOrdinal(), e.getState().getEventId(), e.getLoadStateBuf()
  // }
  (void)collector;
  return success();
}
