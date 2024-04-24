## ServerInterpreted address definition for type "hostname"

A ServerInterpreted address with type "hostname" shall have a value
representing a hostname as defined in [IETF RFC 2396](https://tools.ietf.org/html/rfc2396). Due to Mobile IP
and dynamic DNS, the name service should be consulted at connection
authentication time.  Clients supporting Internationalized Domain
Names must convert to ASCII Compatible Encoding as specified in the
relevant IETF RFC's before use in the X11 protocol.  Note that this
definition of hostname does not allow use of literal IP addresses.

Version 1.0, March 2004
