#ifndef MAIA_RUNTIME_NEWLIB_VFS_H
#define MAIA_RUNTIME_NEWLIB_VFS_H

#include <sys/stat.h>

/* Private Newlib routing helpers; thread-only, fail-fast EBUSY, no heap.
 * Adapter lock precedes the VFS lock. No adapter descriptor is exported as
 * a native VFS handle; compound position operations hold the adapter lock.
 */
int maia_newlib_vfs_read(int fd, char *buffer, int length);
int maia_newlib_vfs_write(int fd, const char *buffer, int length);
int maia_newlib_vfs_fstat(int fd, struct stat *st);

#endif
