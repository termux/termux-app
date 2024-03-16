    area pixman_msvc, code, readonly

    export  pixman_msvc_try_arm_simd_op

pixman_msvc_try_arm_simd_op
    ;; I don't think the msvc arm asm knows how to do SIMD insns
    ;; uqadd8 r3,r3,r3
    dcd 0xe6633f93
    mov pc,lr
    endp

    export  pixman_msvc_try_arm_neon_op

pixman_msvc_try_arm_neon_op
    ;; I don't think the msvc arm asm knows how to do NEON insns
    ;; veor d0,d0,d0
    dcd 0xf3000110
    mov pc,lr
    endp

    end
