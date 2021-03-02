# This Python file uses the following encoding: utf-8
import mne

def readRawCNT(path):
    raw = mne.io.read_raw_cnt(path)
    channel_name = raw.info['ch_name']
    sampleFreq = raw.info['sfreq']  # 采样率，Hz
    events, event_dict = mne.events_from_annotations(raw)
    print(events)
    print(raw[:])

def readRawEEG(vhdr_path):
    raw = mne.io.read_raw_brainvision(vhdr_path)
    events, event_dict = mne.events_from_annotations(raw)
    print(events)
