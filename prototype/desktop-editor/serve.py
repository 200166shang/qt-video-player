#!/usr/bin/env python3
"""Serve the throwaway PlayerLab desktop editor prototype."""

from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
import os


PORT = 8121
ROOT = Path(__file__).resolve().parent


def main() -> None:
    os.chdir(ROOT)
    server = ThreadingHTTPServer(("127.0.0.1", PORT), SimpleHTTPRequestHandler)
    print(f"PlayerLab prototype: http://127.0.0.1:{PORT}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()

