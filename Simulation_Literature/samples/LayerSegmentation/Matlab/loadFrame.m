% function to load a frame from the index
function im=loadFrame(videolabel,findex)
im=im2double(imread([videolabel.framePath videolabel.fileList(findex).fileName]));