#include "../src/moonshot.h"
#include <string.h>
#include <stdio.h>

// Output
static void error()
{
  printf("\033[31;1merror:\033[0m ");
}
static void indent(int i, const char *msg)
{
  for (int a = 0; a < i; a++)
    printf("  ");
  printf("%s", msg);
}
static void help()
{
  indent(0, "Usage: moonshot [options] file\n");
  indent(2, "options include:\n\n");
  indent(0, "Moonshot options\n");
  indent(1, "-o <file>");
  indent(2, "Set output file\n");
  indent(1, "--version");
  indent(2, "Print Moonshot version\n");
  indent(1, "--print");
  indent(3, "Write Lua code to stdout\n");
  indent(1, "--help");
  indent(3, " Print usage options\n");
}

// Argument parsing
static int check_args(FILE **output, char **source, int argc, char **argv, int a)
{
  if (!strcmp(argv[a], "--version"))
  {
    printf("Moonshot v%s\n", VERSION);
    return 2;
  }
  if (!strcmp(argv[a], "--help"))
  {
    help();
    return 2;
  }
  if (!strcmp(argv[a], "--print"))
  {
    if (*output)
    {
      error();
      printf("output is already defined\n");
      return 1;
    }
    *output = stdout;
  }
  else if (!strcmp(argv[a], "-o"))
  {
    if (a == argc - 1)
    {
      help();
      return 1;
    }
    if (*output)
    {
      error();
      printf("output is already defined\n");
      return 1;
    }
    *output = fopen(argv[a + 1], "w");
  }
  else
  {
    if (a < argc - 1)
    {
      help();
      return 1;
    }
    *source = argv[a];
  }
  return 0;
}

int main(int argc, char **argv)
{
  char *source = NULL;
  FILE *output = NULL;
  FILE *input = NULL;

  // Parse arguments
  for (int a = 1; a < argc; a++)
  {
    int res = check_args(&output, &source, argc, argv, a);
    if (res)
    {
      if (output && output != stdout)
        fclose(output);
      return res % 2;
    }
    if (!strcmp(argv[a], "-o"))
      a++;
  }

  // Validate I/O arguments
  if (!source)
  {
    help();
    return 1;
  }
  if (!output)
  {
    output = fopen("output.lua", "w");
  }
  if (!output)
  {
    error();
    printf("failure to open output stream\n");
    return 1;
  }
  input = fopen(source, "r");
  if (!input)
  {
    if (output && output != stdout)
      fclose(output);
    error();
    printf("could not open file \"%s\"\n", source);
    return 1;
  }

  // Compile
  moonshot_init();
  moonshot_configure(input, output);
  init_requires();
  dummy_required_file(source);
  moonshot_compile();
  int n = moonshot_num_errors();
  if (n == 1)
    printf("Moonshot compiler returned 1 error\n");
  if (n > 1)
    printf("Moonshot compiler returned %i errors\n", n);
  for (int a = 0; a < n; a++)
  {
    error();
    printf("%s\n", moonshot_next_error());
  }
  if (output != stdout)
    fclose(output);
  if (input != stdin)
    fclose(input);
  moonshot_destroy();
  return n;
}
