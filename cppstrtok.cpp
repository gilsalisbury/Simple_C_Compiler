// $Id: cppstrtok.cpp,v 1.3 2014-10-07 18:09:11-07 - - $

// Use cpp to scan a file and print line numbers.
// Print out each input line read in, then strtok it for
// tokens.

#include <string>
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

const string CPP = "/usr/bin/cpp";
const size_t LINESIZE = 1024;

// Chomp the last character from a buffer if it is delim.
void chomp (char* string, char delim) {
   size_t len = strlen (string);
   if (len == 0) return;
   char* nlpos = string + len - 1;
   if (*nlpos == delim) *nlpos = '\0';
}


// Run cpp against the lines of the file.
void cpplines (FILE* pipe, char* filename) {
   int linenr = 1;
   char inputname[LINESIZE];
   strcpy (inputname, filename);
   for (;;) {
      char buffer[LINESIZE];
      char* fgets_rc = fgets (buffer, LINESIZE, pipe);
      if (fgets_rc == NULL) break;
      chomp (buffer, '\n');
      //printf ("%s:line %d: [%s]\n", filename, linenr, buffer);
      // http://gcc.gnu.org/onlinedocs/cpp/Preprocessor-Output.html
      int sscanf_rc = sscanf (buffer, "# %d \"%[^\"]\"",
                              &linenr, filename);
      if (sscanf_rc == 2) {
         //printf ("DIRECTIVE: line %d file \"%s\"\n", linenr, filename);
         continue;
      }
      char* savepos = NULL;
      char* bufptr = buffer;
      for (int tokenct = 1;; ++tokenct) {
         char* token = strtok_r (bufptr, " \t\n", &savepos);
         bufptr = NULL;
         if (token == NULL) break; 
         const string* str = intern_stringset (token);
         cout << str;
         //printf ("token %d.%d: [%s]\n",
         //        linenr, tokenct, token);
      }
      ++linenr;
   }
}

int main (int argc, char** argv) {
   int c;
   set_execname (argv[0]);
   //cout << get_execname() << "\n";
   /*((c = getopt(argc, argv, "D:@:ly") ) != -1) {
      switch (c){
         case '@': 
            cout << optarg << "set DEBUGF to 1\n";
            set_debugflags(optarg);
            break;
         case 'n':
            cout << optarg << "switch case to something with n\n";
            break;
         case 'D':
            //set_debugflags(argv[c]);
            cout << optarg << "\n";
            s =  """ #ifdef optarg \
                  #undef optarg  \
                  #else  \
                  #define optarg 1 \
                  #endif """;


            cout << optarg << "pass an argument to cpp\n";
            break;
      }
   }*/
   DEBUGF('d', "this will actually print");
   for (int argi = 1; argi < 2; ++argi) {
      string s;
      while((c = getopt(argc, argv, "D:@:ly") ) != -1) {
         switch (c){
            case '@': 
               //cout << optarg << "set DEBUGF to 1\n";
               set_debugflags(optarg);
               break;
            case 'n':
               //cout << optarg << "switch case to something with n\n";
               break;
            case 'D':
               //cout << optarg << "\n";
               s = " -D" + string(optarg);
               //cout << optarg << "pass an argument to cpp\n";
               break;
            default: 
               errprintf ("usage: oc [-ly] [-@ flag...] [-D string] program.oc\n");
               exit(1);
               break;
         }
      }
      char* filename = argv[argc -1];
      string command = CPP + s + " " + filename;
      //printf ("command=\"%s\"\n", command.c_str());
      FILE* pipe = popen (command.c_str(), "r");
      if (pipe == NULL) {
         //cout << "something happened";
         //syserrprintf (command.c_str());
      }else {
         //cout << "something that was suppose to happened\n";
         cpplines (pipe, filename);

         int pclose_rc = pclose (pipe);
         eprint_status (command.c_str(), pclose_rc);
      }
      dump_stringset(cout);
   }
   return get_exitstatus();
}

