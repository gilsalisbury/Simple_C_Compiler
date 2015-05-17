
// dsalisbu
// 1224878

// Use cpp to scan a file and print line numbers.
// Print out each input line read in, then strtok it for
// tokens and parse for abstract syntax tree.

#include <string>
#include <iostream>
#include <fstream>
using namespace std;


#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wait.h>

#include "auxlib.h"
#include "stringset.h"
#include "lyutils.h"

const string CPP = "/usr/bin/cpp";
const size_t LINESIZE = 1024;
extern int yy_flex_debug;
extern FILE* tokenout;
FILE* astout;

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
   size_t len = strlen (string);
   if (len == 0) return;
   char* nlpos = string + len - 1;
   if (*nlpos == delim) *nlpos = '\0';
}




int main (int argc, char** argv) {
   int c;
   string name, ext, strfn, tokfn, astfn;

   // check if name is long enough to contain .oc
   name = string(argv[argc-1]);
   if(name.length() <3) {
      fprintf(stderr, "error: invalid filename\n");
      exit(1);
   }
   //  if name is long enough check extension == .oc
   ext = name.substr(name.length()-3);
   if (ext != ".oc") {
      fprintf(stderr, "invalid file type\n");
      exit(1);
   }

   // if long enough and correct extension strip
   // off path using basename and use to make output.str
   DEBUGF('d', ext.c_str());
   set_execname (argv[0]);
   strfn = basename(argv[argc-1]);
   strfn = strfn.substr(0, strfn.length()-3);
   tokfn = strfn + ".tok";
   astfn = strfn + ".ast";
   strfn += ".str";
   DEBUGF('d', name.c_str());
   yy_flex_debug = 0;
   // parse option with getopt(3)
   for (int argi = 1; argi < 2; ++argi) {
      string s;
      while((c = getopt(argc, argv, "D:@:ly") ) != -1) {
         switch (c){
            case '@': 
               if ((string(optarg) != "d") && (string(optarg) != "f") 
                                          && (string(optarg) != "a")) {
                  syserrprintf ("usage: oc [-ly] [-@ flag...] "
                     "[-D string] program.oc\n");
                  syserrprintf ("[-@ flag...] valid options "
                     "are:\n\t-@d ~ enable debug printing\n" );
                  exit(1);
               }
               set_debugflags(optarg);
               break;
            case 'D': 
               s = " -D" + string(optarg);
               break;
            case 'l': 
               yy_flex_debug = 1;
               break;
            case 'y':
               yydebug = 1;
               break;
            default: 
               errprintf ("usage: oc [-ly] [-@ flag...] "
                                    "[-D string] program.oc\n");
               exit(1);
               break;
         }
      }

      char* filename = argv[argc -1];
      string command = CPP + s + " " + filename;
      yyin = popen (command.c_str(), "r");
      //lexer_setecho(true);
      if (pipe == NULL) {
         syserrprintf (command.c_str());
         exit(1);
      }else {
         int parsecode;
         tokenout = fopen(tokfn.c_str(), "w");
         astout = fopen(astfn.c_str(), "w");
         parsecode = yyparse();
         if (parsecode == YYEOF) {/*do nothing*/}
         int pclose_rc = pclose (yyin) >> 8;
         if (pclose_rc != 0){
            fprintf(stderr, "invalid file name\n");
            exit(1);
         }else{
            // open outfile &ostream
            stringout.open(strfn, ostream::out);
            dump_stringset(stringout);
            dump_astree(astout, yyparse_astree);
            type_ast(stdout, yyparse_astree);
            fclose(astout);
            stringout.close();
            fclose(tokenout);
            free_ast(yyparse_astree);
         }
         eprint_status (command.c_str(), pclose_rc);
      }
   }
   return get_exitstatus();
}

