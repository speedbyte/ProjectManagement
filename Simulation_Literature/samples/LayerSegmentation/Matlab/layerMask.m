% function to form layer masks
function layer=layerMask(videolabel,frameIndex)

objDepth=[];
objIndex=[];
for i=1:length(videolabel.object)
    if videolabel.object(i).startFrame> frameIndex | videolabel.object(i).endFrame< frameIndex
        continue;
    end
    objDepth(end+1)=videolabel.object(i).depth(frameIndex);
    objIndex(end+1)=i;
end

% if the depth is empty, then no layer masks formed
if isempty(objDepth)
    layer=[];
    return;
end

[foo,idx]=sort(objDepth);
[xx,yy]=meshgrid(1:videolabel.width,1:videolabel.height);

for i=1:length(idx)
    layer(i).index=objIndex(idx(i));
    layer(i).depth=objDepth(idx(i));
    layer(i).name=videolabel.object(layer(i).index).name;
    % generate mask
    mask=inpolygon(xx,yy,videolabel.object(layer(i).index).pts(1,:,frameIndex),videolabel.object(layer(i).index).pts(2,:,frameIndex));
    [left,top,right,bottom]=layerBoundingBox(mask);
    layer(i).boundingBox=[left,top,right,bottom];
    %layer(i).mask=mask(top:bottom,left:right);
    layer(i).mask=mask;
end

layer(end+1).index=-1;
layer(end).depth=-1;
layer(end).boundingBox=[1,1,videolabel.width,videolabel.height];
layer(end).mask=ones(videolabel.height,videolabel.width);
