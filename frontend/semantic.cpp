#include "semantic.h"

#include <cstdio>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using SymbolTable = std::unordered_map<std::string, int>;
using ScopeStack = std::vector<SymbolTable>;

int reportUndeclared(const char *name) {
  std::fprintf(stderr, "Semantic error: use of undeclared variable '%s'\n",
               name);
  return 1;
}

int reportDuplicate(const char *name) {
  std::fprintf(
      stderr,
      "Semantic error: duplicate declaration of variable '%s' in same scope\n",
      name);
  return 1;
}

int useVar(const char *name, const ScopeStack &scopes) {
  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    if (it->find(name) != it->end()) {
      return 0;
    }
  }
  return reportUndeclared(name);
}

int declareVar(const char *name, ScopeStack &scopes) {
  if (scopes.empty()) {
    scopes.emplace_back();
  }
  SymbolTable &current = scopes.back();
  if (current.find(name) != current.end()) {
    return reportDuplicate(name);
  }
  current.emplace(name, 1);
  return 0;
}

int analyzeNode(astNode *node, ScopeStack &scopes);
int analyzeBlock(astNode *node, ScopeStack &scopes, bool createScope);

int analyzeStmt(astNode *node, ScopeStack &scopes) {
  if (node == nullptr) {
    return 0;
  }
  if (node->type != ast_stmt) {
    return analyzeNode(node, scopes);
  }

  switch (node->stmt.type) {
  case ast_block: {
    return analyzeBlock(node, scopes, true);
  }
  case ast_asgn: {
    if (node->stmt.asgn.lhs != nullptr) {
      if (useVar(node->stmt.asgn.lhs->var.name, scopes) != 0) {
        return 1;
      }
    }
    return analyzeNode(node->stmt.asgn.rhs, scopes);
  }
  case ast_if: {
    if (analyzeNode(node->stmt.ifn.cond, scopes) != 0) {
      return 1;
    }
    if (analyzeNode(node->stmt.ifn.if_body, scopes) != 0) {
      return 1;
    }
    if (node->stmt.ifn.else_body != nullptr) {
      if (analyzeNode(node->stmt.ifn.else_body, scopes) != 0) {
        return 1;
      }
    }
    return 0;
  }
  case ast_while: {
    if (analyzeNode(node->stmt.whilen.cond, scopes) != 0) {
      return 1;
    }
    return analyzeNode(node->stmt.whilen.body, scopes);
  }
  case ast_call: {
    if (node->stmt.call.param != nullptr) {
      return analyzeNode(node->stmt.call.param, scopes);
    }
    return 0;
  }
  case ast_ret: {
    if (node->stmt.ret.expr != nullptr) {
      return analyzeNode(node->stmt.ret.expr, scopes);
    }
    return 0;
  }
  case ast_decl: {
    return declareVar(node->stmt.decl.name, scopes);
  }
  default:
    return 0;
  }
}

int analyzeNode(astNode *node, ScopeStack &scopes) {
  if (node == nullptr) {
    return 0;
  }

  switch (node->type) {
  case ast_prog:
    return analyzeNode(node->prog.func, scopes);
  case ast_func: {
    scopes.emplace_back();
    if (node->func.param != nullptr) {
      if (declareVar(node->func.param->var.name, scopes) != 0) {
        return 1;
      }
    }
    int result = analyzeBlock(node->func.body, scopes, false);
    scopes.pop_back();
    return result;
  }
  case ast_stmt:
    return analyzeStmt(node, scopes);
  case ast_var:
    return useVar(node->var.name, scopes);
  case ast_cnst:
    return 0;
  case ast_rexpr:
    if (analyzeNode(node->rexpr.lhs, scopes) != 0) {
      return 1;
    }
    return analyzeNode(node->rexpr.rhs, scopes);
  case ast_bexpr:
    if (analyzeNode(node->bexpr.lhs, scopes) != 0) {
      return 1;
    }
    return analyzeNode(node->bexpr.rhs, scopes);
  case ast_uexpr:
    return analyzeNode(node->uexpr.expr, scopes);
  case ast_extern:
    return 0;
  default:
    return 0;
  }
}

int analyzeBlock(astNode *node, ScopeStack &scopes, bool createScope) {
  if (node == nullptr) {
    return 0;
  }
  if (node->type != ast_stmt || node->stmt.type != ast_block) {
    return analyzeNode(node, scopes);
  }
  if (createScope) {
    scopes.emplace_back();
  }
  std::vector<astNode *> slist = *(node->stmt.block.stmt_list);
  for (astNode *child : slist) {
    if (analyzeNode(child, scopes) != 0) {
      return 1;
    }
  }
  if (createScope) {
    scopes.pop_back();
  }
  return 0;
}

} // namespace

int semanticAnalyze(astNode *root) {
  ScopeStack scopes;
  return analyzeNode(root, scopes);
}