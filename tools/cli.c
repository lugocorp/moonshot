#include "../src/moonshot.h"
#include <string.h>
#include <stdio.h>

// Output
static void error(){
  printf("\033[31;1merror:\033[0m ");
}
static void indent(int i,const char* msg){
  for(int a=0;a<i;a++) printf("  ");
  printf("%s",msg);
}
static int help(){
  indent(0,"Usage: moonshot [options]\n");
  indent(2,"options include:\n\n");
  indent(0,"Moonshot options\n");
  indent(1,"-r");
  indent(5," Execute Lua output after successful compilation\n");
  indent(1,"-i");
  indent(5," Read source code from stdin\n");
  indent(1,"-p");
  indent(5," Write Lua code to stdout\n");
  indent(1,"-f <file>");
  indent(2,"Compile source from <file>\n");
  indent(1,"-v");
  indent(5," Only validate and do not output code\n");
  indent(1,"-o <file>");
  indent(2,"Set output file\n");
  indent(1,"--help");
  indent(3," Print usage options\n");
  return 1;
}

int main(int argc,char** argv){
  FILE* output=NULL;
  FILE* input=NULL;
  int execute=0;
  int write=1;

  // Parse arguments
  for(int a=1;a<argc;a++){
    if(!strcmp(argv[a],"-r")) execute=1;
    else if(!strcmp(argv[a],"-v")) write=0;
    else if(!strcmp(argv[a],"-i")){
      if(input){
        error();
        printf("input is already defined");
        return 1;
      }
      input=stdin;
    }else if(!strcmp(argv[a],"-p")){
      if(output){
        error();
        printf("output is already defined");
        return 1;
      }
      output=stdout;
    }else if(!strcmp(argv[a],"-o")){
      if(a==argc-1) return help();
      char* target=argv[++a];
      if(output){
        error();
        printf("output is already defined");
        return 1;
      }
      output=fopen(target,"w");
      if(!output){
        error();
        printf("could not write to file \"%s\"\n",target);
        return 1;
      }
    }else if(!strcmp(argv[a],"-f")){
      if(a==argc-1) return help();
      char* target=argv[++a];
      if(input){
        error();
        printf("input is already defined");
        return 1;
      }
      input=fopen(target,"r");
      if(!input){
        error();
        printf("could not read from file \"%s\"\n",target);
        return 1;
      }
    }else if(!strcmp(argv[a],"--help")) return help() && 0;
    else return help();
  }
  if(!input) return help();
  if(!output){
    output=fopen("output.lua","w");
    if(!output){
      error();
      printf("could not write to file \"output.lua\"\n");
      return 1;
    }
  }

  // Compile
  moonshot_init();
  moonshot_compile(input);
  int n=moonshot_num_errors();
  if(n==1) printf("Moonshot compiler returned 1 error\n");
  if(n>1) printf("Moonshot compiler returned %i errors\n",n);
  for(int a=0;a<n;a++){
    error();
    printf("%s\n",moonshot_next_error());
  }
  moonshot_destroy();
  return n;
}
