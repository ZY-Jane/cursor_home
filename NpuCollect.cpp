// NpuCollect.cpp — sketch implementation for collect phase.
// Wire fillTriggerLoadState / emit / serialize to your real TriggerOp & VIP layout.

#include "NpuCollect.h"

#include "mlir/IR/BuiltinTypes.h"

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

  // TODO: map to your ODS getters, e.g.:
  // out.eventId = trigger.getEventId();
  // out.multiCoreSync = ... from attr / hwInfo;
  // Value cmd = trigger.getCmd();
  // out.commandBufferSize = getMemRefNumElementsOrBytes(cmd);
  // out.cmdOffsetInConst = getStaticViewOffset(cmd);
  // out.cmdBufferAddr = 0; // fill later in Pack

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
      continue; // same idea as skipInKernelFunction

    TriggerNbgEntry &e = collector.addTrigger();
    e.op = &op;
    e.loc = op.getLoc();

    if (failed(fillTriggerLoadState(e.state, trigger, hwInfo, coreId)))
      return failure();

    // Optional early emit:
    // if (failed(emitLoadStateToVector(e.state, e.loadStateBuf)))
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
  // Bridge to non-MLIR module, e.g.:
  // LoadStateInfo info = toPlainInfo(state);
  // out = buildLoadState(info);   // returns std::vector, grows inside
  // or: out.resize(getLoadStateSize(&info)); fillLoadState(out.data(), ...);
  (void)state;
  out.clear();
  return success();
}

LogicalResult
mlir::acuity::npu::serializeNbg(const NpuCollector &collector,
                                std::vector<uint8_t> &nbgOut) {
  nbgOut.clear();
  // Pseudo:
  // write header
  // for (const TriggerNbgEntry &e : collector.triggers) {
  //   const auto &bytes = e.loadStateBuf.empty()
  //       ? emit_temp(e.state) : e.loadStateBuf;
  //   append section keyed by e.ordinal / e.state.eventId
  // }
  (void)collector;
  return success();
}
