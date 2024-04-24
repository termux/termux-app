## Server-interpreted Authentication Types "localuser" and "localgroup"

On systems which can determine in a secure fashion the credentials of a client
process, the "localuser" and "localgroup" authentication methods provide access
based on those credentials.  The format of the values provided is platform
specific.  For POSIX & UNIX platforms, if the value starts with the character
'#', the rest of the string shall be treated as a decimal uid or gid, otherwise
the string is defined as a user name or group name.

Systems offering this MUST not simply trust a user supplied value (such as an
environment variable or IDENT protocol response).  It is expected many systems
will only support this for clients running on the same host using a local IPC
transport.

Examples:

```
xhost +SI:localuser:alanc
```

```
xhost +SI:localuser:#1234
```

```
xhost +SI:localgroup:wheel
```

```
xhost +SI:localgroup:#0
```

