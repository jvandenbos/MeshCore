#!/usr/bin/env python3
"""Write sim/gallery.html: every BLESSED frame beside the script lines that
produced it. Images are referenced out of golden/, so the gallery is readable
from a clean checkout without building anything; `make check` is what proves
golden/ still matches what the code renders today.

It lives beside golden/ rather than in out/ because the repo root .gitignore
excludes every directory named out/, and git cannot re-include a file under an
ignored directory."""

import html
import os
import re

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPTS = os.path.join(HERE, "scripts")
GOLDEN = os.path.join(HERE, "golden")

CSS = """
:root{--bg:#111;--card:#181818;--line:#2A2A2A;--fg:#CFCFCF;--acc:#35C8DC;--mut:#7E7E7E}
*{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--fg);font:13px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",Helvetica,Arial,sans-serif;padding:28px 16px 80px}
.page{max-width:1080px;margin:0 auto}
h1{font-size:20px;color:#fff;letter-spacing:.04em}
.sub{color:var(--mut);font-size:12px;margin-top:6px;max-width:70ch}
h2{font-size:13px;letter-spacing:.09em;text-transform:uppercase;color:var(--acc);margin:30px 0 4px}
.file{color:var(--mut);font-size:11px;font-family:ui-monospace,Menlo,monospace;margin-bottom:12px}
.frame{display:flex;gap:18px;align-items:flex-start;background:var(--card);border:1px solid var(--line);padding:14px;margin-bottom:14px}
.frame img{width:640px;height:480px;image-rendering:pixelated;background:#000;border:1px solid #333;flex:none}
.meta{min-width:0;flex:1}
.name{color:#fff;font-family:ui-monospace,Menlo,monospace;font-size:12px;margin-bottom:8px}
pre{background:#0C0C0C;border:1px solid var(--line);padding:10px;overflow-x:auto;
    font-family:ui-monospace,Menlo,monospace;font-size:11px;color:#B4B4B4;white-space:pre}
pre b{color:var(--acc);font-weight:600}
@media (max-width:1000px){.frame{flex-direction:column}.frame img{width:100%;height:auto}}
"""


def frames_for(path):
    """Yield (snap_name, [script lines up to and including that snap])."""
    lines = [l.rstrip("\n") for l in open(path)]
    pending = []
    for line in lines:
        pending.append(line)
        m = re.match(r"\s*snap\s+(\S+)\s*$", line)
        if m:
            yield m.group(1), pending
            pending = []


def main():
    parts = [
        "<!doctype html><meta charset='utf-8'>",
        "<title>MERLIN UI - simulator goldens</title>",
        f"<style>{CSS}</style>",
        "<div class='page'>",
        "<h1>MERLIN UI &mdash; simulator goldens</h1>",
        "<p class='sub'>Every frame is a real 320&times;240 RGB565 render from "
        "<code>ui-merlin/</code>, produced by replaying the input script shown "
        "beside it. Displayed at 2&times; with nearest-neighbour scaling, so what "
        "you see is the device pixel grid. These are the blessed frames in "
        "<code>sim/golden/</code>; <code>make check</code> re-renders them and "
        "compares byte-for-byte.</p>",
    ]

    total = 0
    for fname in sorted(os.listdir(SCRIPTS)):
        if not fname.endswith(".txt"):
            continue
        path = os.path.join(SCRIPTS, fname)
        title = fname[3:-4].replace("_", " ") if fname[:2].isdigit() else fname
        parts.append(f"<h2>{html.escape(title)}</h2>")
        parts.append(f"<div class='file'>sim/scripts/{html.escape(fname)}</div>")
        for name, chunk in frames_for(path):
            png = f"{name}.png"
            if not os.path.exists(os.path.join(GOLDEN, png)):
                continue
            total += 1
            body = "\n".join(
                f"<b>{html.escape(l)}</b>" if l.strip().startswith("snap ")
                else html.escape(l)
                for l in chunk
            )
            parts.append(
                "<div class='frame'>"
                f"<img src='golden/{png}' alt='{html.escape(name)}'>"
                "<div class='meta'>"
                f"<div class='name'>{html.escape(png)}</div>"
                f"<pre>{body}</pre>"
                "</div></div>"
            )

    parts.append("</div>")
    with open(os.path.join(HERE, "gallery.html"), "w") as fh:
        fh.write("\n".join(parts))
    print(f"   gallery: sim/gallery.html ({total} frames)")


if __name__ == "__main__":
    main()
