%{

#include "nodes.hpp"
#include "output.hpp"

// bison declarations
extern int yylineno;
extern int yylex();

void yyerror(const char*);

// root of the AST, set by the parser and used by other parts of the compiler
std::shared_ptr<ast::Node> program;

using namespace std;

// TODO: Place any additional declarations here
#include <memory>
using namespace ast;
%}

// TODO: Define tokens here
%token VOID INT BYTE BOOL
%token AND OR NOT
%token TRUE FALSE
%token IF ELSE WHILE RETURN BREAK CONTINUE
%token ADD SUB MULTI DIV
%token LT LE GT GE EQ NE
%token ASSIGN
%token LPAREN RPAREN LBRACE RBRACE
%token SC COMMA
%token ID NUM NUM_B STRING



// TODO: Define precedence and associativity here
%right ASSIGN
%left OR
%left AND
%nonassoc EQ NE  
%nonassoc LT LE GT GE   
%left ADD SUB
%left MULTI DIV
%right NOT
%right CAST
%nonassoc ELSE


%%

// While reducing the start variable, set the root of the AST
Program:  Funcs { program = $1; }
;

// TODO: Define grammar here

// FUNCS

Funcs:
      /* empty */       { $$ = make_shared<Funcs>(); }
      | FuncDecl Funcs    {
          std::shared_ptr<FuncDecl> f = dynamic_pointer_cast<FuncDecl>($1);
          std::shared_ptr<Funcs> fs = dynamic_pointer_cast<Funcs>($2);
          fs->push_front(f);
          $$ = fs;
      }
;



// FUNC DECL

FuncDecl:
    RetType ID LPAREN Formals RPAREN LBRACE Statements RBRACE
    {
      $$ = make_shared<FuncDecl>(
            dynamic_pointer_cast<ast::ID>($2),
            dynamic_pointer_cast<Type>($1),
            dynamic_pointer_cast<Formals>($4),
            dynamic_pointer_cast<Statements>($7)
      );
    }
;



// RETURN TYPE

RetType:
      Type             { $$ = dynamic_pointer_cast<Type>($1); }
      | VOID           { $$ = make_shared<Type>(BuiltInType::VOID); }
;



// FORMALS

Formals:
      /* empty */     { $$ = make_shared<Formals>(); }
      | FormalsList     { $$ = dynamic_pointer_cast<Formals>($1); }
;

FormalsList:
      FormalDecl {
            $$ = make_shared<Formals>(
                  dynamic_pointer_cast<Formal>($1)
            );
      }
      | FormalDecl COMMA FormalsList {
          std::shared_ptr<Formals> fs = dynamic_pointer_cast<Formals>($3);
          fs->push_front(dynamic_pointer_cast<Formal>($1));
          $$ = fs;
      }
;



// FORMAL DECL 

FormalDecl:
      Type ID {
            $$ = make_shared<Formal>(
                dynamic_pointer_cast<ast::ID>($2),
                dynamic_pointer_cast<Type>($1)
            );
      }
;



// STATEMENTS

Statements:
      Statement {
            $$ = make_shared<Statements>(
                  dynamic_pointer_cast<Statement>($1)
            );
      }
      | Statements Statement {
            std::shared_ptr<Statements> st = dynamic_pointer_cast<Statements>($1);
            st->push_back(dynamic_pointer_cast<Statement>($2));
            $$ = st;
      }
;



//STATEMENT

Statement:
    LBRACE Statements RBRACE {
        $$ = make_shared<Block>(
            dynamic_pointer_cast<Statements>($2)
        );
    }

      | Type ID SC {
            $$ = make_shared<VarDecl>(
                  dynamic_pointer_cast<ast::ID>($2),
                  dynamic_pointer_cast<Type>($1)
            );
      }
      | Type ID ASSIGN Exp SC {
          $$ = make_shared<VarDecl>(
                  dynamic_pointer_cast<ast::ID>($2),
                  dynamic_pointer_cast<Type>($1),
                  dynamic_pointer_cast<Exp>($4)
            );
      }
      | ID ASSIGN Exp SC {
          $$ = make_shared<Assign>(
                  dynamic_pointer_cast<ast::ID>($1),
                  dynamic_pointer_cast<Exp>($3)
            );
      }
      | Call SC {
          $$ = dynamic_pointer_cast<Call>($1);
      }
      | RETURN SC {
          $$ = make_shared<Return>();
      }
      | RETURN Exp SC {
          $$ = make_shared<Return>(dynamic_pointer_cast<Exp>($2));
      }
      | IF LPAREN Exp RPAREN Statement {
          $$ = make_shared<If>(
                  dynamic_pointer_cast<Exp>($3),
                  dynamic_pointer_cast<Statement>($5)
            );
      }
      | IF LPAREN Exp RPAREN Statement ELSE Statement {
          $$ = make_shared<If>(
                  dynamic_pointer_cast<Exp>($3),
                  dynamic_pointer_cast<Statement>($5),
                  dynamic_pointer_cast<Statement>($7)
               );
      }
      | WHILE LPAREN Exp RPAREN Statement {
          $$ = make_shared<While>(
                  dynamic_pointer_cast<Exp>($3),
                  dynamic_pointer_cast<Statement>($5)
               );
      }
      | BREAK SC {
          $$ = make_shared<Break>();
      }
      | CONTINUE SC {
          $$ = make_shared<Continue>();
      }
;




//CALL 

Call:
      ID LPAREN ExpList RPAREN {
            $$ = make_shared<Call>(
                  dynamic_pointer_cast<ast::ID>($1),
                  dynamic_pointer_cast<ExpList>($3)
            );
      }
      | ID LPAREN RPAREN {
            $$ = make_shared<Call>(
                  dynamic_pointer_cast<ast::ID>($1),
                  make_shared<ExpList>()
            );
      }
;



// EXPLIST

ExpList:
      Exp {
            $$ = make_shared<ExpList>(
                   dynamic_pointer_cast<Exp>($1)
      );
      }
      | Exp COMMA ExpList {
          std::shared_ptr<ExpList> lst = dynamic_pointer_cast<ExpList>($3);
          lst->push_front(dynamic_pointer_cast<Exp>($1));
          $$ = lst;
      }
;



//TYPE 

Type:
      INT           { $$ = make_shared<Type>(BuiltInType::INT); }
      | BYTE        { $$ = make_shared<Type>(BuiltInType::BYTE); }
      | BOOL        { $$ = make_shared<Type>(BuiltInType::BOOL); }
;



// EXP 

Exp:
      LPAREN Exp RPAREN {
          $$ = $2;  
      }
      | Exp MULTI Exp {
          $$ = make_shared<BinOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  BinOpType::MUL
            );
      }
      | Exp DIV Exp {
          $$ = make_shared<BinOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  BinOpType::DIV
            );
      }
      | Exp ADD Exp {
          $$ = make_shared<BinOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  BinOpType::ADD
            );
      }
      | Exp SUB Exp {
          $$ = make_shared<BinOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  BinOpType::SUB
            );
      }
      | ID      { $$ = $1; }   
      | Call    { $$ = $1; }  
      | NUM     { $$ = $1; } 
      | NUM_B   { $$ = $1; } 
      | STRING  { $$ = $1; }   
      | TRUE    { $$ = $1; }       
      | FALSE   { $$ = $1; }       
      | NOT Exp {
          $$ = make_shared<Not>(dynamic_pointer_cast<Exp>($2));
      }
      | Exp AND Exp {
          $$ = make_shared<And>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3)
            );
      }
      | Exp OR Exp {
          $$ = make_shared<Or>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3)
            );
      }
      | Exp LT Exp {
          $$ = make_shared<RelOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  RelOpType::LT
            );
      }
      | Exp LE Exp {
          $$ = make_shared<RelOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  RelOpType::LE
            );
      }
      | Exp GT Exp {
          $$ = make_shared<RelOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  RelOpType::GT
            );
      }
      | Exp GE Exp {
          $$ = make_shared<RelOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  RelOpType::GE
            );
      }
      | Exp EQ Exp {
          $$ = make_shared<RelOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  RelOpType::EQ
            );
      }
      | Exp NE Exp {
          $$ = make_shared<RelOp>(
                  dynamic_pointer_cast<Exp>($1),
                  dynamic_pointer_cast<Exp>($3),
                  RelOpType::NE
            );
      }
      | LPAREN Type RPAREN Exp %prec CAST {
          $$ = make_shared<Cast>(
                  dynamic_pointer_cast<Exp>($4),
                  dynamic_pointer_cast<Type>($2)
            );
      }
;

%%

// TODO: Place any additional code here

void yyerror(const char *msg) {
    output::errorSyn(yylineno);
    exit(0);
}



