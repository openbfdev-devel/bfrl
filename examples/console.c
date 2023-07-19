#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>
#include <termios.h>
#include <bfrl/readline.h>

static unsigned int
console_read(char *str, unsigned int len, void *data)
{
    return read(STDIN_FILENO, str, len);
}

static void
console_write(const char *str, unsigned int len, void *data)
{
    write(STDOUT_FILENO, str, len);
}

int main(void)
{
    struct termios term, save;
    struct bfrl_state *rstate;
    int retval;

    retval = tcgetattr(STDIN_FILENO, &term);
    if (retval)
        err(retval, "tcgetattr");

    save = term;
    term.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    retval = tcsetattr(STDIN_FILENO, TCSANOW, &term);
    if (retval)
        err(retval, "tcsetattr");

    rstate = bfrl_alloc(NULL, console_read, console_write, NULL);
    if (!rstate)
        err(-ENOMEM, "bfrl_alloc");

    for (;;) {
        const char *line;

        line = bfrl_readline(rstate, "# ", "> ");
        if (!line)
            continue;

        printf("%s\n", line);
        if (!strcmp(line, "exit"))
            break;
    }

    bfrl_free(rstate);
    return tcsetattr(STDIN_FILENO, TCSANOW, &save);
}
