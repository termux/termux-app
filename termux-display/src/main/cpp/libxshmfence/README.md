libxshmfence - Shared memory 'SyncFence' synchronization primitive
------------------------------------------------------------------

This library offers a CPU-based synchronization primitive compatible
with the X SyncFence objects that can be shared between processes
using file descriptor passing.

There are two underlying implementations:

 1) On Linux, the library uses futexes

 2) On other systems, the library uses posix mutexes and condition
    variables.

All questions regarding this software should be directed at the
Xorg mailing list:

  https://lists.x.org/mailman/listinfo/xorg

The primary development code repository can be found at:

  https://gitlab.freedesktop.org/xorg/lib/libxshmfence

Please submit bug reports and requests to merge patches there.

For patch submission instructions, see:

  https://www.x.org/wiki/Development/Documentation/SubmittingPatches

