#include "progs.h"
#include "kernel.h"
#include "../libc/stdlib.h"
#include "../libc/stdio.h"
#include "../libc/vect.h"
#include "../libc/string.h"
#include "../libc/stdint.h"
#include "../libc/power.h"
#include "../multitasking/mt.h"
#include "../drivers/screen.h"
#include "../fs/vfs.h"
#include "../fs/pipe.h"
#include "../syscall/syscall.h"

#define TASK_STACK_SIZE 4096

static inline uint64_t sc0(uint64_t n) //syscall
{
    uint64_t r;
    asm volatile("int $0x80" : "=a"(r) : "a"(n) : "memory");
    return r;
}

static void cmd_help(int, char **);
static void cmd_clear(int, char **);
static void cmd_echo(int, char **);
static void cmd_calc(int, char **);
static void cmd_meminfo(int, char **);
static void cmd_cd(int, char **);
static void cmd_mkdir(int, char **);
static void cmd_shutdown(int, char **);
static void cmd_reboot(int, char **);
static void cmd_ls(int, char **);
static void cmd_cat(int, char **);
static void cmd_test(int, char **);

typedef void (*cmd_fn_t)(int, char **);

typedef struct {
    const char *name;
    cmd_fn_t    fn;
    const char *desc;
} command_t;

static const command_t commands[] = {
    { "help",     cmd_help,     "list commands" },
    { "clear",    cmd_clear,    "clear the screen" },
    { "echo",     cmd_echo,     "print arguments" },
    { "calc",     cmd_calc,     "calc <a> <op> <b>" },
    { "meminfo",  cmd_meminfo,  "print allocated bytes" },
    { "cd",       cmd_cd,       "change directory" },
    { "mkdir",    cmd_mkdir,    "make directory" },
    { "shutdown", cmd_shutdown, "power off" },
    { "reboot",   cmd_reboot,   "reboot" },
    { "ls",       cmd_ls,       "list directory" },
    { "cat",      cmd_cat,      "print files / stdin" },
    { "test",     cmd_test,     "run test task" },
};

#define NCOMMANDS (sizeof(commands) / sizeof(commands[0]))

static const command_t *resolve(const char *name)
{
    for (uint32_t i = 0; i < NCOMMANDS; i++)
        if (strcmp(commands[i].name, name) == 0)
            return &commands[i];
    return NULL;
}

void idle_task(int argc, char **argv)
{
    (void)argc; (void)argv;
    while (1) {
        task_reap();
        yield();
    }
}

static void cmd_help(int argc, char **argv)
{
    (void)argc; (void)argv;
    print("commands:\n");
    for (uint32_t i = 0; i < NCOMMANDS; i++) {
        print("  ");
        print(commands[i].name);
        print(" - ");
        print(commands[i].desc);
        print("\n");
    }
    print("operators: |  <  >  >>\n");
}

static void cmd_clear(int argc, char **argv)
{
    (void)argc; (void)argv;
    sc0(SYSCALL_CLEAR);
}

static void cmd_echo(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        print(argv[i]);
        if (i + 1 < argc) print(" ");
    }
    print("\n");
}

static void cmd_calc(int argc, char **argv)
{
    if (argc != 4) { print("usage: calc <a> <op> <b>\n"); return; }
    int a = atoi(argv[1]), b = atoi(argv[3]);
    char *op = argv[2], res[32] = {0};

    if      (strcmp(op, "+") == 0) itoa(a + b, res);
    else if (strcmp(op, "-") == 0) itoa(a - b, res);
    else if (strcmp(op, "*") == 0) itoa(a * b, res);
    else if (strcmp(op, "/") == 0) {
        if (b == 0) { print("calc: division by zero\n"); return; }
        itoa(a / b, res);
    } else { print("calc: unknown operator\n"); return; }
    print(res);
    print("\n");
}

static void cmd_meminfo(int argc, char **argv)
{
    (void)argc; (void)argv;
    char str[32];
    itoa((int)sc0(SYSCALL_MALLOC_STATS), str);
    print(str);
    print(" bytes\n");
}

static void cmd_cd(int argc, char **argv)
{
    if (argc < 2) { print("usage: cd <dir>\n"); return; }
    if (change_cwd(argv[1]) != 0)
        print("cd: no such directory\n");
}

static void cmd_mkdir(int argc, char **argv)
{
    if (argc < 2) { print("usage: mkdir <dir>\n"); return; }
    if (mkdir(argv[1]) != 0)
        print("mkdir: cannot create directory\n");
}

static void cmd_shutdown(int argc, char **argv)
{
    (void)argc; (void)argv;
    print("Stopping the CPU. Bye!\n");
    sc0(SYSCALL_POWER_OFF);
}

static void cmd_reboot(int argc, char **argv)
{
    (void)argc; (void)argv;
    sc0(SYSCALL_REBOOT);
}

static void cmd_ls(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : ".";
    dirent_t de;
    uint32_t idx = 0;
    while (readdir(path, idx, &de) == 0) {
        de.name[VFS_MAX_NAME - 1] = '\0';
        print(de.name);
        if (de.flags & VFS_FLAG_DIRECTORY) print("/");
        print("  ");
        idx++;
    }
    print("\n");
}

static void cmd_cat(int argc, char **argv)
{
    char buf[257];
    if (argc < 2) {
        int n;
        while ((n = read(0, buf, sizeof(buf) - 1)) > 0)
            write(1, buf, n);
        return;
    }
    for (int i = 1; i < argc; i++) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            print("cat: cannot open "); print(argv[i]); print("\n");
            continue;
        }
        int n;
        while ((n = read(fd, buf, sizeof(buf) - 1)) > 0)
            write(1, buf, n);
        close(fd);
    }
}

static void cmd_test(int argc, char **argv)
{
    if (argc > 0) {
        print("argv:\n");
        for (int i = 0; i < argc; i++) {
            char istr[10]; itoa(i, istr);
            print(istr); print(": "); print(argv[i]); print("\n");
        }
    }
    print("Enter your name: ");
    char str[64]; int i = 0, c;
    while ((c = getc()) != '\n' && c >= 0){
        if (i < (int)sizeof(str) - 1) str[i++] = (char)c;
    }
    str[i] = '\0';
    print("Hi, "); print(str); print("\n");
}

static int tokenize(const char *line, vect_t *out)
{
    const char *p = line;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        char buf[256]; uint32_t n = 0;
        if (*p == '|' || *p == '<' || *p == '>') {
            buf[n++] = *p++;
            if (p[-1] == '>' && *p == '>') buf[n++] = *p++;
        } else {
            while (*p && !isspace((unsigned char)*p) &&
                   *p != '|' && *p != '<' && *p != '>') {
                if (*p == '"') {
                    p++;
                    while (*p && *p != '"') { if (n < sizeof(buf)-1) buf[n++] = *p; p++; }
                    if (*p == '"') p++;
                } else { if (n < sizeof(buf)-1) buf[n++] = *p; p++; }
            }
        }
        buf[n] = '\0';
        char *tok = malloc(n + 1);
        if (!tok) return -1;
        memcpy(tok, buf, n + 1);
        vect_push(out, &tok);
    }
    return 0;
}

#define MAX_STAGES 8
#define MAX_ARGS   32

typedef struct {
    char *argv[MAX_ARGS];
    int   argc;
    char *in, *out;
    int   append;
} stage_t;

static int parse_pipeline(vect_t *toks, stage_t *st, int *nst)
{
    int s = 0, ntok = vect_get_size(toks);
    memset(st, 0, sizeof(stage_t) * MAX_STAGES);
    for (int i = 0; i < ntok; i++) {
        char *t = *(char **)vect_get(toks, i);
        if (strcmp(t, "|") == 0) {
            if (st[s].argc == 0) return -1;
            if (++s >= MAX_STAGES) return -1;
            continue;
        }
        if (strcmp(t, "<") == 0 || strcmp(t, ">") == 0 || strcmp(t, ">>") == 0) {
            if (i + 1 >= ntok) return -1;
            char *file = *(char **)vect_get(toks, ++i);
            if (strcmp(t, "<") == 0) st[s].in = file;
            else { st[s].out = file; st[s].append = (t[1] == '>'); }
            continue;
        }
        if (st[s].argc >= MAX_ARGS - 1) return -1;
        st[s].argv[st[s].argc++] = t;
    }
    if (st[s].argc == 0) return -1;
    *nst = s + 1;
    return 0;
}

void exec(char *cmd)
{
    if (!cmd) return;
    if (*cmd != '\n') return;

    vect_t toks;
    vect_init(&toks, sizeof(char *));
    if (tokenize(cmd, &toks) != 0) goto out;
    if (vect_get_size(&toks) == 0) goto out;

    stage_t *st = malloc(sizeof(stage_t) * MAX_STAGES);
    if (!st) goto out;
    int nst = 0;
    if (parse_pipeline(&toks, st, &nst) != 0) {
        print("syntax error\n");
        free(st); goto out;
    }

    const command_t *c[MAX_STAGES];
    for (int i = 0; i < nst; i++) {
        c[i] = resolve(st[i].argv[0]);
        if (!c[i]) {
            print("unknown command: "); print(st[i].argv[0]); print("\n");
            free(st); goto out;
        }
    }

    vfs_file_t *fin[MAX_STAGES] = {0}, *fout[MAX_STAGES] = {0};
    for (int i = 0; i < nst; i++) {
        if (st[i].in) {
            fin[i] = vfs_open(st[i].in, O_RDONLY);
            if (!fin[i]) { print("cannot open "); print(st[i].in); print("\n"); goto fail_files; }
        }
        if (st[i].out) {
            fout[i] = vfs_open(st[i].out,
                O_WRONLY | O_CREAT | (st[i].append ? O_APPEND : O_TRUNC));
            if (!fout[i]) { print("cannot open "); print(st[i].out); print("\n"); goto fail_files; }
        }
    }

    vfs_file_t *pr[MAX_STAGES - 1], *pw[MAX_STAGES - 1];
    int npipes = nst - 1, made = 0;
    for (int i = 0; i < npipes; i++) {
        if (pipe_create(&pr[i], &pw[i]) != 0) goto fail_pipes;
        made++;
    }

    int ids[MAX_STAGES], spawned = 0;
    for (int i = 0; i < nst; i++) {
        vfs_file_t *fd0 = fin[i]  ? fin[i]  : (i > 0     ? pr[i - 1] : NULL);
        vfs_file_t *fd1 = fout[i] ? fout[i] : (i < nst - 1 ? pw[i]   : NULL);

        if (i > 0     && fd0 != pr[i - 1]) vfs_close(pr[i - 1]);
        if (i < nst - 1 && fd1 != pw[i])   vfs_close(pw[i]);

        if (st[i].argc > 1)
            ids[i] = task_create_ex(c[i]->fn, st[i].argc, st[i].argv, TASK_STACK_SIZE, 1, 1, fd0, fd1, NULL);
        else
            ids[i] = task_create_ex(c[i]->fn, 0, 0, TASK_STACK_SIZE, 1, 1, fd0, fd1, NULL);

        if (ids[i] < 0) {
            if (i > 0     && fd0 == pr[i - 1]) vfs_close(pr[i - 1]);
            if (i < nst - 1 && fd1 == pw[i])   vfs_close(pw[i]);
            if (i < npipes) vfs_close(pr[i]);
            for (int j = i + 1; j < npipes; j++) { vfs_close(pr[j]); vfs_close(pw[j]); }
            if (fin[i])  vfs_close(fin[i]);
            if (fout[i]) vfs_close(fout[i]);
            break;
        }
        spawned++;
    }

    for (int i = 0; i < spawned; i++)
        task_wait(ids[i]);

    free(st);
    goto out;

fail_pipes:
    for (int i = 0; i < made; i++) { vfs_close(pr[i]); vfs_close(pw[i]); }
fail_files:
    for (int i = 0; i < nst; i++) {
        if (fin[i])  vfs_close(fin[i]);
        if (fout[i]) vfs_close(fout[i]);
    }
    free(st);
out:
    for (int i = 0; i < vect_get_size(&toks); i++)
        free(*(char **)vect_get(&toks, i));
    vect_delete(&toks);
}

void shell(int argc, char **argv)
{
    (void)argc; (void)argv;
    while (1) {
        if (_is_current_task_foreground == 1) { yield(); continue; }

        char cwd[256];
        get_cwd(cwd, sizeof(cwd));
        print("[ "); print(cwd); print(" ]> ");

        char str[INPUT_BUF_SIZE];
        int i = 0, c = '0';
        while (c != '\n') {
            c = getc();
            if (c < 0) break;
            if (c == '\b' && i > 0) { backspace(str); --i; continue; }
            str[i++] = (char)c;
            if (i == INPUT_BUF_SIZE - 1) {
                str[i] = '\0';
                print("\nERR: too big input\n");
                i = 0;
            }
        }
        str[i] = '\0';
        exec(str);
    }
}
