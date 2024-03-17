/* This is COOKT.C -- a module to enhance the TemplateParse module in
TEMPARSE.C -- by Paul Kienitz.  It contains the function
        char ** CookTemplate(char *template);
which transforms a human-written template string like "From, To/A, Q = Quiet/S"
into the form acceptable for the second parm of TemplateParse, which would be
{(char *) 3L, " From", "ATo", "SQ=Quiet"} in this case.  See the file
TEMPARSE.DOC for complete instructions.  If you're willing to construct that
second parm "by hand", you can leave the function CookTemplate out of your
program and make it significantly smaller and more efficient.

It also contains the small function
    void MortalSyn(char *template, long index);
which writes out an error message when a template has bad syntax, given the
value in the second pointer of the array returned by CookTemplate when the
first pointer is null, indicating the presence of an error.  Example:
    teepee = " from,2 =to /a, all/s/ , quiet/z";
    result = CookTemplate(teepee);
    if (!result[0]) MortalSyn(teepee, result[1]); else ...
Writes the following to standard output:
    Bad template " from,2 =to /a, all/s/ , quiet/z"
                                        =^=
*/

#include <exec/exec.h>
#include <libraries/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <stdlib.h>
#include <string.h>
#include <Paul.h>

#define toupper(C) ((C) & 0x5F)

#define Whitemask (bit(8) | bit(9) | bit(11) | bit(12) | bit(31))
#define white(C) (C <= 32 ? (bit(C - 1) & Whitemask) : 0L)

/* white(c) is nonzero if c is ' ' or '\n' or '\r' or '\t' or '\f'. */
/* This generates a lot less code than a series of compares does */


/* === MortalSyn is for displaying fatal errors in template syntax === */

private void spewtab(int d)
{
    static char caucasian[] =
"                                                                        ";
    /* spewing one space is very slow, so we do bunches */
    if (d <= 0) return;
    while (d > 70) {
        Write(Output(), caucasian, 70L);
        d -= 70;
    }
    Write(Output(), caucasian, (long) d);
}


PUBLIC void MortalSyn(str template, long offset)
{
    BPTR o = Output();
    if (!o) return;
    Write(o, "\nBad template \"", 15L);
    Write(o, template, (long) strlen(template));
    Write(o, "\".\n             ", 16L);
    spewtab((int) offset);
    Write(o, "=^=\n", 4L);
}

/* convert offset in clean tp to offset in original template */
private long Venal(str t, long o)
{
    int i, j = 1;
    for (i = 0; j < o; i++, j++)
        if (white(t[i])) j--; else if (t[i] == ',') j++;
    while (white(t[i])) i++;
    return (long)i;
}



/* A personal ad found in EXPRESS (The East Bay's Free Weekly):
    IF GOD HAD WANTED America to have elections he would have provided
    candidates.  Ken B.
Found elsewhere in the same column:
    WHERE IS MY FACE?
*/



/* === Here we parse the template itself into keywords and switches. === */

PUBLIC str *CookTemplate(str template)
{
    str last, p, lag, tp;
    str *turn;
    static str badturn[2] = {null, null};
    bool ended, newword;
    short i, nk = 1;    /* nk is number of template keyword slots */
#define pfffttt(p) { badturn[1]=(str)Venal(template,(p)-tp); return badturn; }

    for (i = 0; template[i]; i++)
        if (template[i] == ',')
            nk++;
    if (!(turn = malloc((nk + 1) << 2)))
        return null;
    turn[0] = (str) nk;
    if (!(tp = malloc(i + nk + 1)))
        return null;
    newword = true; ended = false;              /* standardize spaces: */
    *tp = ' ';
    for (p = template, lag = tp + 1; *p; p++)
        if (*p == ',' || *p == '=' || *p == '/') {
            *(lag++) = *p;
            newword = true;
            ended = false;
            if (*p == ',')
                *(lag++) = ' ';
        } else if (white(*p)) {
            if (!newword)
                ended = true;
        } else /* text */
            if (ended && !newword) {
                badturn[1] = (str) (p - template);
                return badturn;
            } else {
                *(lag++) = *p;
                newword = false;
            }
    last = lag; *last = 0;

    i = 1;
    for (lag = p = tp; p <= last; p++) {
        if (*p == '=') {
            if (lag == p - 1)
                pfffttt(p);  /* no word before = */
            if (!p[1] || p[1] == '/' || p[1] == '=' || p[1] == ',')
                pfffttt(p + 1);  /* no word after = */
        }
        if (p - 1 == lag && *p == '"')
            pfffttt(p);  /* no leading quotes */
        if (*p == '/' || !*p || *p == ',') {   /* end of word */
            newword = *p == '/';
            *p = '\0';
            turn[i++] = lag;
            while (newword) {   /* eat /A/S/K flags */
                ++p;
                if (toupper(*p) == 'A') {
                    if (*lag == ' ' || *lag == 'A')
                        *lag = 'A';
                    else if (*lag == 'K' || *lag == 'X')
                        *lag = 'X';
                    else pfffttt(p);
                } else if (toupper(*p) == 'K') {
                    if (!lag[1]) pfffttt(p);
                    if (*lag == ' ' || *lag == 'K')
                        *lag = 'K';
                    else if (*lag == 'A' || *lag == 'X')
                        *lag = 'X';
                    else pfffttt(p);
                } else if (toupper(*p) == 'S') {
                    if (!lag[1]) pfffttt(p);
                    if (*lag == ' ' || *lag == 'S')
                        *lag = 'S';
                    else pfffttt(p);
                } else pfffttt(p);
                newword = *(++p) == '/';
                if (!newword && *p && *p != ',')
                    pfffttt(p);
            }
            lag = p + 1;
        } /* if end-of-word */
    } /* for */
    return (turn);
}
