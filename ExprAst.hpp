#ifndef PJPPROJECT_EXPRAST_HPP
#define PJPPROJECT_EXPRAST_HPP
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "llvm/ADT/APFloat.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/ADT/STLExtras.h"


#include "llvm/IR/LegacyPassManager.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"


using namespace llvm;

class PrototypeAST;


extern std::unique_ptr<LLVMContext> TheContext;

// insert instructions and create new ones
extern std::unique_ptr<IRBuilder<>> Builder;

// TheModule is an LLVM construct that contains functions and global variables.
extern std::unique_ptr<Module> TheModule;

// defined values in the current scope -> LLVM representation
extern std::map<std::string, AllocaInst *> NamedValues;

// static std::unique_ptr<KaleidoscopeJIT> TheJIT;
extern std::map<std::string, std::unique_ptr<PrototypeAST>> FunctionProtos;

extern std::unique_ptr<legacy::FunctionPassManager> TheFPM;

extern ExitOnError ExitOnErr;

extern std::set<std::string> constantVals;

Function *getFunction(std::string Name);
AllocaInst *CreateEntryBlockAlloca(Function *TheFunction, StringRef VarName);


/// ExprAST - Base class for all expression nodes.
class ExprAST {
public:
  virtual ~ExprAST() = default;
  virtual Value *codegen() = 0;

  virtual bool createGlobal();

  virtual const std::string getName() const;
};

/// NumberExprAST - Expression class for numeric literals like "1.0".
class NumberExprAST : public ExprAST {
  int Val;

public:
  // virtual ~NumberExprAST(){};
  NumberExprAST(int Val) : Val(Val) {}
  Value *codegen() override;
};

/// VariableExprAST - Expression class for referencing a variable, like "a".
class VariableExprAST : public ExprAST {
  std::string Name;

public:
  VariableExprAST(std::string Name) : Name(Name) {}
  Value *codegen() override;
  const std::string getName() const override;
};

/// BinaryExprAST - Expression class for a binary operator.
class BinaryExprAST : public ExprAST {
  char Op;
  std::unique_ptr<ExprAST> LHS, RHS;

public:
  BinaryExprAST(char op, std::unique_ptr<ExprAST> LHS,
                std::unique_ptr<ExprAST> RHS)
    : Op(op), LHS(std::move(LHS)), RHS(std::move(RHS)) {}
  Value *codegen() override;
};

/// CallExprAST - Expression class for function calls.
class CallExprAST : public ExprAST {
  std::string Callee;
  std::vector<std::unique_ptr<ExprAST>> Args;

public:
  CallExprAST(const std::string &Callee,
              std::vector<std::unique_ptr<ExprAST>> Args)
    : Callee(Callee), Args(std::move(Args)) {}
  Value *codegen() override;

};

/// PrototypeAST - This class represents the "prototype" for a function,
/// which captures its name, and its argument names (thus implicitly the number
/// of arguments the function takes).
class PrototypeAST {
  std::string Name;
  std::vector<std::string> Args;
  bool IsOperator;
  unsigned Precedence;  // Precedence if a binary op.

public:
  bool isProcedure;
  PrototypeAST(const std::string &name, std::vector<std::string> Args,
               bool IsOperator = false, unsigned Prec = 0, bool isProcedure = false)
  : Name(name), Args(std::move(Args)), IsOperator(IsOperator),
    Precedence(Prec), isProcedure(isProcedure) {}

  Function *codegen();
  const std::string & getName() const;

  bool isUnaryOp() const;
  bool isBinaryOp() const;

  char getOperatorName() const;

  unsigned getBinaryPrecedence() const;
};

/// FunctionAST - This class represents a function definition itself.
class FunctionAST {
  std::unique_ptr<PrototypeAST> Proto;
  std::vector<std::unique_ptr<ExprAST>> Body;

public:
  bool isProcedure;
  FunctionAST(std::unique_ptr<PrototypeAST> Proto,
              std::vector<std::unique_ptr<ExprAST>> Body,
              bool isProcedure = false)
    : Proto(std::move(Proto)), Body(std::move(Body)), isProcedure(isProcedure) {}
  Function *codegen();
};

/// IfExprAST - Expression class for if/then/else.
class IfExprAST : public ExprAST {
  std::unique_ptr<ExprAST> Cond, Else;
  std::vector<std::unique_ptr<ExprAST>> Then;

public:
  bool isElse;
  IfExprAST(std::unique_ptr<ExprAST> Cond, std::vector<std::unique_ptr<ExprAST>> Then,
            std::unique_ptr<ExprAST> Else, bool isElse)
    : Cond(std::move(Cond)), Then(std::move(Then)), 
      Else(std::move(Else)) , isElse(isElse ) {}
  
  Value *codegen() override;
};

/// ForExprAST - Expression class for for/in.
class ForExprAST : public ExprAST {
  bool to;
  std::string VarName;
  std::unique_ptr<ExprAST> Start, End, Step;
  std::vector<std::unique_ptr<ExprAST>> Body;
  

public:
  ForExprAST(const std::string &VarName, std::unique_ptr<ExprAST> Start,
             std::unique_ptr<ExprAST> End, std::unique_ptr<ExprAST> Step,
             std::vector<std::unique_ptr<ExprAST>> Body, bool to)
    : VarName(VarName), Start(std::move(Start)), End(std::move(End)), 
      Step(std::move(Step)), Body(std::move(Body)), to(to) {}

  Value *codegen() override;
};

/// UnaryExprAST - Expression class for a unary operator.
class UnaryExprAST : public ExprAST {
  char Opcode;
  std::unique_ptr<ExprAST> Operand;

public:
  UnaryExprAST(char Opcode, std::unique_ptr<ExprAST> Operand)
    : Opcode(Opcode), Operand(std::move(Operand)) {}

  Value *codegen() override;
};

/// VarExprAST - Expression class for var/in
class VarExprAST : public ExprAST {
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

public:
  VarExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames)
            : VarNames(std::move(VarNames)) {}

  Value *codegen() override;

  bool createGlobal() override;

};

class ConstExprAST : public ExprAST {
  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

public:
  ConstExprAST(std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames) 
                : VarNames(move(VarNames)) {}

  Value * codegen() override;

  bool createGlobal() override;

};

#endif //PJPPROJECT_EXPRAST_HPP