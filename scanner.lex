%{
#include "output.hpp"
#include "nodes.hpp"
#include "parser.tab.h"
%}

%option yylineno
%option noyywrap

whitespace        ([ \t\r\n])

%%


void            { return VOID; }
int             { return INT; }
byte            { return BYTE; }
bool            { return BOOL; }

and             { return AND; }
or              { return OR; }
not             { return NOT; }

true            { yylval = std::make_shared<ast::Bool>(true);  return TRUE; }
false           { yylval = std::make_shared<ast::Bool>(false); return FALSE; }

return          { return RETURN; }
if              { return IF; }
else            { return ELSE; }
while           { return WHILE; }
break           { return BREAK; }
continue        { return CONTINUE; }

;               { return SC; }
,               { return COMMA; }
\(              { return LPAREN; }
\)              { return RPAREN; }
\{              { return LBRACE; }
\}              { return RBRACE; }
=               { return ASSIGN; }

"=="            { return EQ; }
"!="            { return NE; }
"<="            { return LE; }
">="            { return GE; }
"<"             { return LT; }
">"             { return GT; }

\+              { return ADD; }
\-              { return SUB; }
\*              { return MULTI; }
\/              { return DIV; }

[a-zA-Z][a-zA-Z0-9]* {
    yylval = std::make_shared<ast::ID>(yytext);
    return ID;
}

(0|[1-9][0-9]*) {
    yylval = std::make_shared<ast::Num>(yytext);
    return NUM;
}

(0b|[1-9][0-9]*b) {
    yylval = std::make_shared<ast::NumB>(yytext);
    return NUM_B;
}

\"([^\n\r\"\\]|\\[rnt"\\])+\" {
    yylval = std::make_shared<ast::String>(yytext);
    return STRING;
}

{whitespace}+   { /* skip */ }

\/\/[^\r\n]*[\r\n]?    { /* skip comment */ }

.               {
    output::errorLex(yylineno);
    exit(0);
}

%%
