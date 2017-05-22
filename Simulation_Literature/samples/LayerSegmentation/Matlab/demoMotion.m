XMLfile = '../Data/car1_stabilized.xml';

videoLabel = loadVideoLabelMeXML(XMLfile);

figure;
for i = 1:videoLabel.numFrames
    im = loadFrame(videoLabel,i);
    layer =  layerMask(videoLabel,i);
    Im = displayLayerMask(layer);
    clf;
    imshow(im*.5+Im*.5);drawnow;
    pause(0.01);
end