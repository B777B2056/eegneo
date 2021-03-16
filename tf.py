# This Python file uses the following encoding: utf-8

import mne
import numpy as np
import matplotlib.pyplot as plt

error_flag = 0

def plotDWT(path, freq_min, freq_max, channel):
    global error_flag
    tmin = - (0.2 / 0.7) * 5 / freq_min
    tmax = (0.5 / 0.7) * 5 / freq_min
    raw = mne.io.read_raw_edf(path)
    c = mne.find_events(raw, stim_channel='Event')
    epoch = mne.Epochs(raw, c, tmin=tmin, tmax=tmax)
    freqs = np.arange(freq_min, freq_max)
    try:
        power = mne.time_frequency.tfr_morlet(epoch, freqs=freqs, n_cycles=2, return_itc=False)
        power.plot([channel])
        plt.show()
    except ValueError:
        error_flag = 1
    return error_flag
