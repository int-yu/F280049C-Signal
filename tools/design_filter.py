"""Generate a reproducible SOS coefficient report.

Example:
  python tools/design_filter.py --fs 200000 --cutoff 20000 --order 4 --output docs/lowpass_report.md
"""
from __future__ import annotations
import argparse
from pathlib import Path
import numpy as np
from scipy import signal


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--fs", type=float, required=True)
    parser.add_argument("--cutoff", type=float, required=True)
    parser.add_argument("--order", type=int, default=4)
    parser.add_argument("--kind", choices=("lowpass", "highpass"), default="lowpass")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    if not 0.0 < args.cutoff < args.fs / 2.0:
        raise SystemExit("cutoff must be within (0, fs/2)")
    sos = signal.butter(args.order, args.cutoff, btype=args.kind, fs=args.fs, output="sos")
    poles = np.concatenate([np.roots(row[3:]) for row in sos])
    freqs = np.linspace(0.0, args.fs / 2.0, 8193)
    _, h = signal.sosfreqz(sos, worN=freqs, fs=args.fs)
    report = ["# SOS filter report", "", f"- kind: {args.kind}", f"- fs: {args.fs:g} Hz", f"- cutoff: {args.cutoff:g} Hz", f"- order: {args.order}", f"- stable: {bool(np.all(np.abs(poles) < 1.0))}", f"- max pole radius: {np.max(np.abs(poles)):.9f}", f"- peak gain: {np.max(np.abs(h)):.9f}", "", "## sigf32_biquad_coeffs_t", "", "```c", "static const sigf32_biquad_coeffs_t coeffs[] = {"]
    for b0, b1, b2, a0, a1, a2 in sos:
        report.append(f"    {{{b0/a0:.10gf}, {b1/a0:.10gf}, {b2/a0:.10gf}, {a1/a0:.10gf}, {a2/a0:.10gf}}},")
    report += ["};", "```", "", "系数是 DF2T 格式；重新设计或量化后必须复测稳定性、峰值内部增益和通带增益。"]
    args.output.write_text("\n".join(report) + "\n", encoding="utf-8")

if __name__ == "__main__":
    main()