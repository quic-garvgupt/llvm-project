//===- BitcodeWriterPass.cpp - Bitcode writing pass -----------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// BitcodeWriterPass implementation.
//
//===----------------------------------------------------------------------===//

#include "llvm/Bitcode/BitcodeWriterPass.h"
#include "llvm/Analysis/ModuleSummaryAnalysis.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/PassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
using namespace llvm;

//TODO:
// write in detail about how it all comes together.
// clearly state the prolems we are starting to get inti run into
// some MCoptions for initializerecordstreamer
// for call chains - give exact c++ functions
// intention is to write in upstream about difficulty of making this work
// we either need MAM, or just pass the TM as arguments in first patch
// be super clear, where we will not have local target machine
// we also have difficulty assing target achine and MCOptions, difficulty with layering
// not posssibe without passing parameters at a bunch of diff places
// part of the issue is we are also running collectasmsymbols multiple time without reusing the result of previous invocation.

// proposal - either to discourse or github issues
// make it super clear, issues we run into trying to do this
// places where LTO is doing things badly, collectasmsymbols should not be called multiple times
// when we load bitcode in LTO, we call collectamsymbols, what was the point of doing it first time around
// intention is to write in upstream about difficulty of making this work


PreservedAnalyses BitcodeWriterPass::run(Module &M, ModuleAnalysisManager &AM) {
  M.removeDebugIntrinsicDeclarations();

  const ModuleSummaryIndex *Index =
      EmitSummaryIndex ? &(AM.getResult<ModuleSummaryIndexAnalysis>(M))
                       : nullptr;
  WriteBitcodeToFile(M, OS, ShouldPreserveUseListOrder, Index, EmitModuleHash);

  return PreservedAnalyses::all();
}

namespace {
  class WriteBitcodePass : public ModulePass {
    raw_ostream &OS; // raw_ostream to print on
    bool ShouldPreserveUseListOrder;

  public:
    static char ID; // Pass identification, replacement for typeid
    WriteBitcodePass() : ModulePass(ID), OS(dbgs()) {
      initializeWriteBitcodePassPass(*PassRegistry::getPassRegistry());
    }

    explicit WriteBitcodePass(raw_ostream &o, bool ShouldPreserveUseListOrder)
        : ModulePass(ID), OS(o),
          ShouldPreserveUseListOrder(ShouldPreserveUseListOrder) {
      initializeWriteBitcodePassPass(*PassRegistry::getPassRegistry());
    }

    StringRef getPassName() const override { return "Bitcode Writer"; }

    bool runOnModule(Module &M) override {
      M.removeDebugIntrinsicDeclarations();

      WriteBitcodeToFile(M, OS, ShouldPreserveUseListOrder, /*Index=*/nullptr,
                         /*EmitModuleHash=*/false);

      return false;
    }
    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.setPreservesAll();
    }
  };
}

char WriteBitcodePass::ID = 0;
INITIALIZE_PASS_BEGIN(WriteBitcodePass, "write-bitcode", "Write Bitcode", false,
                      true)
INITIALIZE_PASS_DEPENDENCY(ModuleSummaryIndexWrapperPass)
INITIALIZE_PASS_END(WriteBitcodePass, "write-bitcode", "Write Bitcode", false,
                    true)

ModulePass *llvm::createBitcodeWriterPass(raw_ostream &Str,
                                          bool ShouldPreserveUseListOrder) {
  return new WriteBitcodePass(Str, ShouldPreserveUseListOrder);
}

bool llvm::isBitcodeWriterPass(Pass *P) {
  return P->getPassID() == (llvm::AnalysisID)&WriteBitcodePass::ID;
}
