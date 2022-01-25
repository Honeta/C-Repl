#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <math.h>
#include <stdlib.h>
#include <signal.h>
#include <dlfcn.h>

static char line[4096];
long long function_count, expression_count;

void handler(int arg) {
  for (long long i = 1; i <= function_count; ++i) {
    char name_so[100];
    sprintf(name_so, "libfunc%lld.so", i);
    remove(name_so);
  }  
  for (long long i = 1; i <= expression_count; ++i) {
    char name_so[100];
    sprintf(name_so, "libexpr%lld.so", i);
    remove(name_so);
  } 
  exit(0);
}

int isFunction(char line[]) {
  int offset;
  for (offset = 0; line[offset] == ' '; ++offset);
  return strncmp(line + offset, "unsigned ", 9) == 0 
      || strncmp(line + offset, "short ", 4) == 0 
      || strncmp(line + offset, "int ", 4) == 0 
      || strncmp(line + offset, "double ", 7) == 0 
      || strncmp(line + offset, "long ", 5) == 0
      || strncmp(line + offset, "float ", 6) == 0;
}

int main() {
  signal(SIGINT, handler);
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) break;
    if (isFunction(line)) {
      ++function_count;
      FILE *fp = fopen("func.c", "w");
      fprintf(fp, "%s", line);
      fclose(fp);
      char name_so[100];
      sprintf(name_so, "libfunc%lld.so", function_count);
      pid_t pid = fork();
      if (fork() == 0)
        execlp("gcc", "gcc", "-fPIC", "-w", "-shared", "-o", name_so, "func.c", NULL);
      else 
        waitpid(pid, NULL, 0);
      remove("func.c");
      char path_so[100];
      sprintf(path_so, "./%s", name_so);
      void *handle = dlopen(path_so, RTLD_LAZY | RTLD_GLOBAL);
      if (!handle)
        printf("Invalid syntax.\n");
      else
        printf("Get: %s", line);
    } else {
      ++expression_count;
      char name_expr[100];
      sprintf(name_expr, "__expr_wrapper_%lld", expression_count);
      FILE *fp = fopen("expr.c", "w");
      fprintf(fp, "double %s() {return %s;}", name_expr, line);
      fclose(fp);
      char name_so[100];
      sprintf(name_so, "libexpr%lld.so", expression_count);
      pid_t pid = fork();
      if (fork() == 0)
        execlp("gcc", "gcc", "-fPIC", "-w", "-shared", "-o", name_so, "expr.c", NULL);
      else
        waitpid(pid, NULL, 0);
      remove("expr.c");
      char path_so[100];
      sprintf(path_so, "./%s", name_so);
      void *handle = dlopen(path_so, RTLD_LAZY);
      if (!handle) {
        printf("Invalid syntax.\n");
      } else {
        double (*expr)() = dlsym(handle, name_expr);
        printf("%g\n", expr());
        dlclose(handle);
      }
      remove(path_so);
    }
  }
}
