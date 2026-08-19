// NpuCollect.cpp — copy loadState at the module boundary; we own the copy.

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
mlir::acuity::npu::packLoadState(const TriggerParams &params,
                                 LoadStateView &view) {
  view = {};
  // Internal module allocates its own buffer and hands out {data, size}:
  // const HardwareInfo *hw = params.getHwInfo();
  // view.size = getLoadStateSize(...);
  // view.data = /* internal malloc / static scratch */;
  // fillLoadState(const_cast<uint8_t *>(view.data), view.size, ...);
  (void)params;
  return success();
}

void mlir::acuity::npu::releasePackedLoadState(LoadStateView view) {
  // Internal module frees what packLoadState allocated.
  // free(const_cast<uint8_t *>(view.data));
  (void)view;
}

LogicalResult
mlir::acuity::npu::copyLoadState(const TriggerParams &params,
                                 std::vector<uint8_t> &loadStateBuf) {
  LoadStateView view;
  if (failed(packLoadState(params, view)))
    return failure();

  // We allocate and copy. After this, the internal pointer is not ours.
  loadStateBuf.assign(view.data, view.data + view.size);
  releasePackedLoadState(view);
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
    if (failed(copyLoadState(params, loadStateBuf)))
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
