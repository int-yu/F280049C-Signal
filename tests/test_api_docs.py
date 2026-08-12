"""Contract checks for the Chinese API manual and its host-buildable examples."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "include" / "sigf32.h"
REFERENCE = ROOT / "docs" / "API\u53c2\u8003.md"
TUTORIAL = ROOT / "docs" / "API\u8c03\u7528\u6559\u7a0b.md"
MANIFEST = ROOT / "docs" / "api-symbols.txt"
EXAMPLES = (
    "preprocess_filter.c",
    "measure_spectrum.c",
    "tracking_adaptive.c",
    "platform_backends.c",
)


def public_symbols() -> list[str]:
    return sorted(set(re.findall(r"\b(sigf32_[a-z0-9_]+)\s*\(", HEADER.read_text(encoding="utf-8"))))


def test_all_public_functions_are_documented() -> None:
    """A missing public API section must be caught before a release."""
    assert MANIFEST.is_file(), "missing docs/api-symbols.txt"
    manifest = [line.strip() for line in MANIFEST.read_text(encoding="utf-8").splitlines() if line.strip()]
    assert manifest == public_symbols(), "manifest must exactly match public sigf32.h functions"
    reference = REFERENCE.read_text(encoding="utf-8")
    missing = [symbol for symbol in manifest if f"`{symbol}`" not in reference]
    assert not missing, f"undocumented public API: {', '.join(missing)}"


def test_api_examples_compile(tmp_path: Path) -> None:
    """Tutorial programs must compile with the same strict host flags used by CI."""
    assert TUTORIAL.is_file(), "missing docs/API璋冪敤鏁欑▼.md"
    for filename in EXAMPLES:
        source = ROOT / "examples" / "api_usage" / filename
        assert source.is_file(), f"missing example: {source.relative_to(ROOT)}"
        result = subprocess.run(
            ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-Iinclude", "-Ibackends/c2000",
             str(source), "src/sigf32.c", "backends/c2000/sigf32_tmu.c",
             "backends/c2000/sigf32_cla_adapter.c", "backends/c2000/sigf32_rfft_adapter.c",
             "-lm", "-o", str(tmp_path / filename.replace(".c", ""))],
            cwd=ROOT,
            text=True,
            capture_output=True,
        )
        assert result.returncode == 0, result.stderr


def test_api_manual_links_and_placeholders() -> None:
    """Navigation must reach the tutorial and not publish unfinished markers."""
    assert TUTORIAL.is_file(), "missing docs/API璋冪敤鏁欑▼.md"
    for markdown in (REFERENCE, TUTORIAL, ROOT / "README.md", ROOT / "docs" / "README.md"):
        text = markdown.read_text(encoding="utf-8")
        assert not re.search(r"(?mi)^\s*(?:TODO|TBD|FIXME)\b", text)
        for target in re.findall(r"\[[^]]+\]\(([^)#]+)(?:#[^)]+)?\)", text):
            if not target.startswith(("http://", "https://", "mailto:")):
                assert (markdown.parent / target).exists(), f"broken link in {markdown}: {target}"


if __name__ == "__main__":
    raise SystemExit(subprocess.call([sys.executable, "-m", "pytest", __file__, "-q"]))


