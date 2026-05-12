#ifndef CODEGENERATOR_HPP
#define CODEGENERATOR_HPP

#include "visitor.hpp"
#include "output.hpp"
#include "nodes.hpp"
#include <string>
#include <vector>
#include <map>

typedef struct{
    std::string id;
    ast::BuiltInType type;
    int offset;
} VarAtts;

typedef struct{
    std::string id;
    ast::BuiltInType ret_type;
    std::vector<VarAtts> param_types;
    int offset;
} FuncEntry;

class CodeGenerator : public Visitor {
public:
    output::CodeBuffer cb;
    
    CodeGenerator();
    
    // Visitor methods
    void visit(ast::Num&) override;
    void visit(ast::NumB&) override;
    void visit(ast::String&) override;
    void visit(ast::Bool&) override;
    void visit(ast::ID&) override;
    void visit(ast::BinOp&) override;
    void visit(ast::RelOp&) override;
    void visit(ast::Not&) override;
    void visit(ast::And&) override;
    void visit(ast::Or&) override;
    void visit(ast::Type&) override {}
    void visit(ast::Cast&) override;
    void visit(ast::ExpList&) override;
    void visit(ast::Call&) override;
    void visit(ast::Statements&) override;
    void visit(ast::Break&) override;
    void visit(ast::Continue&) override;
    void visit(ast::Return&) override;
    void visit(ast::If&) override;
    void visit(ast::While&) override;
    void visit(ast::VarDecl&) override;
    void visit(ast::Assign&) override;
    void visit(ast::Formal&) override;
    void visit(ast::Formals&) override;
    void visit(ast::FuncDecl&) override;
    void visit(ast::Funcs&) override;
    void visit(ast::Block&) override;
    
private:
    // Symbol table
    std::vector<std::vector<FuncEntry>> symbol_table;
    std::vector<int> offset_table;
    
    // Maps variable name to its stack pointer register
    std::map<std::string, std::string> varPointers;
    
    // Current function name
    std::string currentFunction;
    
    // Loop labels for break/continue
    std::vector<std::string> breakLabels;
    std::vector<std::string> continueLabels;
    
    // Last computed register
    std::string lastReg;
    
    // Variable counter
    int varCounter;
    
    // Helper functions
    std::string getLLVMType(ast::BuiltInType type);
    void checkDivByZero(const std::string& divisor);
    void emitGlobals();
    
    FuncEntry* lookupSymbol(const std::string& name);
    void insertVariable(const std::string& name, ast::BuiltInType type, const int* offset = nullptr);
    void insertFunction(const std::string& name, ast::BuiltInType retType, const std::vector<VarAtts>& params);
    void enterScope();
    void exitScope();
};

#endif