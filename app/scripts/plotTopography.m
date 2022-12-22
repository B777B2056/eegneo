function plotTopography(datavector, locsPath)
    figure
    topoplot(datavector,locsPath,'style','map','electrodes','labels');
    colorbar
    colormap jet
    saveas(gcf, 'eegtopo.jpg');
end

% @Function test
% EEG = pop_biosig(['E:/jr/eegneo/test/data/', 'S001R01.edf']);
% datavec = sum(EEG.data)/64;
% plotTopography(datavec, 'E:/MATLAB/toolbox/eeglab2021.1/sample_locs/Standard-10-20-Cap19.locs');
