/* -=-=- To do:
***** HANDLE case of "Name xxxxx :yyyy" using Rootify() somehow!  (Move works ok.)
** SOMETIMES error messages say "Can't move :" (blank from name).
* Will fail if current directory is zero, when (sameob || !too) && samedir?  (How test?)

Use MatchNext instead of ExNext, to handle multi-part patterns?  Nah.
Simulate move between volumes like Arp move?  Nah.
Fancy feature?: if multi assign in To, find first one on correct volume?  Nah.
*/

/* The idea here is to come up with a MOVE command which will obsolete RENAME
 * by doing things that COPY can do, such as take a pattern spec instead of
 * one file, and by letting the destination name default to the source if you
 * only specify a directory.
 */

/* Written by Paul Kienitz, 10/88 - present, as my very first useful program
 * (first begun, but not first finished!) for my brand new used Amiga 1000.
 * What the hell, it's in the public domain.
 */

/* Records listened to while writing the first version of this program:
 *      The Rhino Brothers Present The World's Worst Records, volumes 1 and 2
 *      U2:  the Forgettable Fire
 *      Peter Case
 *      Gang of Four:  Entertainment!
 *      Bruce Springsteen:  Darkness On The Edge Of Town
 *      Bruce Hornsby:  The Way It Is
 *      the Dead Milkmen:  Big Lizard In My Back Yard
 *      the REPO MAN soundtrack album
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
#include <libraries/dosextens.h>
#include <stdlib.h>        /* unlike early versions, use malloc */
#include <string.h>                     /* remove for aztec 3.6 */
#include <Paul.h>

unsigned toupper(unsigned c);                /* not in string.h */
void _exit(int _code);                    /* don't want fcntl.h */
void (*_cln)(void);                /* Aztec clib infrastructure */

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
    /* BPTR localock; */                /* for MatchNext results */
    long diskey;
    char name[108];
    char /* bool */ matches, useit;
};

fist maybe;

str from, too;                          /* these don't need freeing */
bool quiet, nodirs, icons, samedir, quessed, anything, samemom = false;

BPTR lick = 0, deer, elk;                       /* locks */
FIB *fb;
ubyte patt[PATLIMIT + 3];                       /* for pattern matching */
ubyte mesh[PATLIMIT * 2 + 8];
char namespace[108];
long hair;                                      /* error number */
adr rda;


/* ==================   functions   ================== */


void die(short r)
{
    fist foo;

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
    SetIoErr(hair);
    _exit(r);              /* clean up clib stuff */
}



void Croak(str errheader)
{
    if (hair)
        PrintFault(hair, errheader);
    if (hair == ERROR_NO_FREE_STORE || hair == ERROR_INVALID_COMPONENT_NAME)
        die(20);
    else
        die(hair ? 10 : 0);
}


void CCcheck(void)
{
    if (SetSignal(0L, (ulong) SIGBREAKF_CTRL_C) & SIGBREAKF_CTRL_C) {
        PutStr("\n *** BREAK\n");
        die(5);
    }
}



/* positive if a comes after b alphabetically */

short alphacmp(register ubyte *a, ubyte *b)
{
    return stricmp((str) *a, (str) *b);
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



BSTR beaster(str s)
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

    pat = FilePart(from);
    p = pat;
    for (i = 0; *p && i < PATLIMIT; p++, i++)
        patt[i] = toupper(*p);
    patt[i] = '\0';
    n = pat - from;
    fro = malloc(n + 1);
    strncpy(fro, from, n);
    fro[n] = '\0';
    from = fro;
    i = *p ? -1 : ParsePatternNoCase(patt, mesh, PATLIMIT * 2 + 8);
    if (i < 0) {
        hair = ERROR_INVALID_COMPONENT_NAME;         /* sort of */
        Printf("Can't move; bogus pattern \"%s\".\n", patt);
        die(20);
    }
    if (!i)
        return false;               /* no wildcards */
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



long RenamePacket(str oldn, str newn)
{
    struct FileLock *flake = gbip(deer);
    BSTR own = beaster(oldn), nun;
    long r1;

    if (!own)
        return hair;
    if (!(nun = beaster(newn))) {
        FreeBeast(own);
        return hair;
    }
    r1 = DoPkt4(flake->fl_Task, ACTION_RENAME_OBJECT, deer, own, lick, nun);
    FreeBeast(own);
    FreeBeast(nun);
    return r1 ? 0 : IoErr();
}



void MovePattern(void)
{
    bool cess = false;
    register str nn;
    int x, z;
    char k;
    fist yes, tm, *ptm;

    nn = &fb->fib_FileName[0];
    maybe = null;
    if (!Examine(deer, fb)) {
        hair = IoErr();
        Croak("Examine() failed");
    }
    while (ExNext(deer, fb)) {
        if (!(tm = (fist) malloc(sizeof(*tm)))) {
            hair = ERROR_NO_FREE_STORE;
            break;
        }
        strcpy(tm->name, nn);
        tm->diskey = fb->fib_DiskKey;
        tm->matches = tm->useit = (!nodirs || fb->fib_EntryType < 0)
                                  && (MatchPatternNoCase(mesh, nn));
        for (ptm = &maybe; *ptm && stricmp((*ptm)->name, nn) > 0
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
            if (x > 0 && !stricmp(nn + x, ".info")) {
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
        if (!hair) {
            nn = maybe->name;
            if (hair = RenamePacket(nn, nn)) {
                if (hair == ERROR_INVALID_LOCK)
                    hair == ERROR_RENAME_ACROSS_DEVICES;
                Printf("Error moving %s from %s to %s", nn, from, too);
                PrintFault(hair, "");
            } else if (!quiet)
                Printf(" %s\n", nn);
            /* continue if file disappeared, dest already has that name, or
               attempt to rename a directory inside itself, if any success */
            if ((hair == ERROR_OBJECT_EXISTS || hair == ERROR_OBJECT_IN_USE
                                             || hair == ERROR_OBJECT_NOT_FOUND)
                            && (cess || maybe->next))
                hair = 0;
            if (!hair)
                cess = true;
        }
        CCcheck();
        tm = maybe;
        maybe = maybe->next;
    }
    if (!cess && !quiet && !hair)
        PutStr("Nothing matched pattern.\n");
    if (hair) {
        if (!nn)
            Printf("Couldn't scan %s", from);
        Croak("");
    }
}



void Ready(long alen, str aptr)
{
    adr rar;
    static long drugs[6] = { 0, 0, 0, 0, 0, 0 };
    static char template[] =
#ifdef NAME
        "From/A,To=As,Q=Quiet/S,F=FilesOnly/S,I=Icon/S,M=Move/S";
#else
        "From/A,To=As,Q=Quiet/S,F=FilesOnly/S,I=Icon/S,N=Name/S";
#endif

    if (DOSBase->dl_lib.lib_Version < 37) {
        if (Output())
            Write(Output(), "AmigaDOS 2.04 or later required.\n", 33);
        SetIoErr(ERROR_INVALID_RESIDENT_LIBRARY);
        _exit(20);
    }
    if (!NEWP(fb)) {
        SetIoErr(ERROR_NO_FREE_STORE);
        Croak("Can't move");
    }
    rda = ReadArgs(template, drugs, null);
    if (!rda) {
        hair = IoErr();
        Croak("Invalid command line");
    }
    from = (str) drugs[0];
    too = (str) drugs[1];
    nodirs = drugs[2] & true;
    quiet = drugs[3] & true;
    icons = drugs[4] & true;
#ifdef NAME
    samedir = !drugs[5];
#else
    samedir = drugs[5] & true;
#endif
}



bool DoIt(bool scanning, bool todir, str name)
{
    struct FileLock *s = gbip(deer), *d = gbip(lick);
    bool suck;
    str fromtail = FilePart(from);

    if (scanning) {
        if (samemom) {
            if (!quiet)
                PutStr("Source and destination are the same "
                       "directory; nothing done.\n");
        } else
            MovePattern();
        suck = true;
    } else {
        if (!too && samemom)
            too = name = fromtail;
        if (todir)
            suck = !(hair = RenamePacket(name, name));
        else if (samedir)
            suck = !(hair = RenamePacket(fromtail, name));
        else if (!(suck = Rename(from, too)))               /* name == too */
            hair = IoErr();
        if (suck && icons) {
            if (todir | samedir) {
                from = Cat(todir ? name : fromtail, ".info");
                too = Cat(samedir ? too : name, ".info");
                hair = RenamePacket(from, too);
            } else {
                from = Cat(from, ".info");
                too = Cat(too, ".info");
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



void RelabelVolume(BPTR rootlock, str name)
{
    BSTR nam;
    struct MsgPort *rootask;
    bool suck = true;
    short l;

    hair = 0;
    if (!name || !*name) {
        hair = ERROR_INVALID_COMPONENT_NAME;
        PutStr("No name given to relabel volume with.\n");
        die(10);
    }
    if (suck) {
        l = strlen(name) - 1;
        if (name[l] == ':')
            name[l] = 0;
        if (strchr(name, '/') || strchr(name, ':'))
            suck = false;
    }
    if (!suck) {
        hair = ERROR_OBJECT_WRONG_TYPE;
        PutStr("Cannot move root directory.\n");
        die(10);
    }
    if (nam = beaster(name)) {
        rootask = bip(struct FileLock, rootlock)->fl_Task;
        if (!DoPkt1(rootask, ACTION_RENAME_DISK, nam))
            hair = IoErr();     /* ^^^ use packet so volume is unambiguous */
        FreeBeast(nam);
    }
    Croak("Couldn't relabel volume");           /* no err msg if succeeded */
}



bool SameFilename(str s1, str s2)
{
    register short l2;
    register char c;
    register bool ret;
    register str col = strrchr(s2, ':');

    /* If the second name ends with a colon (designating a device, volume, or
       assign), then this test is not applicable.  Return true, no mismatch. */
    if (col && col[1] == '\0')
        return true;
    s2 = FilePart(s2);
    l2 = strlen(s2) - 1;
    c = s2[l2];
    if (l2 == strlen(s1))
        s2[l2] = 0;
    ret = !stricmp(s1, s2);
    s2[l2] = c;
    return ret;
}



void Rootify(BPTR *lick)
{
    BPTR d2;
    while (*lick = ParentDir(d2 = *lick))
        UnLock(d2);
    *lick = d2;
}


void _main(long alen, str aptr)        /* low-level entrypoint: no argv[], no stdio */
{
    bool suck = true, scanning, todir = false, sameob = false;
    str name, ne;
    BPTR ocd;

    Ready(alen, aptr);
    elk = RLock(from);                          /* does source exist? */
    if (scanning = !elk) {
        hair = IoErr();
        if (!SplitTailPat()) {
            Printf("Can't move %s", from);
            Croak("");
        }
    } else {
        if (!(suck = Examine(elk, fb))) {
            hair = IoErr();
            Croak("Examine() failed");
        }
        name = &namespace[0];
        strcpy(name, fb->fib_FileName);
        if (!SameFilename(name, from)) {
            char nec;                           /* hard or soft link! */
            ne = FilePart(from);
            if (strlen(ne) > 107) {             /* should never happen? */
                hair = ERROR_INVALID_COMPONENT_NAME;
                deer = 0;
            } else {
                strcpy(name, ne);
                ne = PathPart(from);
                nec = *ne;
                if (ne > from)
                    *ne = 0;
                deer = RLock(from);             /* "manual" ParentDir of link */
                hair = IoErr();
            }
            if (!deer) {
                Croak("Cannot rename link");
            }
            *ne = nec;
        } else {
            deer = ParentDir(elk);
            hair = IoErr();
        }
        if (!deer) {
            if (hair == ERROR_NO_FREE_STORE)
                Croak("Cannot rename");
            if (samedir)
                RelabelVolume(elk, too);                /* DOES NOT RETURN */
            else {
                PutStr("Cannot move device or volume\n");
                die(10);
            }
        }
    }
    if (too) {
        if (strchr(too + 1, ':'))               /* fully qualified path? */
            samedir = false;
        if (samedir)
            ocd = CurrentDir(deer);
        lick = RLock(too);                              /* does dest exist? */
        sameob = SameLock(lick, elk) == LOCK_SAME;
        hair = lick ? IoErr() : 0;
        if (samedir)
            CurrentDir(ocd);
    }
    if (sameob || !too) {
        if (lick)
            UnLock(lick);
        if (samedir) {
            lick = DupLock(deer);
            samemom = true;
        } else {
            lick = DupLock(ocd = CurrentDir(0));
            CurrentDir(ocd);
        }
    }
    if (elk)
        UnLock(elk);
    elk = 0;
    if (lick) {
        if (!samemom && !Examine(lick, fb)) {           /* yes;  file or dir? */
            hair = IoErr();
            Croak("Examine() failed");
        }
        if (scanning && fb->fib_EntryType < 0) {
            hair = ERROR_OBJECT_WRONG_TYPE;
            Printf("Can't move a pattern to %s", too);
            Croak("");
        } else if (sameob)
            name = too;
        else if (samemom)
            todir = true;
        else if (fb->fib_EntryType > 0) {               /* it's a dir */
            todir = true;
            samemom = SameLock(deer, lick) == LOCK_SAME;
        } else {
            hair = ERROR_OBJECT_EXISTS;
            suck = false;
        }
    } else                                      /* it doesn't exist yet */
        if (scanning) {
            hair = ERROR_DIR_NOT_FOUND;
            Printf("Can't move a pattern to %s", too);
            Croak("");
        } else {
            name = too;
            if (hair == ERROR_OBJECT_NOT_FOUND)
                hair = 0;
            if (hair)
                suck = false;                           /* bad volume, etc */
            else if (samedir)
                lick = DupLock(deer);
        }

    /* Time to DO IT */
    if (suck)                   /* prevent multiple "please insert"s */
        suck = DoIt(scanning, todir, name);
    if (!suck) {
        Printf("Can't move %s to %s", from, too);
        Croak("");
    }
    hair = 0;
    die(0);
}
