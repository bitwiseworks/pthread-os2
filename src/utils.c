
#include <stdarg.h>
#define INCL_DOSSEMAPHORES
#define INCL_DOSMODULEMGR
#define INCL_DOSMISC
#define INCL_DOSNLS
#define INCL_DOSERRORS
#include <os2emx.h>


/* Return TRUE iff the region of memory starting at START is
   readable. */

int verify_memory (ULONG start, ULONG size)
{
  ULONG rc, cb, flag;

  while (size != 0)
    {
      cb = size;
      rc = DosQueryMem ((PVOID)start, &cb, &flag);
      if (rc != 0)
        return FALSE;
      if (!(flag & (PAG_COMMIT|PAG_SHARED)))
        return FALSE;
      if (flag & PAG_FREE)
        return FALSE;           /* Should not happen */
      if (!(flag & PAG_READ))
        return FALSE;
      if (cb == 0 || cb > size)
        return FALSE;           /* Should not happen */
      start += cb; size -= cb;
    }
  return TRUE;
}
