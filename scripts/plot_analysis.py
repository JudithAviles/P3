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

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <wav_file> [f0_file] [f0ref_file]")
        sys.exit(1)

    wav_path = sys.argv[1]
    rate, sig = read_wav(wav_path)
    base = os.path.splitext(wav_path)[0]

    f0_path = sys.argv[2] if len(sys.argv) > 2 else base + '.f0'
    f0ref_path = sys.argv[3] if len(sys.argv) > 3 else base + '.f0ref'

    f0 = read_f0(f0_path) if os.path.exists(f0_path) else None
    f0ref = read_f0(f0ref_path) if os.path.exists(f0ref_path) else None

    fig, axes = plt.subplots(2, 2, figsize=(14, 8))
    fig.suptitle(f'Pitch Analysis: {os.path.basename(wav_path)} ({rate} Hz)', fontsize=14)

    # 1. 30ms voiced segment + its autocorrelation
    frame_len = int(0.030 * rate)
    frame_shift = int(0.015 * rate)
    t_sig = np.arange(len(sig)) / rate

    if f0 is not None:
        voiced_idx = np.where(f0 > 0)[0]
        if len(voiced_idx) > 0:
            mid = voiced_idx[len(voiced_idx)//2]
            start = mid * frame_shift
            end = start + frame_len
            seg = sig[start:end]
            t_seg = np.arange(len(seg)) / rate
            r = autocorr(seg)

            ax1 = axes[0, 0]
            ax1.plot(t_seg * 1000, seg, 'b-', linewidth=1.5)
            ax1.set_xlabel('Time (ms)')
            ax1.set_ylabel('Amplitude')
            ax1.set_title(f'Voiced segment (~{start/rate:.2f}s), F0={f0[mid]:.1f} Hz')
            ax1.grid(True)

            ax2 = axes[0, 1]
            lags = np.arange(len(r))
            ax2.plot(lags, r, 'r-', linewidth=1.5)
            ax2.axvline(int(rate/f0[mid]), color='k', ls='--', alpha=0.5, label=f'T0={int(rate/f0[mid])} samples')
            ax2.set_xlabel('Lag (samples)')
            ax2.set_ylabel('Autocorrelation')
            ax2.set_title('Autocorrelation of voiced segment')
            ax2.legend()
            ax2.grid(True)

    # 2. F0 contour comparison: estimator vs reference
    ax3 = axes[1, 0]
    if f0 is not None:
        t_f0 = np.arange(len(f0)) * frame_shift / rate
        ax3.plot(t_f0, f0, 'b-', linewidth=1.5, label='Estimator')
    if f0ref is not None:
        t_ref = np.arange(len(f0ref)) * frame_shift / rate
        ax3.plot(t_ref[:len(f0ref)], f0ref, 'r--', linewidth=1.5, label='Reference')
    ax3.set_xlabel('Time (s)')
    ax3.set_ylabel('F0 (Hz)')
    ax3.set_title('F0 Contour')
    ax3.legend()
    ax3.grid(True)
    ax3.set_ylim(bottom=0)

    # 3. Waveform overview
    ax4 = axes[1, 1]
    ax4.plot(t_sig, sig, 'gray', linewidth=0.5)
    ax4.set_xlabel('Time (s)')
    ax4.set_ylabel('Amplitude')
    ax4.set_title('Waveform')
    ax4.grid(True)

    plt.tight_layout()
    out_path = os.path.splitext(wav_path)[0] + '_analysis.png'
    plt.savefig(out_path, dpi=150)
    print(f"Saved plot to {out_path}")
    plt.show()
