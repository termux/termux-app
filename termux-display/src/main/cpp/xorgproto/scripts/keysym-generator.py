#!/usr/bin/env python3
#
# SPDX-License-Identifier: MIT
#
# This script checks XF86keysym.h for the reserved evdev keysym range and/or
# appends new keysym to that range. An up-to-date libevdev must be
# available to guarantee the correct keycode ranges and names.
#
# Run with --help for usage information.
#
#
# File is formatted with Python Black

import argparse
import logging
import os
import sys
import re
import subprocess
from pathlib import Path

try:
    import libevdev
except ModuleNotFoundError as e:
    print(f"Error: {e}", file=sys.stderr)
    print(
        "One or more python modules are missing. Please install those "
        "modules and re-run this tool."
    )
    sys.exit(77)


logging.basicConfig(level=logging.DEBUG, format="%(levelname)s: %(message)s")
logger = logging.getLogger("ksgen")

start_token = re.compile(r"#define _EVDEVK.*")
end_token = re.compile(r"#undef _EVDEVK\n")


def die(msg):
    logger.critical(msg)
    sys.exit(1)


def all_keysyms(directory):
    """
    Extract the key names for all keysyms we have in our repo and return
    them as list.
    """
    keysym_names = []
    pattern = re.compile(r"^#define\s+(?P<name>\w+)\s+(0x[0-9A-Fa-f]+)")
    for path in directory.glob("*keysym*.h"):
        with open(path) as fd:
            for line in fd:
                match = re.match(pattern, line)
                if match:
                    keysym_names.append(match.group("name"))
    return keysym_names


class Kernel(object):
    """
    Wrapper around the kernel git tree to simplify searching for when a
    particular keycode was introduced.
    """

    def __init__(self, repo):
        self.repo = repo

        exitcode, stdout, stderr = self.git_command("git branch --show-current")
        if exitcode != 0:
            die(f"{stderr}")
        if stdout.strip() != "master":
            die(f"Kernel repo must be on the master branch (current: {stdout.strip()})")

        exitcode, stdout, stderr = self.git_command("git tag --sort=version:refname")
        tags = stdout.split("\n")
        self.versions = list(
            filter(lambda v: re.match(r"^v[2-6]\.[0-9]+(\.[0-9]+)?$", v), tags)
        )
        logger.debug(f"Kernel versions: {', '.join(self.versions)}")

    def git_command(self, cmd):
        """
        Takes a single-string git command and runs it in the repo.

        Returns the tuple (exitcode, stdout, stderr)
        """
        # logger.debug(f"git command: {cmd}")
        try:
            result = subprocess.run(
                cmd.split(" "), cwd=self.repo, capture_output=True, encoding="utf8"
            )
            if result.returncode == 128:
                die(f"{result.stderr}")

            return result.returncode, result.stdout, result.stderr
        except FileNotFoundError:
            die(f"{self.repo} is not a git repository")

    def introduced_in_version(self, string):
        """
        Search this repo for the first version with string in the headers.

        Returns the kernel version number (e.g. "v5.10") or None
        """

        # The fastest approach is to git grep every version for the string
        # and return the first. Using git log -G and then git tag --contains
        # is an order of magnitude slower.
        def found_in_version(v):
            cmd = f"git grep -E \\<{string}\\> {v} -- include/"
            exitcode, _, _ = self.git_command(cmd)
            return exitcode == 0

        def bisect(iterable, func):
            """
            Return the first element in iterable for which func
            returns True.
            """
            # bias to speed things up: most keycodes will be in the first
            # kernel version
            if func(iterable[0]):
                return iterable[0]

            lo, hi = 0, len(iterable)
            while lo < hi:
                mid = (lo + hi) // 2
                if func(iterable[mid]):
                    hi = mid
                else:
                    lo = mid + 1
            return iterable[hi]

        version = bisect(self.versions, found_in_version)
        logger.debug(f"Bisected {string} to {version}")
        # 2.6.11 doesn't count, that's the start of git
        return version if version != self.versions[0] else None


TARGET_KEYSYM_COLUMN = 54
"""Column in the file we want the keysym codes to end"""
KEYSYM_NAME_MAX_LENGTH = TARGET_KEYSYM_COLUMN - len("#define ") - len("_EVDEVK(0xNNN)")
KERNEL_VERSION_PADDING = 7


def generate_keysym_line(code, kernel, kver_list=[]):
    """
    Generate the line to append to the keysym file.

    This format is semi-ABI, scripts rely on the format of this line (e.g. in
    xkeyboard-config).
    """
    evcode = libevdev.evbit(libevdev.EV_KEY.value, code)
    if not evcode.is_defined:  # codes without a #define in the kernel
        return None
    if evcode.name.startswith("BTN_"):
        return None

    name = "".join([s.capitalize() for s in evcode.name[4:].lower().split("_")])
    keysym = f"XF86XK_{name}"
    spaces = KEYSYM_NAME_MAX_LENGTH - len(keysym)
    if not spaces:
        raise ValueError(f"Insufficient padding for keysym “{keysym}”.")
    kver = kernel.introduced_in_version(evcode.name) or " "
    if kver_list:
        from fnmatch import fnmatch

        allowed_kvers = [v.strip() for v in kver_list.split(",")]
        for allowed in allowed_kvers:
            if fnmatch(kver, allowed):
                break
        else:  # no match
            return None

    return f"#define {keysym}{' ' * spaces}_EVDEVK(0x{code:03x})  /* {kver: <{KERNEL_VERSION_PADDING}s} {evcode.name} */"


def verify(ns):
    """
    Verify that the XF86keysym.h file follows the requirements. Since we expect
    the header file to be parsed by outside scripts, the requirements for the format
    are quite strict, including things like correct-case hex codes.
    """

    # No other keysym must use this range
    reserved_range = re.compile(r"#define.*0x10081.*")
    normal_range = re.compile(r"#define.*0x1008.*")

    # This is the full pattern we expect.
    expected_pattern = re.compile(
        r"#define XF86XK_\w+ +_EVDEVK\(0x([0-9A-Fa-f]{3})\) +/\* (v[2-6]\.[0-9]+(\.[0-9]+)?)? +KEY_\w+ \*/"
    )
    # This is the comment pattern we expect
    expected_comment_pattern = re.compile(
        r"/\* Use: (?P<name>\w+) +_EVDEVK\(0x(?P<value>[0-9A-Fa-f]{3})\) +   (v[2-6]\.[0-9]+(\.[0-9]+)?)? +KEY_\w+ \*/"
    )

    # Some patterns to spot specific errors, just so we can print useful errors
    define = re.compile(r"^#define .*")
    name_pattern = re.compile(r"#define (XF86XK_[^\s]*)")
    space_check = re.compile(r"#define \w+(\s+)[^\s]+(\s+)")
    hex_pattern = re.compile(r".*0x([a-f0-9]+).*", re.I)
    comment_format = re.compile(r".*/\* ([^\s]+)?\s+(\w+)")
    kver_format = re.compile(r"v[2-6]\.[0-9]+(\.[0-9]+)?")

    in_evdev_codes_section = False
    had_evdev_codes_section = False
    success = True

    all_defines = []

    all_keysym_names = all_keysyms(ns.header.parent)

    class ParserError(Exception):
        pass

    def error(msg, line):
        raise ParserError(f"{msg} in '{line.strip()}'")

    last_keycode = 0
    for line in open(ns.header):
        try:
            if not in_evdev_codes_section:
                if re.match(start_token, line):
                    in_evdev_codes_section = True
                    had_evdev_codes_section = True
                    continue

                if re.match(reserved_range, line):
                    error("Using reserved range", line)
                match = re.match(name_pattern, line)
                if match:
                    all_defines.append(match.group(1))
            else:
                # Within the evdev defines section
                if re.match(end_token, line):
                    in_evdev_codes_section = False
                    continue

                # Comments we only search for a hex pattern and where there is one present
                # we only check for lower case format, ordering and update our last_keycode.
                if not re.match(define, line):
                    match = re.match(expected_comment_pattern, line)
                    if match:
                        hexcode = match.group("value")
                        if hexcode != hexcode.lower():
                            error(f"Hex code 0x{hexcode} must be lower case", line)
                        if hexcode:
                            keycode = int(hexcode, 16)
                            if keycode < last_keycode:
                                error("Keycode must be ascending", line)
                            if keycode == last_keycode:
                                error("Duplicate keycode", line)
                            last_keycode = keycode

                        name = match.group("name")
                        if name not in all_keysym_names:
                            error(f"Unknown keysym {name}", line)
                    elif re.match(hex_pattern, line):
                        logger.warning(f"Unexpected hex code in {line}")
                    continue

                # Anything below here is a #define line
                # Let's check for specific errors
                if re.match(normal_range, line):
                    error("Define must use _EVDEVK", line)

                match = re.match(name_pattern, line)
                if match:
                    if match.group(1) in all_defines:
                        error("Duplicate define", line)
                    all_defines.append(match.group(1))
                else:
                    error("Typo", line)

                match = re.match(hex_pattern, line)
                if not match:
                    error("No hex code", line)
                if match.group(1) != match.group(1).lower():
                    error(f"Hex code 0x{match.group(1)} must be lowercase", line)

                spaces = re.match(space_check, line)
                if not spaces:  # bug
                    error("Matching error", line)
                if "\t" in spaces.group(1) or "\t" in spaces.group(2):
                    error("Use spaces, not tabs", line)

                comment = re.match(comment_format, line)
                if not comment:
                    error("Invalid comment format", line)
                kver = comment.group(1)
                if kver and not re.match(kver_format, kver):
                    error("Invalid kernel version format", line)

                keyname = comment.group(2)
                if not keyname.startswith("KEY_") or keyname.upper() != keyname:
                    error("Kernel keycode name invalid", line)

                # This could be an old libevdev
                if keyname not in [c.name for c in libevdev.EV_KEY.codes]:
                    logger.warning(f"Unknown kernel keycode name {keyname}")

                # Check the full expected format, no better error messages
                # available if this fails
                match = re.match(expected_pattern, line)
                if not match:
                    error("Failed match", line)

                keycode = int(match.group(1), 16)
                if keycode < last_keycode:
                    error("Keycode must be ascending", line)
                if keycode == last_keycode:
                    error("Duplicate keycode", line)

                # May cause a false positive for old libevdev if KEY_MAX is bumped
                if keycode < 0x0A0 or keycode > libevdev.EV_KEY.KEY_MAX.value:
                    error("Keycode outside range", line)

                last_keycode = keycode
        except ParserError as e:
            logger.error(e)
            success = False

    if not had_evdev_codes_section:
        logger.error("Unable to locate EVDEVK section")
        success = False
    elif in_evdev_codes_section:
        logger.error("Unterminated EVDEVK section")
        success = False

    if success:
        logger.info("Verification succeeded")

    return 0 if success else 1


def add_keysyms(ns):
    """
    Print a new XF86keysym.h file, adding any *missing* keycodes to the existing file.
    """
    if verify(ns) != 0:
        die("Header file verification failed")

    # If verification succeeds, we can be a bit more lenient here because we already know
    # what the format of the field is. Specifically, we're searching for
    # 3-digit hexcode in brackets and use that as keycode.
    pattern = re.compile(r".*_EVDEVK\((0x[0-9A-Fa-f]{3})\).*")
    max_code = max(
        [
            c.value
            for c in libevdev.EV_KEY.codes
            if c.is_defined
            and c != libevdev.EV_KEY.KEY_MAX
            and not c.name.startswith("BTN")
        ]
    )

    def defined_keycodes(path):
        """
        Returns an iterator to the next #defined (or otherwise mentioned)
        keycode, all other lines (including the returned one) are passed
        through to printf.
        """
        with open(path) as fd:
            in_evdev_codes_section = False

            for line in fd:
                if not in_evdev_codes_section:
                    if re.match(start_token, line):
                        in_evdev_codes_section = True
                    # passthrough for all other lines
                    print(line, end="")
                else:
                    if re.match(r"#undef _EVDEVK\n", line):
                        in_evdev_codes_section = False
                        yield max_code
                    else:
                        match = re.match(pattern, line)
                        if match:
                            logger.debug(f"Found keycode in {line.strip()}")
                            yield int(match.group(1), 16)
                    print(line, end="")

    kernel = Kernel(ns.kernel_git_tree)
    prev_code = 255 - 8  # the last keycode we can map directly in X
    for code in defined_keycodes(ns.header):
        for missing in range(prev_code + 1, code):
            newline = generate_keysym_line(
                missing, kernel, kver_list=ns.kernel_versions
            )
            if newline:
                print(newline)
        prev_code = code

    return 0


def find_xf86keysym_header():
    """
    Search for the XF86keysym.h file in the current tree or use the system one
    as last resort. This is a convenience function for running the script
    locally, it should not be relied on in the CI.
    """
    paths = tuple(Path.cwd().glob("**/XF86keysym.h"))
    if not paths:
        fallbackdir = Path(os.getenv("INCLUDESDIR") or "/usr/include/")
        path = fallbackdir / "X11" / "XF86keysym.h"
        if not path.exists():
            die(f"Unable to find XF86keysym.h in CWD or {fallbackdir}")
    else:
        if len(paths) > 1:
            die("Multiple XF86keysym.h in CWD, please use --header")
        path = paths[0]

    logger.info(f"Using header file {path}")
    return path


def main():
    parser = argparse.ArgumentParser(description="Keysym parser script")
    parser.add_argument("--verbose", "-v", action="count", default=0)
    parser.add_argument(
        "--header",
        type=str,
        default=None,
        help="Path to the XF86Keysym.h header file (default: search $CWD)",
    )

    subparsers = parser.add_subparsers(help="command-specific help", dest="command")
    parser_verify = subparsers.add_parser(
        "verify", help="Verify the XF86keysym.h matches requirements (default)"
    )
    parser_verify.set_defaults(func=verify)

    parser_generate = subparsers.add_parser(
        "add-keysyms", help="Add missing keysyms to the existing ones"
    )
    parser_generate.add_argument(
        "--kernel-git-tree",
        type=str,
        default=None,
        required=True,
        help="Path to a kernel git repo, required to find git tags",
    )
    parser_generate.add_argument(
        "--kernel-versions",
        type=str,
        default=[],
        required=False,
        help="Comma-separated list of kernel versions to limit ourselves to (e.g. 'v5.10,v5.9'). Supports fnmatch.",
    )
    parser_generate.set_defaults(func=add_keysyms)
    ns = parser.parse_args()

    logger.setLevel(
        {2: logging.DEBUG, 1: logging.INFO, 0: logging.WARNING}.get(ns.verbose, 2)
    )

    if not ns.header:
        ns.header = find_xf86keysym_header()
    else:
        ns.header = Path(ns.header)

    if ns.command is None:
        print("No command specified, defaulting to verify'")
        ns.func = verify

    sys.exit(ns.func(ns))


if __name__ == "__main__":
    main()
