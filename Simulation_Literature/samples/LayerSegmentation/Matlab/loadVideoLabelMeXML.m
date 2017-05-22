% function to load VideoLabelMe XML formate to matlab
function videolabel=loadVideoLabelMeXML(filename)

% first read the file
v=loadXML(filename);

% parse the loaded xml format
fprintf('starting parsing xml file...');

videolabel.framePath = v.annotation.folder;
videolabel.numFrames = str2num(v.annotation.NumFrames);
videolabel.curFrame  = str2num(v.annotation.currentFrame)+1;
for i=1:videolabel.numFrames
    videolabel.fileList(i).fileName  = cell2mat(v.annotation.fileList.fileName(i));
end

% parse each object
for i=1:length(v.annotation.object)
    videolabel.object(i).name       = v.annotation.object(i).name;
    videolabel.object(i).startFrame = str2num(v.annotation.object(i).startFrame)+1; % conversion from C++ to matlab
    videolabel.object(i).endFrame   = str2num(v.annotation.object(i).endFrame)+1;
    
    %% allocate the buffers first
    
    videolabel.object(i).depth       = zeros(1,videolabel.numFrames);
    videolabel.object(i).globalLabel = zeros(1,videolabel.numFrames);
    videolabel.object(i).localLabel  = zeros(1,videolabel.numFrames);
    videolabel.object(i).tracked     = zeros(1,videolabel.numFrames);
    videolabel.object(i).depthLabel  = zeros(1,videolabel.numFrames);
    
%     npts=length(v.annotation.object(i).labels.frame(1).polygon.pt);
%     videolabel.object(i).pts         = zeros(2,npts,videolabel.numFrames);
%     videolabel.object(i).islabeled   = zeros(npts,videolabel.numFrames);
    %% load each frame
    for j=videolabel.object(i).startFrame:videolabel.object(i).endFrame
        % k is the index for v
        k=j-videolabel.object(i).startFrame+1;
        videolabel.object(i).depth(j)      = str2num(v.annotation.object(i).labels.frame(k).depth);
        videolabel.object(i).globalLabel(j)= str2num(v.annotation.object(i).labels.frame(k).globalLabel);
        videolabel.object(i).localLabel(j) = str2num(v.annotation.object(i).labels.frame(k).localLabel);
        videolabel.object(i).tracked(j)    = str2num(v.annotation.object(i).labels.frame(k).tracked);
        videolabel.object(i).depthLabel(j) = str2num(v.annotation.object(i).labels.frame(k).depthLabel);
        
        if isfield(v.annotation.object(i).labels.frame(k),'isPolygon')==1
            videolabel.object(i).isPolygon(j) = str2num(v.annotation.object(i).labels.frame(k).isPolygon);
        else
            videolabel.object(i).isPolygon(j) = 1;
        end
        
        if videolabel.object(i).isPolygon(j)==1
            %% load each point
             npts=length(v.annotation.object(i).labels.frame(k).polygon.pt);
            for ii=1:npts
                videolabel.object(i).pts(1,ii,j)    = str2num(v.annotation.object(i).labels.frame(k).polygon.pt(ii).x)+1; % conversion from C++ to matlab
                videolabel.object(i).pts(2,ii,j)    = str2num(v.annotation.object(i).labels.frame(k).polygon.pt(ii).y)+1; % conversion from C++ to matlab
                videolabel.object(i).islabeled(ii,j)= str2num(v.annotation.object(i).labels.frame(k).polygon.pt(ii).labeled);
            end
        else
            x1=str2num(v.annotation.object(i).labels.frame(k).boundingBox.topLeft.x)+1; % conversion from C++ to matlab
            y1=str2num(v.annotation.object(i).labels.frame(k).boundingBox.topLeft.y)+1;
            x2=str2num(v.annotation.object(i).labels.frame(k).boundingBox.bottomRight.x)+1;
            y2=str2num(v.annotation.object(i).labels.frame(k).boundingBox.bottomRight.x)+1;
            videolabel.object(i).pts(:,:,j)=[x1,y1;x2,y1;x2,y2;x1,y2];
        end
    end
end

% get the dimension of the frame
% search for the relative path if possible
if exist([videolabel.framePath videolabel.fileList(1).fileName])~=2
   [pathname,filename]=separatePathAndFileName(filename);
   videolabel.framePath=[pathname filename(1:end-4) pathname(end)];
end

im=imread([videolabel.framePath videolabel.fileList(1).fileName]);
[height,width,nchannels]=size(im);
videolabel.width=width;
videolabel.height=height;

fprintf('done!\n');