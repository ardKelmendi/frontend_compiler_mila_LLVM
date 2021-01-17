#include "Parser.hpp"

Parser::Parser() : MilaContext(), MilaBuilder(MilaContext), MilaModule("mila", MilaContext) {
}

int CurTok;
std::map<char, int> BinopPrecedence;

bool Parser::Parse() {
    getNextToken();
    return true;
}

const Module& Parser::Generate() {

    // create writeln function
    {
      std::vector<Type*> Ints(1, Type::getInt32Ty(MilaContext));
      FunctionType * FT = FunctionType::get(Type::getInt32Ty(MilaContext), Ints, false);
      Function * F = Function::Create(FT, Function::ExternalLinkage, "writeln", MilaModule);
      for (auto & Arg : F->args())
          Arg.setName("x");
    }

    // create main function
    {
      FunctionType * FT = FunctionType::get(Type::getInt32Ty(MilaContext), false);
      Function * MainFunction = Function::Create(FT, Function::ExternalLinkage, "main", MilaModule);

      // block
      BasicBlock * BB = BasicBlock::Create(MilaContext, "entry", MainFunction);
      MilaBuilder.SetInsertPoint(BB);

      // call writeln with value from lexel
      MilaBuilder.CreateCall(MilaModule.getFunction("writeln"), {
        ConstantInt::get(MilaContext, APInt(32, m_NumVal))
      });

      // return 0
      MilaBuilder.CreateRet(ConstantInt::get(Type::getInt32Ty(MilaContext), 0));
    }

    return this->MilaModule;
}

/*
 * Simple token buffer.
 * CurTok is the current token the parser is looking at
 * getNextToken reads another token from the lexer and updates curTok with ts result
 * Every function in the parser will assume that CurTok is the cureent token that needs to be parsed
 */

int getNextToken() {
    return CurTok = gettok();
}


 /// GetTokPrecedence - Get the precedence of the pending binary operator token.
int GetTokPrecedence() {
  // not asci or 2-character operator
  if (!isascii(CurTok)  && CurTok != tok_notequal  && CurTok != tok_lessequal  
                        && CurTok != tok_greaterequal  && CurTok != tok_assign 
                        && CurTok != tok_or)
    return -1;

  // Make sure it's a declared binop.
  int TokPrec = BinopPrecedence[CurTok];
  if (TokPrec <= 0)
    return -1;
  return TokPrec;
}

// create writeln function
void writeln(){
    std::vector<Type*> Ints(1, Type::getInt32Ty(*TheContext));
    FunctionType * FT = FunctionType::get(Type::getInt32Ty(*TheContext), Ints, false);
    Function * F = Function::Create(FT, Function::ExternalLinkage, "writeln", TheModule.get());
    for (auto & Arg : F->args())
        Arg.setName("x");
}

// create readln function
void readln(){
  std::vector<Type *> Ints(1, Type::getInt32PtrTy(*TheContext));

  FunctionType * FT = FunctionType::get(Type::getInt32Ty(*TheContext), Ints, false);
  Function * F = Function::Create(FT, Function::ExternalLinkage, "readln", TheModule.get());
  for (auto & Arg : F->args())
      Arg.setName("x");

}

/// LogError* - These are little helper functions for error handling.
std::unique_ptr<ExprAST> LogError(const char *Str) {
  fprintf(stderr, "Error: %s\n", Str);
  return nullptr;
}
std::unique_ptr<PrototypeAST> LogErrorP(const char *Str) {
  LogError(Str);
  return nullptr;
}

 std::unique_ptr<ExprAST> ParseExpression();

/// numberexpr ::= number
std::unique_ptr<ExprAST> ParseNumberExpr() {
  // std::cout  << m_NumVal << " <- opc===============\n";
  auto Result = std::make_unique<NumberExprAST>(m_NumVal);
  getNextToken(); // consume the number
  return std::move(Result);
}

/// parenexpr ::= '(' expression ')'
 std::unique_ptr<ExprAST> ParseParenExpr() {
  getNextToken(); // eat (.
  auto V = ParseExpression();
  if (!V)
    return nullptr;

  if (CurTok != ')')
    return LogError("expected ')'");
  getNextToken(); // eat ).
  return V;
}

/// identifierexpr
///   ::= identifier
///   ::= identifier '(' expression* ')'
 std::unique_ptr<ExprAST> ParseIdentifierExpr() {
  std::string IdName = m_IdentifierStr;

  getNextToken(); // eat identifier.

  if (CurTok != '(') // Simple variable ref.
    return std::make_unique<VariableExprAST>(IdName);

  // Call.
  getNextToken(); // eat (
  std::vector<std::unique_ptr<ExprAST>> Args;
  if (CurTok != ')') {
    while (true) {
      if (auto Arg = ParseExpression())
        Args.push_back(std::move(Arg));
      else
        return nullptr;

      if (CurTok == ')')
        break;

      if (CurTok != ',')
        return LogError("Expected ')' or ',' in argument list");
      getNextToken();
    }
  }
  // Eat the ')'.
  getNextToken();

  if (CurTok == ';')
    getNextToken();
  // Eat the ';'.

  return std::make_unique<CallExprAST>(IdName, std::move(Args));
}

/// ifexpr ::= 'if' expression 'then' expression 'else' expression
 std::unique_ptr<ExprAST> ParseIfExpr() {
  getNextToken();  // eat the if.

  // condition.
  auto Cond = ParseExpression();
  if (!Cond)
    return nullptr;

  if (CurTok != tok_then)
    return LogError("expected then");
  getNextToken();  // eat the then

  bool if_block = false;
  if (CurTok == tok_begin){
    if_block = true;
    getNextToken();
  }

  std::vector<std::unique_ptr<ExprAST>> thenBlock;

  while(CurTok != tok_end){
    if (auto E = ParseExpression()){
      if (CurTok == ';')
        getNextToken();
      thenBlock.push_back(move(E));

      // nothing left to parse
      if (!if_block) break; 
    } else  {
      if (CurTok == tok_end)  break;
      return nullptr;
    }
  }

  if (if_block && CurTok != tok_end)
    return LogError("expected end after begin (if block)");
  if (if_block) 
    getNextToken(); // eat end  

  bool isElse = false;
  if (CurTok == tok_else){
    isElse = true;
    getNextToken(); // eat else
    auto Else = ParseExpression();
    if (!Else)
      return nullptr;

    return std::make_unique<IfExprAST>(std::move(Cond), std::move(thenBlock),
                                        std::move(Else), isElse);
  }

  return std::make_unique<IfExprAST>(std::move(Cond), std::move(thenBlock));
}

/*
   for I := 1 to 20 do begin
     for J := 20 downto I do begin
*/

/// forexpr ::= 'for' identifier '=' expr ',' expr (',' expr)? 'in' expression
 std::unique_ptr<ExprAST> ParseForExpr() {
  getNextToken();  // eat the for.

  if (CurTok != tok_identifier)
    return LogError("expected identifier after for");

  std::string IdName = m_IdentifierStr;
  getNextToken();  // eat identifier.

  if (CurTok != tok_assign)
    return LogError("expected ':=' after for");
  getNextToken();  // eat ':='.


  auto Start = ParseExpression();
  if (!Start)
    return nullptr;

  bool to;
  if (CurTok == tok_to){
    to = true;
  } else if (CurTok == tok_downto) {
    to = false;
  } else  {
    return LogError("expected 'to' or 'downto' after for");
  }
  getNextToken(); // eat 'to' or 'downto;.

  auto End = ParseExpression();
  if (!End)
    return nullptr;

  // The step value is optional.
  std::unique_ptr<ExprAST> Step;
  if (CurTok == tok_do) {
    getNextToken(); // eat 'do'.
  } else LogError("expected 'do' after for");

  if (CurTok == tok_begin){
    getNextToken(); // eat 'begin'.
  } else LogError("expected 'begin' after for");

  std::vector<std::unique_ptr<ExprAST>> body;

  while (CurTok != tok_end) {
    if (auto expr = ParseExpression()) {
        if (CurTok == ';')
            getNextToken(); // eat ;.
        body.push_back(move(expr));
    }
    else {
        if (CurTok == tok_end) {
            getNextToken(); // eat end
            return std::make_unique<ForExprAST>(IdName, move(Start), move(End), move(Step), move(body), to);
        }
        return nullptr;
    }
  }
  if (CurTok == tok_end)
    getNextToken(); // eat end
  
  return std::make_unique<ForExprAST>(IdName, std::move(Start), std::move(End), 
                                      std::move(Step), std::move(body), to);
}


/*
var I, J, TEMP : integer;
*/
void HandleListVars( std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> & VarNames){
  bool firstIter = true;
  while (1) {

    if (CurTok != tok_identifier && !firstIter) break;

    if (!firstIter){
      
      
      std::string Name = m_IdentifierStr;
      getNextToken();  // eat identifier.

      // Read the optional initializer.
      std::unique_ptr<ExprAST> Init = nullptr;
      VarNames.emplace_back(Name, std::move(Init));
      // VarNames.push_back(std::make_pair(Name, std::move(Init)));
    }
        
    // End of var list, exit loop.
    if (CurTok != ',') 
      break;
    getNextToken(); // eat the ','.

    if (CurTok != tok_identifier){
      LogErrorP("expected identifier list after var");
      return;
    }
    firstIter = false;
  }

    if (CurTok != ':'){
      LogErrorP("expected ':' keyword after 'var'");
      return;
    } 

    getNextToken();  // eat ';'.

    if (CurTok != tok_integer){
      LogErrorP("expected integer keyword after 'var'");
      return;
    } 

    getNextToken();  // eat ';'.

    if (CurTok != ';'){
      LogErrorP("expected ';' keyword after 'var'");
      return;
    } 
    getNextToken();  // eat ';'.
}

/*
var
    n: integer;
    f: integer;
*/


void HandleSequenceVars( std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> & VarNames){

  bool firstIter = true;
  while (1) {

    if (CurTok != tok_identifier && !firstIter) break;

    if (!firstIter){
      std::string Name = m_IdentifierStr;
      getNextToken();  // eat identifier.

      // Read the optional initializer.
      std::unique_ptr<ExprAST> Init = nullptr;
      VarNames.emplace_back(Name, std::move(Init));
      // VarNames.push_back(std::make_pair(Name, std::move(Init)));
    }
        
    if (CurTok == ':') {
      getNextToken(); // eat the ':'.

      if (CurTok != tok_integer){
        LogErrorP("expected integer after : for var");
        return;
      }
      getNextToken(); // eat the 'integer'.

      if (CurTok != ';'){
        LogErrorP("expected ; after integer for var");
        return;
      }

      getNextToken(); // eat the ';'.

    } else {
      LogErrorP("expected ':' or',' after var");
      return;

    }
    firstIter = false;
  }
}

/// varexpr ::= 'var' identifier ('=' expression)?
//                    (',' identifier ('=' expression)?)* 'in' expression
 std::unique_ptr<ExprAST> ParseVarExpr() {
  getNextToken();  // eat the var.

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

  // At least one variable name is required.
  if (CurTok != tok_identifier)
    return LogError("expected identifier after var");

  std::string Name = m_IdentifierStr;
  getNextToken();  // eat identifier.

  std::unique_ptr<ExprAST> Init = nullptr;
  VarNames.emplace_back(Name, std::move(Init));
  // VarNames.push_back(std::make_pair(Name, std::move(Init)));
  
  if (CurTok == ':'){
    HandleSequenceVars(VarNames);
  }
  else if (CurTok == ','){

    HandleListVars(VarNames);
 
  } else  {
    return LogError("expected ',' or ':' after identifier for var");
  }
  
  return std::make_unique<VarExprAST>(std::move(VarNames));
}

std::unique_ptr<ExprAST> ParseConstExpr(){
  getNextToken(); // eat 'const'

  std::vector<std::pair<std::string, std::unique_ptr<ExprAST>>> VarNames;

  if (CurTok != tok_identifier)
    return LogError("expected identifier after 'const'");

  while(1){
    if (CurTok != tok_identifier) break;

    std::string Name = m_IdentifierStr;
    constantVals.insert(m_IdentifierStr);
    getNextToken(); // eat identifier

    std::unique_ptr<ExprAST> Init = nullptr;
    if (CurTok == '=') {
      getNextToken(); // eat the '='.

      Init = ParseExpression();
      if (!Init)
        return nullptr;
    }
    else {
        return LogError("Constant not initialized");
    }
    VarNames.emplace_back(Name, move(Init));
    // VarNames.push_back(std::make_pair(Name, std::move(Init)));

      // End of var list, exit loop.
      if (CurTok != ';')
        break;
      getNextToken(); // eat the ';'.

      if (CurTok == tok_function || CurTok == tok_begin || CurTok == tok_var)
          break;
      if (CurTok != tok_identifier)
          return LogError("expected identifier list after var99");
  }

  return std::make_unique<ConstExprAST>(move(VarNames));

}

/// primary
///   ::= begin / end
///   ::= identifierexpr
///   ::= numberexpr
///   ::= parenexpr
///   ::= ifexpr
///   ::= forexpr
///   ::= varexpr
 std::unique_ptr<ExprAST> ParsePrimary() {

  switch (CurTok) {
    default:
      if (CurTok == tok_begin || CurTok == tok_end) return nullptr;
        return LogError("unknown token when expecting an expression");
    case tok_identifier:
      return ParseIdentifierExpr();
    case tok_number:
      return ParseNumberExpr();
    case '(':
      return ParseParenExpr();
    case tok_if:
      return ParseIfExpr();
    case tok_for:
      return ParseForExpr();
    case tok_var:
      return ParseVarExpr();
  }
}

/// unary
///   ::= primary
///   ::= '!' unary
 std::unique_ptr<ExprAST> ParseUnary() {
  // If the current token is not an operator, it must be a primary expr.
  if (!isascii(CurTok) || CurTok == '(' || CurTok == ',')
    return ParsePrimary();

  // If this is a unary operator, read it.
  int Opc = CurTok;
  getNextToken();
  if (auto Operand = ParseUnary())
    return std::make_unique<UnaryExprAST>(Opc, std::move(Operand));
  return nullptr;
}

/// binoprhs
///   ::= ('+' primary)*
 std::unique_ptr<ExprAST> ParseBinOpRHS(int ExprPrec, std::unique_ptr<ExprAST> LHS) {
  // If this is a binop, find its precedence.
  while (true) {
    int TokPrec = GetTokPrecedence();

    // If this is a binop that binds at least as tightly as the current binop,
    // consume it, otherwise we are done.
    if (TokPrec < ExprPrec)
      return LHS;

    // Okay, we know this is a binop.
    int BinOp = CurTok;
    getNextToken(); // eat binop

    // Parse the primary expression after the binary operator.
    auto RHS = ParseUnary();
    if (!RHS)
      return nullptr;

    // If BinOp binds less tightly with RHS than the operator after RHS, let
    // the pending operator take RHS as its LHS.
    int NextPrec = GetTokPrecedence();
    if (TokPrec < NextPrec) {
      RHS = ParseBinOpRHS(TokPrec + 1, std::move(RHS));
      if (!RHS)
        return nullptr;
    }

    // Merge LHS/RHS.
    LHS = std::make_unique<BinaryExprAST>(BinOp, std::move(LHS), std::move(RHS));
  }
}

/// expression
///   ::= primary binoprhs
///
 std::unique_ptr<ExprAST> ParseExpression() {
  auto LHS = ParseUnary();
  if (!LHS)
    return nullptr;

  return ParseBinOpRHS(0, std::move(LHS));
}

/// prototype
///   ::= id '(' id* ')'
 std::unique_ptr<PrototypeAST> ParsePrototype() {
  bool isProcedure = false; // can be procedure of function
  if (CurTok == tok_procedure){
    isProcedure = true;
    getNextToken(); // eat 'procedure' or 'forward'
  }

  if (CurTok != tok_identifier)
    return LogErrorP("Expected function name in prototype");

  std::string FnName = m_IdentifierStr;
  getNextToken();

  if (CurTok != '(')
    return LogErrorP("Expected '(' in prototype");

  std::vector<std::string> ArgNames;      
  while (CurTok != ')' && getNextToken() == tok_identifier){
    ArgNames.push_back(m_IdentifierStr);
    getNextToken();
    if (CurTok == ':')
      getNextToken(); // eat :
    else
      return LogErrorP("Expected ':'");
    
    if (CurTok == tok_integer) 
      getNextToken(); // eat integer
    else 
      return LogErrorP("Expected 'integer'");
  } 
  if (CurTok != ')')
    return LogErrorP("Expected ')' in prototype");

  getNextToken(); // eat ')'.

  // : int 
  if (CurTok == ':'){
    if (CurTok == ':')
      getNextToken(); // eat :
    else
      return LogErrorP("Expected ':'");
    
    if (CurTok == tok_integer) 
      getNextToken(); // eat integer
    else 
      return LogErrorP("Expected 'integer'");
  }

  // change here procedure to true
  return std::make_unique<PrototypeAST>(FnName, std::move(ArgNames), false, 0, isProcedure);
}

/// definition ::= 'def' prototype expression

 std::unique_ptr<FunctionAST> ParseDefinition() {
  bool isProcedure = false;
  if (CurTok == tok_procedure)
    isProcedure = true;
  getNextToken(); // eat function.
  auto Proto = ParsePrototype();
  if (!Proto)
    return nullptr;

  // : int 
  if (CurTok == ':'){
    if (CurTok == ':')
      getNextToken(); // eat :
    else{
      LogErrorP("Expected ':'");
      return nullptr;
    }
    
    if (CurTok == tok_integer) 
      getNextToken(); // eat integer
    else {
      LogErrorP("Expected 'integer'");
      return nullptr;
    }
  }

  if (CurTok == ';')
    getNextToken();
  // if (CurTok == ':')
    // getNextToken(); // eat ;

  if (m_IdentifierStr == "forward"){
    getNextToken(); // eat forward/
    if (CurTok == ';')
      getNextToken(); // eat ;
    return nullptr;
  }

  if (CurTok != tok_begin && CurTok != tok_var) {
    LogErrorP("Expected begin");
  }

  std::vector<std::unique_ptr<ExprAST>> astExprs;

  if (CurTok == tok_var)
    astExprs.push_back(ParseExpression());

  if (CurTok == tok_begin)
    getNextToken(); // eat begin;

  while(1){
    if (CurTok == tok_end) break;
    else if (auto E = ParseExpression()){
      if (CurTok == ';')
        getNextToken();
      astExprs.push_back(std::move(E));
    } else  {
      if (CurTok == tok_end) break;
      return nullptr;
    }
  }
  return std::make_unique<FunctionAST>(std::move(Proto), std::move(astExprs), isProcedure);  
}

/// toplevelexpr ::= expression

 std::unique_ptr<FunctionAST> ParseTopLevelExpr() {
  std::vector<std::unique_ptr<ExprAST>> vecBody;
  if (auto E = ParseExpression()) {
    // Make an anonymous proto.
    auto Proto = std::make_unique<PrototypeAST>("main",
                                                std::vector<std::string>());
    vecBody.push_back(std::move(E)); 
    return std::make_unique<FunctionAST>(std::move(Proto), std::move(vecBody), false);
  }
  return nullptr;
}

/// external ::= 'forward' prototype
 std::unique_ptr<PrototypeAST> ParseForward() {
  getNextToken(); // eat forward.
  return ParsePrototype();
}

//===----------------------------------------------------------------------===//
// Top-Level parsing
//===----------------------------------------------------------------------===//

void InitializeModuleAndPassManager() {
  // Open a new module.
  TheContext = std::make_unique<LLVMContext>();
  TheModule = std::make_unique<Module>("mila", *TheContext);

  // Create a new builder for the module.
  Builder = std::make_unique<IRBuilder<>>(*TheContext);

  // Create a new pass manager attached to it.
  TheFPM = std::make_unique<legacy::FunctionPassManager>(TheModule.get());

  // Promote allocas to registers.
  TheFPM->add(createPromoteMemoryToRegisterPass());
  // Do simple "peephole" optimizations and bit-twiddling optzns.
  TheFPM->add(createInstructionCombiningPass());
  // Reassociate expressions.
  TheFPM->add(createReassociatePass());
  // Eliminate Common SubExpressions.
  TheFPM->add(createGVNPass());
  // Simplify the control flow graph (deleting unreachable blocks, etc).
  TheFPM->add(createCFGSimplificationPass());

  TheFPM->doInitialization();
}


void HandleDefinition() {
  if (auto FnAST = ParseDefinition()) {
    if (auto *FnIR = FnAST->codegen()) {
      fprintf(stderr, "Read function definition:");
      // FnIR->print(errs());
      // fprintf(stderr, "\n");
      // InitializeModuleAndPassManager();
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

 void HandleForward() {
  if (auto ProtoAST = ParseForward()) {
    if (auto *FnIR = ProtoAST->codegen()) {
      fprintf(stderr, "Read extern: ");
      // FnIR->print(errs());
      // fprintf(stderr, "\n");
      //  FunctionProtos[ProtoAST->getName()] = std::move(ProtoAST);
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

 void HandleTopLevelExpression() {
  // Evaluate a top-level expression into an anonymous function.
  if (auto FnAST = ParseTopLevelExpr()) {
    // if (auto *FnIR = FnAST->codegen()) {
      // fprintf(stderr, "Read top-level expression:");
      // FnIR->print(errs());
      // fprintf(stderr, "\n");
      FnAST->codegen();
      // Remove the anonymous expression.
      // FnIR->eraseFromParent();
    // }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

void HandleVarGlobal(){
  if (auto FnAST = ParseVarExpr()) {
    if (auto result = FnAST->createGlobal()) {
        fprintf(stderr, "Read Var definition\n");
          //  TheModule->print(errs(), nullptr);
          //  fprintf(stderr, "\n");
    }
  }
  else {
      // Skip token for error recovery.
      getNextToken();
  }
}

void HandleConstVal(){
 if (auto FnAST = ParseConstExpr()) {
    if (auto result = FnAST->createGlobal()) {
        fprintf(stderr, "Read Const definition\n");
      //  TheModule->print(errs(), nullptr);
      //  fprintf(stderr, "\n");
    }
  } else {
    // Skip token for error recovery.
    getNextToken();
  }
}

/// top ::= definition | external | expression | ';'
void MainLoop() {
  while (true) {
    switch (CurTok) {
    case tok_eof:
      return;
    case ';': // ignore top-level semicolons.
      getNextToken();
      break;
    case tok_function:
      HandleDefinition();
      break;
    case tok_procedure:
      HandleDefinition();
      break;  
    case tok_forward:
      HandleForward();
      break;  
    case tok_var:
      HandleVarGlobal();
      break;
    case '.': // program ends
      return;
    case tok_end:
      getNextToken();
      break;
    case tok_const:
      HandleConstVal();
      break;
    default:
      HandleTopLevelExpression();
      break;
    }
  }
}

