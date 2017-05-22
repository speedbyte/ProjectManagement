% function to get the bounding box of a layer
function [left,top,right,bottom]=layerBoundingBox(mask)
[height,width]=size(mask);
index=find(mask>0);
[xx,yy]=meshgrid(1:width,1:height);
XX=xx(index);
YY=yy(index);
top=min(YY(:));
bottom=max(YY(:));
left=min(XX(:));
right=max(XX(:));