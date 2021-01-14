#ifndef PJPPROJECT_PARSER_HPP
#define PJPPROJECT_PARSER_HPP

#include "Lexer.hpp"
#include "ExprAst.hpp"

#include "llvm/IR/Instructions.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace llvm;

class Parser {
public:
    Parser();
    ~Parser() = default;

    bool Parse();             // parse
    const Module& Generate(); // generate

private:

    int CurTok;               // to keep the current token

    LLVMContext MilaContext;   // llvm context
    IRBuilder<> MilaBuilder;   // llvm builder
    Module MilaModule;         // llvm module


};

/// BinopPrecedence - This holds the precedence for each binary operator that is
/// defined.
extern std::map<char, int> BinopPrecedence;


/// CurTok/getNextToken - Provide a simple token buffer.  CurTok is the current
/// token the parser is looking at.  getNextToken reads another token from the
/// lexer and updates CurTok with its results.    
extern int CurTok;
int getNextToken();
int GetTokPrecedence();

std::unique_ptr<ExprAST> LogError(const char *Str);
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str);

 std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
 std::unique_ptr<ExprAST> ParseNumberExpr();
 std::unique_ptr<ExprAST> ParseParenExpr();
 std::unique_ptr<ExprAST> ParseIdentifierExpr();
 std::unique_ptr<ExprAST> ParsePrimary();
 std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS);
 std::unique_ptr<ExprAST> ParseExpression();
 std::unique_ptr<PrototypeAST> ParsePrototype();
 std::unique_ptr<FunctionAST> ParseDefinition();
 std::unique_ptr<FunctionAST> ParseTopLevelExpr();
 std::unique_ptr<PrototypeAST> ParseExtern();

 std::unique_ptr<ExprAST> ParseIfExpr();
 std::unique_ptr<ExprAST> ParsePrimary();

 std::unique_ptr<ExprAST> ParseForExpr();
 std::unique_ptr<ExprAST> ParseUnary();
 std::unique_ptr<ExprAST> ParseVarExpr();
 std::unique_ptr<ExprAST> ParseConstExpr();


void InitializeModuleAndPassManager();

 void HandleDefinition();
 void HandleExtern();
 void HandleTopLevelExpression();

void writeln();
void readln();

void HandleVarGlobal();
void HandleConstVal();


void MainLoop();





#endif //PJPPROJECT_PARSER_HPP
