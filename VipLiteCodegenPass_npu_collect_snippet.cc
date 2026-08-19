// Paste into VipLiteCodegenPass.cc (replace collectNNNbg).
// Requires: #include "NpuCollect.h"  (adjust to your actual path)
//
// Chain: collectNbg → collectNbgBlocks → collectNbgBlockOps
//
// void runOnOperation() override {
//   auto funcOp = getOperation();
//   auto moduleOp = funcOp->getParentOfType<ModuleOp>();
//   SmallVector<Schedule::DispatchCallOp> callOps(
//       funcOp.getOps<Schedule::DispatchCallOp>());
//   ...
//   for (auto callOp : callOps) {
//     auto dispatchOp =
//         moduleOp.lookupSymbol<Schedule::DispatchOp>(callOp.getCallee());
//     AM_CHECK(dispatchOp, "Get dispatch callee fail: ", callOp.getCallee());
//
//     NpuCollector npuTriggerCollector;
//     // hwInfo: use your Pass member / getter (name may differ).
//     if (failed(collectNbg(npuTriggerCollector, dispatchOp, hwInfo))) {
//       signalPassFailure();
//       return;
//     }
//
//     // TODO: serializeNbg(npuTriggerCollector, nbgOut);
//   }
// }

#include "NpuCollect.h"

using namespace mlir;
using namespace mlir::acuity::npu;

// Example body for the loop in VipLiteCodegenPass::runOnOperation.
static LogicalResult collectOneDispatch(NpuCollector &collector,
                                        Operation *dispatchOp,
                                        const HardwareInfo &hwInfo) {
  return collectNbg(collector, dispatchOp, hwInfo);
}
