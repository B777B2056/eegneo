import mne
import matplotlib.pyplot as plt

TOPO_PLOT_BUF_LEN = 32

class TopoPlot:
    def __init__(self, sfreq:float, ch_names:list, save_path:str) -> None:
        self.sfreq = sfreq
        self.montage = mne.channels.make_standard_montage('standard_1020')
        self.info = mne.create_info(ch_names=ch_names, sfreq=sfreq, ch_types='eeg')
        self.path = save_path
        self.n_chan = len(ch_names)
        self.data = [[] for i in range(self.n_chan)]

    def appendData(self, data:list) -> None:
        global TOPO_PLOT_BUF_LEN
        if len(self.data[0]) >= TOPO_PLOT_BUF_LEN:
            for i in range(self.n_chan):
                self.data[i].pop(0)
        for i in range(self.n_chan):
            self.data[i].append(data[i])

    def plot(self) -> None:
        evoked = mne.EvokedArray(self.data, self.info)      
        evoked.set_montage(self.montage) 
        mne.viz.plot_topomap(evoked.data[:, 0], evoked.info, size=3, show=False)
        plt.savefig(self.path)
        plt.cla()
        plt.close('all')
        