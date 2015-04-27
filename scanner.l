%{
// $Id: scanner.l,v 1.5 2015-04-16 17:17:51-07 - - $

#include "auxlib.h"
#include "lyutils.h"

#define YY_USER_ACTION  { lexer_useraction (); }

%}

%option 8bit

%option nodefault
%option nounput
%option noyywrap
%option verbose
%option warn




LETTER          [A-Za-z_]
DIGIT           [0-9]
NUMBER			 ({DIGIT}*)
IDENT           ({LETTER}({LETTER}|{DIGIT})*)
CHRCON			 ('([^\\'\n]|\\[\\'"0nt])')
STRLIT			 (\"([^\\"\n]|\\[\\'"0nt])*\")

%%

"#".*    		{ lexer_include(); }

"void"			{ return yylval_token (TOK_VOID);		}
"bool"			{ return yylval_token (TOK_BOOL);		}
"char"			{ return yylval_token (TOK_CHAR);		}
"int"				{ return yylval_token (TOK_INT);			}
"string"			{ return yylval_token (TOK_STRING);		}
"struct"			{ return yylval_token (TOK_STRUCT);		}
"if"				{ return yylval_token (TOK_IF);			}
"else"			{ return yylval_token (TOK_ELSE);		}
"while"			{ return yylval_token (TOK_WHILE);		}
"return"			{ return yylval_token (TOK_RETURN);		}
"false"			{ return yylval_token (TOK_FALSE);		}
"true"			{ return yylval_token (TOK_TRUE);		}
"null"			{ return yylval_token (TOK_NULL);		}
"ord"				{ return yylval_token (TOK_ORD);			}
"chr"				{ return yylval_token (TOK_CHR);			}
"new"				{ return yylval_token (TOK_NEW);			}

[ \t]+         { }
\n             { lexer_newline(); }

{NUMBER}			{ return yylval_token (TOK_INTCON);		}
{IDENT}			{ return yylval_token (TOK_IDENT);		}
{CHRCON}			{ return yylval_token (TOK_CHARCON); 	}
{STRLIT}			{ return yylval_token (TOK_STRINGCON); }

"="            { return yylval_token ('=');   			}
"=="				{ return yylval_token (TOK_EQ);			}
"!="				{ return yylval_token (TOK_NE);			}
"<"				{ return yylval_token (TOK_LT);			}
">"				{ return yylval_token (TOK_GT);			}
"<="				{ return yylval_token (TOK_LE);			}
">="				{ return yylval_token (TOK_GE);			}
"+"            { return yylval_token ('+');   			}
"-"            { return yylval_token ('-'); 				}
"*"            { return yylval_token ('*');  			}
"/"            { return yylval_token ('/');   			}
"%"				{ return yylval_token ('%');  	 		}
"!"				{ return yylval_token ('!');  			}
"^"            { return yylval_token ('^'); 				}
"("            { return yylval_token ('(');   			}
")"            { return yylval_token (')');  			}
";"            { return yylval_token (';');   			}
","            { return yylval_token (',');   			}
"."            { return yylval_token ('.');   			}
"{"				{ return yylval_token ('{');   			}
"}"				{ return yylval_token ('}');   			}
"["				{ return yylval_token ('[');   			}
"]"				{ return yylval_token (']');   			}
"[]"				{ return yylval_token (TOK_ARRAY);		}

.              { lexer_badchar (*yytext);     			}


%%

RCSC("$Id: scanner.l,v 1.5 2015-04-16 17:17:51-07 - - $")

