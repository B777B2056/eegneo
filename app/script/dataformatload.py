# This Python file uses the following encoding: utf-8
import mne
import numpy
import matplotlib
matplotlib.use("Qt5Agg")

raw = 0  # 原始数据
ERROR_FLAG = 0  # 错误标志
sampleFreq = 0  # 采样率（float）
channel_name = []  # 通道名称（list）
events = []  # 事件（事件数x3， 第一列是事件发生的时间，第三列是事件id）
event_dict = {}  # 事件名称与事件id的映射（key=事件id，value=事件名称）
data = []  # 数据点（通道数x样本点数量）
time = []  # 数据点对应的时间（单位为秒）

def readRawEDF(path, eventChannel = None):
    global raw, sampleFreq, channel_name, events, event_dict, data, time,  ERROR_FLAG
    if raw == 0:
        raw = mne.io.read_raw_edf(path)
    a, d = raw.get_data(return_times=True)
    a = a * 1e6
    a = a.tolist()
    d = d.tolist()
    b = raw.info['ch_names']  
    sampleFreq = raw.info['sfreq']
    if eventChannel == None:
        c, cd = mne.events_from_annotations(raw)
    else:
        try:
            c = mne.find_events(raw, stim_channel=eventChannel)
            cd = {}
            ERROR_FLAG = 0
        except ValueError:
            ERROR_FLAG = 1
            return 
    c = c.tolist()
    channel_name = b[:]
    events = c[:]
    event_dict = cd.copy()
    data = a[:]     
    time = d[:]

def readRawEEG(vhdr_path):
    global raw, sampleFreq, channel_name, events, event_dict, data, time
    if raw == 0:
        raw = mne.io.read_raw_brainvision(vhdr_path)
    a, d = raw.get_data(return_times=True)
    a = a * 1e6
    a = a.tolist()
    d = d.tolist()
    b = raw.info['ch_names']  
    sampleFreq = raw.info['sfreq']
    c, cd = mne.events_from_annotations(raw)
    c = c.tolist()
    channel_name = b[:]
    events = c[:]
    event_dict = cd.copy()
    data = a[:]     
    time = d[:]

def plotDWT(freq_min, freq_max, channel):
    global raw, events, ERROR_FLAG
    tmin = - (0.2 / 0.7) * 5 / freq_min
    tmax = (0.5 / 0.7) * 5 / freq_min
    epoch = mne.Epochs(raw, events, tmin=tmin, tmax=tmax)
    freqs = numpy.arange(freq_min, freq_max)
    try:
        power = mne.time_frequency.tfr_morlet(epoch, freqs=freqs, n_cycles=2, return_itc=False)
        power.plot(picks=[channel])
        ERROR_FLAG = 0
    except ValueError:
        ERROR_FLAG = 1

def getSampleFreq():
    return sampleFreq

def getChannelName():
    return channel_name

def getEvents():
    return events

def getEventDict():
    return event_dict

def getData():
    return data

def getTime():
    return time

def getErrorFlag():
    return ERROR_FLAG

raw = mne.io.read_raw_fif('C:\\Users\\19373\\Desktop\\Python上位机安装与展示\\Python上位机安装与展示\\Python上位机3.0\\eeg_viewer\\data\\Records\\fif\\2021_01_23_17_50_55-raw.fif')
raw.plot()
matplotlib.pyplot.show()