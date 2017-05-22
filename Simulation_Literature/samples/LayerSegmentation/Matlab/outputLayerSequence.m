videolabel=loadVideoLabelMeXML('E:\Programs\VideoLabeling\Data\car2_stabilized2.xml');
dstpath='E:\workstation\CVPR08\Matlab\MotionDataBase\car2\';

figure;
for i=1:videolabel.numFrames
    fprintf('Process frame %s...',videolabel.fileList(i).fileName);
    layer=layerMask(videolabel,i);
    Im=displayLayerMask(layer);
    imshow(Im);drawnow;
    imwrite(Im,[dstpath videolabel.fileList(i).fileName],'Quality',96);
    fprintf('done!\n');
end