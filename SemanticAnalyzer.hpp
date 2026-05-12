#ifndef SEMANTICANALYZER_HPP
#define SEMANTICANALYZER_HPP

#include "visitor.hpp"
#include "output.hpp"
#include "nodes.hpp"
#include <unordered_set>



typedef struct{
    std::string id;
    ast::BuiltInType type;
    int offset;
} VariableAtts;

typedef struct{
    std::string id;
    ast::BuiltInType ret_type;
    std::vector<VariableAtts> param_types;
    int offset;
} SymbolTableEntry;



class SemanticAnalyzer : public Visitor {
public:
    output::ScopePrinter scopep;

    SemanticAnalyzer();

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
void visit(ast::Block &node) override;

    bool isNumericType(ast::BuiltInType type);
    void validateBinaryOperands(std::shared_ptr<ast::Exp> left, std::shared_ptr<ast::Exp> right, int lineNumber);

    SymbolTableEntry* retrieveFunctionSymbol(const std::string& name, int line);
    void validateCallArguments(ast::Call &node, SymbolTableEntry* func);

    void processBranchStatement(std::shared_ptr<ast::Statement> branch);

    void checkFunctionReturnType(std::shared_ptr<ast::Type> retType);
    void verifyParameterIsUnique(const std::string& name, int line,std::unordered_set<std::string>& existingNames);
    std::vector<VariableAtts> buildFormalParametersList(std::shared_ptr<ast::Formals> formals);
    void validateBooleanOperands(std::shared_ptr<ast::Exp> left, std::shared_ptr<ast::Exp> right, int line);
    bool existsInCurrentScope(const std::string& name) ;


private:

    std::vector<std::vector<SymbolTableEntry>> symbol_table;
    std::vector<int> offset_table;
    unsigned int loopDepth;
    std::string curr_func;

    void declareVariable(std::string id, ast::BuiltInType type, const int* offset_ptr = nullptr);
    void declareFunction(std::string id, ast::BuiltInType return_type , std::vector<VariableAtts> params);

    void enterScope();
    void exitScope();

    SymbolTableEntry* lookupSymbol(std::string id);

    void collectFunctionSignatures(ast::Funcs& node);
    bool isAssignable(ast::BuiltInType dst, ast::BuiltInType src) {
        return (dst == src) ||
        (dst == ast::BuiltInType::INT && src == ast::BuiltInType::BYTE);
    }
// std::string typeToString(ast::BuiltInType t) {
//     switch (t) {
//          case ast::BuiltInType::INT: return "int";
//         case ast::BuiltInType::BYTE: return "byte";
//          case ast::BuiltInType::BOOL: return "bool";
//          case ast::BuiltInType::VOID: return "void";
//           case ast::BuiltInType::STRING: return "string";
//           default: return "META_UNDEFINED";
//       }
//   }

     std::string toString(ast::BuiltInType type) {
        switch (type) {
            case ast::BuiltInType::INT:
                return "INT";
            case ast::BuiltInType::BOOL:
                return "BOOL";
            case ast::BuiltInType::BYTE:
                return "BYTE";
            case ast::BuiltInType::VOID:
                return "VOID";
            case ast::BuiltInType::STRING:
                return "STRING";
            default:
                return "unknown";
        }
    }

std::vector<std::string> buildPrototype(SymbolTableEntry* func) {
    std::vector<std::string> res;
    for (auto& p : func->param_types) {
        if (p.type == ast::BuiltInType::VOID) break;
        res.push_back(toString(p.type));
    }
    return res;
}


};

#endif
