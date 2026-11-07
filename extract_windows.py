import os
import re
import shutil
import subprocess

INPUT_DAT = "download0.dat"
EXTRACT_DIR = "extracted"
OUTPUT_DAT = "patched_download0.dat"

FLS = r"sleuthkit\bin\fls.exe"
ICAT = r"sleuthkit\bin\icat.exe"
ISTAT = r"sleuthkit\bin\istat.exe"

BLOCK_SIZE = 4096  # UFS Fragment Size per fsstat (was wrongly 65536 -> 16x offsets -> 2GB blob)

def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True, shell=False)

def extract():
    if not os.path.exists(INPUT_DAT):
        print("[!] download0.dat not found")
        return

    if os.path.exists(EXTRACT_DIR):
        shutil.rmtree(EXTRACT_DIR)

    os.makedirs(EXTRACT_DIR, exist_ok=True)

    print("[*] Reading UFS image...")

    result = run([FLS, "-rp", INPUT_DAT])

    if result.returncode != 0:
        print(result.stderr)
        return

    for line in result.stdout.splitlines():
        if "r/r" not in line:
            continue

        m = re.search(r"r/r\s+(\d+):\s+(.+)$", line)
        if not m:
            continue

        inode = m.group(1)
        rel_path = m.group(2).strip().lstrip("/")

        if rel_path.startswith("$OrphanFiles"):
            continue

        out_path = os.path.join(EXTRACT_DIR, *rel_path.split("/"))
        os.makedirs(os.path.dirname(out_path), exist_ok=True)

        print("[+] Extract:", rel_path)

        with open(out_path, "wb") as f:
            subprocess.run(
                [ICAT, INPUT_DAT, inode],
                stdout=f,
                stderr=subprocess.DEVNULL,
                shell=False
            )

    print("\n[✓] Extract done:", EXTRACT_DIR)

def get_inode(image, rel_path):
    result = run([FLS, "-rp", image])

    for line in result.stdout.splitlines():
        m = re.search(r"r/r\s+(\d+):\s+(.+)$", line)
        # exact full-path match (endswith matched binloader.js for "loader.js")
        if m and m.group(2).strip().rstrip("/") == rel_path:
            return m.group(1)

    return None

def get_size_and_blocks(image, inode):
    result = run([ISTAT, image, inode])

    size = None
    blocks = []

    for line in result.stdout.splitlines():
        if "Size:" in line or "size:" in line:
            nums = re.findall(r"\d+", line)
            if nums:
                size = int(nums[-1])

        if re.match(r"^\s*\d+(\s+\d+)*\s*$", line):
            blocks.extend([int(x) for x in line.split()])

    return size, blocks

def patch():
    if not os.path.exists(INPUT_DAT):
        print("[!] download0.dat not found")
        return

    if not os.path.exists(EXTRACT_DIR):
        print("[!] extracted folder not found")
        return

    if os.path.exists(OUTPUT_DAT):
        os.remove(OUTPUT_DAT)

    shutil.copy2(INPUT_DAT, OUTPUT_DAT)

    print("[*] Creating patched file:", OUTPUT_DAT)

    for root, dirs, files in os.walk(EXTRACT_DIR):
        for name in files:
            local_path = os.path.join(root, name)
            rel_path = os.path.relpath(local_path, EXTRACT_DIR).replace("\\", "/")

            inode = get_inode(OUTPUT_DAT, rel_path)

            if not inode:
                print("[!] Not found:", rel_path)
                continue

            old_size, blocks = get_size_and_blocks(OUTPUT_DAT, inode)
            new_size = os.path.getsize(local_path)

            print("\n[*] Patch:", rel_path)
            print("    Old size:", old_size)
            print("    New size:", new_size)

            if not old_size or not blocks:
                print("    [!] Cannot read inode info")
                continue

            if new_size > old_size:
                print("    [!] Skip: new file is bigger than original")
                continue

            with open(local_path, "rb") as f:
                data = f.read()

            # write fragment-by-fragment (files may be interleaved with others)
            # istat lists fragments in logical order; each fragment holds 4096
            # bytes of the file unless it is the last one
            if len(blocks) * BLOCK_SIZE < old_size:
                print("    [!] Cannot read inode info")
                continue

            # JS/JSON: pad with newlines (NUL breaks JS parsing); others: zeros
            pad = b"\n" if name.endswith((".js", ".json")) else b"\x00"
            pos = 0

            with open(OUTPUT_DAT, "r+b") as img:
                for frag in blocks:
                    if pos >= old_size:
                        break
                    n = min(BLOCK_SIZE, old_size - pos)
                    take = min(n, max(0, new_size - pos))
                    img.seek(frag * BLOCK_SIZE)
                    img.write(data[pos:pos + take])
                    img.write(pad * (n - take))
                    pos += n

            print("    [✓] OK")

    print("\n[✓] Done:", OUTPUT_DAT)

print("""
==============================
 Windows download0.dat TOOL
==============================
1 - Extract download0.dat
2 - Build / Patch download0.dat
3 - Exit
==============================
""")

choice = input("Choose: ").strip()

if choice == "1":
    extract()
elif choice == "2":
    patch()
else:
    print("Exit")