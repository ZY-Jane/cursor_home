// Copy into VipLiteCodegenPass.cc
// Replace collectNNNbg(...) with collectLoadStates (info only; NBG is serializeNbg later).

void runOnOperation() override {
  auto funcOp = getOperation();
  auto moduleOp = funcOp->getParentOfType<ModuleOp>();
  SmallVector<Schedule::DispatchCallOp> callOps(
      funcOp.getOps<Schedule::DispatchCallOp>());
  MLIRContext *ctx = &getContext();
  IRRewriter rewriter(ctx);

  for (auto callOp : callOps) {
    auto dispatchOp =
        moduleOp.lookupSymbol<Schedule::DispatchOp>(callOp.getCallee());
    AM_CHECK(dispatchOp, "Get dispatch callee fail: ", callOp.getCallee());

    llvm::outs() << "\n==============codegen============\n";
    dispatchOp.print(llvm::outs(), OpPrintingFlags().enableDebugInfo());
    llvm::outs() << "\n==============codegen============\n";

    NpuCollector npuTriggerCollector;
    if (failed(collectLoadStates(npuTriggerCollector, dispatchOp, hwInfo))) {
      signalPassFailure();
      return;
    }
  }
}
