# F280049C Detailed API Usage Manual Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a complete Chinese API reference and task-oriented tutorial whose examples compile against the real F280049C library.

**Architecture:** `docs/API参考.md` remains the authoritative per-symbol reference; `docs/API调用教程.md` explains end-to-end data flows; focused programs in `examples/api_usage/` are the executable source behind documentation snippets. A Python documentation test compares public header symbols with an explicit coverage manifest and compiles every example.

**Tech Stack:** Markdown, C11, Python 3.11, pytest, GCC, TI C2000 Compiler 25.11.1.LTS, C2000Ware 26.01.00.00.

## Global Constraints

- Do not change `include/sigf32.h` ABI or algorithm behavior.
- Do not use dynamic allocation or hidden mutable global workspaces.
- Phase is per-unit `[0,1)` internally; rates are Hz and sample counts are elements unless explicitly documented as bytes.
- Do not invent package pin numbers or benchmark results.
- Every example must compile with `-Wall -Wextra -Werror`.

---

### Task 1: Documentation Contract Test

**Files:**
- Create: `tests/test_api_docs.py`
- Create: `docs/api-symbols.txt`
- Modify: `.github/workflows/host-tests.yml`

**Interfaces:**
- Consumes: declarations matching `sigf32_*(` in `include/sigf32.h`.
- Produces: a newline-separated symbol manifest and pytest checks for documentation coverage, placeholders, local Markdown links, and example compilation.

- [ ] Write `test_all_public_functions_are_documented` and `test_api_examples_compile` before adding the manifest/tutorial.
- [ ] Run `python -m pytest tests/test_api_docs.py -q`; expect failure because `docs/api-symbols.txt`, `docs/API调用教程.md`, and examples do not exist.
- [ ] Add the exact public symbol manifest generated from the header and make the test require every symbol in `docs/API参考.md`.
- [ ] Add the new pytest file to the existing CI invocation and run it to the next expected failure: missing examples/tutorial.
- [ ] Commit with `Test detailed F280 API documentation`.

### Task 2: Core API Reference

**Files:**
- Rewrite: `docs/API参考.md`
- Test: `tests/test_api_docs.py`

**Interfaces:**
- Consumes: all types, enums, structs, and declarations from `include/sigf32.h`.
- Produces: one searchable section per public function family with exact prototypes and lifecycle rules.

- [ ] Expand status codes, diagnostics, phase conversion, calibration, DC blocker, moving average, AGC, FIR, decimator, biquad, statistics, zero crossing, and correlation.
- [ ] For each family include prototype, parameter table, units/ranges, workspace ownership, result validity, reset behavior, ISR/block context, minimal snippet, and failure handling.
- [ ] Run the symbol coverage test; expect remaining frequency-domain/tracking symbols to fail.
- [ ] Document windows, Goertzel, spectrum metrics, NCO, IQ, PLL, FLL, LMS/NLMS, transfer function, TMU, CLA, and RFFT boundaries.
- [ ] Run `python -m pytest tests/test_api_docs.py -q`; expect symbol coverage to pass.
- [ ] Commit with `Document every F280 API`.

### Task 3: Compilable Task Tutorials

**Files:**
- Create: `docs/API调用教程.md`
- Create: `examples/api_usage/preprocess_filter.c`
- Create: `examples/api_usage/measure_spectrum.c`
- Create: `examples/api_usage/tracking_adaptive.c`
- Create: `examples/api_usage/platform_backends.c`

**Interfaces:**
- Each program includes `sigf32.h`, owns static state/workspace, and returns zero after checking statuses.
- Backend example includes adapter headers and keeps C2000-only paths behind the existing macros.

- [ ] Add compilation expectations to `test_api_examples_compile`; run and observe missing-file failure.
- [ ] Implement preprocessing/filter example showing ADC-voltage calibration, DC block, FIR block processing, SOS IIR, decimation, reset, and diagnostic handling.
- [ ] Implement measurement example showing RMS/frequency, Goertzel, spectrum workspace sizing, THD/SNR checks, and sub-sample correlation.
- [ ] Implement tracking example showing phase-continuous NCO, IQ, FLL-seeded PLL, lock checks, and NLMS error output.
- [ ] Implement backend example showing portable/TMU sine-cosine and documented C2000Ware RFFT workspace layout.
- [ ] Write the tutorial around `ADC/DMA -> preprocessing -> block/stream algorithm -> result/display/DAC`, linking each complete program.
- [ ] Run GCC example compilation and all existing tests; expect all pass.
- [ ] Commit with `Add F280 API usage tutorials`.

### Task 4: Target and Documentation Verification

**Files:**
- Modify: `README.md`
- Modify: `docs/README.md`
- Modify: `.github/workflows/host-tests.yml`

**Interfaces:**
- Produces direct navigation to reference/tutorial and CI enforcement of example compilation and documentation coverage.

- [ ] Add prominent quick links and a “recommended reading order” to both indexes.
- [ ] Compile portable examples with GCC and target-compatible sources with TI C2000 Compiler using `--c99 --fp_mode=relaxed --float_support=fpu32 --tmu_support=tmu0`.
- [ ] Run `python -m pytest -q`, all C regressions, Markdown/UTF-8/link/placeholder checks, and `git diff --check`.
- [ ] Push `main`, wait for GitHub Actions, and report the run URL.
- [ ] Commit with `Complete F280 API manual` before pushing.
