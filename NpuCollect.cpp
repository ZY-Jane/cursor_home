// NpuCollect.cpp — collect phase using TriggerNbgEntry constructors.

#include "NpuCollect.h"

using namespace mlir;
using namespace mlir::acuity::npu;

LogicalResult
mlir::acuity::npu::fillTriggerLoadState(TriggerLoadState &out,
                                        nn::TriggerOp trigger,
                                        const HardwareInfo &hwInfo,
                                        int64_t coreId) {
  out = {};
  out.coreId = coreId;
  out.hwInfo = &hwInfo;

  // TODO: wire to your ODS getters, e.g.:
  // out.eventId = trigger.getEventId();
  // out.multiCoreSync = ...;
  // Value cmd = trigger.getCmd();
  // out.commandBufferSize = ...;
  // out.cmdOffsetInConst = ...;
  // out.cmdBufferAddr = 0;

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

    // Construct entry, then add to collector (ordinal = walk order).
    collector.add(TriggerNbgEntry(collector.nextOrdinal(), &op, std::move(state)));

    // Optional early emit into the entry we just added:
    // if (failed(emitLoadStateToVector(collector.triggers.back().state,
    //                                  collector.triggers.back().loadStateBuf)))
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
  // Bridge to non-MLIR module:
  // out = buildLoadState(toPlainInfo(state));
  (void)state;
  out.clear();
  return success();
}

LogicalResult
mlir::acuity::npu::serializeNbg(const NpuCollector &collector,
                                std::vector<uint8_t> &nbgOut) {
  nbgOut.clear();
  // for (const TriggerNbgEntry &e : collector.triggers) {
  //   append by e.ordinal / e.state.eventId / e.loadStateBuf
  // }
  (void)collector;
  return success();
}
