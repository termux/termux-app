# OpenJDK 17 on InVxTermux

InVxTermux does **not** ship OpenJDK inside the APK. A Termux-built JDK is
~96 MiB per architecture (~220 MiB installed) and updates through
`termux-packages`; bundling it like Bun would balloon every APK download for
users who never run Java.

Use the seeded helper instead:

```bash
td-jdk-setup
java -version
jdk-doctor   # if something still fails
```

## Why `pkg install openjdk-17` fails or misbehaves

| Symptom | Likely cause | What to do |
|--------|----------------|------------|
| Package not found | Play Store Termux or stale mirrors | Install from [GitHub releases](https://github.com/involvex/termux-app/releases); `pkg update` |
| Stuck on **Setting up openjdk-17** | `update-alternatives` + many man pages (slow with `mandoc`) | Wait several minutes; or `pkg remove mandoc` and retry |
| `dlopen` / `libandroid-shmem` | Recommends skipped (`--no-install-recommends`) | `pkg install libandroid-shmem openjdk-17-x`; or run `td-jdk-setup` |
| `libiconv_open` in `libsplashscreen.so` | Old OpenJDK build | `pkg upgrade openjdk-17` (fixed upstream in 17.0.16+) |
| **Bad system call** when running `java` | Debian/Ubuntu JDK in proot, or wrong binary | Only use Termux `openjdk-17` in the main prefix; not `apt install default-jdk` on the host |
| `dpkg` / configure errors after interrupt | Half-configured package | `dpkg --configure -a` then `td-jdk-setup` |

This fork already rewrites `com.termux` paths in apt/dpkg and maintainer scripts
(`TermuxShellEnvironment`), so OpenJDK postinst scripts should run against
`com.involvex.termux_app` like other packages.

## Bundling into the app (decision)

| Approach | Pros | Cons |
|----------|------|------|
| **pkg / `td-jdk-setup` (current)** | Small APK, security updates via mirrors | Needs network on first install |
| **Embed `.deb` in APK (like bootstrap)** | Offline install | +~100 MiB per ABI; duplicate of pkg; GPL redistribution care |
| **Embed JVM only (custom extract)** | Slightly smaller than full deb | Must replicate `update-alternatives` + profile.d; high maintenance |

If we embed later, the realistic pattern is **on-demand download** to
`$PREFIX/var/cache/invapp/` (similar to `opencode-setup`), not `.incbin` in
native libs—unless we accept universal APKs well over 200 MiB.

## References

- Termux package: [openjdk-17](https://github.com/termux/termux-packages/tree/master/packages/openjdk-17)
- Missing `libandroid-shmem` dependency: [termux-packages#20376](https://github.com/termux/termux-packages/issues/20376)
- `libiconv_open` / splash screen: [termux-packages#25368](https://github.com/termux/termux-packages/issues/25368)
