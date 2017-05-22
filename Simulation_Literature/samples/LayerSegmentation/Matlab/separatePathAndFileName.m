% function to separate folder and file name from an input string
function [pathname,filename]=separatePathAndFileName(filename)
indx1=strfind(filename,'\');
indx2=strfind(filename,'/');
if isempty(indx1) & isempty(indx2)
    pathname=[];
    return;
end
if isempty(indx1)
    indx1=1;
end
if isempty(indx2)
    indx2=1;
end
index=max(indx1(end),indx2(end));
pathname=filename(1:index);
filename=filename(index+1:end);