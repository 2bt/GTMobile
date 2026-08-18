#!/usr/bin/env python3
"""Assemble original player.s with Exomizer and write golden binaries.

Requires:
  GOATTRACKER_DIR  GoatTracker src directory (contains player.s and asm/)
  EXOMIZER_ASM     path to the exomizer_asm helper
  SNG2SID_DATADIR  tools/sng2sid (default: script's ../../)
"""
import os
import subprocess
import sys
from pathlib import Path

FULL = {
    "base": 4096, "zpbase": 252, "SIDBASE": 54272,
    "SOUNDSUPPORT": 0, "VOLSUPPORT": 0, "BUFFEREDWRITES": 0,
    "GHOSTREGS": 0, "ZPGHOSTREGS": 0, "FIXEDPARAMS": 1, "SIMPLEPULSE": 1,
    "PULSEOPTIMIZATION": 1, "REALTIMEOPTIMIZATION": 1, "NOAUTHORINFO": 1,
    "NOEFFECTS": 0, "NOGATE": 0, "NOFILTER": 0, "NOFILTERMOD": 0,
    "NOPULSE": 0, "NOPULSEMOD": 0, "NOWAVEDELAY": 0, "NOWAVECMD": 0,
    "NOREPEAT": 0, "NOTRANS": 0, "NOPORTAMENTO": 0, "NOTONEPORTA": 0,
    "NOVIB": 0, "NOINSTRVIB": 0, "NOSETAD": 0, "NOSETSR": 0, "NOSETWAVE": 0,
    "NOSETWAVEPTR": 0, "NOSETPULSEPTR": 0, "NOSETFILTPTR": 0,
    "NOSETFILTCTRL": 0, "NOSETFILTCUTOFF": 0, "NOSETMASTERVOL": 0,
    "NOFUNKTEMPO": 0, "NOGLOBALTEMPO": 0, "NOCHANNELTEMPO": 0,
    "NOFIRSTWAVECMD": 0, "NOCALCULATEDSPEED": 0, "NONORMALSPEED": 0,
    "NOZEROSPEED": 0, "NUMCHANNELS": 3, "NUMSONGS": 1, "FIRSTNOTE": 0,
    "FIRSTNOHRINSTR": 2, "FIRSTLEGATOINSTR": 3, "NUMHRINSTR": 1,
    "NUMNOHRINSTR": 1, "NUMLEGATOINSTR": 1, "ADPARAM": 15, "SRPARAM": 0,
    "DEFAULTTEMPO": 5, "FIRSTWAVEPARAM": 9, "GATETIMERPARAM": 2,
}
MINIMAL = dict(FULL)
for k in list(MINIMAL):
    if k.startswith("NO") and k != "NOAUTHORINFO":
        MINIMAL[k] = 1
MINIMAL["NOAUTHORINFO"] = 1

STUBS = """
mt_freqtbllo: .BYTE (0)
mt_freqtblhi: .BYTE (0)
mt_songtbllo:
                .BYTE (mt_song0 % 256)
                .BYTE (mt_song1 % 256)
                .BYTE (mt_song2 % 256)
mt_songtblhi:
                .BYTE (mt_song0 / 256)
                .BYTE (mt_song1 / 256)
                .BYTE (mt_song2 / 256)
mt_patttbllo:
                .BYTE (mt_patt0 % 256)
mt_patttblhi:
                .BYTE (mt_patt0 / 256)
mt_insad: .BYTE (0)
mt_inssr: .BYTE (0)
mt_inswaveptr: .BYTE (0)
mt_inspulseptr: .BYTE (0)
mt_insfiltptr: .BYTE (0)
mt_insvibparam: .BYTE (0)
mt_insvibdelay: .BYTE (0)
mt_insgatetimer: .BYTE (0)
mt_insfirstwave: .BYTE (0)
mt_wavetbl: .BYTE (0)
mt_notetbl: .BYTE (0)
mt_pulsetimetbl: .BYTE (0)
mt_pulsespdtbl: .BYTE (0)
mt_filttimetbl: .BYTE (0)
mt_filtspdtbl: .BYTE (0)
mt_speedlefttbl: .BYTE (0)
mt_speedrighttbl: .BYTE (0)
mt_song0: .BYTE ($ff, 0)
mt_song1: .BYTE ($ff, 0)
mt_song2: .BYTE ($ff, 0)
mt_patt0: .BYTE (0)
"""


def with_defs(base, extra):
    d = dict(base)
    d.update(extra)
    return d


CASES = [
    ("full", FULL, False, False),
    ("minimal", MINIMAL, False, False),
    ("buffered", with_defs(FULL, {"BUFFEREDWRITES": 1}), False, False),
    ("volume", with_defs(FULL, {"VOLSUPPORT": 1}), False, False),
    ("sfx", with_defs(FULL, {"SOUNDSUPPORT": 1, "BUFFEREDWRITES": 1, "NUMCHANNELS": 3}), False, False),
    ("ghost_zp", with_defs(FULL, {"GHOSTREGS": 1, "ZPGHOSTREGS": 1, "BUFFEREDWRITES": 1, "zpbase": 2}), False, False),
    ("ghost_abs", with_defs(FULL, {"GHOSTREGS": 1, "ZPGHOSTREGS": 0, "BUFFEREDWRITES": 1}), False, True),
    ("author", with_defs(FULL, {"NOAUTHORINFO": 0}), False, False),
    ("ch1", with_defs(FULL, {"NUMCHANNELS": 1}), False, False),
    ("ch2", with_defs(FULL, {"NUMCHANNELS": 2}), False, False),
    ("unfixed", with_defs(FULL, {"FIXEDPARAMS": 0, "SIMPLEPULSE": 0}), False, False),
    ("alt_full", FULL, True, False),
    ("pulse_only", with_defs(MINIMAL, {"NOPULSE": 0, "NOPULSEMOD": 0, "NOSETPULSEPTR": 0}), False, False),
    ("filter_only", with_defs(MINIMAL, {"NOFILTER": 0, "NOFILTERMOD": 0, "NOSETFILTPTR": 0}), False, False),
    ("vib_noporta", with_defs(MINIMAL, {"NOVIB": 0, "NOINSTRVIB": 0, "NOEFFECTS": 0}), False, False),
    ("wavecmd", with_defs(MINIMAL, {"NOWAVECMD": 0, "NOEFFECTS": 0}), False, False),
    ("funktempo", with_defs(MINIMAL, {"NOFUNKTEMPO": 0, "NOGLOBALTEMPO": 0}), False, False),
    ("no_gate", with_defs(MINIMAL, {"NOGATE": 0}), False, False),
]


def define_text(defs):
    return "".join(f"{k:16s} = {v}\n" for k, v in defs.items())


def ghost_abs_rewrite(src: str) -> str:
    out = []
    i = 0
    while i < len(src):
        if src[i] == "<" and src[i + 1 : i + 6] == "ghost":
            out.append(" ")
            i += 1
        else:
            out.append(src[i])
            i += 1
    return "".join(out)


def assemble(name, defs, alt, ghost_abs):
    player = (GT / ("altplayer.s" if alt else "player.s")).read_text()
    if ghost_abs:
        player = ghost_abs_rewrite(player)
    src = define_text(defs) + player + STUBS
    r = subprocess.run([str(ASM)], input=src.encode(), capture_output=True)
    if r.returncode != 0:
        sys.stderr.write(r.stderr.decode())
        raise SystemExit(f"exomizer failed for {name}")
    path = OUT / f"{name}.bin"
    path.write_bytes(r.stdout)
    print(f"wrote {path} ({len(r.stdout)} bytes)")


def main():
    gt = Path(os.environ.get("GOATTRACKER_DIR", "/home/dlangner/Programming/c++/GoatTracker_2.76"))
    if (gt / "src" / "player.s").exists():
        gt = gt / "src"
    asm = Path(os.environ.get("EXOMIZER_ASM", str(Path(__file__).resolve().parents[4] / "build/exomizer_asm")))
    datadir = Path(os.environ.get("SNG2SID_DATADIR", str(Path(__file__).resolve().parents[2])))
    global GT, ASM, OUT
    GT = gt
    ASM = asm
    OUT = datadir / "test/player/golden"
    if not ASM.exists():
        raise SystemExit(f"exomizer_asm not found: {ASM}")
    OUT.mkdir(parents=True, exist_ok=True)
    for case in CASES:
        assemble(*case)


if __name__ == "__main__":
    main()
