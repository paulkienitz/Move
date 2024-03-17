/* The idea here is to come up with a MOVE command which will obsolete RENAME
 * by doing things that COPY can do, such as take a pattern spec instead of
 * one file, and by letting the destination name default to the source if you
 * only specify a directory.  (This has a surprisingly tricky set of different
 * paths and odd cases for such a simple idea.)
 */

/* Written by Paul Kienitz, begun 10/88 as my very first useful program
 * for my brand new used Amiga 1000.  What the hell, it's in the public domain.
 * (Not actually finished and released until the 2020s!?)
 */

/* Records listened to while writing the first version of this program:
 *      The Rhino Brothers Present The World's Worst Records, volumes 1 and 2
 *      U2:  the Forgettable Fire
 *      Peter Case
 *      Gang of Four:  Entertainment!
 *      Bruce Springsteen:  Darkness On The Edge Of Town
 *      Bruce Hornsby:  The Way It Is
 *      the Dead Milkmen:  Big Lizard In My Back Yard
 *      the Repo Man soundtrack album
 *      Translator:  Heartbeats And Triggers
 * (I have to acknowledge my sources, don't I?)
 *
 * Forget what everybody tells you; Solid Gold is way better than
 * Entertainment! -- ultrakillerguitar rocknroll.
 *
 * Plus I also listened to my home team, the Oakland A's, getting their asses
 * kicked in the fifth and final game of the 1988 world series.  That was quite
 * a while ago, wasn't it?
 */


/* ========== Enough of that, here's the declarations ========== */

#include <exec/exec.h>
#include <dos/dosextens.h>
#include <clib/dos_protos.h>
#include <stdlib.h>        /* unlike early versions, use malloc */
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <Paul.h>

unsigned toupper(unsigned c);                /* not in string.h */
void _exit(int _code);                    /* don't want fcntl.h */

#ifdef ONE
/* temparse functions: */
str *CookTemplate(str template);
str *ParseLine(int *pargc, str line);
str *TemplateParse(str *argv, str *plates);
void _cli_parse(struct Process *pp, long alen, str aptr);
extern str *_argv;
extern int _argc;
extern long has_unclosed_quote;

long CmplPat(char[], short[]);
long Match(char[], short[], char[]);
long HasPat( char Pattern[] );

char *FaultMessage(long errorcode);
void VSprintf(char *buf, char *format, void *args);

long DoDosPacket(struct MsgPort *handler, long action, long arg1, long arg2,
                 long arg3, long arg4, long arg5, long arg6, long arg7);
long CompareLocks(BPTR lock1, BPTR lock2);
#else
#  define CompareLocks SameLock /* not as good, doesn't handle nulls right */
#endif

#define PATLIMIT 128

#define FIB struct FileInfoBlock


struct DosLibrary *DOSBase;

typedef struct fode *fist;

/* Earlier versions of this code only supported filenames up to 30 chars,
the limit of AmigaDOS's native filesystems.  Now we support filenames of
up to 107 chars, the limit of Examine/ExNext.  Filenames of this length can
be seen and used on non-native filesystems, such as when an Amiga emulator
accesses the filesystem of the host machine. */

struct fode {
    fist next;
    /* BPTR localock; */                /* for MatchNext results? */
    long diskey;
    char name[108];
    char /* bool */ matches, useit;
};

fist maybe;

str from, too, frame, origfrom, finalfrom, finaltoo; /* don't need freeing */
bool samedir, quiet, nodirs, icons, moveonly, nameonly;  /* template flags */
bool anything, colonial;

BPTR lick = 0, deer, elk;                       /* locks */
FIB *fb;
struct Process *thisProcess;
ubyte patt[PATLIMIT + 3];                       /* for pattern matching */
ubyte mesh[PATLIMIT * 2 + 8];
char namespace[108];
long hair = 0;                                  /* error number */
bool dos1;                                      /* stick to AmigaDOS 1.x */
adr rda = null;

/* a better toupper for iso-8859-1 */
const char toupper_map[256] =
    "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
        "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
    " !\"#$%&'()*+,-./0123456789:;<=>?"
    "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
    "`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~\x7F"
    "\x80\x81\x82\x83\x84\x85\x86\x87\x88\x89\x8A\x8B\x8C\x8D\x8E\x8F"
        "\x90\x91\x92\x93\x94\x95\x96\x97\x98\x99\x9A\x9B\x9C\x9D\x9E\x9F"
    "\xA0¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿"
    "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞß"
    "ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ÷ØÙÚÛÜÝÞÿ";

#ifdef NAME
#  ifdef ONE
const char version[] = "\0$VER: Name 1.4 (17.03.2024) using temparse library";
#  else
const char version[] = "\0$VER: Name 1.4 (17.03.2024) without AmigaDOS 1.x support";
#  endif
#else
#  ifdef ONE
const char version[] = "\0$VER: Move 1.4 (17.03.2024) using temparse library";
#  else
const char version[] = "\0$VER: Move 1.4 (17.03.2024) without AmigaDOS 1.x support";
#  endif
#endif


/* ==================   functions   ================== */


void SetError(long e)
{
    if (dos1)
        thisProcess->pr_Result2 = e;
    else
        SetIoErr(e);
}


void Die(short r)
{
    if (lick)
        UnLock(lick);
    if (deer)
        UnLock(deer);
    if (elk && elk != deer)
        UnLock(elk);
    if (fb)
        FREE(fb);
    if (rda)
        FreeArgs(rda);
    SetError(hair);
    _exit(r);              /* clean up clib stuff */
}


#ifndef ONE
#  asm
        public  _mob

_mob:   move.b  d0,(a3)+
        rts
#  endasm
static void mob(char d, char *s);


void VSprintf(char *buf, char *format, void *args)
{
    RawDoFmt(format, args, mob, buf);
}
#endif


void Put(str msg)
{
    if (Output())
        Write(Output(), msg, strlen(msg));
}


void Putf(str format, ...)
{
    static char buf[1024];
    VSprintf(buf, format, &format + 1);
    Put(buf);
}


void Croak(str errheader)
{
    if (hair) {
#ifdef ONE
        if (dos1) {
            str msg = FaultMessage(hair);
            if (errheader)
                Putf("%s: ", errheader);
            if (!msg)
                Putf("Error %ld\n", hair);
            else
                Putf("%s\n", msg);
            if (hair == ERROR_REQUIRED_ARG_MISSING ||
                           hair == ERROR_KEY_NEEDS_ARG ||
                           hair == ERROR_TOO_MANY_ARGS ||
                           hair == ERROR_UNMATCHED_QUOTES)
                hair = ERROR_LINE_TOO_LONG;  /* so 1.x Why can understand */
        } else
#endif
            PrintFault(hair, errheader);
    }
    if (hair == ERROR_NO_FREE_STORE || hair == ERROR_INVALID_COMPONENT_NAME
                                    || hair == ERROR_LINE_TOO_LONG)
        Die(20);
    else
        Die(hair ? 10 : 0);
}


void CroakE(str errheader, long err)
{
    hair = err;
    Croak(errheader);
}


void CCcheck(void)
{
    if (SetSignal(0, SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
        Put("\n *** BREAK\n");
        Die(5);
    }
}


str PathTail(str path)
{
#ifdef ONE
    if (dos1) {
        str p = strrchr(path, '/');
        if (!p)
            p = strrchr(path, ':');
        return p ? p + 1 : path;
    } else
#endif
        return FilePart(path);
}


#define FibIsDirectory(fib) ((fib)->fib_DirEntryType > 0 && \
                             (fib)->fib_DirEntryType != ST_SOFTLINK)


bool SameFilename(register str s1, register str s2)
{
    s2 = PathTail(s2);
    if (!*s2)
        return true;    /* empty means unknown means presume good */
    while (*s1 && toupper_map[(ubyte) *s1] == toupper_map[(ubyte) *s2])
        s1++, s2++;
    return !*s1 && !*s2;
    /* Old versions of this handle some special case I no longer remember the
       reason for, by ignoring a final char on one side... it might have been
       for a FAT filesystem that used names like DOS:FOOBAR.TXT[ to indicate
       newline conversion?   What was that... MSH?  No, that has ;A and ;B but
       not [ or ]... CrossDOS?  I don't think current versions do it either. */
}


str Cat(str front, str back)
{
    long lf = front ? strlen(front) : 0;
    long lb = back ? strlen(back) : 0;
    str new = malloc(lf + lb + 1);
    if (front)
        strcpy(new, front);
    if (back)
        strcpy(new + lf, back);
    return new;
}


BSTR Beaster(str s)            /* caller must FreeBeast */
{
    int l;
    ubyte *r;

    for (l = 0; s[l] && l < 256; l++)
        ;
    if (!(r = AllocP(l + 1))) {
        hair = ERROR_NO_FREE_STORE;
        return 0;
    }
    strncpy(r + 1, s, (size_t) l);
    *r = l;
    return (long) r >> 2;
}


void FreeBeast(BSTR b)
{
    ubyte *s;

    if (!b)
        return;
    s = gbip(b);
    FreeMem(s, 1 + *s);
}



bool GetArgsByTemplate(str template, long *argsFound)
{
#ifdef ONE
    if (dos1) {
        /* under 1.x, use temparse.lib to simulate ReadArgs...
           but note that it only supports /A, /K, and /S, as
           AmigaDOS 1.x had not yet defined /F, /M, /N, or /T */
        str *plates, *matches;
        char buf[512];
        int a;
        plates = CookTemplate(template);
        if (!*plates) {                 /* should never happen */
            hair = ERROR_LINE_TOO_LONG;
            return false;
        }
        if (!_argv[2] && _argv[1] && !_argv[1][-1] && !strcmp(_argv[1], "?")) {
            Putf("%s: ", template);
            a = Read(Input(), buf, sizeof(buf) - 1);
            if (a) {
                buf[a--] = '\0';
                if (buf[a] == '\n')     /* ReadArgs actually requires \n, */
                    buf[a] = '\0';      /* but TemplateParse don't like it */
                _argv = ParseLine(&_argc, buf);
            } else
                CroakE("Failure reading new arguments!\n", IoErr());
        }
        matches = TemplateParse(_argv, plates);  /* overwrites plates! */
        if (!*matches) {
            hair = IoErr();
            return false;
        }
        for (a = 1; a <= (long) matches[0]; a++)
            argsFound[a - 1] = (long) matches[a];
        return true;
    }
#endif
    rda = ReadArgs(template, (long *) argsFound, null);
    if (!rda) {
        hair = IoErr();
        return false;
    }
    return true;
}


void Ready(long alen, str aptr)
{
    static long drugs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static char template[] =
#ifdef NAME
        "From/A,To=As,Q=Quiet/S,F=FilesOnly/S,I=Icon/S,M=Move/S,MoveOnly/S,NameOnly/S";
#else
        "From/A,To=As,Q=Quiet/S,F=FilesOnly/S,I=Icon/S,N=Name/S,MoveOnly/S,NameOnly/S";
#endif

    dos1 = DOSBase->dl_lib.lib_Version < 37;
    thisProcess = (struct Process *) FindTask(null);
#ifndef ONE
    if (dos1) {
        Put("AmigaDOS 2.04 or later required.\n");
        SetError(ERROR_INVALID_RESIDENT_LIBRARY);
        _exit(20);
    }
#else
    if (dos1)
        _cli_parse(thisProcess, alen, aptr);
#endif
    if (!GetArgsByTemplate(template, drugs))
        Croak("Cannot proceed");
    from      = (str) drugs[0];
    too       = (str) drugs[1];
    quiet     = !!drugs[2];
    nodirs    = !!drugs[3];
    icons     = !!drugs[4];
#ifdef NAME
    samedir   = !drugs[5];
#else
    samedir   = !!drugs[5];
#endif
    moveonly  = !!drugs[6];
    nameonly  = !!drugs[7];
    origfrom  = from;
    finalfrom = from;
    finaltoo  = too && *too ? too :
                (samedir ? "source directory" : "current directory");
    colonial  = from[0] && from[strlen(from) - 1] == ':';
}



/* Takes dir/pat string in from and translates it into a compiled pattern in
   mesh.  Sets deer to a lock on the directory to be scanned using the
   pattern.  Returns true for success.  Eventually obsolete this and use
   MatchNext() instead in MovePattern()? */

bool SplitTailPat(void)
{
    ustr p, fro;
    ustr pat = from;
    bool f = true;
    short i, n;

    pat = PathTail(from);
    p = pat;
    for (i = 0; *p && i < PATLIMIT; p++, i++)
        patt[i] = toupper(*p);       /* toupper lower ascii only, for OFS */
    patt[i] = '\0';
    n = pat - from;
    fro = malloc(n + 1);
    strncpy(fro, from, n);
    fro[n] = '\0';
    from = fro;
    finalfrom = *from ? from : "current directory";
    SetError(0);
    if (*p)
        i = -1;
#ifdef ONE
    else if (dos1)
        i = !HasPat(patt) ? 0 :
            CmplPat(patt, (short *) mesh) ? 1 : -1;
#endif
    else
        i = ParsePatternNoCase(patt, mesh, PATLIMIT * 2 + 8);
    if (i < 0) {
        hair = IoErr();
        if (!hair) hair = ERROR_INVALID_COMPONENT_NAME;         /* sort of */
        Putf("Can't move; bogus pattern \"%s\".\n", patt);
        Die(20);
    }
    if (!i)
        return false;           /* no wildcards */
    /* cosmetics: truncate final slash in from if it contains non-slashes */
    for (p = from; *p; p++)
        if (*p != '/')
            f = true;
    if (f && *(--p) == '/')
        *p = 0;
    if (!(deer = RLock(from))) {
        hair = IoErr();
        return false;
    }
    return true;
}


long RenamePacket(BPTR oldlock, str oldname, BPTR newlock, str newname)
{
    BSTR own, nun;
    long r1;
    struct MsgPort *bootport = thisProcess->pr_FileSystemTask;
    struct FileLock *flake = gbip(oldlock);
    struct MsgPort *flakeport = flake ? flake->fl_Task : bootport;

    if (CompareLocks(oldlock, newlock) == LOCK_DIFFERENT)
        return ERROR_RENAME_ACROSS_DEVICES;     /* DoPkt won't catch this! */
    own = Beaster(oldname);
    if (!own)
        return hair;
    if (!(nun = Beaster(newname))) {
        FreeBeast(own);
        return hair;
    }
#ifdef ONE
    if (dos1)
        r1 = DoDosPacket(flakeport, ACTION_RENAME_OBJECT,
                         oldlock, own, newlock, nun, 0, 0, 0);
    else
#endif
        r1 = DoPkt4(flake->fl_Task, ACTION_RENAME_OBJECT,
                    oldlock, own, newlock, nun);
    FreeBeast(own);
    FreeBeast(nun);
    return r1 ? 0 : IoErr();
}


void MovePattern(void)
{
    bool cess = false;
    register str nn;
    int x, z;
    long wig = 0;
    char k;
    fist yes, tm, *ptm;

    nn = &fb->fib_FileName[0];
    maybe = null;
    if (!Examine(deer, fb))
        CroakE("Examine() failed", IoErr());
    while (ExNext(deer, fb)) {
        if (!(tm = (fist) malloc(sizeof(*tm)))) {
            hair = ERROR_NO_FREE_STORE;
            break;
        }
        strcpy(tm->name, nn);
        tm->diskey = fb->fib_DiskKey;
        tm->matches = tm->useit = (!nodirs || fb->fib_EntryType < 0) && (
#ifdef ONE
                                  dos1 ? Match(patt, (short *) mesh, nn) :
#endif
                                         MatchPatternNoCase(mesh, nn));
        for (ptm = &maybe; *ptm && stricmp((*ptm)->name, nn) > 0   /*****  ARGH, USE TOUPPER_MAP! *****/
                         ; ptm = &(*ptm)->next)
            ;
        tm->next = *ptm;        /* insert in list, maintaining Z to A sort */
        *ptm = tm;
        CCcheck();
    }
    if ((hair = IoErr()) == ERROR_NO_MORE_ENTRIES)
        hair = 0;

    if (icons && !hair)
        for (yes = maybe; yes; yes = yes->next) {
            nn = yes->name;
            x = strlen(nn) - 5;
            if (x > 0 && !stricmp(nn + x, ".info")) {         /*****  ARGH, USE TOUPPER_MAP! *****/
                k = nn[x];
                nn[x] = 0;
                for (tm = yes->next; tm; tm = tm->next) {
                    z = stricmp(nn, tm->name);
                    if (z > 0)
                        break;
                    else if (tm->matches && !z) {
                        yes->useit = true;
                        break;
                    }
                }
                nn[x] = k;
            }
        }
    nn = null;
    yes = null;
    while (maybe) {
        tm = maybe->next;
        if (maybe->useit) {
            for (ptm = &yes; *ptm && (*ptm)->diskey < maybe->diskey
                           ; ptm = &(*ptm)->next)
                ;
            maybe->next = *ptm;         /* insert in yes list in diskey order */
            *ptm = maybe;
        }
        maybe = tm;
    }
    maybe = yes;
    while (maybe) {
        nn = maybe->name;
        if (hair = RenamePacket(deer, nn, lick, nn)) {
            /* continue if file disappeared, dest already has that name,
               or attempt to rename a directory inside itself */
            if ((hair == ERROR_OBJECT_EXISTS || hair == ERROR_OBJECT_NOT_FOUND
                                             || hair == ERROR_OBJECT_IN_USE)) {
                if (!quiet)
#ifdef ONE
                    if (dos1)
                        Putf("Could not move %s: %s\n", nn, FaultMessage(hair));
                    else
#endif
                        Put("Could not move "), PrintFault(hair, nn);
                if (!wig)
                    wig = hair;
                hair = 0;
            } else {                            /* treat error as fatal */
                if (hair == ERROR_INVALID_LOCK)
                    hair = ERROR_RENAME_ACROSS_DEVICES;
                Putf("Error moving %s from %s to %s", nn, finalfrom, finaltoo);
                Croak("");
            }
        } else {
            if (!quiet)
                Putf("  %s\n", nn);
            cess = true;
        }
        CCcheck();
        tm = maybe;
        maybe = maybe->next;
    }
    if (wig && !hair && !cess)
        hair = wig;
    if (!cess && !quiet && !hair)
        Put("Nothing matched pattern.\n");
    if (!cess)
        Die(5);
    if (hair) {
        if (!nn)
            Putf("Couldn't scan %s", from);
        Croak("");
    }
}


bool DoIt(bool scanning, bool samemom, bool todir, str name)
{
    bool suck = true;
    str newname = name, tail = PathTail(from);

    if (samemom && (!too || !*too)) {
        newname = tail;
        if (!*newname)
            newname = name;   /* can't recapitalize with a bad tail */
    }
    if (samemom && (scanning || moveonly)) {
        if (!quiet)
            Put("Source and destination are the same directory; "
                "nothing done.\n");
    } else if (scanning) {
        MovePattern();
    } else if (samemom && frame && *tail &&
                          !strcmp(frame, PathTail(newname))) {
        if (!quiet)
            Put("Source and destination are the same; nothing done.\n");
    } else {
        if (newname != name)
            too = finaltoo = name = newname;
        if (!frame)
            frame = tail;
        /* There are three paths here: todir -> move to another directory
           without changing the name, samedir -> change the name within the
           same directory, and a generic fallback path.  But when exactly is
           the fallback used, and what about cases where todir and samemom
           are both true? */
        if (todir)
            suck = !(hair = RenamePacket(deer, name, lick, name));
        else if (samedir)
            suck = !(hair = RenamePacket(deer, frame, lick, name));
        else if (!(suck = Rename(from, too)))               /* name == too */
            hair = IoErr();
        if (suck && icons) {
            if (todir | samedir) {
                from = Cat(todir ? name : frame, ".info");
                too  = Cat(samedir ? too : name, ".info");
                hair = RenamePacket(deer, from, lick, too);
            } else {
                from = Cat(from, ".info");
                too  = Cat(too, ".info");
                hair = 0;
                if (!Rename(from, too))
                    hair = IoErr();
            }
            if (hair == ERROR_OBJECT_NOT_FOUND)
                hair = 0;
            suck = !hair;
        }
    }
    return suck;
}



BPTR LockVolumeRoot(BPTR startingLock)
{
    /* locking ":" relative to startingLock may not work, so: */
    BPTR parentLock = DupLock(startingLock), parentLock2;
    while (parentLock2 = ParentDir(parentLock)) {
        UnLock(parentLock);
        parentLock = parentLock2;
    }
    /* ...you'd think the OS would have a better way to do this */
    return parentLock;
}


void RelabelVolume(BPTR rootlock, str name)
{
    BSTR nam;
    struct MsgPort *rootask;
    struct DevProc *devpr;
    short l;

    hair = 0;
    if (dos1) {
        rootask = DeviceProc(origfrom);
        if (rootask && deer && colonial) {
            /* assigns are ok if they point to a root, otherwise: */
            hair = ERROR_OBJECT_WRONG_TYPE;
            Put("Cannot rename via assign.\n");
            Die(10);
        }   /* if we want to detect all assigns, check IoErr */
    } else {
        devpr = GetDeviceProc(origfrom, null); 
        if ((devpr && devpr->dvp_Flags & DVPF_ASSIGN) || (deer && colonial)) {
            /* simple assigns don't set DVPF_ASSIGN, check ^^ for parent */
            FreeDeviceProc(devpr);
            hair = ERROR_OBJECT_WRONG_TYPE;
            Put("Cannot rename via assign.\n");
            Die(10);
            /* It is sometimes possible to rename a directory that an assign
               points to, but if you want to try this, be sure to call
               GetDeviceProc again to check that it isn't a multi-assign,
               because those not only aren't logical to rename, they have
               also sometimes been seen to cause big crashes. */
        }
        rootask = devpr->dvp_Port;
        FreeDeviceProc(devpr);
    }

    if (!name)
        name = from;      /* recapitalize (we don't detect non-change) */
    else if (!*name) {
        hair = ERROR_INVALID_COMPONENT_NAME;
        Put("No name given to relabel volume with.\n");
        Die(10);
    }
    l = strlen(name) - 1;
    if (name[l] == ':')
        name[l] = 0;
    if (strchr(name, '/') || strchr(name, ':')) {
        hair = ERROR_OBJECT_WRONG_TYPE;
        Put("Cannot move root directory.\n");
        Die(10);
    }

    if (nam = Beaster(name)) {
        long r;
#ifdef ONE
        if (dos1)
            r = DoDosPacket(rootask, ACTION_RENAME_DISK, nam, 0, 0, 0, 0, 0, 0);
        else
#endif
            r = DoPkt1(rootask, ACTION_RENAME_DISK, nam);
        if (!r)
            hair = IoErr();     /* ^^^ use packet so volume is unambiguous */
        FreeBeast(nam);
    }
    Croak("Couldn't relabel volume");           /* no err msg if succeeded */
}



void _main(long alen, str aptr)     /* low-level entry: no argv, no stdio */
{
    bool suck = true, fromisdir = false, todir = false;
    bool sameob = false, samemom = false, manx = false, scanning;
    str name, ne;
    BPTR ocd;

    Ready(alen, aptr);
    if (!NEWP(fb))
        CroakE("Can't move", ERROR_NO_FREE_STORE);
    elk = RLock(from);                          /* does source exist? */
    if (scanning = !elk) {
        hair = IoErr();
        if (!SplitTailPat()) {
            Put("Can't move ");
            Croak(origfrom);
        }
        frame = null;
    } else {
        /* Renaming (and several other operations) work not off a lock on the
           target, but a lock on a parent plus a name (or relative path).  So
           we'd love to just take ParentDir(elk) and the tip name, but sorry,
           that fails when from points to a soft or hard link.  In that case,
           elk is a lock on the link's target, and its parent lock may not be
           where the link is located.  MatchFirst can hit the link directly,
           but only if from's tail contains no wildcard characters, and it has
           to scan the directory.  Therefore we split from into head and tail,
           and lock the head.  This is needed for samedir and todir cases. */
        frame = null;
        ne = PathTail(from);
        if (strlen(ne) > 107) {               /* probably can't happen */
            hair = ERROR_INVALID_COMPONENT_NAME;
            deer = 0;
            suck = false;
        } else if (*ne) {
            BPTR doe = ParentDir(elk);
            char nec;
            strcpy(name = namespace, ne);
            if (ne > from && ne[-1] == '/')
                ne--;
            nec = *ne;
            *ne = 0;
            deer = RLock(from);         /* may now be an empty string */
            hair = IoErr();
            *ne = nec;
            /* deer and name now represent from, even if it's a link */
            if (deer && CompareLocks(doe, deer) == LOCK_SAME) {
                /* from is not a link into another dir, but check further */
                if (Examine(elk, fb) && SameFilename(name, fb->fib_FileName)) {
                    /* elk is what it claims to be, so save capitalization */
                    strcpy(frame = name, fb->fib_FileName);
                    fromisdir = FibIsDirectory(fb);
                }
            }
            UnLock(doe);
        } else {        /* from ends in a slash or colon */
            manx = true;                /* it's tailless */
            deer = ParentDir(elk);
            hair = IoErr();
            if (!(suck = Examine(elk, fb)))
                CroakE("Examine() failed", IoErr());
            else
                strcpy(frame = name = namespace, fb->fib_FileName);
        }
        /* deer is the directory the source object(s) are in... unless: */
        if (!deer || colonial) {
            if (hair == ERROR_NO_FREE_STORE)
                Croak("Can't move");
            if (samedir)
                RelabelVolume(elk, too);                /* DOES NOT RETURN */
            else {
                Put("Cannot move device, volume, or assign.\n");
                hair = ERROR_OBJECT_WRONG_TYPE;
                Die(10);
            }
        }
    }

    /* that's got the From side sorted, now let's look at the To side */
    if (too) {
        if (strchr(too, ':'))     /* fully qualified or root-relative path */
            samedir = false;      /* deactivate NAME mode, not applicable */
        if (samedir)
            ocd = CurrentDir(deer);
        lick = RLock(too);                              /* does dest exist? */
        sameob = elk && CompareLocks(lick, elk) == LOCK_SAME;
        hair = lick ? IoErr() : 0;
        if (samedir)
            CurrentDir(ocd);
    }
    if (sameob || !too) {
        if (lick)
            UnLock(lick);
        if (samedir) {
            lick = DupLock(deer);                   /* dest is source dir */
            samemom = true;
        } else {
            lick = DupLock(ocd = CurrentDir(0));    /* dest is current dir */
            CurrentDir(ocd);
            /* samemom = sameob? */
        }
    }
    if (elk)
        UnLock(elk);
    elk = 0;
    if (lick || ((!too || !*too) && !ocd)) {           /* dest identified? */
        if (!samemom && !Examine(lick, fb))            /* yes;  file or dir? */
            CroakE("Examine() failed", IoErr());
        if (nameonly && too) {
            Put("Can't rename as ");
            CroakE(finaltoo, ERROR_OBJECT_EXISTS);
        } else if (scanning && !samemom && fb->fib_EntryType < 0) {
            Put("Can't move a pattern to ");
            CroakE(finaltoo, ERROR_OBJECT_WRONG_TYPE);
        } else if (sameob) {
            name = too;
            if (fromisdir)                              /* renaming a dir */
                samemom = true;
        } else if (samemom)
            todir = true;
        else if (FibIsDirectory(fb)) {                  /* dest is a dir */
            todir = true;
            samemom = CompareLocks(deer, lick) == LOCK_SAME;
        } else {
            hair = ERROR_OBJECT_EXISTS;     /* can't move to existing file */
            suck = false;
        }
        if (manx && samemom && !too) {
            Put("TO is required when FROM ends in \":\" or \"/\".\n");
            hair = dos1 ? ERROR_LINE_TOO_LONG : ERROR_REQUIRED_ARG_MISSING;
            Die(20);
        }
    } else {                                     /* dest doesn't exist yet */
        if (moveonly) {
            Put("Can't move to ");
            CroakE(finaltoo, ERROR_DIR_NOT_FOUND);
        } else if (scanning) {
            Put("Can't move a pattern to ");
            CroakE(finaltoo, ERROR_DIR_NOT_FOUND);
        } else {
            name = too;
            if (hair == ERROR_OBJECT_NOT_FOUND)
                hair = 0;
            if (hair)
                suck = false;                           /* bad volume, etc */
            else if (samedir) {
                if (too[0] == ':') {    /* rename packet can't handle this */
                    lick = LockVolumeRoot(deer);
                    name = ++too;
                } else
                    lick = DupLock(deer);
            }
        }
    }

    /* Time to DO IT */
    if (suck)                   /* prevent multiple "please insert"s */
        suck = DoIt(scanning, samemom, todir, name);
    if (!suck) {
        if (samedir)
            Putf("Can't name %s as %s", origfrom, finaltoo);
        else
            Putf("Can't move %s to %s", origfrom, finaltoo);
        Croak("");
    }
    hair = 0;
    Die(0);
}
