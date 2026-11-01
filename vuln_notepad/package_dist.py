#!/usr/bin/env python3
"""Build the player attachment from an explicit allowlist (Python 3 only)."""

import gzip
import hashlib
import io
from pathlib import Path
import tarfile


ROOT = Path(__file__).resolve().parent
FILES = (
    ("build/vuln_notepad", "vuln_notepad", 0o755),
    ("build/libc.so.6", "libc.so.6", 0o755),
    ("build/ld-linux-x86-64.so.2", "ld-linux-x86-64.so.2", 0o755),
    ("distribution/run.sh", "run.sh", 0o755),
    ("distribution/README.md", "README.md", 0o644),
    ("distribution/Dockerfile", "Dockerfile", 0o644),
    ("distribution/compose.yaml", "compose.yaml", 0o644),
    ("distribution/.dockerignore", ".dockerignore", 0o644),
    ("distribution/flag.txt", "flag.txt", 0o644),
)


def main():
    missing = [source for source, _, _ in FILES if not (ROOT / source).is_file()]
    if missing:
        raise SystemExit(
            "missing build inputs:\n  " + "\n  ".join(missing)
            + "\nrun ./build_dist.sh (or make dist) first"
        )

    # Read all inputs before replacing an existing release.
    entries = [(name, mode, (ROOT / source).read_bytes())
               for source, name, mode in FILES]
    output = ROOT / "dist"
    output.mkdir(exist_ok=True)
    archive = output / "vuln_notepad.tar.gz"
    temporary = output / "vuln_notepad.tar.gz.tmp"
    try:
        with temporary.open("wb") as raw:
            with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as gz:
                with tarfile.open(fileobj=gz, mode="w", format=tarfile.USTAR_FORMAT) as tar:
                    for name, mode, data in entries:
                        info = tarfile.TarInfo(f"vuln_notepad/{name}")
                        info.size = len(data)
                        info.mode = mode
                        info.mtime = 0
                        tar.addfile(info, io.BytesIO(data))
        temporary.replace(archive)
    finally:
        temporary.unlink(missing_ok=True)
    digest = hashlib.sha256(archive.read_bytes()).hexdigest()
    (output / "SHA256SUMS").write_text(f"{digest}  {archive.name}\n", encoding="ascii")
    print(f"Created {archive}")


if __name__ == "__main__":
    main()
