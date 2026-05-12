#include "SemanticAnalyzer.hpp"
#include "output.hpp"
#include <unordered_set>


bool SemanticAnalyzer::existsInCurrentScope(const std::string& name) {
    for (auto& entry : symbol_table.back()) {
        if (entry.id == name) {
            return true;
        }
    }
    return false;
}

SemanticAnalyzer::SemanticAnalyzer(){
    symbol_table.push_back(std::vector<SymbolTableEntry>());
    offset_table.push_back(0);
    loopDepth = 0;
    VariableAtts printArg = {"param", ast::BuiltInType::STRING, 0};
    declareFunction("print", ast::BuiltInType::VOID, {printArg});

    VariableAtts printiArg = {"param", ast::BuiltInType::INT, 0};
    declareFunction("printi", ast::BuiltInType::VOID, {printiArg});
}

void SemanticAnalyzer::visit(ast::Num &node){
    node.type = ast::BuiltInType::INT;
}

void SemanticAnalyzer::visit(ast::NumB &node){
    if(node.value < 0 || node.value > 255) {
        output::errorByteTooLarge(node.line, node.value);
    }
    node.type = ast::BuiltInType::BYTE;
}

void SemanticAnalyzer::visit(ast::String &node)
{
    node.type = ast::BuiltInType::STRING;
}

void SemanticAnalyzer::visit(ast::Bool &node)
{
    node.type = ast::BuiltInType::BOOL;
}

void SemanticAnalyzer::visit(ast::ID &node)
{
    SymbolTableEntry* symbolInfo = lookupSymbol(node.value);
    
    if(!symbolInfo)
    {
        node.type = ast::BuiltInType::META_UNDEFINED;
        return;
    }
    
    if(!symbolInfo->param_types.empty())
    {
        node.type = ast::BuiltInType::META_FUNCTION;
    }
    else
    {
        node.type = symbolInfo->ret_type;
    }
}



// Helper function to validate expression nodes and report errors
static void validateExpression(std::shared_ptr<ast::Exp> expr, int errorLine = 0){
    if(!expr) return;
    
    int reportLine = (errorLine == 0) ? expr->line : errorLine;
    
    if(expr->type == ast::BuiltInType::META_UNDEFINED)
    {
        auto idNode = std::dynamic_pointer_cast<ast::ID>(expr);
        output::errorUndef(reportLine, idNode->value);
    }
    else if(expr->type == ast::BuiltInType::META_FUNCTION)
    {
        auto idNode = std::dynamic_pointer_cast<ast::ID>(expr);
        output::errorDefAsFunc(reportLine, idNode->value);
    }
}

bool SemanticAnalyzer::isNumericType(ast::BuiltInType type)
{
    return (type == ast::BuiltInType::INT || type == ast::BuiltInType::BYTE);
}

void SemanticAnalyzer::validateBinaryOperands(std::shared_ptr<ast::Exp> left,std::shared_ptr<ast::Exp> right, int lineNumber)
{
    validateExpression(left, 0);
    validateExpression(right, 0);
    
    if(!isNumericType(left->type) || !isNumericType(right->type))
    {
        output::errorMismatch(lineNumber);
    }
}

void SemanticAnalyzer::visit(ast::BinOp &node)
{
    // Process both operands
    node.left->accept(*this);
    node.right->accept(*this);
    
    validateBinaryOperands(node.left, node.right, node.line);
    
    if(node.left->type == ast::BuiltInType::BYTE && node.right->type == ast::BuiltInType::BYTE){
        node.type = ast::BuiltInType::BYTE;
    }
    else{
        node.type = ast::BuiltInType::INT;
    }
}

//selen added thissss
void SemanticAnalyzer::visit(ast::RelOp &node)
{
    node.type = ast::BuiltInType::BOOL;

    // Evaluate operands
    node.left->accept(*this);
    node.right->accept(*this);

    validateExpression(node.left, 0);
    validateExpression(node.right, 0);

    bool leftValid = (node.left->type == ast::BuiltInType::INT || node.left->type == ast::BuiltInType::BYTE);
    bool rightValid = (node.right->type == ast::BuiltInType::INT || node.right->type == ast::BuiltInType::BYTE);

    if(!leftValid || !rightValid){
        output::errorMismatch(node.line);
    }
}

void SemanticAnalyzer::visit(ast::Not &node){ 
    node.exp->accept(*this);
    validateExpression(node.exp, 0);

    if(node.exp->type != ast::BuiltInType::BOOL)
    {
        output::errorMismatch(node.line);
    }
    node.type = ast::BuiltInType::BOOL;
}

void SemanticAnalyzer::validateBooleanOperands(std::shared_ptr<ast::Exp> left, std::shared_ptr<ast::Exp> right, int line)
{
    validateExpression(left, 0);
    validateExpression(right, 0);
    
    if(left->type != ast::BuiltInType::BOOL || right->type != ast::BuiltInType::BOOL)
    {
        output::errorMismatch(line);
    }
}

void SemanticAnalyzer::visit(ast::And &node)
{
    node.left->accept(*this);
    node.right->accept(*this);
    validateBooleanOperands(node.left, node.right, node.line);
    node.type = ast::BuiltInType::BOOL;
}

void SemanticAnalyzer::visit(ast::Or &node)
{
    node.left->accept(*this);
    node.right->accept(*this);
    validateBooleanOperands(node.left, node.right, node.line);
    node.type = ast::BuiltInType::BOOL;
}

void SemanticAnalyzer::visit(ast::Cast &node)
{
    node.exp->accept(*this);
    node.target_type->accept(*this);
    validateExpression(node.exp, 0);

    ast::BuiltInType sourceType = node.exp->type;
    ast::BuiltInType destinationType = node.target_type->type;


    bool sourceValid = (sourceType == ast::BuiltInType::INT ||sourceType == ast::BuiltInType::BYTE);
    bool targetValid = (destinationType == ast::BuiltInType::INT || destinationType == ast::BuiltInType::BYTE);

    if(!sourceValid || !targetValid)
    {
        output::errorMismatch(node.line);
    }
    node.type = destinationType;
}

void SemanticAnalyzer::visit(ast::ExpList &node){
    for(auto &expression : node.exps)
    {
        expression->accept(*this);
    }
}



//this took forever dont touch any function related to CALL
SymbolTableEntry* SemanticAnalyzer::retrieveFunctionSymbol(const std::string& name, int line){
    SymbolTableEntry* symbol = lookupSymbol(name);
    
    if(!symbol) {
        output::errorUndefFunc(line, name);
    }

    if(symbol->param_types.empty()) {
        output::errorDefAsVar(line, name);
    }
    
    return symbol;
}

void SemanticAnalyzer::validateCallArguments(ast::Call &node, SymbolTableEntry* func){
    auto prototype = buildPrototype(func);
    size_t argCount = node.args->exps.size();

    // First, check for undefined variables or functions used as variables in arguments
    for(size_t i = 0; i < argCount; ++i) {
        auto& arg = node.args->exps[i];
        if(arg->type == ast::BuiltInType::META_UNDEFINED) {
            auto idNode = std::dynamic_pointer_cast<ast::ID>(arg);
            if(idNode) {
                output::errorUndef(node.line, idNode->value);
            }
        }
        else if(arg->type == ast::BuiltInType::META_FUNCTION) {
            auto idNode = std::dynamic_pointer_cast<ast::ID>(arg);
            if(idNode) {
                output::errorDefAsFunc(node.line, idNode->value);
            }
        }
    }

    if(argCount != prototype.size()) {
        output::errorPrototypeMismatch(node.line, node.func_id->value, prototype);
    }

    for(size_t i = 0; i < argCount; ++i) {
        ast::BuiltInType provided = node.args->exps[i]->type;
        ast::BuiltInType expected = func->param_types[i].type;
        
        if(!isAssignable(expected, provided)) {
            output::errorPrototypeMismatch(node.line, node.func_id->value, prototype);
        }
    }
}

void SemanticAnalyzer::visit(ast::Call &node){
    node.func_id->accept(*this);
    SymbolTableEntry* functionEntry = retrieveFunctionSymbol(node.func_id->value, node.line);
    node.args->accept(*this);
    validateCallArguments(node, functionEntry);
    node.type = functionEntry->ret_type;
}


void SemanticAnalyzer::visit(ast::Statements &node){
    for(auto &stmt : node.statements) {
        stmt->accept(*this);
    }
}


void SemanticAnalyzer::visit(ast::Break &node){
    if(loopDepth == 0) {
        output::errorUnexpectedBreak(node.line);
    }
}

void SemanticAnalyzer::visit(ast::Continue &node){
    if(loopDepth == 0) {
        output::errorUnexpectedContinue(node.line);
    }
}

void SemanticAnalyzer::visit(ast::Return &node){
    SymbolTableEntry* currentFunction = lookupSymbol(curr_func);
    if(!currentFunction) return;
    if(!node.exp) {
        //cout<<"hererrerere";
        if(currentFunction->ret_type != ast::BuiltInType::VOID)
            output::errorMismatch(node.line);
        return;
    }

    if(currentFunction->ret_type == ast::BuiltInType::VOID)
        output::errorMismatch(node.line);

    node.exp->accept(*this);
    validateExpression(node.exp, 0);

    if(!isAssignable(currentFunction->ret_type, node.exp->type))
        output::errorMismatch(node.line);
}

void SemanticAnalyzer::processBranchStatement(std::shared_ptr<ast::Statement> branch){
    bool isBlock = (dynamic_cast<ast::Statements*>(branch.get()) != nullptr);
    
    if(isBlock) {
        enterScope();
        branch->accept(*this);
        exitScope();
    } else {
        branch->accept(*this);
    }
}




//mayyy
// void SemanticAnalyzer::visit(ast::If &node){
//     node.condition->accept(*this);
//     validateExpression(node.condition, node.line);
//     if(node.condition->type != ast::BuiltInType::BOOL) 
//         output::errorMismatch(node.condition->line);
    
//     // Always create one scope for if
//     enterScope();
//     node.then->accept(*this);
//     exitScope();
    
//     if(node.otherwise) {
//         enterScope();
//         node.otherwise->accept(*this);
//         exitScope();
//     }
// }

// void SemanticAnalyzer::visit(ast::While &node){
//     node.condition->accept(*this);
//     validateExpression(node.condition, node.line);
//     if(node.condition->type != ast::BuiltInType::BOOL){
//         output::errorMismatch(node.condition->line);
//     }
    
//     loopDepth++;
//     enterScope();
//     node.body->accept(*this);
//     exitScope();
//     loopDepth--;
// }


void SemanticAnalyzer::visit(ast::If &node){
    node.condition->accept(*this);
    validateExpression(node.condition, node.line);
    if(node.condition->type != ast::BuiltInType::BOOL)
        // output::errorMismatch(node.line);
 output::errorMismatch(node.condition->line);
    // if/else always create a scope per specification
    enterScope();
    node.then->accept(*this);
    exitScope();

    if(node.otherwise) {
        enterScope();
        node.otherwise->accept(*this);
        exitScope();
    }
}

void SemanticAnalyzer::visit(ast::While &node){
    node.condition->accept(*this);
    validateExpression(node.condition, node.line);
    if(node.condition->type != ast::BuiltInType::BOOL)
        // output::errorMismatch(node.line);
 output::errorMismatch(node.condition->line);
    loopDepth++;
    // while always creates a scope per specification
    enterScope();
    node.body->accept(*this);
    exitScope();
    loopDepth--;
}


void SemanticAnalyzer::visit(ast::VarDecl &node)
{
    node.type->accept(*this);

    if (lookupSymbol(node.id->value)) {
        output::errorDef(node.id->line, node.id->value);
    }

    if (node.init_exp) {
        node.init_exp->accept(*this);
        validateExpression(node.init_exp, 0);

        if (!isAssignable(node.type->type, node.init_exp->type))
            output::errorMismatch(node.line);
    }

    declareVariable(node.id->value, node.type->type);
}


void SemanticAnalyzer::visit(ast::Assign &node){
    node.id->accept(*this);
    node.exp->accept(*this);

    validateExpression(node.exp, 0);
    
    if(node.exp->type == ast::BuiltInType::STRING)
    {
        output::errorMismatch(node.exp->line);
    }
    SymbolTableEntry* varInfo = lookupSymbol(node.id->value);
    if(!varInfo)
    {
        output::errorUndef(node.line, node.id->value);
    }
    else if(!varInfo->param_types.empty())
    {
        output::errorDefAsFunc(node.line, node.id->value);
    }
    
    if(!isAssignable(varInfo->ret_type, node.exp->type))
    {
        output::errorMismatch(node.line);
    }
}

void SemanticAnalyzer::visit(ast::Formal &node){
    node.type->accept(*this);
    node.id->accept(*this);
}

void SemanticAnalyzer::visit(ast::Formals &node){
    for(auto &formalParam : node.formals){
        formalParam->accept(*this);
    }
}

void SemanticAnalyzer::visit(ast::FuncDecl &node){
    enterScope();
    curr_func = node.id->value;

    SymbolTableEntry* funcInfo = lookupSymbol(node.id->value);
    for(auto& parameter : funcInfo->param_types) {
        if(parameter.type == ast::BuiltInType::VOID) break;
        declareVariable(parameter.id, parameter.type, &parameter.offset);
    }
    node.body->accept(*this);
    exitScope();
}

///////////////////////////////////////////////////
//make sure this works later!!! dont FORGETTTTTT :)))) I DID :)
void SemanticAnalyzer::checkFunctionReturnType(std::shared_ptr<ast::Type> retType){
    retType->accept(*this);
    if(retType->type == ast::BuiltInType::STRING) {
        output::errorMismatch(retType->line);
    }
}

void SemanticAnalyzer::verifyParameterIsUnique(const std::string& name, int line,std::unordered_set<std::string>& existingNames){
    if(existingNames.count(name)){
        output::errorDef(line, name);
    }
    if(lookupSymbol(name)){
        output::errorDef(line, name);
    }
    existingNames.insert(name);
}

std::vector<VariableAtts> SemanticAnalyzer::buildFormalParametersList(std::shared_ptr<ast::Formals> formals){
    std::vector<VariableAtts> params;
    std::unordered_set<std::string> seenNames;
    int index = 0;
    
    for(auto& formal : formals->formals) {
        formal->type->accept(*this);
        const std::string& name = formal->id->value;
        
        verifyParameterIsUnique(name, formal->id->line, seenNames);
        
        params.push_back({
            name,
            formal->type->type,
            -(index + 1)
        });
        index++;
    }
    if(params.empty()) {
        params.push_back({"dummy", ast::BuiltInType::VOID, 0});
    }
    return params;
}

void SemanticAnalyzer::collectFunctionSignatures(ast::Funcs& node){
    for(auto& funcDecl : node.funcs) {
        std::string funcName = funcDecl->id->value;
        
        if(lookupSymbol(funcName)) {
            output::errorDef(funcDecl->id->line, funcName);
        }
        checkFunctionReturnType(funcDecl->return_type);
        std::vector<VariableAtts> parameters = buildFormalParametersList(funcDecl->formals);
        declareFunction(funcName, funcDecl->return_type->type, parameters);
    }
}
void SemanticAnalyzer::visit(ast::Funcs &node){
    collectFunctionSignatures(node);
    SymbolTableEntry* mainFunc = lookupSymbol("main");
    if(!mainFunc || mainFunc->ret_type != ast::BuiltInType::VOID ||mainFunc->param_types.size() != 1 ||mainFunc->param_types[0].type != ast::BuiltInType::VOID) {
        output::errorMainMissing();
    }

    for(auto& funcDecl : node.funcs) {
        funcDecl->accept(*this);
    }
}
///////////////////////////////////////

void SemanticAnalyzer::declareVariable(std::string varName, ast::BuiltInType varType, const int* customOffset){
    int varOffset;
    
    if(customOffset) {
        varOffset = *customOffset;
    } else {
        varOffset = offset_table.back();
    }

    SymbolTableEntry newEntry = {varName, varType, std::vector<VariableAtts>(), varOffset};
    symbol_table.back().push_back(newEntry);
    scopep.emitVar(varName, varType, varOffset);
    
    if(!customOffset) {
        offset_table.back()++;
    }
}

void SemanticAnalyzer::declareFunction(std::string funcName, ast::BuiltInType retType, std::vector<VariableAtts> parameters){
    SymbolTableEntry newEntry = {funcName, retType, parameters, 0};
    symbol_table.front().push_back(newEntry);
    
    std::vector<ast::BuiltInType> paramTypeList;
    for(auto &param : parameters) {
        if(param.type == ast::BuiltInType::VOID) break;
        paramTypeList.push_back(param.type);
    }
    
    scopep.emitFunc(funcName, retType, paramTypeList);
}

void SemanticAnalyzer::enterScope(){
    symbol_table.push_back(std::vector<SymbolTableEntry>());
    offset_table.push_back(offset_table.back());
    scopep.beginScope();
}

void SemanticAnalyzer::exitScope(){
    symbol_table.pop_back();
    offset_table.pop_back();
    scopep.endScope();
}

SymbolTableEntry* SemanticAnalyzer::lookupSymbol(std::string symbolName){
    // Search from innermost scope (back) to outermost (front)
    for(auto it = symbol_table.rbegin(); it != symbol_table.rend(); ++it) {
        for(auto &entry : *it) {
            if(entry.id == symbolName) {
                return &entry;
            }
        }
    }
    return nullptr;
}
void SemanticAnalyzer::visit(ast::Block &node) {
    enterScope();
    node.statements->accept(*this);
    exitScope();
}
