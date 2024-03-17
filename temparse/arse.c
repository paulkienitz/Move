/***********   What we have here is an alternative routine for parsing the
Cli argument line into separate argv elements.  This enables Manx C programs
to do it the BCPL way, with embedded quotes denoted by *" and newlines by *N.
(And asterisks by ** and ESC by *E.)  This also enables the program to tell
which arguments were quoted, by seeing whether argv[n][-1] is a quote or a
nul (I'll let you guess which means which) so you can tell the difference
between a keyword like ALL and a file named "All".

This module is NECESSARY for any Aztec C program that wants to use my
TemplateParse module.  Otherwise the input arguments can be garbled if any
of them have quoted strings.

Besides separating words by whitespace, it also separates them when it finds
an equal sign.  This supports BCPL-style argument templates which can include
key=value pairs, which can be formatted as key="value", or key = value, or
key= value, or key =value.  Since we do not know at parsing time whether key
is a template keyword or not, or even whether a template is necessarily being
used, we hope to preserve some of these distinctions in case they might have
some meaning to the application.  Therefore, for cases where there's a space
before or after an equal sign, we leave the sign attached to the beginning or
end of the argument it's found with.  We split the argument only in the case
where there is no whitespace, and we do so after the sign, so that quoting is
usable on the value.  So if the command line includes key="quoted phrase", we
produce two argv words containing "key=" and "quoted phrase", with the latter
having a quote-mark in the byte preceding the letter q.  Our TemplateParse
function knows how to handle leading and trailing equal-signs when matching
up to known key names in the template; if not matching, the values get taken
literally with the equal sign included.

This includes a function _cli_parse() which is called by the startup code. 
You don't call it, you just reference it, and it will be automatically called
by Aztec C before invoking your main function.  (Unless you use _main instead
of main, that is -- in that case you must call _cli_parse yourself.)  The
declaration to include is
    void _cli_parse(struct Process *pp, long alen, char *aptr);
If you use _main, its input parameters are alen and aptr, which you pass
through to _cli_parse.  Pass (struct Process *) FindTask(NULL) as the first
argument, pp.

There is also a function
    char **ParseLine(char *line, int ac);
which will return an argv-like array parsed as if the string you pass it had
been the argument line.  It leaves a null in element zero of the returned
array, and has another null after the last string pointer like the real argv.
It puts the argc value in the var pointed to by ac.  This number counts
element zero, so it's one more than the number of words found.

Updated 2024 to use the most current version of Aztec C.

by Paul Kienitz.  Public domain.
*/


#include <exec/exec.h>
#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <stdlib.h>
#include <string.h>
#include <Paul.h>


extern int  _argc, _arg_len;
extern str  _arg_lin;
extern str *_argv;


PUBLIC str  extra_argline;
PUBLIC long has_unclosed_quote = DOSFALSE;


#ifdef deboog

/* Shit for debugging messages from places where stdio not set up yet */
#define say(M, L) Write(Output(), M, (long) (L > 0 ? L : 0))
#define dd Delay(100L)
#define spew(M) say(M, chend(M))

int chend(m) str m;
{
    int l;
    for(l = 0; l < 256; l++)
        if (!m[l])
            break;
    return l;
}

void Hexpew(addd) adr addd;
{
    int hid,nyb;
    char hax[8];
    for (hid = 0; hid <= 7; hid++) {
        nyb = ((ulong) addd >> (4*hid)) & 15L;
        hax[7 - hid] = (nyb >= 10 ? (char) nyb + 'A' - 10 : (char) nyb + '0');
    }
    Write(Output(), hax, 8L);
}

#endif


/* tab, linefeed, return, page feed, line feed, space (no nonbreak-space) */
#define Whitemask (bit(8) | bit(9) | bit(11) | bit(12) | bit(31))
#define white(C) ((ubyte) (C) <= 32 ? (bit((C) - 1) & Whitemask) : 0L)

/* tab, page feed, space */
#define Whitmask (bit(8) | bit(11) | bit(31))
#define whit(C) ((ubyte) (C) <= 32 ? (bit((C) - 1) & Whitmask) : 0L)

#define toupper(C) ((C) & 0x5F)  /* crude but sufficient */



#ifndef NEWSHIT

/* This function moves the argline into space, separating words with nuls and
   converting asterisk quotations.  It returns argc.  space[-1] MUST BE 0. */

private int ChooLine(str line, int len, str space)
{
    register int ch;
    register char c;
    register bool quoted = false, nostar = true;
    register str lag = space - 1;
    int ac = 1;

    for (ch = 0; ch < len; ch++) {
        c = line[ch];
        if (c == '\r' || c == '\n') break;
        if (!quoted && (white(c) || c == '=')) {
            if (*lag)
                ac++;
            else
                lag--;  /* prevent multiple nuls */
            if (c == '=')
                *++lag = c;   /* spaces disappear, equal signs don't */
            c = '\0';
        }
        if (c == '"') {
            if (*lag) {
                if (quoted & nostar) {
                    quoted = false;
                    c = '\0';
                    ac++;
                }
            } else
                quoted = true;
        }
        if (!nostar) {
            register u = toupper(c);
            if (u == 'N') c = '\n';
            if (u == 'E') c = 27;  /* ESC */
        }
        if (nostar = ((c != '*' | quoted) | (nostar ^ true)))
            *++lag = c;
    }
    lag[1] = '\0';
    if (*lag)
        ac++;  /* when no final whitespace */
    if ((nostar = len - ch) > 0) {              /* misusing bool as short */
        extra_argline = malloc(nostar--);
        strncpy(extra_argline, line + ch + 1, nostar);
        extra_argline[nostar] = 0;
    } else
        extra_argline = null;
    has_unclosed_quote = quoted ? DOSTRUE : DOSFALSE;
    return ac;
}

#else

/* alternative version that I hoped would be smaller but seems not to be */

private int ChooLine(str line, int len, str space)
{
    register str lag = space, p = line;
    str end = line + len;
    register bool star;
    register char c;
    int ac = 1, exten;
    for (;;) {
        while (p < end && whit(*p)) p++;
        if (eol(*p) || p >= end)
            break;
        ac++;
spew("At the beginning of word "); Hexpew((long) ac);
spew(", the line is '");
say(p, end - p); spew("'.\n"); dd;
Chk_Abort();
        if (*p == '"') {  /* quoted word */
            *(lag++) = *(p++);
            star = false;
            while (p < end && !eol(*p) && (*p != '"' || star)) {
                c = *(p++);
spew("The character being moved is '");
say(p - 1, 1); spew("'.\n"); dd;
                if (star) {
                    if (toupper(c) == 'N') c = '\n';
                    else if (toupper(c) == 'E') c = 27;
                }
                *(lag++) = c;
                if (c == '*') {
                    if (star ^= true) lag--;
                } else star = false;
            }
        } else
            while (p < end && !white(*p))
                *(lag++) = *(p++);  /* non-quoted */
        *(lag++) = 0;
        p++;
    }
  breakk:
    if ((exten = end - p - 1) > 0) {
        extra_argline = p + 1;
        extra_argline = malloc(exten + 1);
        strncpy(extra_argline, p + 1, exten);
        extra_argline[exten] = 0;
    } else
	extra_argline = null;
    return ac;
}

#endif



/* This function sets up an argv array (already allocated) to point to the
   nul-separated words in space. */

private void MakeStray(str *av, str space, register int ac)
{
    register int f;
    register str p = space;
    for (f = 1; f < ac; f++) {
        av[f] = p + (*p == '"');
        p += strlen(p) + 1;
    }
    av[ac] = null;
}


PUBLIC long CheckForUnclosedQuote(void)
{
    struct Process *me = (struct Process *) FindTask(null);
    me->pr_Result2 = has_unclosed_quote ? /* ERROR_UNMATCHED_QUOTES : 0; */
                                          ERROR_LINE_TOO_LONG : 0;
    /* it turns out 2.x doesn't even use ERROR_UNMATCHED_QUOTES */
    return has_unclosed_quote;
}



/* This function is called by the startup function _main to convert the CLI
   argument string into _argv and _argc, which get passed to main. */

PUBLIC void _cli_parse(struct Process *pp, long alen, str aptr)
{
    struct CommandLineInterface *cli =
                bip(struct CommandLineInterface, pp->pr_CLI);
    register str poik = bip(char, cli->cli_CommandName), eek = aptr;
    register int coml = *poik;
    _arg_len = coml + (int) alen + 2;
    while ((eek = strchr(eek, '=')))
        eek++, _arg_len++;   /* each = needs an additional byte allocated */
    if (!(_arg_lin = Alloc(_arg_len)))        /* this is freed by _exit() */
        return;
    strncpy(_arg_lin, poik + 1, coml);
    _arg_lin[coml] = '\0';
    poik = _arg_lin + coml + 1;
    _argc = ChooLine(aptr, (int) alen, poik);
    if (!(_argv = Alloc((_argc + 1) * sizeof(str))))          /* likewise */
        return;
    MakeStray(_argv, poik, _argc);
    *_argv = _arg_lin;
    CheckForUnclosedQuote();
}



/* This function converts its string argument to an argv-like array. */

PUBLIC str *ParseLine(int *pargc, str line)
{
    register int len = strlen(line);
    register str space, eek = line;
    register str *av;

    if (!space)
        return null;
    while ((eek = strchr(eek, '=')))
        eek++, len++;   /* each = needs an additional byte allocated */
    space = malloc(len + 2);
#ifndef NEWSHIT
    *(space++) = '\0';
#endif
    *pargc = ChooLine(line, len, space);
    if (!(av = malloc((*pargc + 1) * sizeof(str))))
        return null;
    MakeStray(av, space, *pargc);
    *av = null;
    CheckForUnclosedQuote();
    return av;
}
