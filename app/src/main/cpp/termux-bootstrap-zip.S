     .global blob
     .global blob_size
     .section .rodata
 blob:
 #if defined __i686__
     .incbin "bootstrap-legacy-i686.zip"
 #elif defined __x86_64__
     .incbin "bootstrap-legacy-x86_64.zip"
 #elif defined __aarch64__
     .incbin "bootstrap-legacy-aarch64.zip"
 #elif defined __arm__
     .incbin "bootstrap-legacy-arm.zip"
 #else
 # error Unsupported arch
 #endif
 1:
 blob_size:
     .int 1b - blob
