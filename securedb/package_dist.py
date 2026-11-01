#!/usr/bin/env python3
"""Build the player attachment from an explicit allowlist (Python 3 only)."""

import gzip
import hashlib
import io
from pathlib import Path
import tarfile


ROOT = Path(__file__).resolve().parent
FILES = (
    ("securedb", "securedb", 0o755),
    ("libc.so.6", "libc.so.6", 0o755),
    ("ld-2.31.so", "ld-2.31.so", 0o755),
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
            + "\nrun ./setup_glibc231.sh and ./build_securedb_glibc231.sh first"
        )

    # Read all inputs before replacing an existing release.
    entries = [(name, mode, (ROOT / source).read_bytes())
               for source, name, mode in FILES]
    output = ROOT / "dist"
    output.mkdir(exist_ok=True)
    archive = output / "securedb.tar.gz"
    temporary = output / "securedb.tar.gz.tmp"
    try:
        with temporary.open("wb") as raw:
            with gzip.GzipFile(filename="", mode="wb", fileobj=raw, mtime=0) as gz:
                with tarfile.open(fileobj=gz, mode="w", format=tarfile.USTAR_FORMAT) as tar:
                    for name, mode, data in entries:
                        info = tarfile.TarInfo(f"securedb/{name}")
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
