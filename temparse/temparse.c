/*
TEMPARSE.C  -- a module to allow Aztec C programs to parse their argument
lines according to a template, e.g. FROM,TO/A,ALL/S,QUIET/S and suchlike. 
Not indented to be portable to other compilers, though much of it could
probably remain unchanged.  Includes the function
   
    char **TemplateParse(char **argv, char **plates);

It uses a function _cli_parse(), found in object file arse.o, which replaces
the standard Manx function of that name.  It has the effect of parsing argv
just like the standard BCPL Cli programs.  See the file temparse.doc for
details on how to use these in your programs.

By Paul Kienitz.  This code is in the public domain.  Use it all you want,
modify it if you're man enough, but please don't remove my name from it.
Written in Aztec C.  To compile, all you need to do is just "cc temparse".
Keep the doc file together with the object modules/library and the source:
this file temparse.c, arse.c, cookt.c, and the include file Paul.h.

You MUST link in the object module ARSE.O for this to work, and you probably
also want COOKT.O.  You don't have to declare anything for arse.  These
functions can be made into a Manx library, so you can use just the parts you
need, cookt being optional.
*/


#include <dos/dosextens.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <pragmas/exec_lib.h>
#include <pragmas/dos_lib.h>
#include <ctype.h>
#include <stdlib.h>
#include <Paul.h>

#ifdef TESTFRAME
#include <stdio.h>
void  MortalSyn(str template, long offset);
#endif


//import void ParseLine();        /* pull in the ARSE module */


private char credit[] = "\n TemplateParse() module by Paul Kienitz. \n";


#define Whitemask (bit(8) | bit(9) | bit(11) | bit(12) | bit(31))
#define white(C) (C <= 32 ? (bit(C - 1) & Whitemask) : 0L)
//#      define toupper(C) ((C) & 0x5F)  /* real toupper is failing weirdly? */

/* white(c) is nonzero if c is ' ' or '\n' or '\r' or '\t' or '\f'. */
/* This generates a lot less code than a series of compares does */



/* === This returns the match end if one of the words separated by equal
       signs in set is the same as w, independent of case. === */

private int keyword_match(str set, str w)
{
    int x = 0;
    if (w[-1] == '"')
        return 0;
    do {
        while (w[x] && set[x] && set[x] != '=' && w[x] != '=' &&
                       toupper(set[x]) == toupper(w[x]))
            x++;
        if ((w[x] == '='|| !w[x]) && (set[x] == '=' || !set[x]))
            return x;
        set += x;
        while (*set && *set != '=')
            set++;
        if (*set == '=')
            set++;
        x = 0;
    } while (*set);
    return 0;
}



/* === "Here he is, mister TemplateParse..."   (lvjkb x x x)  === */

PUBLIC str *TemplateParse(str *argv, str *plates)
{
    int nkeys = (long) plates[0], m = 0, k, w, argc;
    static str empty = "";
    static str badresult[2] = {null, null};
    str *result, *av, keyav;
#define pkag *(plates[k])
#define ralph(E, C) { badresult[1] = E; \
                      ((struct Process *) FindTask(NULL))->pr_Result2 = C; \
                      return badresult; }

    if (IoErr() == ERROR_LINE_TOO_LONG)   /* be strict about arg syntax */
        ralph("argument line invalid or too long", ERROR_LINE_TOO_LONG);
    if (!nkeys)           /* should never happen with a const template */
        ralph("bad template", ERROR_BAD_TEMPLATE);
    for (argc = 0; argv[argc + 1]; argc++) ;
    if (!(result = calloc(nkeys + 1, 4)) || !(av = malloc((argc + 1) << 2)))
        ralph("insufficient free store", ERROR_NO_FREE_STORE);
    for (w = 1; w <= argc; w++)        /* pool of unclaimed args */
        av[w] = argv[w];
    result[0] = (str) nkeys;

    for (w = 1; w <= argc; w++) {      /* First we scan for key words */
        for (k = 1; k <= nkeys; k++)
            if ((m = keyword_match(plates[k] + 1, av[w])))
                break;
        if (m) {   /* aha, a named parm */
#ifdef TESTFRAME
printf("Explicit keyword «%s» (#%d):  ", plates[k], k);
#endif
            keyav = av[w];
            av[w] = null;
            if (pkag == 'S')
                result[k] = (str) DOSTRUE;      /* compatible with ReadArgs */
            else if (keyav[m] == '=' && keyav[m + 1]) {
                if (keyav[m + 1] == '=')
                    ralph("argument line invalid or too long",
                          ERROR_LINE_TOO_LONG);
                /* actually AmigaDOS sometimes tolerates two = signs, but
                   not consistently... I suspect it was not intended */
                result[k] = keyav + m + 1;    /* value is within this word */
            } else if (av[++w]) {
                if (av[w][-1] != '"' && av[w][0] == '=') {
                    if (keyav[m] == '=' || av[w][1] == '=')    /* double = */
                        ralph("argument line invalid or too long",
                              ERROR_LINE_TOO_LONG);
                    if (!av[w][1])      /* bare = as a word by itself? */
                        av[w++] = null;             /* go to next word */
                    else
                        av[w]++;        /* go to next character in word */
                }
                result[k] = av[w];
            }
#ifdef TESTFRAME
if (pkag == 'S') puts("(switch marked true).");
else if (w > argc) puts("...woops, no value!");
else printf("«%s».\n", result[k]);
#endif
            if (w > argc)
                ralph("value after keyword missing", ERROR_KEY_NEEDS_ARG);
            plates[k] = null;
            av[w] = null;
        }
    }

    for (w = 1; w <= argc; w++) if (av[w]) {   /* positional parms */
        k = 1;
        while (k <= nkeys && !(plates[k] && pkag != 'S' 
                          && pkag != 'K' && pkag != 'X'))
            k++;
        if (k > nkeys)
            ralph("wrong number of arguments", ERROR_TOO_MANY_ARGS);
#ifdef TESTFRAME
printf("A positional parm for «%s» (#%d):  «%s».\n", plates[k], k, av[w]);
#endif
        result[k] = av[w];
        plates[k] = null;
    }

    for (k = 0; k < nkeys; k++)
        if (plates[k] && (pkag == 'A' || pkag == 'X'))
            ralph("required argument missing", ERROR_REQUIRED_ARG_MISSING);
    return result;
}



#ifdef TESTFRAME

void main(int argc, str *argv)
{
    str *plates, *result, *CookTemplate(str);
    static char template1[] = "FROM/A,TO,Q=QUIET/S,ND=NODIRS/S";
    static char template2[] = "R=Regular/a/k,Special/K,Foo/s,BAR";
    int r;  bool fault = false;
    char template[256];

    printf("What template? [default %s] ", template1);
    gets(template);
    if (fault = !*template)
        strcpy(template, template1);
    else if (*template == '\t') {
        strcpy(template, template2);
        printf("Using %s\n", template2);
    }

    plates = CookTemplate(template);
    if (!plates[0])
        MortalSyn(template, (long) plates[1]);
    else {
        printf("%d keywords parsed.\n", (int)(long) plates[0]);
        for (r = 1; r <= (int)(long) plates[0]; r++)
            printf("returnee[%d] = «%s».\n", r, plates[r]);
        puts("");
        result = TemplateParse(argv, plates);

        if (result[0]) {
            puts("");
            for (r = 1; r <= (long) result[0]; r++) 
                if (result[r] == (str) DOSTRUE)
                    printf("Result %d got DOSTRUE.\n", r);
                else if (result[r])
                    printf("Result %d got «%s».\n", r, result[r]);
            if (fault) {
                printf("\nQUIET switch:  ");
                if (result[3]) printf("present (%s)\n", result[3]);
                else printf("absent\n");
                printf("NODIRS switch:  ");
                if (result[4]) printf("present (%s)\n", result[4]);
                else printf("absent\n");
                printf("FROM spec:  «%s»\n", result[1]);
                printf("TO spec:  «%s»\n", result[2]);
            }
        } else
            printf("Error %ld with message:  %s.\n", IoErr(), result[1]);
    }
}

void _wb_parse() { }

#endif
