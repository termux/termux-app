# PR (draft for discussion)

> **title**: `feat(extra-keys): add an optional tekl DSL authoring path for extra-keys`

---

## 0 TL;DR

This is a proposal with a **working reference implementation**. It adds an
**optional** way to author `extra-keys`:

- A new `extra-keys-tekl` property holds tekl source.
- A ~300-line pure-Kotlin compiler turns it straight into Termux's internal
  `ExtraKeyButton` model — no JSON round-trip.
- The existing `extra-keys` JSON path stays **exactly as it is**; legacy configs keep
  working; nothing is migrated or removed.
- tekl errors are caught at compile time with line + column.
- Zero new dependencies; GPLv3.

## 0.5 It already runs today (MVP)

The language and toolchain are **already built and exercised** — not a sketch. See
the public repo:

> **https://github.com/poisongod/termux-extrakey-lang** (GPLv3)

It ships a Node.js implementation of the full pipeline used in this PR's design:

```
tokenizer → parser → compiler → (validation) → CLI
```

Try it now inside a fresh Termux:

```bash
git clone https://github.com/poisongod/termux-extrakey-lang
cd termux-extrakey-lang
npm install && npm test        # 59 tests: grammar, mapping, errors, warnings
npm link

termux-extrakey-tekl keys.tekl --reload    # compile+write+reload termux.properties
termux-extrakey-tekl keys.tekl --watch     # recompile on every save
termux-extrakey-tjek <file|->              # validate existing JSON configs
```

`keys.tekl` example (the repo's own sample; see `README.md` for the full mapping):

```
ESC TAB CTRL ALT HOME UP END PGUP
PGDN DOWN LEFT RIGHT DEL BKSP INS;
CTRL(C) ALT(A) FN(F) SHIFT(S)
CTRL(A):toggle HOME:Start ESC.F1;
DRAWER KEYBOARD SCROLL SPACE ENTER
BACKSLASH QUOTE APOSTROPHE
F1 F2 F3 F4 F5 F6 F7 F8 F9 F10 F11 F12;
```

While editing `keys.tekl`, the `--watch` mode recompiles and refreshes the keyboard
immediately — the same UX this PR would enable natively in the app.

The PR would port this tested compiler into `termux-shared`/Kotlin and connect it to the
property loader; the grammar, tests and docs all already exist.

---

## 1 Background and motivation

### 1.1 `extra-keys` is a config surface many users touch, and authoring has friction

JSON was a reasonable choice when `extra-keys` launched; it is a well-known format and
maps directly to the internal model. Over time, a few friction points have surfaced in
user reports:

- **Escaping**: `termux.properties` adds its own escaping rules on top of JSON's. Getting
  characters like `\` or quotes right through two escaping layers is a common source of
  user confusion (e.g. issues around `\` and quotes in `extra-keys` values).
- **No positioned diagnostics**: a parse failure surfaces as a load-time
  `JSONException` without a line or column, so users must debug the whole string by eye.
- **Verbosity**: a single combo key (`CTRL+C`) needs
  `{"macro":"CTRL C","display":"CTRL C"}`; rows of a dozen keys become hard to read,
  and there is no natural way to add a comment.

None of this is anyone's fault — it is the inherent shape of "JSON inside
`.properties`". It is also not something this PR tries to "fix away"; it simply offers a
second, friendlier authoring surface next to the existing one.

### 1.2 Users already build their own authoring tools

Because the JSON path is unforgiving, users in the wild have created helper scripts that
edit `extra-keys` in `termux.properties` and reload settings — including setups shared in
community posts. That is a signal: people want a higher-level, safer way to author their
keyboard. tekl is a formalization of exactly that: a small language with a compiler, so
the "helper script" becomes maintainable and validated.

### 1.3 Precedent: a keyboard layout is a small language

A layout is a matrix of cells with a small fixed semantics (plain key / combo / label /
popup). Key-binding configs in vim, zsh or ssh chose DSLs over JSON for the same reason:
the format follows the structure of the thing being configured, rather than the other way
around. Termux already treats `extra-keys` as a shareable, agnostic component in
`termux-shared`; a DSL authoring path fits that direction.

---

## 2 What tekl looks like

### 2.1 Grammar (four productions)

```
N ::= "ESC" | "TAB" | ... | [A-Za-z0-9_]*   // plain keys
S ::= "CTRL" | "ALT" | "FN" | "SHIFT"        // modifiers
A ::= N | S | A(A) | A:N
M ::= A | A.A | M M | M'\n' | M';\n'
```

| written | meaning | internal model |
| --- | --- | --- |
| `ESC` | plain key | `"ESC"` |
| `CTRL(C)` | combo | `{"macro":"CTRL C","display":"CTRL C"}` |
| `HOME:Start` | label | `{"key":"HOME","display":"Start"}` |
| `ESC.F1` | popup | `{"key":"ESC","popup":"F1"}` |
| `CTRL(C):copy.F5` | combo+label+popup | `{"macro":"CTRL C","display":"copy","popup":"F5"}` |
| `;` | new row | — |

All reserved words are existing Termux key names. Keys are identifiers, so the
double-escaping scenarios above cannot occur by construction.

### 2.2 Authoring benefits

- **Positioned errors**: `CTRL(C` reports line/column with a caret instead of a generic
  load-time failure.
- **Compile-time warnings** (non-fatal): empty label, uneven row lengths, missing
  `DRAWER`/`KEYBOARD`.
- **Readability & diffability**: a row is one short line; comments are natural.

---

## 3 Design and footprint

### 3.1 Integration

```
termux-shared/.../TermuxPropertyConstants.java        +KEY_EXTRA_KEYS_TEKL
termux-shared/.../extrakeys/ExtraKeysInfo.java        branch: tekl -> List<ExtraKeyButton>
termux-shared/.../extrakeys/tekl/*.kt                  new compiler (tokenizer/parser/compiler)
app/src/main/java/.../terminal/io/TermuxTerminalExtraKeys.java   read + dispatch
app/src/test/...                                        tekl test suite
```

- `extra-keys` handling is untouched; when `extra-keys-tekl` is absent/empty, behavior is
  byte-identical to today.
- The compiler is a pure function (string → `List<ExtraKeyButton>`), no Android API, unit
  testable in isolation — usable later by plugins or a TUI editor, and by the companion
  CLI (`termux-extrakey-tekl`, with `--watch`/`--reload`), which is part of the same
  language project.
- Rendering (`ExtraKeysView`) sees the same model as today; no changes there.

### 3.2 Explicitly out of scope

- No migration script, no default change, no removal of the JSON path.
- tekl stays opt-in; JSON remains fully supported indefinitely.

### 3.3 Maturity

Covered in §0.5: the compiler, CLI and 59 tests already exist and are publicly
reviewable; the integration part of this PR is a port plus property wiring, not a
greenfield design.

---

## 4 Success criteria

- A user writes the same keyboard in tekl and it renders after `termux-reload-settings`.
- A typo like `CTRL(C` produces a positioned message instead of a silent failure.
- All existing JSON configs keep working; CI stays green.

---

## 5 Open questions (happy to follow maintainer guidance)

1. Property discovery: dedicated `extra-keys-tekl` key vs value-prefix detection?
   (leaning dedicated key, but flexible)
2. Keep the JSON path read-only forever (leaning yes)?
3. Sync strategy between the Kotlin port and the JS reference?
4. Diagnostics language: English vs i18n?

Thanks for reading — feedback very welcome.