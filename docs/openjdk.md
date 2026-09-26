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
| `openjdk-17-x` Depends on `openjdk-17` / **fix-broken** loop | `openjdk-17` failed but `-x` (recommend) left apt broken | `invapp-openjdk-recover` then install **17 before -x** (see below) |
| `unable to execute … tmp.ci/preinst: No such file or directory` | `openjdk-17` **preinst has no shebang** (dpkg execs it directly) or shebang points at `com.termux` | Install latest app build (redirector prepends `#!$PREFIX/bin/sh`), or repack deb (below); ensure `LD_PRELOAD=$PREFIX/lib/libinvapp-redirector.so` |

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

## Fix broken apt after a failed install

`openjdk-17-x` is often pulled in as a recommend **with** `openjdk-17`. If the
main package fails on `preinst`, apt can end up with `-x` installed (or half-installed)
while `openjdk-17` is missing — then every `pkg`/`apt` command complains about
unmet dependencies.

```bash
export LD_PRELOAD=$PREFIX/lib/libinvapp-redirector.so
invapp-openjdk-recover    # or manual purge below
```

Manual equivalent:

```bash
export LD_PRELOAD=$PREFIX/lib/libinvapp-redirector.so
dpkg --purge --force-remove-reinstreq openjdk-17-x openjdk-17
apt --fix-broken install -y
```

Then install **only** `openjdk-17` first (repack if needed), **then** recommends:

```bash
pkg install -y libandroid-shmem libandroid-spawn libiconv libjpeg-turbo zlib littlecms alsa-plugins
invapp-openjdk-repack-install   # if plain pkg still fails on preinst
# OR: pkg install -y openjdk-17
pkg install -y openjdk-17-x ca-certificates-java resolv-conf
```

## Workaround: repack the deb on-device (no app update)

If `pkg install openjdk-17` still fails, add a shebang to the control scripts and reinstall:

```bash
DEB="$HOME/../cache/apt/archives/openjdk-17_17.0.20_aarch64.deb"
WORK="$PREFIX/tmp/openjdk-repack"
rm -rf "$WORK" && mkdir -p "$WORK" && cd "$WORK"
ar x "$DEB"
tar xf control.tar.xz
for s in preinst postinst prerm postrm; do
  [ -f "$s" ] || continue
  if ! head -1 "$s" | grep -q '^#!'; then
    { echo "#!$PREFIX/bin/sh"; cat "$s"; } > "$s.new" && mv "$s.new" "$s"
  fi
  sed -i "s/com.termux/com.involvex.termux_app/g" "$s"
  chmod 700 "$s"
done
tar -cJf control.tar.xz ./*
printf '2.0\n' > debian-binary
ar cr openjdk-17-fixed.deb debian-binary control.tar.xz data.tar.xz
dpkg -i openjdk-17-fixed.deb
```

## References

- Termux package: [openjdk-17](https://github.com/termux/termux-packages/tree/master/packages/openjdk-17)
- Missing `libandroid-shmem` dependency: [termux-packages#20376](https://github.com/termux/termux-packages/issues/20376)
- `libiconv_open` / splash screen: [termux-packages#25368](https://github.com/termux/termux-packages/issues/25368)
