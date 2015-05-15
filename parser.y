%{
// dsalisbu
// 1224878

#include "lyutils.h"
#include "astree.h"
#include<assert.h>

void sym (astree* tree, int symbol);


//static void* yycalloc (size_t size);

%}

%debug
%defines
%error-verbose
%token-table
%verbose

%token TOK_VOID TOK_BOOL TOK_CHAR TOK_INT TOK_STRING
%token TOK_IF TOK_ELSE TOK_WHILE TOK_RETURN TOK_STRUCT
%token TOK_FALSE TOK_TRUE TOK_NULL TOK_NEW TOK_ARRAY
%token TOK_EQ TOK_NE TOK_LT TOK_LE TOK_GT TOK_GE
%token TOK_IDENT TOK_INTCON TOK_CHARCON TOK_STRINGCON

%token TOK_BLOCK TOK_CALL TOK_IFELSE TOK_INITDECL TOK_INDEX
%token TOK_POS TOK_NEG TOK_NEWARRAY TOK_TYPEID TOK_FIELD
%token TOK_ORD TOK_CHR TOK_ROOT TOK_DECLID '(' ')' '{' '}'
%token TOK_NEWSTRING TOK_PARAMLIST TOK_PARAM TOK_FUNCTION 
%token TOK_RETURNVOID TOK_DUMMY TOK_VARDECL TOK_PROTOTYPE


%right  TOK_IF  TOK_ELSE
%right  '='
%nonassoc  TOK_EQ  TOK_NE  TOK_LT  TOK_LE  TOK_GT  TOK_GE 
%left   '+' '-'
%left   '*' '/' '%'
%right  TOK_POS TOK_NEG '!' TOK_NEW  TOK_ORD  TOK_CHAR 
%left   '[' '.' '('


%start  start

%%


start       : program            { yyparse_astree = $1; }
            ;                                              


program     : program structdef  { $$ = adopt1 ($1, $2); }
            | program function   { $$ = adopt1 ($1, $2); }
            | program statement  { $$ = adopt1 ($1, $2); }
            | program error '}'  { $$ = $1; }             
            | program error ';'  { $$ = $1; }             
            |                    { $$ = new_parseroot (); }
            ; 


function    : identdecl identdeclst ')' funcblock  
                                    { $$ = synthtok(TOK_FUNCTION, $1);
                                    adopt2($$, $2, $4); free_ast($3); }
            
            | identdecl '(' ')' funcblock          
                                    { $$ = synthtok(TOK_FUNCTION, $1); 
                                    adopt2($1, $2, $4); free_ast($3);
                                    sym($2,TOK_PARAMLIST); }
            
            | identdecl identdeclst ')' ';'        
                                    { $$ = synthtok(TOK_PROTOTYPE, $1);
                                    adopt1($$, $2); free_ast($3); }
            
            | identdecl '(' ')' ';' { $$ = synthtok(TOK_PROTOTYPE, $1);
                                    adopt1($1, $2); free_ast($3); 
                                    sym($2,TOK_PARAMLIST); }
            ; 


identdeclst : identdeclst ',' identdecl   
                                    { $$ = adopt1 ($1, $3);
                                    free_ast($2); }
            
            | '(' identdecl         { $$ = adopt1 ($1, $2); 
                                    sym($1,TOK_PARAMLIST); }
            ;


identdecl   : basetype TOK_ARRAY TOK_IDENT 
                                    {  $$ = adopt2($2, $1, $3);
                                    sym($3, TOK_DECLID); }
            
            | basetype TOK_IDENT    {  $$ = adopt1 ($1, $2); 
                                    sym($2, TOK_DECLID); }

            | TOK_IDENT TOK_ARRAY TOK_IDENT 
                                    {  $$ = adopt2($2, $1, $3);
                                    sym($3, TOK_DECLID); 
                                    sym($1, TOK_TYPEID); }
            
            | TOK_IDENT TOK_IDENT   {  $$ = adopt1 ($1, $2); 
                                    sym($2, TOK_DECLID); 
                                    sym($1, TOK_TYPEID); }
            ;


structdef   : TOK_STRUCT TOK_IDENT fielddeclst '}'    
                                    { $$ = adopt1($1, $2); 
                                    adoptall($1, $3);
                                    free_ast($4); 
                                    sym($2, TOK_TYPEID); }
            
            | TOK_STRUCT TOK_IDENT '{' '}'             
                                    { free_ast2($3, $4); 
                                    sym($2, TOK_TYPEID);
                                    $$ = adopt1($1, $2); }
            ; 


fielddeclst : fielddeclst fielddecl { $$ = adopt1 ($1, $2); }
            | '{' fielddecl         { $$ = adopt1 ($1, $2); }
            ;


fielddecl   : basetype TOK_ARRAY TOK_IDENT ';'  
                                    { $$ = adopt2($2, $1, $3); 
                                    sym($3, TOK_FIELD); 
                                    free_ast($4); }
            
            | basetype TOK_IDENT ';'{ free_ast($3); 
                                    $$ = adopt1 ($1, $2); 
                                    sym ($2, TOK_FIELD); }

            | TOK_IDENT TOK_ARRAY TOK_IDENT ';'  
                                    { $$ = adopt2($2, $1, $3); 
                                    sym($3, TOK_FIELD); 
                                    free_ast($4); 
                                    sym($1, TOK_TYPEID); }
            
            | TOK_IDENT TOK_IDENT ';'{ free_ast($3); 
                                    $$ = adopt1 ($1, $2); 
                                    sym ($2, TOK_FIELD); 
                                    sym($1, TOK_TYPEID); }
            ;


block       : stmtlist '}'          { $$ = $1; sym($1, TOK_BLOCK); }
            | '{''}'                { $$ = $1; sym($1, TOK_BLOCK); }
            | ';'                   { $$ = $1; }
            ; 


funcblock   : stmtlist '}'          { $$ = $1; sym($1, TOK_BLOCK); }
            | '{''}'                { $$ = $1; sym($1, TOK_BLOCK); }
            ; 


stmtlist    : stmtlist statement    { $$ = adopt1 ($1, $2); }
            | '{' statement         { $$ = adopt1 ($1, $2); }
            ;


statement   : block                 { $$ = $1; }
            | vardecl               { $$ = $1; } 
            | expr ';'              { $$ = $1; free_ast($2); }
            | while                 { $$ = $1; }
            | ifelse                { $$ = $1; }
            | return                { $$ = $1; }
            ;      


while       : TOK_WHILE '(' expr ')' statement  
                                    { free_ast2 ($2,$4); 
                                    $$ = adopt2 ($1, $3, $5); }
            ;


ifelse      : if else               { $$ = adopt1 ($1, $2);  
                                    sym ($1,TOK_IFELSE); }
            
            | if %prec TOK_ELSE     { $$ = $1; sym ($1, TOK_IFELSE); }
            ;


if          : TOK_IF '(' expr ')' statement   
                                    { free_ast2 ($2, $4); 
                                    $$ = adopt2 ($1, $3, $5); }
            ;


else        : TOK_ELSE statement    { $$ = $2; free_ast($1);  }
            ;


return      : TOK_RETURN expr ';'   { free_ast($3); 
                                    $$ = adopt1($1, $2); }
            
            | TOK_RETURN ';'        { free_ast($2); $$ = $1; 
                                    sym($1,TOK_RETURNVOID); }


basetype    : TOK_VOID              { $$ = $1; } 
            | TOK_BOOL              { $$ = $1; }
            | TOK_CHAR              { $$ = $1; }
            | TOK_INT               { $$ = $1; }
            | TOK_STRING            { $$ = $1; }
            | TOK_TYPEID            { $$ = $1; }
            ;

argslist    :  argslist ',' expr    { $$ = adopt1 ($1, $3); 
                                    free_ast ($2); }
            
            |  '(' expr             { $$ = adopt1 ($1, $2); }
            

expr        : allocator             { $$ = $1; }
            | call                  { $$ = $1; }
            | variable              { $$ = $1; }
            | constant              { $$ = $1; }
            | expr '=' expr         { $$ = adopt2($2, $1, $3); }
            | expr TOK_EQ expr      { $$ = adopt2($2, $1, $3); }
            | expr TOK_NE expr      { $$ = adopt2($2, $1, $3); }
            | expr TOK_LT expr      { $$ = adopt2($2, $1, $3); }
            | expr TOK_LE expr      { $$ = adopt2($2, $1, $3); }
            | expr TOK_GT expr      { $$ = adopt2($2, $1, $3); }
            | expr TOK_GE expr      { $$ = adopt2($2, $1, $3); }
            | expr '+' expr         { $$ = adopt2($2, $1, $3); }
            | expr '-' expr         { $$ = adopt2($2, $1, $3); }
            | expr '/' expr         { $$ = adopt2($2, $1, $3); }
            | expr '*' expr         { $$ = adopt2($2, $1, $3); }
            | expr '%' expr         { $$ = adopt2($2, $1, $3); }

            | '-' expr              { $$ = adopt1($1, $2); 
                                    sym($1,TOK_NEG); }

            | '+' expr              { $$ = adopt1($1, $2); 
                                    sym($1,TOK_POS); }
            
            | TOK_ORD expr %prec TOK_POS   
                                    { $$ = adopt1($1, $2); }
            
            | TOK_CHR expr %prec TOK_POS   
                                    { $$ = adopt1($1, $2); }
                                    
            | '!' expr              { $$ = adopt1($1, $2); }
            | '.' expr              { $$ = adopt1($1, $2); }
            | '(' expr ')'          { $$ = $2; free_ast2($1, $3); }
            ;


vardecl     : identdecl '=' expr ';'    
                                    {free_ast($4); 
                                    $$ = adopt2 ($2, $1, $3); 
                                    sym($2,TOK_VARDECL); }
            ;


allocator   : TOK_NEW TOK_IDENT '('')'  
                                    { $$ = adopt1 ($2, $1); 
                                    sym($2,TOK_TYPEID); 
                                    free_ast2($3, $4); }
            
            | TOK_NEW TOK_STRING '(' expr ')'  
                                    { $$ = adopt1 ($2, $4); 
                                    sym($2,TOK_NEWSTRING); 
                                    free_ast2($1, $3); 
                                    free_ast($5); }
            
            | TOK_NEW basetype '[' expr ']'    
                                    { $$ = adopt2 ($1, $2, $4); 
                                    sym($1,TOK_NEWARRAY); 
                                    sym($2,TOK_TYPEID);
                                    free_ast2($3, $5);}
            ;


call        : TOK_IDENT argslist ')'{ free_ast($3); 
                                    $$ = makeheir ($2, $1); 
                                    sym($2,TOK_CALL); } 
            
            | TOK_IDENT '(' ')'     { free_ast($3); 
                                    $$ = adopt1 ($2, $1); 
                                    sym($2,TOK_CALL); }
            ;


variable    : expr '[' expr ']'     { free_ast($4); 
                                    $$ = adopt2($2, $1, $3);
                                    sym($2, TOK_INDEX); }
            
            | TOK_IDENT             { $$ = $1; }
            
            | expr '.' TOK_IDENT    { $$ = adopt2($2, $1, $3); 
                                    sym($3,TOK_FIELD); }
            ;


constant    : TOK_INTCON                  { $$ = $1; }
            | TOK_CHARCON                 { $$ = $1; }
            | TOK_STRINGCON               { $$ = $1; }
            | TOK_FALSE                   { $$ = $1; }
            | TOK_TRUE                    { $$ = $1; }
            | TOK_NULL                    { $$ = $1; }
            ;


%%

const char *get_yytname (int symbol) {
   return yytname [YYTRANSLATE (symbol)];
}


bool is_defined_token (int symbol) {
   return YYTRANSLATE (symbol) > YYUNDEFTOK;
}

void sym(astree* tree, int symbol) {
   tree->symbol = symbol;
}


/*static void* yycalloc (size_t size) {
   void* result = calloc (1, size);
   assert (result != NULL);
   return result;
}*/


