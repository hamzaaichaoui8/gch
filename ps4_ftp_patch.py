#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PS4 FTP - download0.dat backup + replace
Connects to PS4 FTP, finds <CUSA>/download0.dat, backs it up (remote rename
+ optional local copy), then uploads patched_download0.dat renamed properly.
"""
import argparse
import ftplib
import os
import socket
import sys
from datetime import datetime

CANDIDATE_DIRS = [
    "/user/download", "/data/download", "/mnt/sandbox/download",
    "/mnt/sandbox/data/download", "/user/app", "/data/app",
]

def connect(host, ports, timeout=15):
    last_err = None
    for port in ports:
        for user, pw in [("", ""), ("anonymous", "anonymous"), ("anonymous", "")]:
            try:
                ftp = ftplib.FTP()
                ftp.connect(host, port, timeout=timeout)
                if user:
                    ftp.login(user, pw)
                try:
                    ftp.set_pasv(True)
                except Exception:
                    pass
                print(f"[OK] Connected {host}:{port} (user '{user or 'anonymous'}')")
                print(f"     Server banner: {ftp.getwelcome().strip()}")
                return ftp
            except (OSError, ftplib.error_perm, ftplib.error_reply) as e:
                last_err = f"port {port} user {user}: {e}"
    print(f"[FAIL] Cannot connect ({last_err})")
    return None

def remote_size(ftp, path):
    try:
        s = ftp.size(path)
        return int(s) if s is not None else None
    except Exception:
        return None

def list_dir(ftp, path):
    try:
        return ftp.nlst(path or ".")
    except Exception:
        return []

def find_target(ftp, cusa, fname):
    # 1) direct candidate dirs
    for base in CANDIDATE_DIRS:
        p = f"{base}/{cusa}/{fname}"
        if remote_size(ftp, p):
            print(f"[OK] Found via candidates: {p}")
            return p
    # 2) BFS for directory named <cusa> containing fname
    print("[*] Searching FTP tree for", cusa, "(this may take a bit)...")
    root = ftp.pwd()
    seen = 0
    queue = [root if root != "/" else ""]
    while queue and seen < 6000:
        d = queue.pop(0)
        for entry in list_dir(ftp, d or "."):
            name = entry.rstrip("/").rsplit("/", 1)[-1]
            full = f"{d}/{name}".replace("//", "/")
            seen += 1
            if name == cusa:
                p = f"{full}/{fname}"
                if remote_size(ftp, p):
                    print(f"[OK] Found by search: {p}")
                    return p
                if remote_size(ftp, p) is None and any(full + "/" + x for x in []):
                    pass
            # try to descend only into local-looking dirs
            if not name.startswith(".") and seen < 6000:
                probe = full
                try:
                    ftp.cwd(probe)
                    ftp.cwd("/" if root == "/" else root)
                    queue.append(full)
                except Exception:
                    pass
    return None

def download_backup(ftp, path, local_out):
    size = remote_size(ftp, path) or 0
    got = [0]
    def cb(block):
        got[0] += len(block)
        if got[0] % (4 * 1024 * 1024) < 4096 * 50:
            sys.stdout.write(f"\r    local backup {got[0]}/{size} bytes")
            sys.stdout.flush()
    with open(local_out, "wb") as f:
        ftp.retrbinary(f"RETR {path}", cb, 1024 * 128)
    print(f"\n[OK] Local backup saved: {local_out}")

def upload(ftp, local_path, path):
    size = os.path.getsize(local_path)
    got = [0]
    def cb(block):
        got[0] += len(block)
        sys.stdout.write(f"\r    upload {got[0] / (1024*1024):.1f}/{size / (1024*1024):.1f} MB")
        sys.stdout.flush()
    with open(local_path, "rb") as f:
        ftp.storbinary(f"STOR {path}", f, 1024 * 128)
    print()
    rs = remote_size(ftp, path)
    if rs != size:
        print(f"[FAIL] Upload size mismatch: remote {rs} != local {size}")
        return False
    print(f"[OK] Uploaded + verified: {path} ({rs} bytes)")
    return True

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--host", default="192.168.1.7")
    ap.add_argument("--ports", default="2121,21")
    ap.add_argument("--cusa", default="CUSA00960")
    ap.add_argument("--remote-file", default="download0.dat")
    ap.add_argument("--local", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "patched_download0.dat"))
    ap.add_argument("--local-backup-dir", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "backup"))
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--no-local-backup", action="store_true", help="skip 262MB download, keep only remote .bak rename")
    args = ap.parse_args()

    if not os.path.exists(args.local):
        print(f"[FAIL] Local patched file not found: {args.local}")
        return 2
    local_size = os.path.getsize(args.local)
    print(f"[*] Local patch file: {args.local} ({local_size} bytes)")

    if args.dry_run:
        print("[DRY RUN] no changes will be made")

    ftp = connect(args.host, [int(p) for p in args.ports.split(",")])
    if not ftp:
        return 2

    try:
        target = find_target(ftp, args.cusa, args.remote_file)
        if not target:
            print("[FAIL] download0.dat not found on console")
            return 2
        orig = remote_size(ftp, target)
        print(f"[*] Target: {target} ({orig} bytes)")

        if args.dry_run:
            print("[DRY RUN] plan: rename backup -> upload -> verify. Exiting.")
            return 0

        # 1) remote backup = instant rename on console FS
        stamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        bak = f"{target}.bak_{stamp}"
        try:
            ftp.rename(target, bak)
            print(f"[OK] Console backup (rename): {bak}")
        except Exception as e:
            print(f"[WARN] rename failed ({e}) chip; falling back to slow local backup only")
            bak = None

        # 2) optional true local backup copy of ORIGINAL from console
        if not args.no_local_backup:
            os.makedirs(args.local_backup_dir, exist_ok=True)
            src = bak if bak else target
            download_backup(ftp, src, os.path.join(args.local_backup_dir, f"download0.dat.orig_{stamp}.dat"))

        # 3) restore name for the new file then upload
        if bak:
            ftp.rename(bak, target)
        elif not args.no_local_backup:
            pass  # target free only if renamed; otherwise must clear old file
        else:
            print("[FAIL] cannot free original slot")
            return 2

        if not upload(ftp, args.local, target):
            if bak:
                print("[*] Restoring original from backup...")
                ftp.rename(target, target + ".bad")
                return 3
        print("\n[DONE] Console updated successfully.")
        return 0
    finally:
        try:
            ftp.quit()
        except Exception:
            ftp.close()

if __name__ == "__main__":
    sys.exit(main())
