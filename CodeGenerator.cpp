#include "CodeGenerator.hpp"
#include <sstream>

CodeGenerator::CodeGenerator() : varCounter(0) {
    emitGlobals();
    
    // Initialize global scope
    symbol_table.push_back(std::vector<FuncEntry>());
    offset_table.push_back(0);
    
    // Add print function
    VarAtts printParam = {"param", ast::BuiltInType::STRING, 0};
    insertFunction("print", ast::BuiltInType::VOID, {printParam});
    
    // Add printi function
    VarAtts printiParam = {"param", ast::BuiltInType::INT, 0};
    insertFunction("printi", ast::BuiltInType::VOID, {printiParam});
}

void CodeGenerator::emitGlobals() {
    // Declare external functions
    cb.emit("declare i32 @printf(i8*, ...)");
    cb.emit("declare void @exit(i32)");
    cb.emit("");
    
    // Define format strings
    cb.emit("@.int_specifier = constant [4 x i8] c\"%d\\0A\\00\"");
    cb.emit("@.str_specifier = constant [4 x i8] c\"%s\\0A\\00\"");
    cb.emit("@.div_zero_msg = constant [23 x i8] c\"Error division by zero\\00\"");
    cb.emit("");
    
    // Define printi function
    cb.emit("define void @printi(i32) {");
    cb.emit("    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0");
    cb.emit("    call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)");
    cb.emit("    ret void");
    cb.emit("}");
    cb.emit("");
    
    // Define print function
    cb.emit("define void @print(i8*) {");
    cb.emit("    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0");
    cb.emit("    call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)");
    cb.emit("    ret void");
    cb.emit("}");
    cb.emit("");
}

std::string CodeGenerator::getLLVMType(ast::BuiltInType type) {
    switch(type) {
        case ast::BuiltInType::INT: return "i32";
        case ast::BuiltInType::BYTE: return "i32";  // Store as i32
        case ast::BuiltInType::BOOL: return "i32";  // Store as i32
        case ast::BuiltInType::VOID: return "void";
        case ast::BuiltInType::STRING: return "i8*";
        default: return "i32";
    }
}

void CodeGenerator::checkDivByZero(const std::string& divisor) {
    std::string checkReg = cb.freshVar();
    cb << checkReg << " = icmp eq i32 " << divisor << ", 0" << std::endl;
    
    std::string errorLabel = cb.freshLabel();
    std::string okLabel = cb.freshLabel();
    
    cb << "br i1 " << checkReg << ", label " << errorLabel << ", label " << okLabel << std::endl;
    
    cb.emitLabel(errorLabel);
    std::string strPtr = cb.freshVar();
    cb << strPtr << " = getelementptr [23 x i8], [23 x i8]* @.div_zero_msg, i32 0, i32 0" << std::endl;
    cb.emit("call void @print(i8* " + strPtr + ")");
    cb.emit("call void @exit(i32 1)");
    cb.emit("ret void");
    
    cb.emitLabel(okLabel);
}

void CodeGenerator::visit(ast::Num &node) {
    lastReg = cb.freshVar();
    cb << lastReg << " = add i32 0, " << node.value << std::endl;
}

void CodeGenerator::visit(ast::NumB &node) {
    lastReg = cb.freshVar();
    cb << lastReg << " = add i32 0, " << node.value << std::endl;
}

void CodeGenerator::visit(ast::String &node) {
    std::string strVar = cb.emitString(node.value);
    lastReg = cb.freshVar();
    int len = node.value.length() + 1;
    cb << lastReg << " = getelementptr [" << len << " x i8], [" 
       << len << " x i8]* " << strVar << ", i32 0, i32 0" << std::endl;
}

void CodeGenerator::visit(ast::Bool &node) {
    lastReg = cb.freshVar();
    cb << lastReg << " = add i32 0, " << (node.value ? "1" : "0") << std::endl;
}

void CodeGenerator::visit(ast::ID &node) {
    FuncEntry* entry = lookupSymbol(node.value);
    if (!entry) return;
    
    if (entry->param_types.empty()) {
        // It's a variable
        if (entry->offset < 0) {
            // Function parameter
            lastReg = "%" + std::to_string(-(entry->offset + 1));
        } else {
            // Local variable
            std::string ptr = varPointers[node.value];
            lastReg = cb.freshVar();
            cb << lastReg << " = load i32, i32* " << ptr << std::endl;
        }
    }
}

void CodeGenerator::visit(ast::BinOp &node) {
    // Evaluate operands
    node.left->accept(*this);
    std::string leftReg = lastReg;
    
    node.right->accept(*this);
    std::string rightReg = lastReg;
    
    lastReg = cb.freshVar();
    
    // Division by zero check
    if (node.op == ast::BinOpType::DIV) {
        checkDivByZero(rightReg);
    }
    
    // Handle byte operations with truncation
    bool isByte = (node.left->type == ast::BuiltInType::BYTE && 
                   node.right->type == ast::BuiltInType::BYTE);
    
    if (isByte) {
        // Truncate to i8, perform operation, extend back
        std::string leftByte = cb.freshVar();
        std::string rightByte = cb.freshVar();
        std::string resultByte = cb.freshVar();
        
        cb << leftByte << " = trunc i32 " << leftReg << " to i8" << std::endl;
        cb << rightByte << " = trunc i32 " << rightReg << " to i8" << std::endl;
        
        switch(node.op) {
            case ast::BinOpType::ADD:
                cb << resultByte << " = add i8 " << leftByte << ", " << rightByte << std::endl;
                break;
            case ast::BinOpType::SUB:
                cb << resultByte << " = sub i8 " << leftByte << ", " << rightByte << std::endl;
                break;
            case ast::BinOpType::MUL:
                cb << resultByte << " = mul i8 " << leftByte << ", " << rightByte << std::endl;
                break;
            case ast::BinOpType::DIV:
                cb << resultByte << " = udiv i8 " << leftByte << ", " << rightByte << std::endl;
                break;
        }
        
        cb << lastReg << " = zext i8 " << resultByte << " to i32" << std::endl;
    } else {
        // Regular i32 operation
        switch(node.op) {
            case ast::BinOpType::ADD:
                cb << lastReg << " = add i32 " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinOpType::SUB:
                cb << lastReg << " = sub i32 " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinOpType::MUL:
                cb << lastReg << " = mul i32 " << leftReg << ", " << rightReg << std::endl;
                break;
            case ast::BinOpType::DIV:
                cb << lastReg << " = sdiv i32 " << leftReg << ", " << rightReg << std::endl;
                break;
        }
    }
}

void CodeGenerator::visit(ast::RelOp &node) {
    node.left->accept(*this);
    std::string leftReg = lastReg;
    
    node.right->accept(*this);
    std::string rightReg = lastReg;
    
    std::string cmpReg = cb.freshVar();
    std::string opStr;
    
    switch(node.op) {
        case ast::RelOpType::EQ: opStr = "eq"; break;
        case ast::RelOpType::NE: opStr = "ne"; break;
        case ast::RelOpType::LT: opStr = "slt"; break;
        case ast::RelOpType::GT: opStr = "sgt"; break;
        case ast::RelOpType::LE: opStr = "sle"; break;
        case ast::RelOpType::GE: opStr = "sge"; break;
    }
    
    cb << cmpReg << " = icmp " << opStr << " i32 " << leftReg << ", " << rightReg << std::endl;
    
    lastReg = cb.freshVar();
    cb << lastReg << " = zext i1 " << cmpReg << " to i32" << std::endl;
}

void CodeGenerator::visit(ast::Not &node) {
    node.exp->accept(*this);
    std::string expReg = lastReg;
    
    // Convert to bool, then negate
    std::string boolReg = cb.freshVar();
    cb << boolReg << " = trunc i32 " << expReg << " to i1" << std::endl;
    
    std::string notReg = cb.freshVar();
    cb << notReg << " = xor i1 " << boolReg << ", 1" << std::endl;
    
    lastReg = cb.freshVar();
    cb << lastReg << " = zext i1 " << notReg << " to i32" << std::endl;
}

void CodeGenerator::visit(ast::And &node) {
    // Evaluate left operand
    node.left->accept(*this);
    std::string leftReg = lastReg;
    
    std::string evalRightLabel = cb.freshLabel();
    std::string shortCircuitLabel = cb.freshLabel();
    std::string mergeLabel = cb.freshLabel();
    
    // Branch based on left value
    std::string leftBool = cb.freshVar();
    cb << leftBool << " = trunc i32 " << leftReg << " to i1" << std::endl;
    cb << "br i1 " << leftBool << ", label " << evalRightLabel << ", label " << shortCircuitLabel << std::endl;
    
    // Short circuit path: left is false, return 0
    cb.emitLabel(shortCircuitLabel);
    std::string zeroReg = cb.freshVar();
    cb << zeroReg << " = add i32 0, 0" << std::endl;
    cb << "br label " << mergeLabel << std::endl;
    
    // Evaluate right operand
    cb.emitLabel(evalRightLabel);
    node.right->accept(*this);
    std::string rightReg = lastReg;
    // Get a label name for where we are now (after evaluating right)
    std::string afterRightLabel = cb.freshLabel();
    cb << "br label " << afterRightLabel << std::endl;
    
    cb.emitLabel(afterRightLabel);
    cb << "br label " << mergeLabel << std::endl;
    
    // Merge point
    cb.emitLabel(mergeLabel);
    lastReg = cb.freshVar();
    cb << lastReg << " = phi i32 [" << zeroReg << ", " << shortCircuitLabel 
       << "], [" << rightReg << ", " << afterRightLabel << "]" << std::endl;
}

void CodeGenerator::visit(ast::Or &node) {
    // Evaluate left operand
    node.left->accept(*this);
    std::string leftReg = lastReg;
    
    std::string evalRightLabel = cb.freshLabel();
    std::string shortCircuitLabel = cb.freshLabel();
    std::string mergeLabel = cb.freshLabel();
    
    // Branch based on left value
    std::string leftBool = cb.freshVar();
    cb << leftBool << " = trunc i32 " << leftReg << " to i1" << std::endl;
    cb << "br i1 " << leftBool << ", label " << shortCircuitLabel << ", label " << evalRightLabel << std::endl;
    
    // Short circuit path: left is true, return 1
    cb.emitLabel(shortCircuitLabel);
    std::string oneReg = cb.freshVar();
    cb << oneReg << " = add i32 0, 1" << std::endl;
    cb << "br label " << mergeLabel << std::endl;
    
    // Evaluate right operand
    cb.emitLabel(evalRightLabel);
    node.right->accept(*this);
    std::string rightReg = lastReg;
    // Get a label name for where we are now (after evaluating right)
    std::string afterRightLabel = cb.freshLabel();
    cb << "br label " << afterRightLabel << std::endl;
    
    cb.emitLabel(afterRightLabel);
    cb << "br label " << mergeLabel << std::endl;
    
    // Merge point
    cb.emitLabel(mergeLabel);
    lastReg = cb.freshVar();
    cb << lastReg << " = phi i32 [" << oneReg << ", " << shortCircuitLabel 
       << "], [" << rightReg << ", " << afterRightLabel << "]" << std::endl;
}

void CodeGenerator::visit(ast::Cast &node) {
    node.exp->accept(*this);
    // In our implementation, cast is a no-op since everything is i32
    // Just pass through the register
}

void CodeGenerator::visit(ast::ExpList &node) {
    for (auto& exp : node.exps) {
        exp->accept(*this);
    }
}

void CodeGenerator::visit(ast::Call &node) {
    FuncEntry* func = lookupSymbol(node.func_id->value);
    if (!func) return;
    
    // Evaluate arguments
    std::vector<std::string> argRegs;
    std::vector<ast::BuiltInType> argTypes;
    
    for (auto& arg : node.args->exps) {
        arg->accept(*this);
        argRegs.push_back(lastReg);
        argTypes.push_back(arg->type);
    }
    
    // Build call
    std::string retType = getLLVMType(func->ret_type);
    
    if (func->ret_type == ast::BuiltInType::VOID) {
        cb << "call void @" << node.func_id->value << "(";
    } else {
        lastReg = cb.freshVar();
        cb << lastReg << " = call " << retType << " @" << node.func_id->value << "(";
    }
    
    for (size_t i = 0; i < argRegs.size(); i++) {
        if (i > 0) cb << ", ";
        cb << getLLVMType(argTypes[i]) << " " << argRegs[i];
    }
    
    cb << ")" << std::endl;
}

void CodeGenerator::visit(ast::Statements &node) {
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
}

void CodeGenerator::visit(ast::Break &node) {
    if (!breakLabels.empty()) {
        cb << "br label " << breakLabels.back() << std::endl;
    }
}

void CodeGenerator::visit(ast::Continue &node) {
    if (!continueLabels.empty()) {
        cb << "br label " << continueLabels.back() << std::endl;
    }
}

void CodeGenerator::visit(ast::Return &node) {
    if (node.exp) {
        node.exp->accept(*this);
        cb << "ret i32 " << lastReg << std::endl;
    } else {
        cb.emit("ret void");
    }
}

void CodeGenerator::visit(ast::If &node) {
    node.condition->accept(*this);
    std::string condReg = lastReg;
    
    // Convert to i1
    std::string condBool = cb.freshVar();
    cb << condBool << " = trunc i32 " << condReg << " to i1" << std::endl;
    
    std::string thenLabel = cb.freshLabel();
    std::string elseLabel = node.otherwise ? cb.freshLabel() : cb.freshLabel();
    std::string mergeLabel = cb.freshLabel();
    
    cb << "br i1 " << condBool << ", label " << thenLabel << ", label " << elseLabel << std::endl;
    
    // Then branch
    cb.emitLabel(thenLabel);
    enterScope();
    node.then->accept(*this);
    exitScope();
    cb << "br label " << mergeLabel << std::endl;
    
    // Else branch
    cb.emitLabel(elseLabel);
    if (node.otherwise) {
        enterScope();
        node.otherwise->accept(*this);
        exitScope();
    }
    cb << "br label " << mergeLabel << std::endl;
    
    // Merge
    cb.emitLabel(mergeLabel);
}

void CodeGenerator::visit(ast::While &node) {
    std::string condLabel = cb.freshLabel();
    std::string bodyLabel = cb.freshLabel();
    std::string endLabel = cb.freshLabel();
    
    breakLabels.push_back(endLabel);
    continueLabels.push_back(condLabel);
    
    cb << "br label " << condLabel << std::endl;
    
    // Condition
    cb.emitLabel(condLabel);
    node.condition->accept(*this);
    std::string condReg = lastReg;
    
    std::string condBool = cb.freshVar();
    cb << condBool << " = trunc i32 " << condReg << " to i1" << std::endl;
    cb << "br i1 " << condBool << ", label " << bodyLabel << ", label " << endLabel << std::endl;
    
    // Body
    cb.emitLabel(bodyLabel);
    enterScope();
    node.body->accept(*this);
    exitScope();
    cb << "br label " << condLabel << std::endl;
    
    // End
    cb.emitLabel(endLabel);
    
    breakLabels.pop_back();
    continueLabels.pop_back();
}

void CodeGenerator::visit(ast::VarDecl &node) {
    std::string varName = node.id->value;
    
    // Allocate on stack
    std::string ptr = cb.freshVar();
    cb << ptr << " = alloca i32" << std::endl;
    varPointers[varName] = ptr;
    
    // Initialize
    std::string initReg;
    if (node.init_exp) {
        node.init_exp->accept(*this);
        initReg = lastReg;
    } else {
        // Default to 0
        initReg = cb.freshVar();
        cb << initReg << " = add i32 0, 0" << std::endl;
    }
    
    cb << "store i32 " << initReg << ", i32* " << ptr << std::endl;
    
    // Add to symbol table
    insertVariable(varName, node.type->type);
}

void CodeGenerator::visit(ast::Assign &node) {
    node.exp->accept(*this);
    std::string valueReg = lastReg;
    
    std::string ptr = varPointers[node.id->value];
    cb << "store i32 " << valueReg << ", i32* " << ptr << std::endl;
}

void CodeGenerator::visit(ast::Formal &node) {
    // Handled in FuncDecl
}

void CodeGenerator::visit(ast::Formals &node) {
    // Handled in FuncDecl
}

void CodeGenerator::visit(ast::FuncDecl &node) {
    currentFunction = node.id->value;
    varPointers.clear();
    
    FuncEntry* func = lookupSymbol(node.id->value);
    if (!func) return;
    
    std::string retType = getLLVMType(node.return_type->type);
    
    // Function signature - parameters need proper names
    cb << "define " << retType << " @" << node.id->value << "(";
    
    std::vector<std::string> paramRegs;
    for (size_t i = 0; i < node.formals->formals.size(); i++) {
        if (i > 0) cb << ", ";
        std::string paramReg = cb.freshVar();
        paramRegs.push_back(paramReg);
        cb << "i32 " << paramReg;
    }
    
    cb << ") {" << std::endl;
    
    enterScope();
    
    // Allocate and store parameters
    for (size_t i = 0; i < node.formals->formals.size(); i++) {
        auto& formal = node.formals->formals[i];
        std::string varName = formal->id->value;
        
        std::string ptr = cb.freshVar();
        cb << ptr << " = alloca i32" << std::endl;
        cb << "store i32 " << paramRegs[i] << ", i32* " << ptr << std::endl;
        
        varPointers[varName] = ptr;
        insertVariable(varName, formal->type->type);
    }
    
    // Function body
    node.body->accept(*this);
    
    // Default return if needed
    if (node.return_type->type == ast::BuiltInType::VOID) {
        cb.emit("ret void");
    } else {
        std::string zeroReg = cb.freshVar();
        cb << zeroReg << " = add i32 0, 0" << std::endl;
        cb << "ret i32 " << zeroReg << std::endl;
    }
    
    exitScope();
    
    cb.emit("}");
    cb.emit("");
}

void CodeGenerator::visit(ast::Funcs &node) {
    // First pass: collect function signatures
    for (auto& func : node.funcs) {
        std::vector<VarAtts> params;
        for (size_t i = 0; i < func->formals->formals.size(); i++) {
            auto& formal = func->formals->formals[i];
            params.push_back({formal->id->value, formal->type->type, -(int)(i+1)});
        }
        if (params.empty()) {
            params.push_back({"dummy", ast::BuiltInType::VOID, 0});
        }
        insertFunction(func->id->value, func->return_type->type, params);
    }
    
    // Second pass: generate code
    for (auto& func : node.funcs) {
        func->accept(*this);
    }
}

void CodeGenerator::visit(ast::Block &node) {
    enterScope();
    node.statements->accept(*this);
    exitScope();
}

// Helper implementations
FuncEntry* CodeGenerator::lookupSymbol(const std::string& name) {
    for (auto it = symbol_table.rbegin(); it != symbol_table.rend(); ++it) {
        for (auto& entry : *it) {
            if (entry.id == name) {
                return &entry;
            }
        }
    }
    return nullptr;
}

void CodeGenerator::insertVariable(const std::string& name, ast::BuiltInType type, const int* offset) {
    int off = offset ? *offset : offset_table.back()++;
    FuncEntry entry = {name, type, {}, off};
    symbol_table.back().push_back(entry);
}

void CodeGenerator::insertFunction(const std::string& name, ast::BuiltInType retType, const std::vector<VarAtts>& params) {
    FuncEntry entry = {name, retType, params, 0};
    symbol_table.front().push_back(entry);
}

void CodeGenerator::enterScope() {
    symbol_table.push_back(std::vector<FuncEntry>());
    offset_table.push_back(offset_table.back());
}

void CodeGenerator::exitScope() {
    symbol_table.pop_back();
    offset_table.pop_back();
}
