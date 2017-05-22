srcpath = 'M:\workstation\CVPR08\Data';
dstpath = 'D:\Workstation\CVPR2008\Data';

folderlist = dir(srcpath);

for i = 1:length(folderlist)
    if ~folderlist(i).isdir
        continue;
    end
    if strcmp(folderlist(i).name,'.')
        continue;
    end
    if strcmp(folderlist(i).name,'..')
        continue;
    end
    disp(folderlist(i).name);
    dpath = fullfile(dstpath,folderlist(i).name);
    if exist(dpath,'dir')~=7
        mkdir(dpath);
    end
    copyfile(fullfile(srcpath,folderlist(i).name,'*.jpg'),dpath);
end