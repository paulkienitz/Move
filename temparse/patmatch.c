/*******
 Made reentrant by Paul Kienitz 12/31/90.  Used function prototypes just to
 avoid booboos.  Also replaced _toupper with real toupper.  Added check for
 oversized pattern, and HasPattern function, in 2024.
*******/

/* PatMatch.c - Implements AmigaDos Regular Expression Pattern Matching.
**
**  This program will test whether a string is an AmigaDos regular expression
**  It may be used to implement wildcard expressions such as:
**
**    "copy #?.c to <dir>" to copy any file ending in .c
**
**  The program has two entry points: CmplPat, and Match.
**
**    CmplPat - takes a pattern and returns an auxilliary array of short
**              ints which is used by Match.  The pattern is not modified
**              in any way.  CmplPat returns 1 if no errors were detected
**              while compiling the pattern; otherwise it returns 0;
**
**    Match   - takes the pattern, the auxilliary vector, and the string
**              to be matched.  It returns 1 if the string matches the
**              pattern; otherwise it returns 0;
**
**  Translated from BCPL by:
**              Jeff Lydiatt
**              Richmond B.C. Canada
**              16 May 1986.
**
**  Source: "A Compact Function for Regular Expression Pattern Matching",
**           Software - Practice and Experience, September 1979.
**
**  Useage:
**     To test if "file.c" matches the regular expression "#?.c"
**     char *Pat = "#?.c";
**     char *Str = "file.c";
**     WORD Aux[128];
**
**     if ( CmplPat( Pat, Aux ) == 0 )
**        {
**           printf("Bad Wildcard Expression\n");
**           exit(1);
**        }
**     if ( Match( Pat, Aux, Str ) == 1 )
**        printf("String matches the pattern\n");
**     else
**        printf("String does NOT match the pattern\n");
**/

/*--- Included files ----*/

#include <stdio.h>
#include <exec/types.h>
#include <ctype.h>
#include <string.h>

#define  EOS '\0'

/*----------------------------------------------------------------*/
/*                   The Interpreter                              */
/*----------------------------------------------------------------*/


struct GM {
    BOOL  Succflag;
    short Wp;
    short *Work;
};


static void Put(short N, register struct GM *m)
{
   register short *ip;
   register short *to;

   if ( N == 0 )
      m->Succflag = TRUE;
   else
      {
        for ( ip = m->Work + 1, to = m->Work + m->Wp; ip <= to; ip++)
           if ( *ip == N )
              return;
        m->Work[ ++m->Wp ] = N;
      }
}

long Match( char Pattern[], short Aux[], char Str[] )
{
   struct GM m;
   short     W[ 128 ];
   short     S = 0;
   short     I, N, Q, P, Strlength;
   char      K, Ch;

   m.Work = W;
   m.Wp = 0;
   m.Succflag = FALSE;
   Strlength = strlen( Str );
   Put( 1, &m );

   if ( Aux[ 0 ] != 0 )
      Put( Aux[ 0 ], &m );

   for(;;)
      {
        /* First complete the closure */
        for( N=1; N <= m.Wp; N++ )
          {
             P = m.Work[ N ];
             K = Pattern[ P-1 ];
             Q = Aux[ P ];
             switch( K )
                {
                  case '#': Put( P + 1, &m );
                  case '%': Put( Q, &m );
                  default : break;
                  case '(':
                  case '|': Put( P + 1, &m);
                            if ( Q != 0 )
                               Put( Q, &m );
                }
           }

        if ( S >= Strlength )
           return m.Succflag;
        if ( m.Wp == 0 )
           return FALSE;
        Ch = Str[ S++ ];

        /* Now deal with the match items */

        N = m.Wp;
        m.Wp = 0;
        m.Succflag = FALSE;

        for ( I = 1; I <= N; I++)
          {
             P = m.Work[ I ];
             K = Pattern[ P - 1 ];
             switch( K )
               {
                 case '#':
                 case '|':
                 case '%':
                 case '(': break;
                 case '\'': K = Pattern[ P ];
                 default : if ( toupper( Ch ) != toupper( K ) )
                              break;
                 case '?': /* Successful match */
                           Put( Aux[ P ], &m );
                } /* End Switch */
          } /* End For I */
     } /* End for(;;) */
}


long HasPat( char Pattern[] )
{
    return strpbrk( Pattern, "?#(|)%'" ) != NULL;
}


/*----------------------------------------------------------------*/
/*                     The Compiler                               */
/*----------------------------------------------------------------*/

struct CM {
    short *Aux;
    char  *Pat;
    short PatP, Patlen;
    BOOL  Errflag;
    char  Ch;
};

#define CC register struct CM *c



static void Rch( CC ) /* Read next character from Pat */
{
   if ( c->PatP >= c->Patlen )
      c->Ch = EOS;
   else
      c->Ch = c->Pat[ c->PatP++ ];
}



static void Nextitem( CC ) /* Get next char from Pat; recognize ' escape char */
{
   if ( c->Ch == '\'' )
      Rch( c );
   Rch( c );
}


static void Setexits( short List, short Val, short *Aux )
{
   short A;

   do
     {
        A = Aux[ List ];
        Aux[ List ] = Val;
        List = A;
     }
        while ( List != 0 );
}

static short Join( short A, short B, short *Aux )
{
    short T = A;

    if ( A == 0 )
        return B;
    while ( Aux[ A ] != 0 )
        A = Aux[ A ];
    Aux[ A ] = B;
    return T;
}

static short Prim( CC )      /* Parse a Prim symbol */
{
   short   A = c->PatP;
   char Op = c->Ch;
   short  Exp( short A, CC );

   Nextitem( c );
   switch( Op )
     {
        case EOS:                               /* empty string after | ? */
        case ')':                               /* missing ( ? */
        case '|': c->Errflag = TRUE;            /* empty string before | ? */
        default : return A;
        case '#': Setexits( Prim( c ), A, c->Aux );
                  return A;
        case '(': A = Exp( A, c );
                  if ( c->Ch != ')' )
                        c->Errflag = TRUE;      /* missing ) ? */
                  Nextitem( c );
                  return A;
     }
   return 0;   /* satisfy compiler */
}

static short Exp( short AltP, CC )    /* Parse an expression */
{
   short Exits = 0;
   short A;

   for (;;)
        {
           A = Prim( c );
           if ( c->Ch == '|' || c->Ch == ')' || c->Ch == EOS )
              {
                Exits = Join( Exits, A, c->Aux );
                if ( c->Ch != '|' )
                   return Exits;
                c->Aux[ AltP ] = c->PatP;
                AltP = c->PatP;
                Nextitem( c );
              }
           else
              Setexits( A, c->PatP, c->Aux );
        }
}

long CmplPat( char Pattern[], short Aux[] )
{
   short     i;
   struct CM c;
   size_t    strlen();

   c.Pat = Pattern;
   c.Aux = Aux;
   c.PatP = 0;
   c.Patlen = strlen( c.Pat );
   c.Errflag = FALSE;
   if ( c.Patlen >= 128 )
       return FALSE;        /* CmplPattern should always be size 128 */

   for ( i = 0; i <= c.Patlen; i++ )
      c.Aux[ i ] = 0;
   Rch( &c );
   Setexits( Exp( 0, &c ), 0, &c.Aux[0] );
   return (!c.Errflag);
}
