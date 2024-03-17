#include <dos/dos.h>
#include <clib/exec_protos.h>


#asm
        public  _mob

_mob:   move.b  d0,(a3)+
        rts
#endasm
static void mob(char d, char *s);


void VSprintf(char *buf, char *format, void *args)
{
    RawDoFmt(format, args, mob, buf);
}


char *FaultMessage(long errorcode)
{
    switch (errorcode) {
        case ERROR_NO_FREE_STORE:            // 103
            return "insufficient free store";
            // 2.x "not enough memory available"
        case ERROR_TASK_TABLE_FULL:          // 105
            return "task table full";
            // 2.x "process table full"
        case ERROR_LINE_TOO_LONG:            // 120
            return "argument line invalid or too long";
        case ERROR_FILE_NOT_OBJECT:          // 121
            return "file is not an object module";
            // 2.x "file is not executable"
        case ERROR_INVALID_RESIDENT_LIBRARY: // 122
            return "invalid resident library during load";
        case ERROR_OBJECT_IN_USE:            // 202
            return "object in use";
            // 2.x "object is in use"
        case ERROR_OBJECT_EXISTS:            // 203
            return "object already exists";
        case ERROR_DIR_NOT_FOUND:            // 204
            return "directory not found";
        case ERROR_OBJECT_NOT_FOUND:         // 205
            return "object not found";
        case ERROR_BAD_STREAM_NAME:          // 206
            return "invalid window description";
        case ERROR_ACTION_NOT_KNOWN:         // 209
            return "packet request type unknown";
        case ERROR_INVALID_COMPONENT_NAME:   // 210
            return "stream name component invalid";
            // 2.x "object name invalid"
        case ERROR_INVALID_LOCK:             // 211
            return "invalid object lock";
        case ERROR_OBJECT_WRONG_TYPE:        // 212
            return "object not of required type";
            // 2.x "object is not of required type"
        case ERROR_DISK_NOT_VALIDATED:       // 213
            return "disk not validated";
        case ERROR_DISK_WRITE_PROTECTED:     // 214
            return "disk write-protected";
            // 2.x "disk is write-protected"
        case ERROR_RENAME_ACROSS_DEVICES:    // 215
            return "rename across devices attempted";
        case ERROR_DIRECTORY_NOT_EMPTY:      // 216
            return "directory not empty";
        case ERROR_DEVICE_NOT_MOUNTED:       // 218
            return "device (or volume) not mounted";
            // 2.x "device (or volume) is not mounted"
        case ERROR_SEEK_ERROR:               // 219
            return "seek failure";
        case ERROR_COMMENT_TOO_BIG:          // 220
            return "comment too big";
            // 2.x "comment is too long"
        case ERROR_DISK_FULL:                // 221
            return "disk full";
            // 2.x "disk is full"
        case ERROR_DELETE_PROTECTED:         // 222
            return "file is protected from deletion";
        case ERROR_WRITE_PROTECTED:          // 223
            return "file is write protected";
        case ERROR_READ_PROTECTED:           // 224
            return "file is read protected";
        case ERROR_NOT_A_DOS_DISK:           // 225
            return "not a valid DOS disk";
        case ERROR_NO_DISK:                  // 226
            return "no disk in drive";
        case ERROR_NO_MORE_ENTRIES:          // 232
            return "no more entries in directory";

        // The following are not defined until AmigaDOS 2, but are
        // returned by TemplateParse, and we want to be specific there.
        case ERROR_BAD_TEMPLATE:            // 114
            return "bad template";
        /* case ERROR_BAD_NUMBER:           // 115 - restore if supporting /N
            return "bad number"; */
        case ERROR_REQUIRED_ARG_MISSING:    // 116
            return "required argument missing";
        case ERROR_KEY_NEEDS_ARG:           // 117
            return "value after keyword missing";
        case ERROR_TOO_MANY_ARGS:           // 118
            return "wrong number of arguments";
        /* case ERROR_UNMATCHED_QUOTES:     // 119 - 2.x doesn't use this?
            return "unmatched quotes"; */

        default:
            return NULL;
        // if null, use VSprintf(buf, "Error %ld", &errorcode) or equivalent
    }
    return 0L;         /* satisfy compiler */
}
