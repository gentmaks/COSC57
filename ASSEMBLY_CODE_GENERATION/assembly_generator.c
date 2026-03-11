/*
Author: Gent Maksutaj
Description: Backend for MiniC that performs local register allocation
and emits x86 32-bit assembly from LLVM IR.
*/
#include "assembly_generator.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <llvm-c/IRReader.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct RAState {
  // Instruction -> physical register index (0..2), or -1 if spilled.
  std::unordered_map<LLVMValueRef, int> regMap;
  // Instruction -> linear index within current basic block.
  std::unordered_map<LLVMValueRef, int> instIndex;
  // Value instruction -> [definition index, last-use index].
  std::unordered_map<LLVMValueRef, std::pair<int, int>> liveRange;
  // Value instruction -> local use count (spill heuristic).
  std::unordered_map<LLVMValueRef, int> useCount;
};

struct CodegenState {
  std::unordered_map<LLVMBasicBlockRef, std::string> bbLabels;
  std::unordered_map<LLVMValueRef, int> offsetMap;
  int localMem = 0;
};

static const char *kPhysRegs[] = {"%ebx", "%ecx", "%edx"};

static bool isAllocaInst(LLVMValueRef inst) {
  return LLVMGetInstructionOpcode(inst) == LLVMAlloca;
}

static bool isVoidValue(LLVMValueRef v) {
  return LLVMGetTypeKind(LLVMTypeOf(v)) == LLVMVoidTypeKind;
}

static bool isValueInstruction(LLVMValueRef inst) {
  return !isAllocaInst(inst) && !isVoidValue(inst);
}

static bool isConstInt(LLVMValueRef v) { return LLVMIsAConstantInt(v) != nullptr; }

static long long constIntValue(LLVMValueRef v) {
  return static_cast<long long>(LLVMConstIntGetSExtValue(v));
}

static int lookupReg(const RAState &state, LLVMValueRef v) {
  auto it = state.regMap.find(v);
  return (it == state.regMap.end()) ? -1 : it->second;
}

static const char *regName(int reg) {
  return (reg >= 0 && reg < 3) ? kPhysRegs[reg] : "%eax";
}

static bool isInBlock(const RAState &state, LLVMValueRef v) {
  return state.instIndex.find(v) != state.instIndex.end();
}

static bool endsAt(const RAState &state, LLVMValueRef v, int idx) {
  auto it = state.liveRange.find(v);
  return it != state.liveRange.end() && it->second.second == idx;
}

static bool overlap(const std::pair<int, int> &a, const std::pair<int, int> &b) {
  return a.first <= b.second && b.first <= a.second;
}

static void computeLiveness(LLVMBasicBlockRef bb, RAState &state) {
  state.instIndex.clear();
  state.liveRange.clear();
  state.useCount.clear();

  // Number instructions (excluding allocas) for local linear scan.
  int idx = 0;
  for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != nullptr;
       inst = LLVMGetNextInstruction(inst)) {
    if (isAllocaInst(inst)) {
      continue;
    }
    state.instIndex[inst] = idx++;
  }

  // Initialize each value's live range to its definition point.
  for (const auto &entry : state.instIndex) {
    LLVMValueRef inst = entry.first;
    int i = entry.second;
    if (isValueInstruction(inst)) {
      state.liveRange[inst] = {i, i};
      state.useCount[inst] = 0;
    }
  }

  // Extend end points for every use seen in the block.
  for (const auto &entry : state.instIndex) {
    LLVMValueRef inst = entry.first;
    int i = entry.second;
    unsigned operands = LLVMGetNumOperands(inst);
    for (unsigned j = 0; j < operands; j++) {
      LLVMValueRef op = LLVMGetOperand(inst, j);
      auto lr = state.liveRange.find(op);
      if (lr == state.liveRange.end()) {
        continue;
      }
      if (lr->second.second < i) {
        lr->second.second = i;
      }
      state.useCount[op] = state.useCount[op] + 1;
    }
  }
}

static LLVMValueRef findSpillCandidate(LLVMValueRef inst, const RAState &state) {
  auto thisRangeIt = state.liveRange.find(inst);
  if (thisRangeIt == state.liveRange.end()) {
    return nullptr;
  }
  const auto thisRange = thisRangeIt->second;

  // Pick overlapping live range with farthest end (latest next-use pressure).
  LLVMValueRef best = nullptr;
  int bestEnd = -1;
  for (const auto &entry : state.liveRange) {
    LLVMValueRef v = entry.first;
    if (v == inst || !overlap(entry.second, thisRange)) {
      continue;
    }
    int r = lookupReg(state, v);
    if (r < 0) {
      continue;
    }
    if (entry.second.second > bestEnd) {
      bestEnd = entry.second.second;
      best = v;
    }
  }
  return best;
}

static void releaseOperandRegs(LLVMValueRef inst, int idx, RAState &state,
                               std::vector<int> &available) {
  unsigned operands = LLVMGetNumOperands(inst);
  for (unsigned i = 0; i < operands; i++) {
    LLVMValueRef op = LLVMGetOperand(inst, i);
    if (!isInBlock(state, op) || !endsAt(state, op, idx)) {
      continue;
    }
    int r = lookupReg(state, op);
    if (r < 0) {
      continue;
    }
    bool already = false;
    for (int a : available) {
      if (a == r) {
        already = true;
        break;
      }
    }
    if (!already) {
      available.push_back(r);
    }
  }
}

static bool isArithOp(LLVMOpcode op) {
  return op == LLVMAdd || op == LLVMSub || op == LLVMMul;
}

static void allocateRegistersForFunction(LLVMValueRef fn, RAState &state) {
  state.regMap.clear();

  for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(fn); bb != nullptr;
       bb = LLVMGetNextBasicBlock(bb)) {
    computeLiveness(bb, state);
    std::vector<int> available = {0, 1, 2};

    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != nullptr;
         inst = LLVMGetNextInstruction(inst)) {
      if (isAllocaInst(inst)) {
        continue;
      }
      int idx = state.instIndex[inst];
      if (!isValueInstruction(inst)) {
        releaseOperandRegs(inst, idx, state, available);
        continue;
      }

      bool assigned = false;
      LLVMOpcode op = LLVMGetInstructionOpcode(inst);
      if (isArithOp(op) && LLVMGetNumOperands(inst) >= 2) {
        LLVMValueRef lhs = LLVMGetOperand(inst, 0);
        int lhsReg = lookupReg(state, lhs);
        if (lhsReg >= 0 && endsAt(state, lhs, idx)) {
          state.regMap[inst] = lhsReg;
          assigned = true;
          releaseOperandRegs(inst, idx, state, available);
        }
      }

      if (assigned) {
        continue;
      }

      if (!available.empty()) {
        int r = available.back();
        available.pop_back();
        state.regMap[inst] = r;
        releaseOperandRegs(inst, idx, state, available);
        continue;
      }

      LLVMValueRef spillV = findSpillCandidate(inst, state);
      if (spillV == nullptr) {
        state.regMap[inst] = -1;
        releaseOperandRegs(inst, idx, state, available);
        continue;
      }

      int spillReg = lookupReg(state, spillV);
      int spillUses = state.useCount[spillV];
      int thisUses = state.useCount[inst];
      int spillEnd = state.liveRange[spillV].second;
      int thisEnd = state.liveRange[inst].second;

      if (spillUses > thisUses || thisEnd > spillEnd) {
        state.regMap[inst] = -1;
      } else {
        state.regMap[inst] = spillReg;
        state.regMap[spillV] = -1;
      }
      releaseOperandRegs(inst, idx, state, available);
    }
  }
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

static void createBBLabels(LLVMValueRef fn, CodegenState &cg) {
  cg.bbLabels.clear();
  int id = 0;
  for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(fn); bb != nullptr;
       bb = LLVMGetNextBasicBlock(bb)) {
    cg.bbLabels[bb] = ".LBB" + std::to_string(id++);
  }
}

static void printDirectives(FILE *out, LLVMValueRef fn) {
  size_t len = 0;
  const char *name = LLVMGetValueName2(fn, &len);
  std::fprintf(out, ".text\n");
  std::fprintf(out, ".globl %.*s\n", static_cast<int>(len), name);
  std::fprintf(out, ".type %.*s, @function\n", static_cast<int>(len), name);
  std::fprintf(out, "%.*s:\n", static_cast<int>(len), name);
}

static void printFunctionEnd(FILE *out) {
  std::fprintf(out, "  leave\n");
  std::fprintf(out, "  ret\n");
}

static void mapOffset(CodegenState &cg, LLVMValueRef v, int offset) {
  cg.offsetMap[v] = offset;
}

static int getOffset(CodegenState &cg, LLVMValueRef v) {
  auto it = cg.offsetMap.find(v);
  if (it != cg.offsetMap.end()) {
    return it->second;
  }
  cg.localMem += 4;
  int newOffset = -cg.localMem;
  cg.offsetMap[v] = newOffset;
  return newOffset;
}

static void getOffsetMap(LLVMValueRef fn, CodegenState &cg) {
  cg.offsetMap.clear();
  cg.localMem = 4;

  LLVMValueRef param = nullptr;
  if (LLVMCountParams(fn) > 0) {
    param = LLVMGetParam(fn, 0);
    mapOffset(cg, param, 8);
  }

  // Build stack-slot offsets for allocas and propagated temporaries.
  for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(fn); bb != nullptr;
       bb = LLVMGetNextBasicBlock(bb)) {
    for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != nullptr;
         inst = LLVMGetNextInstruction(inst)) {
      LLVMOpcode op = LLVMGetInstructionOpcode(inst);
      if (op == LLVMAlloca) {
        cg.localMem += 4;
        mapOffset(cg, inst, -cg.localMem);
        continue;
      }
      if (op == LLVMStore) {
        LLVMValueRef src = LLVMGetOperand(inst, 0);
        LLVMValueRef dst = LLVMGetOperand(inst, 1);
        if (param != nullptr && src == param) {
          mapOffset(cg, dst, getOffset(cg, src));
        } else {
          mapOffset(cg, src, getOffset(cg, dst));
        }
        continue;
      }
      if (op == LLVMLoad) {
        LLVMValueRef src = LLVMGetOperand(inst, 0);
        mapOffset(cg, inst, getOffset(cg, src));
      }
    }
  }
}

static void emitMovOperandToReg(FILE *out, LLVMValueRef v, const char *dstReg,
                                const RAState &ra, CodegenState &cg) {
  if (isConstInt(v)) {
    std::fprintf(out, "  movl $%lld, %s\n", constIntValue(v), dstReg);
    return;
  }
  int r = lookupReg(ra, v);
  if (r >= 0) {
    const char *srcReg = regName(r);
    if (std::strcmp(srcReg, dstReg) != 0) {
      std::fprintf(out, "  movl %s, %s\n", srcReg, dstReg);
    }
    return;
  }
  std::fprintf(out, "  movl %d(%%ebp), %s\n", getOffset(cg, v), dstReg);
}

static void emitBinOpRhs(FILE *out, LLVMValueRef rhs, const char *opAsm,
                         const char *dstReg, const RAState &ra,
                         CodegenState &cg) {
  if (isConstInt(rhs)) {
    std::fprintf(out, "  %s $%lld, %s\n", opAsm, constIntValue(rhs), dstReg);
    return;
  }
  int r = lookupReg(ra, rhs);
  if (r >= 0) {
    std::fprintf(out, "  %s %s, %s\n", opAsm, regName(r), dstReg);
    return;
  }
  std::fprintf(out, "  %s %d(%%ebp), %s\n", opAsm, getOffset(cg, rhs), dstReg);
}

static const char *icmpJumpMnemonic(LLVMIntPredicate p) {
  switch (p) {
  case LLVMIntEQ:
    return "je";
  case LLVMIntNE:
    return "jne";
  case LLVMIntSGT:
    return "jg";
  case LLVMIntSGE:
    return "jge";
  case LLVMIntSLT:
    return "jl";
  case LLVMIntSLE:
    return "jle";
  default:
    return "jne";
  }
}

static void emitInstruction(FILE *out, LLVMValueRef inst, LLVMValueRef fn,
                            const RAState &ra, CodegenState &cg) {
  LLVMOpcode op = LLVMGetInstructionOpcode(inst);

  if (op == LLVMRet) {
    if (LLVMGetNumOperands(inst) == 1) {
      LLVMValueRef rv = LLVMGetOperand(inst, 0);
      if (isConstInt(rv)) {
        std::fprintf(out, "  movl $%lld, %%eax\n", constIntValue(rv));
      } else {
        int r = lookupReg(ra, rv);
        if (r >= 0) {
          std::fprintf(out, "  movl %s, %%eax\n", regName(r));
        } else {
          std::fprintf(out, "  movl %d(%%ebp), %%eax\n", getOffset(cg, rv));
        }
      }
    }
    std::fprintf(out, "  popl %%ebx\n");
    printFunctionEnd(out);
    return;
  }

  if (op == LLVMLoad) {
    int dst = lookupReg(ra, inst);
    if (dst >= 0) {
      LLVMValueRef ptr = LLVMGetOperand(inst, 0);
      std::fprintf(out, "  movl %d(%%ebp), %s\n", getOffset(cg, ptr),
                   regName(dst));
    }
    return;
  }

  if (op == LLVMStore) {
    LLVMValueRef src = LLVMGetOperand(inst, 0);
    LLVMValueRef dstPtr = LLVMGetOperand(inst, 1);
    LLVMValueRef param = (LLVMCountParams(fn) > 0) ? LLVMGetParam(fn, 0) : nullptr;
    if (param != nullptr && src == param) {
      return;
    }

    int dstOff = getOffset(cg, dstPtr);
    if (isConstInt(src)) {
      std::fprintf(out, "  movl $%lld, %d(%%ebp)\n", constIntValue(src), dstOff);
      return;
    }

    int srcReg = lookupReg(ra, src);
    if (srcReg >= 0) {
      std::fprintf(out, "  movl %s, %d(%%ebp)\n", regName(srcReg), dstOff);
    } else {
      std::fprintf(out, "  movl %d(%%ebp), %%eax\n", getOffset(cg, src));
      std::fprintf(out, "  movl %%eax, %d(%%ebp)\n", dstOff);
    }
    return;
  }

  if (op == LLVMCall) {
    std::fprintf(out, "  pushl %%ecx\n");
    std::fprintf(out, "  pushl %%edx\n");

    unsigned numOps = LLVMGetNumOperands(inst);
    unsigned argCount = (numOps > 0) ? (numOps - 1) : 0;
    if (argCount == 1) {
      LLVMValueRef arg = LLVMGetOperand(inst, 0);
      if (isConstInt(arg)) {
        std::fprintf(out, "  pushl $%lld\n", constIntValue(arg));
      } else {
        int r = lookupReg(ra, arg);
        if (r >= 0) {
          std::fprintf(out, "  pushl %s\n", regName(r));
        } else {
          std::fprintf(out, "  pushl %d(%%ebp)\n", getOffset(cg, arg));
        }
      }
    }

    LLVMValueRef callee = (numOps > 0) ? LLVMGetOperand(inst, numOps - 1) : nullptr;
    size_t len = 0;
    const char *calleeName = (callee != nullptr) ? LLVMGetValueName2(callee, &len) : "";
    std::fprintf(out, "  call %.*s\n", static_cast<int>(len), calleeName);

    if (argCount == 1) {
      std::fprintf(out, "  addl $4, %%esp\n");
    }
    std::fprintf(out, "  popl %%edx\n");
    std::fprintf(out, "  popl %%ecx\n");

    if (!isVoidValue(inst)) {
      int r = lookupReg(ra, inst);
      if (r >= 0) {
        std::fprintf(out, "  movl %%eax, %s\n", regName(r));
      } else {
        std::fprintf(out, "  movl %%eax, %d(%%ebp)\n", getOffset(cg, inst));
      }
    }
    return;
  }

  // Branch emission relies on icmp predicate produced earlier in the block.
  if (op == LLVMBr) {
    if (LLVMGetNumOperands(inst) == 1) {
      LLVMBasicBlockRef target = LLVMValueAsBasicBlock(LLVMGetOperand(inst, 0));
      std::fprintf(out, "  jmp %s\n", cg.bbLabels[target].c_str());
      return;
    }

    LLVMValueRef cond = LLVMGetOperand(inst, 0);
    LLVMBasicBlockRef trueBB = LLVMValueAsBasicBlock(LLVMGetOperand(inst, 2));
    LLVMBasicBlockRef falseBB = LLVMValueAsBasicBlock(LLVMGetOperand(inst, 1));

    LLVMIntPredicate pred = LLVMIntEQ;
    if (LLVMGetInstructionOpcode(cond) == LLVMICmp) {
      pred = LLVMGetICmpPredicate(cond);
    }
    std::fprintf(out, "  %s %s\n", icmpJumpMnemonic(pred),
                 cg.bbLabels[trueBB].c_str());
    std::fprintf(out, "  jmp %s\n", cg.bbLabels[falseBB].c_str());
    return;
  }

  if (isArithOp(op)) {
    int r = lookupReg(ra, inst);
    const char *dstReg = (r >= 0) ? regName(r) : "%eax";
    LLVMValueRef lhs = LLVMGetOperand(inst, 0);
    LLVMValueRef rhs = LLVMGetOperand(inst, 1);
    emitMovOperandToReg(out, lhs, dstReg, ra, cg);

    const char *opAsm = (op == LLVMAdd) ? "addl" : (op == LLVMSub) ? "subl" : "imull";
    emitBinOpRhs(out, rhs, opAsm, dstReg, ra, cg);

    if (r < 0) {
      std::fprintf(out, "  movl %%eax, %d(%%ebp)\n", getOffset(cg, inst));
    }
    return;
  }

  if (op == LLVMICmp) {
    int r = lookupReg(ra, inst);
    const char *dstReg = (r >= 0) ? regName(r) : "%eax";
    LLVMValueRef lhs = LLVMGetOperand(inst, 0);
    LLVMValueRef rhs = LLVMGetOperand(inst, 1);
    emitMovOperandToReg(out, lhs, dstReg, ra, cg);
    emitBinOpRhs(out, rhs, "cmpl", dstReg, ra, cg);
    return;
  }
}

void generateAssemblyForModule(LLVMModuleRef module, const char *outputPath) {
  FILE *out = std::fopen(outputPath, "w");
  if (!out) {
    std::fprintf(stderr, "Failed to open output file: %s\n", outputPath);
    return;
  }

  RAState ra;
  CodegenState cg;

  for (LLVMValueRef fn = LLVMGetFirstFunction(module); fn != nullptr;
       fn = LLVMGetNextFunction(fn)) {
    if (LLVMCountBasicBlocks(fn) == 0) {
      continue;
    }

    allocateRegistersForFunction(fn, ra);
    createBBLabels(fn, cg);
    getOffsetMap(fn, cg);

    printDirectives(out, fn);
    std::fprintf(out, "  pushl %%ebp\n");
    std::fprintf(out, "  movl %%esp, %%ebp\n");
    std::fprintf(out, "  subl $%d, %%esp\n", cg.localMem);
    std::fprintf(out, "  pushl %%ebx\n");

    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(fn); bb != nullptr;
         bb = LLVMGetNextBasicBlock(bb)) {
      std::fprintf(out, "%s:\n", cg.bbLabels[bb].c_str());
      for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst != nullptr;
           inst = LLVMGetNextInstruction(inst)) {
        emitInstruction(out, inst, fn, ra, cg);
      }
    }
    std::fprintf(out, "\n");
  }

  std::fclose(out);
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    std::fprintf(stderr, "Usage: %s <input.ll> [output.s]\n", argv[0]);
    return 1;
  }

  const char *inputPath = argv[1];
  const char *outputPath = (argc == 3) ? argv[2] : "out.s";

  LLVMModuleRef module = createLLVMModuleFromFile(inputPath);
  if (!module) {
    std::fprintf(stderr, "Failed to read module: %s\n", inputPath);
    return 1;
  }

  generateAssemblyForModule(module, outputPath);
  LLVMDisposeModule(module);
  return 0;
}
