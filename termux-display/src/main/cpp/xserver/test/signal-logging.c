/**
 * Copyright © 2012 Canonical, Ltd.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a
 *  copy of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice (including the next
 *  paragraph) shall be included in all copies or substantial portions of the
 *  Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

/* Test relies on assert() */
#undef NDEBUG

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdint.h>
#include <unistd.h>
#include "assert.h"
#include "misc.h"

#include "tests-common.h"

struct number_format_test {
    uint64_t number;
    char string[21];
    char hex_string[17];
};

struct signed_number_format_test {
    int64_t number;
    char string[21];
};

struct float_number_format_test {
    double number;
    char string[21];
};

static Bool
check_signed_number_format_test(long int number)
{
    char string[21];
    char expected[21];

    sprintf(expected, "%ld", number);
    FormatInt64(number, string);
    if(strncmp(string, expected, 21) != 0) {
        fprintf(stderr, "Failed to convert %jd to decimal string (expected %s but got %s)\n",
                (intmax_t) number, expected, string);
        return FALSE;
    }

    return TRUE;
}

static Bool
check_float_format_test(double number)
{
    char string[21];
    char expected[21];

    /* we currently always print float as .2f */
    sprintf(expected, "%.2f", number);

    FormatDouble(number, string);
    if(strncmp(string, expected, 21) != 0) {
        fprintf(stderr, "Failed to convert %f to string (%s vs %s)\n",
                number, expected, string);
        return FALSE;
    }

    return TRUE;
}

static Bool
check_number_format_test(long unsigned int number)
{
    char string[21];
    char expected[21];

    sprintf(expected, "%lu", number);

    FormatUInt64(number, string);
    if(strncmp(string, expected, 21) != 0) {
        fprintf(stderr, "Failed to convert %ju to decimal string (%s vs %s)\n",
                (intmax_t) number, expected, string);
        return FALSE;
    }

    sprintf(expected, "%lx", number);
    FormatUInt64Hex(number, string);
    if(strncmp(string, expected, 17) != 0) {
        fprintf(stderr, "Failed to convert %ju to hexadecimal string (%s vs %s)\n",
                (intmax_t) number, expected, string);
        return FALSE;
    }

    return TRUE;
}

/* FIXME: max range stuff */
double float_tests[] = { 0, 5, 0.1, 0.01, 5.2342, 10.2301,
                         -1, -2.00, -0.6023, -1203.30
                        };

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverflow"

static void
number_formatting(void)
{
    int i;
    long unsigned int unsigned_tests[] = { 0,/* Zero */
                                           5, /* Single digit number */
                                           12, /* Two digit decimal number */
                                           37, /* Two digit hex number */
                                           0xC90B2, /* Large < 32 bit number */
                                           0x15D027BF211B37A, /* Large > 32 bit number */
                                           0xFFFFFFFFFFFFFFFF, /* Maximum 64-bit number */
    };

    long int signed_tests[] = { 0,/* Zero */
                                5, /* Single digit number */
                                12, /* Two digit decimal number */
                                37, /* Two digit hex number */
                                0xC90B2, /* Large < 32 bit number */
                                0x15D027BF211B37A, /* Large > 32 bit number */
                                0x7FFFFFFFFFFFFFFF, /* Maximum 64-bit signed number */
                                -1, /* Single digit number */
                                -12, /* Two digit decimal number */
                                -0xC90B2, /* Large < 32 bit number */
                                -0x15D027BF211B37A, /* Large > 32 bit number */
                                -0x7FFFFFFFFFFFFFFF, /* Maximum 64-bit signed number */
    } ;

    for (i = 0; i < ARRAY_SIZE(unsigned_tests); i++)
        assert(check_number_format_test(unsigned_tests[i]));

    for (i = 0; i < ARRAY_SIZE(signed_tests); i++)
        assert(check_signed_number_format_test(signed_tests[i]));

    for (i = 0; i < ARRAY_SIZE(float_tests); i++)
        assert(check_float_format_test(float_tests[i]));
}
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
static void logging_format(void)
{
    const char *log_file_path = "/tmp/Xorg-logging-test.log";
    const char *str = "%s %d %u %% %p %i";
    char buf[1024];
    int i;
    unsigned int ui;
    long li;
    unsigned long lui;
    FILE *f;
    char read_buf[2048];
    char *logmsg;
    uintptr_t ptr;

    /* set up buf to contain ".....end" */
    memset(buf, '.', sizeof(buf));
    strcpy(&buf[sizeof(buf) - 4], "end");

    LogInit(log_file_path, NULL);
    assert((f = fopen(log_file_path, "r")));

#define read_log_msg(msg) do {                                  \
        msg = fgets(read_buf, sizeof(read_buf), f);             \
        assert(msg != NULL);                                   \
        msg = strchr(read_buf, ']');                            \
        assert(msg != NULL);                                    \
        assert(strlen(msg) > 2);                                \
        msg = msg + 2; /* advance past [time.stamp] */          \
    } while (0)

    /* boring test message */
    LogMessageVerbSigSafe(X_ERROR, -1, "test message\n");
    read_log_msg(logmsg);
    assert(strcmp(logmsg, "(EE) test message\n") == 0);

    /* long buf is truncated to "....en\n" */
    LogMessageVerbSigSafe(X_ERROR, -1, buf);
    read_log_msg(logmsg);
    assert(strcmp(&logmsg[strlen(logmsg) - 3], "en\n") == 0);

    /* same thing, this time as string substitution */
    LogMessageVerbSigSafe(X_ERROR, -1, "%s", buf);
    read_log_msg(logmsg);
    assert(strcmp(&logmsg[strlen(logmsg) - 3], "en\n") == 0);

    /* strings containing placeholders should just work */
    LogMessageVerbSigSafe(X_ERROR, -1, "%s\n", str);
    read_log_msg(logmsg);
    assert(strcmp(logmsg, "(EE) %s %d %u %% %p %i\n") == 0);

    /* literal % */
    LogMessageVerbSigSafe(X_ERROR, -1, "test %%\n");
    read_log_msg(logmsg);
    assert(strcmp(logmsg, "(EE) test %\n") == 0);

    /* character */
    LogMessageVerbSigSafe(X_ERROR, -1, "test %c\n", 'a');
    read_log_msg(logmsg);
    assert(strcmp(logmsg, "(EE) test a\n") == 0);

    /* something unsupported % */
    LogMessageVerbSigSafe(X_ERROR, -1, "test %Q\n");
    read_log_msg(logmsg);
    assert(strstr(logmsg, "BUG") != NULL);
    LogMessageVerbSigSafe(X_ERROR, -1, "\n");
    fseek(f, 0, SEEK_END);

    /* string substitution */
    LogMessageVerbSigSafe(X_ERROR, -1, "%s\n", "substituted string");
    read_log_msg(logmsg);
    assert(strcmp(logmsg, "(EE) substituted string\n") == 0);

    /* Invalid format */
    LogMessageVerbSigSafe(X_ERROR, -1, "%4", 4);
    read_log_msg(logmsg);
    assert(strcmp(logmsg, "(EE) ") == 0);
    LogMessageVerbSigSafe(X_ERROR, -1, "\n");
    fseek(f, 0, SEEK_END);

    /* %hld is bogus */
    LogMessageVerbSigSafe(X_ERROR, -1, "%hld\n", 4);
    read_log_msg(logmsg);
    assert(strstr(logmsg, "BUG") != NULL);
    LogMessageVerbSigSafe(X_ERROR, -1, "\n");
    fseek(f, 0, SEEK_END);

    /* number substitution */
    ui = 0;
    do {
        char expected[30];
        sprintf(expected, "(EE) %u\n", ui);
        LogMessageVerbSigSafe(X_ERROR, -1, "%u\n", ui);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        sprintf(expected, "(EE) %x\n", ui);
        LogMessageVerbSigSafe(X_ERROR, -1, "%x\n", ui);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        if (ui == 0)
            ui = 1;
        else
            ui <<= 1;
    } while(ui);

    lui = 0;
    do {
        char expected[30];
        sprintf(expected, "(EE) %lu\n", lui);
        LogMessageVerbSigSafe(X_ERROR, -1, "%lu\n", lui);
        read_log_msg(logmsg);

        sprintf(expected, "(EE) %lld\n", (unsigned long long)ui);
        LogMessageVerbSigSafe(X_ERROR, -1, "%lld\n", (unsigned long long)ui);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        sprintf(expected, "(EE) %lx\n", lui);
        printf("%s\n", expected);
        LogMessageVerbSigSafe(X_ERROR, -1, "%lx\n", lui);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        sprintf(expected, "(EE) %llx\n", (unsigned long long)ui);
        LogMessageVerbSigSafe(X_ERROR, -1, "%llx\n", (unsigned long long)ui);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        if (lui == 0)
            lui = 1;
        else
            lui <<= 1;
    } while(lui);

    /* signed number substitution */
    i = 0;
    do {
        char expected[30];
        sprintf(expected, "(EE) %d\n", i);
        LogMessageVerbSigSafe(X_ERROR, -1, "%d\n", i);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        sprintf(expected, "(EE) %d\n", i | INT_MIN);
        LogMessageVerbSigSafe(X_ERROR, -1, "%d\n", i | INT_MIN);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        if (i == 0)
            i = 1;
        else
            i <<= 1;
    } while(i > INT_MIN);

    li = 0;
    do {
        char expected[30];
        sprintf(expected, "(EE) %ld\n", li);
        LogMessageVerbSigSafe(X_ERROR, -1, "%ld\n", li);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        sprintf(expected, "(EE) %ld\n", li | LONG_MIN);
        LogMessageVerbSigSafe(X_ERROR, -1, "%ld\n", li | LONG_MIN);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        sprintf(expected, "(EE) %lld\n", (long long)li);
        LogMessageVerbSigSafe(X_ERROR, -1, "%lld\n", (long long)li);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        sprintf(expected, "(EE) %lld\n", (long long)(li | LONG_MIN));
        LogMessageVerbSigSafe(X_ERROR, -1, "%lld\n", (long long)(li | LONG_MIN));
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        if (li == 0)
            li = 1;
        else
            li <<= 1;
    } while(li > LONG_MIN);


    /* pointer substitution */
    /* we print a null-pointer differently to printf */
    LogMessageVerbSigSafe(X_ERROR, -1, "%p\n", NULL);
    read_log_msg(logmsg);
    assert(strcmp(logmsg, "(EE) 0x0\n") == 0);

    ptr = 1;
    do {
        char expected[30];
#ifdef __sun /* Solaris doesn't autoadd "0x" to %p format */
        sprintf(expected, "(EE) 0x%p\n", (void*)ptr);
#else
        sprintf(expected, "(EE) %p\n", (void*)ptr);
#endif
        LogMessageVerbSigSafe(X_ERROR, -1, "%p\n", (void*)ptr);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);
        ptr <<= 1;
    } while(ptr);


    for (i = 0; i < ARRAY_SIZE(float_tests); i++) {
        double d = float_tests[i];
        char expected[30];
        sprintf(expected, "(EE) %.2f\n", d);
        LogMessageVerbSigSafe(X_ERROR, -1, "%f\n", d);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        /* test for length modifiers, we just ignore them atm */
        LogMessageVerbSigSafe(X_ERROR, -1, "%.3f\n", d);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        LogMessageVerbSigSafe(X_ERROR, -1, "%3f\n", d);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);

        LogMessageVerbSigSafe(X_ERROR, -1, "%.0f\n", d);
        read_log_msg(logmsg);
        assert(strcmp(logmsg, expected) == 0);
    }


    LogClose(EXIT_NO_ERROR);
    unlink(log_file_path);

#undef read_log_msg
}
#pragma GCC diagnostic pop /* "-Wformat-security" */

int
signal_logging_test(void)
{
    number_formatting();
    logging_format();

    return 0;
}
