#include "alg.h"

#include <sys/wait.h>
#include <unistd.h>

/** Mostra como usar o alg e encerra. */
static void usage(void)
{
    fprintf(stderr, "uso: alg [-O] [--emit-c] [-o saida] programa.alg\n"
                    "  sem -o:     compila e roda (stdin/stdout passam direto)\n"
                    "  -o saida:   gera o executável\n"
                    "  --emit-c:   mostra o C gerado\n"
                    "  -O:         tira as checagens de índice/divisão (competitiva)\n");
    exit(2);
}

/** Lê o arquivo inteiro para a memória. */
static char *read_file(const char *path)
{
    FILE *file = fopen(path, "rb");

    if (!file)
    {
        perror(path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *text = allocate_zeroed((size_t)size + 1);

    if (fread(text, 1, (size_t)size, file) != (size_t)size)
    {
        perror(path);
        exit(1);
    }

    fclose(file);
    return text;
}

/** Roda um programa e espera; devolve o código de saída. */
static int run_command(char *const argv[])
{
    pid_t pid = fork();

    if (pid == 0)
    {
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }

    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

/** Compila o .alg; com -o gera o executável, senão compila e roda. */
int main(int argc, char **argv)
{
    const char *input_path = NULL;
    const char *output_path = NULL;
    int emit_c = 0;
    int no_checks = 0;

    for (int i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "-O"))
        {
            no_checks = 1;
        }
        else if (!strcmp(argv[i], "--emit-c"))
        {
            emit_c = 1;
        }
        else if (!strcmp(argv[i], "-o") && i + 1 < argc)
        {
            output_path = argv[++i];
        }
        else if (argv[i][0] == '-' || input_path)
        {
            usage();
        }
        else
        {
            input_path = argv[i];
        }
    }

    if (!input_path)
    {
        usage();
    }

    source_file_name = input_path;

    Program parsed_program = {0};
    TokenList toks = tokenize(read_file(input_path), &parsed_program.language);
    parsed_program.statements = parse(&toks, parsed_program.language);
    check_program(&parsed_program);
    analyze_ranges(&parsed_program);
    parsed_program.no_checks = no_checks;

    TextBuffer c_code = {0};
    generate_c(&parsed_program, &c_code);

    if (emit_c)
    {
        fputs(c_code.text, stdout);
        return 0;
    }

    const char *temporary = getenv("TMPDIR") ? getenv("TMPDIR") : "/tmp";
    char c_path[4096];
    char binary_path[4096];
    snprintf(c_path, sizeof c_path, "%s/alg-XXXXXX.c", temporary);

    int file_descriptor = mkstemps(c_path, 2);

    if (file_descriptor < 0 ||
        write(file_descriptor, c_code.text, c_code.count) != (ssize_t)c_code.count)
    {
        perror("alg: arquivo temporário");
        return 1;
    }

    close(file_descriptor);
    snprintf(binary_path, sizeof binary_path, "%.*s.bin", (int)strlen(c_path) - 2, c_path);

    const char *compiler = getenv("ALG_CC") ? getenv("ALG_CC") : "cc";
    char *ccargv[] = {
        (char *)compiler,
        "-std=gnu11",
        "-O2",
        "-w",
        no_checks ? "-DALG_NOCHECK" : "-DALG_CHECKED",
        "-o",
        (char *)(output_path ? output_path : binary_path),
        c_path,
        "-lm",
        "-lpthread",
        NULL,
    };

    int exit_code = run_command(ccargv);
    unlink(c_path);

    if (exit_code != 0)
    {
        fprintf(stderr, "alg: o compilador C falhou (isso é bug do alg, por favor reporte)\n");
        return 1;
    }

    if (output_path)
    {
        return 0;
    }

    char *progv[] = {binary_path, NULL};
    exit_code = run_command(progv);
    unlink(binary_path);
    return exit_code;
}
