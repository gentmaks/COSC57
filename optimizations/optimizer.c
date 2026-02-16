#include "optimizer.h"

#include <llvm-c/IRReader.h>

#include <cstdint>
#include <cstdio>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using StoreSet = std::unordered_set<int>;

struct ExprValue {
  LLVMValueRef canonicalInst;
  bool isLoad;
  LLVMValueRef loadPtr;
  unsigned loadPtrGeneration;
};

struct BlockData {
  LLVMBasicBlockRef bb;
  std::vector<int> preds;
  StoreSet gen;
  StoreSet kill;
  StoreSet in;
  StoreSet out;
};

static bool setEquals(const StoreSet &a, const StoreSet &b) {
  if (a.size() != b.size()) {
    return false;
  }
  for (int v : a) {
    if (b.find(v) == b.end()) {
      return false;
    }
  }
  return true;
}

static LLVMValueRef storePointerOperand(LLVMValueRef storeInst) {
  return LLVMGetOperand(storeInst, 1);
}

static LLVMValueRef storeValueOperand(LLVMValueRef storeInst) {
  return LLVMGetOperand(storeInst, 0);
}

static bool storesKillEachOther(LLVMValueRef storeA, LLVMValueRef storeB) {
  return storeA != storeB &&
         storePointerOperand(storeA) == storePointerOperand(storeB);
}

static bool isTerminatorInstruction(LLVMValueRef instruction) {
  return LLVMIsATerminatorInst(instruction) != nullptr;
}

static bool isIgnoredForCSE(LLVMValueRef instruction) {
  LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
  return opcode == LLVMCall || opcode == LLVMStore || opcode == LLVMAlloca ||
         isTerminatorInstruction(instruction);
}

static bool isSideEffectInstruction(LLVMValueRef instruction) {
  if (isTerminatorInstruction(instruction) ||
      LLVMGetInstructionOpcode(instruction) == LLVMCall) {
    return true;
  }

  switch (LLVMGetInstructionOpcode(instruction)) {
  case LLVMStore:
  case LLVMAlloca:
  case LLVMFence:
  case LLVMAtomicCmpXchg:
  case LLVMAtomicRMW:
  case LLVMVAArg:
  case LLVMLandingPad:
    return true;
  default:
    return false;
  }
}

static bool isConstantStore(LLVMValueRef storeInst) {
  return LLVMIsConstant(storeValueOperand(storeInst)) != 0;
}

static bool areSameConstantValue(LLVMValueRef a, LLVMValueRef b) {
  if (a == b) {
    return true;
  }
  return LLVMGetValueKind(a) == LLVMConstantIntValueKind &&
         LLVMGetValueKind(b) == LLVMConstantIntValueKind &&
         LLVMConstIntGetSExtValue(a) == LLVMConstIntGetSExtValue(b);
}

static int blockIndexOf(const std::vector<BlockData> &blocks,
                        LLVMBasicBlockRef bb) {
  for (size_t i = 0; i < blocks.size(); i++) {
    if (blocks[i].bb == bb) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

static int storeIndexOf(const std::vector<LLVMValueRef> &stores,
                        LLVMValueRef storeInst) {
  for (size_t i = 0; i < stores.size(); i++) {
    if (stores[i] == storeInst) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

static void eraseKilledStores(StoreSet &set, LLVMValueRef killer,
                              const std::vector<LLVMValueRef> &allStores) {
  for (auto it = set.begin(); it != set.end();) {
    if (storesKillEachOther(killer, allStores[*it])) {
      it = set.erase(it);
    } else {
      ++it;
    }
  }
}

static std::string makeExprKey(LLVMOpcode opcode, unsigned numOperands,
                               LLVMValueRef op0, LLVMValueRef op1) {
  return std::to_string(static_cast<unsigned>(opcode)) + "|" +
         std::to_string(numOperands) + "|" +
         std::to_string(static_cast<unsigned long long>(
             reinterpret_cast<uintptr_t>(op0))) +
         "|" +
         std::to_string(
             static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(op1)));
}

LLVMModuleRef createLLVMModuleFromFile(const char *filename) {
  char *errorMessage = nullptr;
  LLVMMemoryBufferRef memoryBuffer = nullptr;
  LLVMModuleRef module = nullptr;

  LLVMCreateMemoryBufferWithContentsOfFile(filename, &memoryBuffer,
                                           &errorMessage);
  if (errorMessage != nullptr) {
    std::fprintf(stderr, "%s\n", errorMessage);
    LLVMDisposeMessage(errorMessage);
    return nullptr;
  }

  LLVMParseIRInContext(LLVMGetGlobalContext(), memoryBuffer, &module,
                       &errorMessage);
  if (errorMessage != nullptr) {
    std::fprintf(stderr, "%s\n", errorMessage);
    LLVMDisposeMessage(errorMessage);
    return nullptr;
  }

  return module;
}

bool runCommonSubexpressionElimination(LLVMValueRef function) {
  bool changed = false;

  for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != nullptr;
       bb = LLVMGetNextBasicBlock(bb)) {
    std::unordered_map<std::string, ExprValue> table;
    std::unordered_map<LLVMValueRef, unsigned> ptrGeneration;

    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != nullptr;
         inst = LLVMGetNextInstruction(inst)) {
      if (LLVMGetInstructionOpcode(inst) == LLVMStore) {
        LLVMValueRef ptr = storePointerOperand(inst);
        ptrGeneration[ptr] = ptrGeneration[ptr] + 1;
        continue;
      }

      if (isIgnoredForCSE(inst)) {
        continue;
      }

      const unsigned numOperands = LLVMGetNumOperands(inst);
      if (numOperands == 0 || numOperands > 2) {
        continue;
      }

      const std::string key = makeExprKey(
          LLVMGetInstructionOpcode(inst), numOperands, LLVMGetOperand(inst, 0),
          (numOperands == 2) ? LLVMGetOperand(inst, 1) : nullptr);
      const bool isLoad = LLVMGetInstructionOpcode(inst) == LLVMLoad;
      bool replaced = false;

      auto it = table.find(key);
      if (it != table.end()) {
        const ExprValue &candidate = it->second;
        if (!isLoad) {
          LLVMReplaceAllUsesWith(inst, candidate.canonicalInst);
          replaced = changed = true;
        } else {
          LLVMValueRef ptr = LLVMGetOperand(inst, 0);
          unsigned currentGen = ptrGeneration[ptr];
          if (candidate.isLoad && candidate.loadPtr == ptr &&
              candidate.loadPtrGeneration == currentGen) {
            LLVMReplaceAllUsesWith(inst, candidate.canonicalInst);
            replaced = changed = true;
          }
        }
      }

      if (!replaced) {
        ExprValue value{inst, isLoad, nullptr, 0};
        if (isLoad) {
          value.loadPtr = LLVMGetOperand(inst, 0);
          value.loadPtrGeneration = ptrGeneration[value.loadPtr];
        }
        table[key] = value;
      }
    }
  }

  return changed;
}

bool runConstantFolding(LLVMValueRef function) {
  bool changed = false;

  for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != nullptr;
       bb = LLVMGetNextBasicBlock(bb)) {
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != nullptr;
         inst = LLVMGetNextInstruction(inst)) {
      LLVMOpcode opcode = LLVMGetInstructionOpcode(inst);
      if (LLVMGetNumOperands(inst) != 2 ||
          (opcode != LLVMAdd && opcode != LLVMSub && opcode != LLVMMul)) {
        continue;
      }

      LLVMValueRef lhs = LLVMGetOperand(inst, 0);
      LLVMValueRef rhs = LLVMGetOperand(inst, 1);
      if (!LLVMIsConstant(lhs) || !LLVMIsConstant(rhs)) {
        continue;
      }

      LLVMValueRef folded = nullptr;
      if (opcode == LLVMAdd) {
        folded = LLVMConstAdd(lhs, rhs);
      } else if (opcode == LLVMSub) {
        folded = LLVMConstSub(lhs, rhs);
      } else if (LLVMGetValueKind(lhs) == LLVMConstantIntValueKind &&
                 LLVMGetValueKind(rhs) == LLVMConstantIntValueKind) {
        long long product =
            LLVMConstIntGetSExtValue(lhs) * LLVMConstIntGetSExtValue(rhs);
        folded = LLVMConstInt(LLVMTypeOf(lhs),
                              static_cast<unsigned long long>(product), 1);
      }

      if (folded != nullptr) {
        LLVMReplaceAllUsesWith(inst, folded);
        changed = true;
      }
    }
  }

  return changed;
}

bool runDeadCodeElimination(LLVMValueRef function) {
  bool anyChange = false;
  bool changed = true;

  while (changed) {
    changed = false;
    std::vector<LLVMValueRef> toDelete;

    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != nullptr;
         bb = LLVMGetNextBasicBlock(bb)) {
      for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != nullptr;
           inst = LLVMGetNextInstruction(inst)) {
        if (LLVMGetFirstUse(inst) == nullptr &&
            !isSideEffectInstruction(inst)) {
          toDelete.push_back(inst);
        }
      }
    }

    for (LLVMValueRef inst : toDelete) {
      LLVMInstructionEraseFromParent(inst);
      changed = true;
      anyChange = true;
    }
  }

  return anyChange;
}

bool runStoreLoadConstantPropagation(LLVMValueRef function) {
  bool changed = false;

  std::vector<BlockData> blocks;
  for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(function); bb != nullptr;
       bb = LLVMGetNextBasicBlock(bb)) {
    blocks.push_back(BlockData{bb});
  }
  if (blocks.empty()) {
    return false;
  }

  std::vector<LLVMValueRef> allStores;
  for (const auto &block : blocks) {
    for (LLVMValueRef inst = LLVMGetFirstInstruction(block.bb); inst != nullptr;
         inst = LLVMGetNextInstruction(inst)) {
      if (LLVMGetInstructionOpcode(inst) == LLVMStore) {
        allStores.push_back(inst);
      }
    }
  }

  for (size_t i = 0; i < blocks.size(); i++) {
    LLVMValueRef term = LLVMGetBasicBlockTerminator(blocks[i].bb);
    if (!term) {
      continue;
    }
    for (unsigned s = 0; s < LLVMGetNumSuccessors(term); s++) {
      int succIndex = blockIndexOf(blocks, LLVMGetSuccessor(term, s));
      if (succIndex >= 0) {
        blocks[succIndex].preds.push_back(static_cast<int>(i));
      }
    }
  }

  for (auto &block : blocks) {
    for (LLVMValueRef inst = LLVMGetFirstInstruction(block.bb); inst != nullptr;
         inst = LLVMGetNextInstruction(inst)) {
      if (LLVMGetInstructionOpcode(inst) != LLVMStore) {
        continue;
      }
      int idx = storeIndexOf(allStores, inst);
      eraseKilledStores(block.gen, inst, allStores);
      block.gen.insert(idx);
    }
  }

  for (auto &block : blocks) {
    for (LLVMValueRef inst = LLVMGetFirstInstruction(block.bb); inst != nullptr;
         inst = LLVMGetNextInstruction(inst)) {
      if (LLVMGetInstructionOpcode(inst) != LLVMStore) {
        continue;
      }
      int idx = storeIndexOf(allStores, inst);
      for (size_t j = 0; j < allStores.size(); j++) {
        if (idx != static_cast<int>(j) &&
            storesKillEachOther(inst, allStores[j])) {
          block.kill.insert(static_cast<int>(j));
        }
      }
    }
    block.out = block.gen;
  }

  bool dataflowChanged = true;
  while (dataflowChanged) {
    dataflowChanged = false;
    for (auto &block : blocks) {
      StoreSet newIn;
      for (int pred : block.preds) {
        newIn.insert(blocks[pred].out.begin(), blocks[pred].out.end());
      }

      StoreSet newOut = newIn;
      for (int k : block.kill) {
        newOut.erase(k);
      }
      newOut.insert(block.gen.begin(), block.gen.end());

      if (!setEquals(block.in, newIn) || !setEquals(block.out, newOut)) {
        dataflowChanged = true;
      }
      block.in = std::move(newIn);
      block.out = std::move(newOut);
    }
  }

  for (auto &block : blocks) {
    StoreSet running = block.in;
    std::vector<LLVMValueRef> loadsToDelete;

    for (LLVMValueRef inst = LLVMGetFirstInstruction(block.bb); inst != nullptr;
         inst = LLVMGetNextInstruction(inst)) {
      LLVMOpcode opcode = LLVMGetInstructionOpcode(inst);
      if (opcode == LLVMStore) {
        eraseKilledStores(running, inst, allStores);
        running.insert(storeIndexOf(allStores, inst));
        continue;
      }

      if (opcode != LLVMLoad) {
        continue;
      }

      LLVMValueRef loadPtr = LLVMGetOperand(inst, 0);
      LLVMValueRef agreedConstant = nullptr;
      bool sawRelevantStore = false;
      bool safe = true;

      for (int idx : running) {
        LLVMValueRef storeInst = allStores[idx];
        if (storePointerOperand(storeInst) != loadPtr) {
          continue;
        }
        sawRelevantStore = true;
        if (!isConstantStore(storeInst)) {
          safe = false;
          break;
        }
        LLVMValueRef storedValue = storeValueOperand(storeInst);
        if (agreedConstant == nullptr) {
          agreedConstant = storedValue;
        } else if (!areSameConstantValue(agreedConstant, storedValue)) {
          safe = false;
          break;
        }
      }

      if (sawRelevantStore && safe && agreedConstant != nullptr) {
        LLVMReplaceAllUsesWith(inst, agreedConstant);
        loadsToDelete.push_back(inst);
        changed = true;
      }
    }

    for (LLVMValueRef load : loadsToDelete) {
      LLVMInstructionEraseFromParent(load);
    }
  }

  return changed;
}

bool optimizeFunction(LLVMValueRef function) {
  bool changed = false;

  changed |= runCommonSubexpressionElimination(function);
  changed |= runConstantFolding(function);
  changed |= runDeadCodeElimination(function);
  changed |= runStoreLoadConstantPropagation(function);

  bool iterationChanged = true;
  while (iterationChanged) {
    iterationChanged = false;
    iterationChanged |= runStoreLoadConstantPropagation(function);
    iterationChanged |= runConstantFolding(function);
    iterationChanged |= runDeadCodeElimination(function);
    changed |= iterationChanged;
  }

  return changed;
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    std::fprintf(stderr, "Usage: %s <input.ll> [output.ll]\n", argv[0]);
    return 1;
  }

  const char *inputPath = argv[1];
  const char *outputPath = (argc == 3) ? argv[2] : "optimized.ll";

  LLVMModuleRef module = createLLVMModuleFromFile(inputPath);
  if (!module) {
    std::fprintf(stderr, "Failed to read module: %s\n", inputPath);
    return 1;
  }

  for (LLVMValueRef function = LLVMGetFirstFunction(module);
       function != nullptr; function = LLVMGetNextFunction(function)) {
    if (LLVMCountBasicBlocks(function) > 0) {
      optimizeFunction(function);
    }
  }

  char *errorMessage = nullptr;
  if (LLVMPrintModuleToFile(module, outputPath, &errorMessage) != 0) {
    if (errorMessage) {
      std::fprintf(stderr, "%s\n", errorMessage);
      LLVMDisposeMessage(errorMessage);
    }
    return 1;
  }

  LLVMDisposeModule(module);
  return 0;
}
