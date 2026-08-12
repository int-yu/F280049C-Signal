"""Independent NumPy/SciPy reference helpers for F280049C-Signal."""
from __future__ import annotations
import numpy as np


def tone(fs: float, count: int, frequency: float, amplitude: float = 1.0, phase: float = 0.0) -> np.ndarray:
    n = np.arange(count, dtype=float)
    return amplitude * np.sin(2.0 * np.pi * frequency * n / fs + phase)


def rms(x: np.ndarray) -> float:
    return float(np.sqrt(np.mean(np.square(np.asarray(x, dtype=float)))))


def zero_cross_frequency(x: np.ndarray, fs: float) -> float:
    x = np.asarray(x, dtype=float)
    hits = []
    for i in range(1, len(x)):
        if x[i - 1] <= 0.0 < x[i]:
            hits.append((i - 1) - x[i - 1] / (x[i] - x[i - 1]))
    if len(hits) < 2:
        raise ValueError("insufficient positive zero crossings")
    return float(fs / np.mean(np.diff(hits)))


def window_metrics(name: str, count: int) -> tuple[float, float]:
    n = np.arange(count, dtype=float)
    if name == "hann":
        w = 0.5 - 0.5 * np.cos(2 * np.pi * n / (count - 1))
    elif name == "rect":
        w = np.ones(count)
    else:
        raise ValueError(name)
    coherent_gain = np.mean(w)
    enbw = count * np.sum(w * w) / np.sum(w) ** 2
    return float(coherent_gain), float(enbw)


def single_tone_amplitude(x: np.ndarray, fs: float, frequency: float) -> float:
    n = np.arange(len(x), dtype=float)
    z = np.sum(x * np.exp(-2j * np.pi * frequency * n / fs))
    return float(2.0 * abs(z) / len(x))