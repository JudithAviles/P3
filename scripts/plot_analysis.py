#!/usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
import struct
import sys
import os

def read_wav(path):
    with open(path, 'rb') as f:
        riff = f.read(12)
        f.seek(22)
        channels = struct.unpack('<H', f.read(2))[0]
        f.seek(24)
        rate = struct.unpack('<I', f.read(4))[0]
        f.seek(36)
        bps = struct.unpack('<H', f.read(2))[0]
        f.seek(40)
        data = f.read()
        if bps == 16:
            samples = np.frombuffer(data, dtype='<i2').astype(np.float32)
        elif bps == 32:
            samples = np.frombuffer(data, dtype='<i4').astype(np.float32)
        else:
            samples = np.frombuffer(data, dtype='<i1').astype(np.float32)
        if channels > 1:
            samples = samples.reshape(-1, channels)[:, 0]
    return rate, samples / max(1e-10, np.max(np.abs(samples)))

def read_f0(path):
    return np.loadtxt(path)

def autocorr(x):
    n = len(x)
    r = np.zeros(n)
    for l in range(n):
        r[l] = np.sum(x[l:] * x[:n-l]) / n
    return r

def compute_frame_features(sig, rate, frame_len, frame_shift, min_f0=20, max_f0=500):
    n_len = int(frame_len * rate)
    n_shift = int(frame_shift * rate)
    npitch_min = int(rate / max_f0)
    npitch_max = int(rate / min_f0) + 1
    npitch_max = min(npitch_max, n_len // 2)

    window = 0.54 - 0.46 * np.cos(2 * np.pi * np.arange(n_len) / (n_len - 1))

    pots = []
    r1norms = []
    rmaxnorms = []
    f0s = []

    for start in range(0, len(sig) - n_len, n_shift):
        frame = sig[start:start + n_len] * window

        r = autocorr(frame)

        pot = 10 * np.log10(r[0] + 1e-10)
        pots.append(pot)

        r1norm = r[1] / r[0] if r[0] != 0 else 0
        r1norms.append(r1norm)

        lag_start = npitch_min
        lag_end = min(npitch_max, len(r) - 1)
        if lag_start < len(r):
            rmax = np.max(r[lag_start:lag_end])
            rmaxnorm = rmax / r[0] if r[0] != 0 else 0
        else:
            rmaxnorm = 0
        rmaxnorms.append(rmaxnorm)

        lag = lag_start + np.argmax(r[lag_start:lag_end]) if lag_start < len(r) else 0
        if pot > -42 and r1norm > 0.47 and rmaxnorm > 0.33:
            f0s.append(rate / lag if lag > 0 else 0)
        else:
            f0s.append(0)

    return np.array(pots), np.array(r1norms), np.array(rmaxnorms), np.array(f0s)

def plot_wavesurfer_params(wav_path, f0_path=None, f0ref_path=None, output_path=None):
    rate, sig = read_wav(wav_path)
    base = os.path.splitext(wav_path)[0]
    frame_len = 0.030
    frame_shift = 0.015

    pot, r1norm, rmaxnorm, f0_est = compute_frame_features(sig, rate, frame_len, frame_shift)

    f0ref = None
    if f0ref_path and os.path.exists(f0ref_path):
        f0ref_data = np.loadtxt(f0ref_path)
        if len(f0ref_data) > len(pot):
            f0ref = f0ref_data[1:-1][:len(pot)]
        else:
            f0ref = f0ref_data

    f0 = f0_est
    n_frames = len(pot)
    t = np.arange(n_frames) * frame_shift

    fig, axes = plt.subplots(4, 1, figsize=(12, 10), sharex=True)
    fig.suptitle(f'Wavesurfer Parameters: {os.path.basename(wav_path)}', fontsize=14)

    axes[0].plot(t, f0, 'b-', linewidth=1)
    if f0ref is not None:
        axes[0].plot(t[:len(f0ref)], f0ref, 'r--', linewidth=1, alpha=0.7, label='Reference')
    axes[0].set_ylabel('F0 (Hz)')
    axes[0].set_title('Pitch Contour')
    axes[0].legend(['Estimated', 'Reference'] if f0ref is not None else ['Estimated'])
    axes[0].grid(True)
    axes[0].set_ylim(bottom=0)

    axes[1].plot(t, pot, 'g-', linewidth=1)
    axes[1].axhline(y=-42, color='k', ls='--', alpha=0.5, label='threshold=-42dB')
    axes[1].set_ylabel('Power (dB)')
    axes[1].set_title('Signal Power (r[0])')
    axes[1].legend()
    axes[1].grid(True)

    axes[2].plot(t, r1norm, 'm-', linewidth=1)
    axes[2].axhline(y=0.47, color='k', ls='--', alpha=0.5, label='threshold=0.47')
    axes[2].set_ylabel('r1/r0')
    axes[2].set_title('Normalized Autocorrelation at Lag 1 (r1norm)')
    axes[2].legend()
    axes[2].grid(True)

    axes[3].plot(t, rmaxnorm, 'c-', linewidth=1)
    axes[3].axhline(y=0.33, color='k', ls='--', alpha=0.5, label='threshold=0.33')
    axes[3].set_ylabel('rmax/r0')
    axes[3].set_xlabel('Time (s)')
    axes[3].set_title('Normalized Autocorrelation at Pitch Lag (rmaxnorm)')
    axes[3].legend()
    axes[3].grid(True)

    plt.tight_layout()

    if output_path:
        plt.savefig(output_path, dpi=150)
        print(f"Saved wavesurfer params to {output_path}")
    else:
        out_path = base + '_wavesurfer.png'
        plt.savefig(out_path, dpi=150)
        print(f"Saved wavesurfer params to {out_path}")
    plt.close()

def plot_autocorr_segment(wav_path, f0_path=None, output_path=None):
    rate, sig = read_wav(wav_path)
    base = os.path.splitext(wav_path)[0]

    frame_len = 0.030
    frame_shift = 0.015
    n_len = int(frame_len * rate)
    n_shift = int(frame_shift * rate)

    f0 = read_f0(f0_path) if f0_path and os.path.exists(f0_path) else None

    voiced_idx = np.where(f0 > 0)[0] if f0 is not None else []
    if len(voiced_idx) == 0:
        print("No voiced frames found")
        return

    mid = voiced_idx[len(voiced_idx) // 2]
    start = mid * n_shift
    end = start + n_len
    seg = sig[start:end]

    window = 0.54 - 0.46 * np.cos(2 * np.pi * np.arange(len(seg)) / (len(seg) - 1))
    seg_windowed = seg * window
    r = autocorr(seg_windowed)

    t_seg = np.arange(len(seg)) / rate
    lag_max = int(rate / f0[mid]) if f0 is not None and f0[mid] > 0 else 0

    fig, axes = plt.subplots(1, 2, figsize=(12, 4))

    axes[0].plot(t_seg * 1000, seg, 'b-', linewidth=1.5)
    axes[0].set_xlabel('Time (ms)')
    axes[0].set_ylabel('Amplitude')
    axes[0].set_title(f'Voiced Segment (~{start/rate:.3f}s), F0={f0[mid]:.1f} Hz' if f0 is not None else 'Voiced Segment')
    axes[0].grid(True)

    lags = np.arange(len(r))
    axes[1].plot(lags, r, 'r-', linewidth=1.5)
    if lag_max > 0:
        axes[1].axvline(lag_max, color='k', ls='--', alpha=0.7, label=f'T0={lag_max} samples')
    axes[1].set_xlabel('Lag (samples)')
    axes[1].set_ylabel('Autocorrelation')
    axes[1].set_title('Autocorrelation of Voiced Segment')
    axes[1].legend()
    axes[1].grid(True)

    plt.tight_layout()

    if output_path:
        plt.savefig(output_path, dpi=150)
        print(f"Saved autocorr plot to {output_path}")
    else:
        out_path = base + '_autocorr.png'
        plt.savefig(out_path, dpi=150)
        print(f"Saved autocorr plot to {out_path}")
    plt.close()

def plot_comparison(wav_path, f0_path=None, f0ref_path=None, output_path=None):
    rate, sig = read_wav(wav_path)
    base = os.path.splitext(wav_path)[0]
    frame_shift = 0.015
    frame_len = 0.030

    pot, r1norm, rmaxnorm, f0_est = compute_frame_features(sig, rate, frame_len, frame_shift)

    f0 = None
    if f0_path and os.path.exists(f0_path):
        f0_data = np.loadtxt(f0_path)
        if len(f0_data) > 2:
            f0 = f0_data[1:-1][:len(pot)]
        else:
            f0 = f0_data
    else:
        f0 = f0_est

    f0ref = None
    if f0ref_path and os.path.exists(f0ref_path):
        f0ref_data = np.loadtxt(f0ref_path)
        if len(f0ref_data) > len(pot):
            f0ref = f0ref_data[1:-1][:len(pot)]
        else:
            f0ref = f0ref_data

    n_frames = len(pot)
    t_sig = np.arange(len(sig)) / rate
    t_f0 = np.arange(n_frames) * frame_shift

    fig, axes = plt.subplots(2, 1, figsize=(12, 6), sharex=True)

    axes[0].plot(t_sig, sig, 'gray', linewidth=0.5)
    axes[0].set_ylabel('Amplitude')
    axes[0].set_title('Waveform')
    axes[0].grid(True)

    if f0 is not None:
        axes[1].plot(t_f0[:len(f0)], f0, 'b-', linewidth=1.5, label='Our Estimator')
    if f0ref is not None:
        axes[1].plot(t_f0[:len(f0ref)], f0ref, 'r--', linewidth=1.5, label='Wavesurfer Reference')
    axes[1].set_xlabel('Time (s)')
    axes[1].set_ylabel('F0 (Hz)')
    axes[1].set_title('F0 Contour Comparison')
    axes[1].legend()
    axes[1].grid(True)
    axes[1].set_ylim(bottom=0)

    plt.tight_layout()

    if output_path:
        plt.savefig(output_path, dpi=150)
        print(f"Saved comparison plot to {output_path}")
    else:
        out_path = base + '_comparison.png'
        plt.savefig(out_path, dpi=150)
        print(f"Saved comparison plot to {out_path}")
    plt.close()

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"""Usage: {sys.argv[0]} <wav_file> [options]

Options:
  --wavesurfer        Generate wavesurfer parameters plot (pot, r1norm, rmaxnorm)
  --autocorr         Generate autocorrelation segment plot
  --compare          Generate comparison plot (estimator vs reference)
  --all              Generate all plots (default)
  --output <path>    Output file path

Examples:
  {sys.argv[0]} prueba.wav --wavesurfer
  {sys.argv[0]} prueba.wav --autocorr
  {sys.argv[0]} prueba.wav prueba.f0 prueba.f0ref --all
""")
        sys.exit(1)

    wav_path = sys.argv[1]
    f0_path = None
    f0ref_path = None
    plots = ['all']
    output = None

    i = 2
    while i < len(sys.argv):
        if sys.argv[i] == '--wavesurfer':
            plots = ['wavesurfer']
        elif sys.argv[i] == '--autocorr':
            plots = ['autocorr']
        elif sys.argv[i] == '--compare':
            plots = ['compare']
        elif sys.argv[i] == '--all':
            plots = ['all']
        elif sys.argv[i] == '--output' and i + 1 < len(sys.argv):
            output = sys.argv[i + 1]
            i += 1
        elif not sys.argv[i].startswith('--') and f0_path is None:
            f0_path = sys.argv[i]
        elif not sys.argv[i].startswith('--') and f0ref_path is None:
            f0ref_path = sys.argv[i]
        i += 1

    if 'all' in plots:
        plot_wavesurfer_params(wav_path, f0_path, f0ref_path)
        plot_autocorr_segment(wav_path, f0_path)
        plot_comparison(wav_path, f0_path, f0ref_path)
    else:
        if 'wavesurfer' in plots:
            plot_wavesurfer_params(wav_path, f0_path, f0ref_path, output)
        if 'autocorr' in plots:
            plot_autocorr_segment(wav_path, f0_path, output)
        if 'compare' in plots:
            plot_comparison(wav_path, f0_path, f0ref_path, output)