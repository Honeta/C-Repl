#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <math.h>
#include <dlfcn.h>

int isFunction(char line[])
{
  if (line[0] == 'i' && line[1] == 'n' && line[2] == 't' && line[3] == ' ')
    return 1;
  return 0;
}

int main(int argc, char *argv[])
{
  static char line[4096];
  int function_count = 0, expression_count = 0;
  while (1)
  {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin))
    {
      break;
    }
    //printf("Got %zu chars.\n", strlen(line)); // WTF?
    if (isFunction(line))
    {
      ++function_count;
      FILE *fp = fopen("func.c", "w");
      fprintf(fp, "%s", line);
      fclose(fp);
      char name_so[100];
      sprintf(name_so, "libfunc%d.so", function_count);
      pid_t pid = fork();
      if (fork() == 0)
        execlp("gcc", "gcc", "-fPIC", "-w", "-shared", "-o", name_so, "func.c", NULL);
      else
      {
        int status;
        waitpid(pid, &status, 0);
      }
      remove("func.c");
      char path_so[100];
      sprintf(path_so, "./%s", name_so);
      void *handle = dlopen(path_so, RTLD_LAZY | RTLD_GLOBAL);
      if (!handle)
        printf("Invalid syntax.\n");
      else
        printf("Get: %s", line);
    }
    else
    {
      ++expression_count;
      char name_expr[100];
      sprintf(name_expr, "__expr_wrapper_%d", expression_count);
      FILE *fp = fopen("expr.c", "w");
      fprintf(fp, "int %s() {return %s;}", name_expr, line);
      fclose(fp);
      char name_so[100];
      sprintf(name_so, "libexpr%d.so", expression_count);
      pid_t pid = fork();
      if (fork() == 0)
        execlp("gcc", "gcc", "-fPIC", "-w", "-shared", "-o", name_so, "expr.c", NULL);
      else
      {
        int status;
        waitpid(pid, &status, 0);
      }
      remove("expr.c");
      char path_so[100];
      sprintf(path_so, "./%s", name_so);
      void *handle = dlopen(path_so, RTLD_LAZY);
      if (!handle)
        printf("Invalid syntax.\n");
      else
      {
        int (*expr)() = dlsym(handle, name_expr);
        printf("%d\n", expr());
        dlclose(handle);
      }
      remove(path_so);
    }
  }
}
