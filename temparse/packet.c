#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <clib/dos_protos.h>
#include <pragmas/exec_lib.h>
#include <pragmas/dos_lib.h>
#include <dos/dosextens.h>

#define PUBLIC


extern struct DosLibrary *DOSBase;


PUBLIC struct StandardPacket *CreateDosPacket(struct MsgPort *replyport)
{
    struct StandardPacket *packet = NULL;
    if (replyport)
        packet = AllocMem(sizeof(struct StandardPacket), MEMF_PUBLIC | MEMF_CLEAR);
    if (packet) {
        packet->sp_Msg.mn_Node.ln_Name = (char *) &(packet->sp_Pkt);
        packet->sp_Msg.mn_Node.ln_Type = NT_MESSAGE;
        packet->sp_Msg.mn_ReplyPort = replyport;
        packet->sp_Pkt.dp_Link = &(packet->sp_Msg);
        packet->sp_Pkt.dp_Port = replyport;
    }
    return packet;
}


PUBLIC long InvokeDosPacket(struct StandardPacket *packet,
                            struct MsgPort *handler, long *result2)
{
    struct MsgPort *replyport = packet->sp_Pkt.dp_Port;   /* gets clobbered */
    PutMsg(handler, (struct Message *) packet);
    WaitPort(replyport);
    GetMsg(replyport);
    if (result2)
        *result2 = packet->sp_Pkt.dp_Res2;
    return packet->sp_Pkt.dp_Res1;
}


PUBLIC void FreeDosPacket(struct StandardPacket *packet)
{
    if (packet)
        FreeMem(packet, sizeof(struct StandardPacket));
}


PUBLIC long DoDosPacket(struct MsgPort *handler, long action,
                        long arg1, long arg2, long arg3, long arg4,
                        long arg5, long arg6, long arg7)
{
    struct Process *me = (struct Process *) FindTask(NULL);
    struct MsgPort *replyport = &me->pr_MsgPort;   // or CreatePort(NULL, 0);
    struct StandardPacket *packet = CreateDosPacket(replyport);
    long result;

    if (!packet) {
        // if (replyport) DeletePort(replyport);
        me->pr_Result2 = ERROR_NO_FREE_STORE;
        return NULL;
    }
    packet->sp_Pkt.dp_Type = action;
    packet->sp_Pkt.dp_Arg1 = arg1;
    packet->sp_Pkt.dp_Arg2 = arg2;
    packet->sp_Pkt.dp_Arg3 = arg3;
    packet->sp_Pkt.dp_Arg4 = arg4;
    packet->sp_Pkt.dp_Arg5 = arg5;
    packet->sp_Pkt.dp_Arg6 = arg6;
    packet->sp_Pkt.dp_Arg7 = arg7;

    result = InvokeDosPacket(packet, handler, &me->pr_Result2);
    FreeDosPacket(packet);
    // DeletePort(replyport);
    return result;
}


PUBLIC long CompareLocks(BPTR lock1, BPTR lock2)
{
    struct FileLock *fl1 = BADDR(lock1), *fl2 = BADDR(lock2);
    struct MsgPort *port1, *port2, *bootport;
    BPTR vol1, vol2, bootvol = 0;
    long key1, key2, bootkey = -1;

    if (lock1 == lock2)
        return LOCK_SAME;
    if (lock1 && lock2 && DOSBase->dl_lib.lib_Version >= 36) {
        long official = SameLock(lock1, lock2);
        if (DOSBase->dl_lib.lib_Version == 36 && official == LOCK_SAME_VOLUME
                                    && fl1->fl_Volume != fl2->fl_Volume)
            return LOCK_DIFFERENT;      /* attempt to patch known bug */
        return official;
    }

    bootport = ((struct Process *) FindTask(NULL))->pr_FileSystemTask;
    port1 = fl1 ? fl1->fl_Task : bootport;
    port2 = fl2 ? fl2->fl_Task : bootport;
    if (port1 != port2)
        return LOCK_DIFFERENT;
        
    if (!lock1 || !lock2) {
        /* under 1.x we can get away with avoiding AllocMem */
        char infospace[sizeof(struct InfoData) + 4];
        struct InfoData *info = (APTR) (((ULONG) (infospace + 4)) & ~3L);
        Info(0, info);
        bootvol = info->id_VolumeNode;
    }
    vol1 = fl1 ? fl1->fl_Volume : bootvol;
    vol2 = fl2 ? fl2->fl_Volume : bootvol;
    if (vol1 != vol2)
        return LOCK_DIFFERENT;
    
    if (!lock1 || !lock2) {
        char fibspace[sizeof(struct FileInfoBlock) + 4];
        struct FileInfoBlock *fib = (APTR) (((ULONG) (fibspace + 4)) & ~3L);
        if (Examine(0, fib))
            bootkey = fib->fib_DiskKey;
    }
    key1 = fl1 ? fl1->fl_Key : bootkey;
    key2 = fl2 ? fl2->fl_Key : bootkey;
    return key1 == key2 ? LOCK_SAME : LOCK_SAME_VOLUME;
}
