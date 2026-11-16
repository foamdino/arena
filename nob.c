// arena build script
// Bootstrap: ./bootstrap.sh
// Subsequent: ./nob  (builds and runs the tests; nob rebuilds itself if nob.c changes)

#define NOB_IMPLEMENTATION
#include "vendor/nob.h"
#include <stdio.h>
#include <unistd.h>

// UB aborts instead of logging, so a test that hits it fails
#define CFLAGS "-Wall", "-Wextra", "-Wpedantic", "-std=c11", "-D_DEFAULT_SOURCE", \
               "-O0", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=undefined", \
               "-I."

static void write_compile_commands(void)
{
    FILE *f = fopen("compile_commands.json", "w");
    if (!f) { nob_log(NOB_WARNING, "cannot write compile_commands.json"); return; }

    char cwd[1024];
    if (!getcwd(cwd, sizeof cwd)) { fclose(f); return; }
    fprintf(f,
        "[\n"
        "  {\n"
        "    \"directory\": \"%s\",\n"
        "    \"command\": \"cc -std=c11 -D_DEFAULT_SOURCE -I. -c test/test_arena.c\",\n"
        "    \"file\": \"test/test_arena.c\"\n"
        "  }\n"
        "]\n",
        cwd);
    fclose(f);
}

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    write_compile_commands();
    if (!nob_mkdir_if_not_exists("build")) return 1;

    Nob_Cmd cmd = {0};
    nob_cmd_append(&cmd, "cc", CFLAGS, "-o", "build/test_arena", "test/test_arena.c");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    nob_cmd_append(&cmd, "./build/test_arena");
    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;
    return 0;
}
