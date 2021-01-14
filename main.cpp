#include "Parser.hpp"

#include <stdio.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <llvm/IR/Verifier.h>
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/LegacyPassManager.h"

#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

//Use tutorials in: https://llvm.org/docs/tutorial/

int main (int argc, char *argv[]) {
    
    // Install standard binary operators.
    // 1 is lowest precedence.
    BinopPrecedence['='] = 2;
    
    BinopPrecedence['>']                = 10;
    BinopPrecedence[tok_greaterequal]   = 10;
    BinopPrecedence['<']                = 10;
    BinopPrecedence[tok_lessequal]      = 10;
    BinopPrecedence[tok_notequal]       = 10;

    BinopPrecedence['+'] = 20;
    BinopPrecedence['-'] = 20;
    
    BinopPrecedence['*'] = 40;
    BinopPrecedence['/'] = 40; // highest.
 
    // eat 'program' 'name' ';'
    for (int i = 0; i < 4; i++) 
        getNextToken();

    InitializeModuleAndPassManager();
    // create writeln and readln functions
    readln();
    writeln();

    MainLoop();

    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    // InitializeNativeTarget();
    // InitializeNativeTargetAsmPrinter();
    // InitializeNativeTargetAsmParser();
    

    auto TargetTriple = sys::getDefaultTargetTriple();
    TheModule->setTargetTriple(TargetTriple);
    
    std::string Error;
    auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

    // Print an error and exit if we couldn't find the requested target.
    // This generally occurs if we've forgotten to initialise the
    // TargetRegistry or we have a bogus target triple.
    if (!Target) {
        errs() << "errors: " << Error;
        return 1;
    }


    // generic cpu without any additional features, options or relocation model
    auto CPU = "generic";
    auto Features = "";

    Function * mainFunction = getFunction("main");
    Builder->CreateRet(Builder->getInt32(0));
    verifyFunction(*mainFunction);

    TargetOptions opt;
    auto RM = Optional<Reloc::Model>();
    auto TheTargetMachine =
    Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);

    TheModule->setDataLayout(TheTargetMachine->createDataLayout());

    auto Filename = "output.o";
    std::error_code EC;
    raw_fd_ostream dest(Filename, EC, sys::fs::OF_None);

    if (EC) {
        errs() << "Could not open file: " << EC.message();
        return 1;
    }

    legacy::PassManager pass;
    auto FileType = CGFT_ObjectFile;

    if (TheTargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        errs() << "TheTargetMachine can't emit a file of this type";
        return 1;
    }


        // Promote allocas to registers.
    pass.add(createPromoteMemoryToRegisterPass());
    // Do simple "peephole" optimizations and bit-twiddling optzns.
    pass.add(createInstructionCombiningPass());
    // Reassociate expressions.
    pass.add(createReassociatePass());
    // Reassociate expressions.
    pass.add(createJumpThreadingPass());
    pass.add(createCFGSimplificationPass());
    TheModule->print(errs(), nullptr);

    pass.run(*TheModule);
    dest.flush();

    // outs() << "Wrote " << Filename << "\n";

    system("clang  output.o fce.c -o output.out");

    return 0;

/*
    Parser parser;

    if (!parser.Parse()) {
        return 1;
    }

    parser.Generate().print(outs(), nullptr);
    
    return 0;
    */
}
