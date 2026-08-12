from pathlib import Path
import sys
import numpy as np

sys.path.insert(0, str(Path(__file__).parents[1] / "tools"))
from golden_model import rms, single_tone_amplitude, tone, window_metrics, zero_cross_frequency


def test_tone_rms_and_frequency() -> None:
    x = tone(4096.0, 4096, 64.0, amplitude=0.8)
    assert abs(rms(x) - 0.8 / np.sqrt(2.0)) < 1e-12
    assert abs(zero_cross_frequency(x, 4096.0) - 64.0) < 1e-10


def test_coherent_tone_amplitude() -> None:
    x = tone(4096.0, 1024, 64.0, amplitude=0.75)
    assert abs(single_tone_amplitude(x, 4096.0, 64.0) - 0.75) < 1e-12


def test_hann_metrics() -> None:
    coherent_gain, enbw = window_metrics("hann", 1024)
    assert 0.499 < coherent_gain < 0.5
    assert 1.50 < enbw < 1.51