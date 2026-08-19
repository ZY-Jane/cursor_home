// Drop this into VipLiteCodegenPass::runOnOperation, in the callOp loop,
// in place of collectNNNbg(npuTriggerCollector, dispatchOp).

NpuCollector npuTriggerCollector;
if (failed(collectNbg(npuTriggerCollector, dispatchOp, hwInfo))) {
  signalPassFailure();
  return;
}
