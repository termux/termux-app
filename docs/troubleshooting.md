# Troubleshooting

| Symptom | Fix |
|---|---|
| `Unknown signal 31 (SIGSYS)` on Bun | Use bundled `$PREFIX/libexec/bun`; never `bun.sh/install`. Delete `node_modules` + lockfile if a PC lockfile pinned `linux-*` natives, then `bun i` on the phone |
| `/usr/bin/env: bad interpreter` on npm/npx | Open a **new** session after update (redirector rewrites shebangs). `pkg install nodejs` if npm is missing |
| `Permission denied` on `tsc` / bins | Project is under shared storage (**noexec**). Move to `~/repos` |
| OpenJDK install stuck / `java` dlopen / Bad system call | Run `td-jdk-setup` (installs recommends incl. `libandroid-shmem`). `jdk-doctor` for details. See [OpenJDK 17](openjdk.md) |
| OpenCode `Permission denied` / LIBC / postinstall / SIGSYS | Re-run `opencode-setup` (GitHub tarball + glibc). Do not `unset LD_PRELOAD` |
| OpenCode `AI_APICallError` / “typo in the url or port” | Local `:4096` OK; Zen/provider broken. `rm -rf ~/.cache/opencode`, fix `@opencode-ai/plugin@local` → `latest`, re-auth, seed glibc DNS/CA, `opencode-setup`, retry from `~/repos/…` |
| OpenCode `getifaddrs returned an error` | Non-fatal for `0.0.0.0` bind; never `--mdns`. Override `OPENCODE_HOST=127.0.0.1 td-ai` if needed. Preview uses `127.0.0.1:4096` / `/global/health` |
| Preview blank / wrong port | OpenCode is `:4096` — there is no `/api` on `:5000`. Drawer **AI** opens Preview after health; tap Reload if early |
| Tab completion missing for bun/pkg/npm | Open a **new** session after app update. Optional: `pkg install bash-completion`. Completers live under `$PREFIX/etc/bash_completion.d/` |
| Extra keys only show ESC/nav | Swipe the key bar **left** for workflow page (pull / bun i / …). **⋯ → Customize keys…** or swipe from the right edge for Tools |
| Green-bar `ls` folders | Hacker theme ANSI clash — new session or replace `~/.termux/colors.properties` |
| Phantom kill `[signal 9]` (Android 12+) | OS phantom/CPU killer — see upstream Termux issue [#2366](https://github.com/termux/termux-app/issues/2366) |
| Plugin install fail | Same signature + `sharedUserId` source only; uninstall all, reinstall set |

More agent-oriented notes live in [`AGENTS.md`](https://github.com/involvex/termux-app/blob/master/AGENTS.md)
in the repository.
