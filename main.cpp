#include "output.hpp"
#include "nodes.hpp"
#include "SemanticAnalyzer.hpp"
#include "CodeGenerator.hpp"
#include <iostream>

using namespace std;

// Extern from the bison-generated parser
extern int yyparse();
extern std::shared_ptr<ast::Node> program;

int main() {
    // Parse the input
    yyparse();
    
    // Run semantic analysis first
    SemanticAnalyzer semanticAnalyzer;
    program->accept(semanticAnalyzer);
    
    // If semantic analysis passed (no exit), generate code
    CodeGenerator codeGen;
    program->accept(codeGen);
    
    // Output LLVM IR
    std::cout << codeGen.cb;
    
    return 0;
}