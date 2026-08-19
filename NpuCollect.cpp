// NpuCollect.cpp — packer returns vector; we move it onto TriggerNbgEntry.

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

FailureOr<std::vector<uint8_t>>
mlir::acuity::npu::packLoadState(const TriggerParams &params) {
  std::vector<uint8_t> buf;
  // Same as CL: internal builds std::string and returns it.
  // const HardwareInfo *hw = params.getHwInfo();
  // buf = buildLoadState(..., hw);
  (void)params;
  return buf;
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

    FailureOr<std::vector<uint8_t>> packed = packLoadState(params);
    if (failed(packed))
      return failure();

    collector.add(TriggerNbgEntry(collector.nextOrdinal(), &op,
                                  std::move(*packed)));
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
