# PAV P3 - Handover

## Project State
C++11 autocorrelation-based F0 estimator for speech signals. Build: Meson+Ninja (`make release` / `make debug`). Current score: **90.18%** on `pitch_db/train/`.

## Architecture
- `src/get_pitch/get_pitch.cpp` — CLI entry, frame iteration, I/O, preprocessing/postprocessing
- `src/get_pitch/pitch_analyzer.h/.cpp` — Core class: autocorrelation, windowing, unvoiced decision  
- `src/pav/` — Support lib (WAV I/O, `digital_filter.h/.cpp` for LPF)

## What Changed (from baseline 89.99%)

### Preprocessing — LPF + Decimation (score: 90.18%)
In `get_pitch.cpp` after normalization, before framing:
- 2nd-order Butterworth LPF (coeffs: `b=[0.2929,0.5858,0.2929]`, `a=[1,0,0.1716]`) at ~5 kHz cutoff
- Decimate by 2: **20 kHz → 10 kHz** (all 50 training files are 20 kHz, NOT 16 kHz)
- Reduces frame samples from 600→300 (30ms), shift 300→150 (15ms)
- Helps because fewer autocorrelation lags → cleaner peak search, and decimation attenuates noise above 5 kHz

### Debug Feature — toggled ON
In `pitch_analyzer.cpp:135`: `#if 1` — prints `pot\tr1/r0\tr[lag]/r[0]` per frame to stdout. Swap to `#if 0` for production.

### New Scripts (installed to `$prefix/bin/`)
| Script | Purpose |
|---|---|
| `scripts/plot_analysis.py` | Generates 4-panel plot (segment, autocorrelation, F0 contour vs ref, waveform) |
| `scripts/grid_search.sh` | Sweeps pot/r1/rmax thresholds for unvoiced gate tuning |

## Files Changed
- `src/get_pitch/get_pitch.cpp` — added LPF+decimation, `#include "digital_filter.h"`, `#include <algorithm>`
- `src/get_pitch/pitch_analyzer.cpp` — `#if 0` → `#if 1` (debug print)
- `scripts/plot_analysis.py` — new file
- `scripts/grid_search.sh` — new file
- `scripts/meson.build` — added new scripts to install list

## Key Observations
- **LPF+decimation alone** gives 90.18% (from 89.99%). Gross errors 76→77 (neutral), MSE 2.05%→2.13% (slight increase), but V/UV improves (unvoiced→voiced: 309→290, voiced→unvoiced: 490→481).
- **Median filter postprocessing** tested with windows 3 and 5, conditional and unconditional — all versions hurt the score (89.15–89.64%). Removing it outperforms. May need a more sophisticated approach (e.g., Viterbi smoothing, dynamic programming).
- **Center clipping** at 30% threshold destroyed the score (55%) — too aggressive for these signals.

## TODO / Ideas for Further Improvement

### 1. Better Unvoiced Gate Thresholds
The grid search script can sweep parameters, but currently needs code changes to modify thresholds (they're hardcoded in the `PitchAnalyzer` constructor defaults). Consider:
- Exposing `--pot-th`, `--r1-th`, `--rmax-th` as CLI flags in docopt
- Running `scripts/grid_search.sh` to find optimal values
- The `set_unvoiced_thresholds()` method already exists on the analyzer

### 2. Postprocessing — Dynamic Programming / Viterbi
Replace the median filter with a smoother that respects the F0 continuity constraint:
- Cost = |f0[t] - f0[t-1]| + penalty for V/UV transitions
- Can fix octave jumps without smoothing legitimate pitch variation

### 3. Alternative Preprocessing
- **Center clipping** with frame-adaptive threshold (e.g., 30% of each frame's max, not global max) could work better
- **Spectral subtraction** or **cepstral liftering** before autocorrelation

### 4. CLI Argument Restoration
The handoff originally had thresholds as CLI args. Consider restoring them for easy grid searching:
```
--pot-th=<dB>    Power threshold [default: -40]
--r1-th=<f>      r1/r0 threshold [default: 0.30]  
--rmax-th=<f>    rmax/r0 threshold [default: 0.40]
```

### 5. Test on Held-Out Data
Current eval runs on `pitch_db/train/`. A proper test set or cross-validation would validate generalization.

## Commands
```bash
make release              # Build and install
bash scripts/run_get_pitch.sh   # Run full evaluation
python3 scripts/plot_analysis.py pitch_db/train/sb050.wav  # Generate plots
bash scripts/grid_search.sh     # (needs CLI threshold flags first)
```
