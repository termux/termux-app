libandroid-shmem
================
System V shared memory (shmget, shmat, shmdt and shmctl) emulation on Android using ashmem for use in [Termux](https://termux.com/).

The shared memory segments it creates will be automatically destroyed when the creating process destroys them or dies, which differs from the System V shared memory behaviour.

Based on previous work in https://github.com/pelya/android-shmem.

Hacking
=======
The project can be developed on Android devices using Termux. Clone the repo and run `make` in the `tests/` folder after editing the library or test cases.
