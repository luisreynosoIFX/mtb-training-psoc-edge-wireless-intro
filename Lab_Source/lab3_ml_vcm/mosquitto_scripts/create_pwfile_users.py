#!/usr/bin/env python3
"""Create/update Mosquitto password entries for a username range.

Usage:
    python create_pwfile_users.py
    python create_pwfile_users.py --start 5 --end 12

Notes:
    - Default range is user1..user30, all with password "pwd".
    - A fixed user "explorer" with password "explorer_pwd" is always added.
    - Output file is pwfile.txt.
"""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def run_mosquitto_passwd(
    exe: str,
    pwfile: Path,
    username: str,
    password: str,
    create_file: bool = False,
) -> None:
    cmd = [exe]
    if create_file:
        cmd.append("-c")
    cmd.extend(["-b", str(pwfile), username, password])

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        stderr = result.stderr.strip() or "(no stderr)"
        raise RuntimeError(f"Command failed for {username}: {' '.join(cmd)}\n{stderr}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Add Mosquitto password entries for users like user1..user30.",
    )
    parser.add_argument(
        "--start",
        type=int,
        default=1,
        help="Start index for usernames (default: 1).",
    )
    parser.add_argument(
        "--end",
        type=int,
        default=30,
        help="End index for usernames, inclusive (default: 30).",
    )
    return parser.parse_args()


def main() -> int:
    # Example: --start 10 --end 20 creates user10..user20
    args = parse_args()

    if args.start < 1 or args.end < 1:
        print("Error: --start and --end must be >= 1.", file=sys.stderr)
        return 1
    if args.start > args.end:
        print("Error: --start must be less than or equal to --end.", file=sys.stderr)
        return 1

    exe = "mosquitto_passwd.exe"
    pwfile = Path("pwfile.txt")
    password = "pwd"

    try:
        for i in range(args.start, args.end + 1):
            username = f"user{i}"
            run_mosquitto_passwd(
                exe=exe,
                pwfile=pwfile,
                username=username,
                password=password,
                create_file=(i == args.start),
            )
            print(f"Added {username}")

        # Always add the fixed explorer account
        run_mosquitto_passwd(exe=exe, pwfile=pwfile, username="explorer", password="explorer_pwd")
        print("Added explorer")
    except FileNotFoundError:
        print(
            f"Error: '{exe}' was not found. Ensure it is in PATH or update the script.",
            file=sys.stderr,
        )
        return 1
    except RuntimeError as exc:
        print(str(exc), file=sys.stderr)
        return 1

    print(f"Done. Wrote users user{args.start}..user{args.end} and explorer to {pwfile}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
