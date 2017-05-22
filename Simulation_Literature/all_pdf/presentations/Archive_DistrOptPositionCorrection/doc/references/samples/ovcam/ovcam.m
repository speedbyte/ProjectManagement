
frame=1;
images = [];

ImageBlur = 0.04 * ones(5,5);
H = fspecial('gaussian',[5 5],1.5);


try
  close all;
  
  %% imaqreset;
  if ~exist('vid')
    %%vid = videoinput('winvideo', 1, 'RGB24_640x480');        
    vid = videoinput('winvideo', 1, 'RGB24_320x240');
    preview(vid);
    %%set(vid1.source, 'Brightness', 255);
    disp 'preview';
  end  
    
  figure(1);
  set(gcf, 'BackingStore', 'off');  
  figure(2);
  set(gcf, 'toolbar', 'none');
  set(gcf, 'MenuBar', 'none');
  set(gcf, 'BackingStore', 'off');
  figure(3);
  set(gcf, 'toolbar', 'none');
  set(gcf, 'MenuBar', 'none');  
  set(gcf, 'BackingStore', 'off');  

  while(1)
    tmp=getsnapshot(vid);
      
    tmp = rgb2gray(tmp);
    %%tmp = imresize(tmp,[240 320],'bilinear');      
    %%tmp = imresize(tmp,0.25,'bilinear');
    tmp = imresize(tmp,0.5,'nearest');
    %%tmp = double(tmp);
    %%tmp = im2uint8(tmp);
    %%tmp = imfilter(tmp, ImageBlur);  
    %%tmp = conv2(tmp,H,'same');      
     
    images(:,:,1) = tmp;
    
    if (size(images,3) >= 2)
      set(0,'CurrentFigure',2)      
      imshow(tmp);
      %%colormap(gray);
                 
      [Vx, Vy] = opticalflow(images, 100.0, 1);
      
      Vx(1,1) = 1;
      Vy(1,1) = 1;        
      %{
      [ny,nx]=size(Vx);
      for y=1:ny
        for x=1:nx
          d=sqrt(Vx(y,x)^2 + Vy(y,x)^2);
          if d < 0.3 
            Vx(x,y) = 0;
            Vy(x,y) = 0;
          end           
        end
      end
      %}      
                                                                        
      xgrid = 1:5:size(Vx,2);
      ygrid = 1:5:size(Vx,1);
      [xi,yi]=meshgrid(xgrid, ygrid);
      Vxi = interp2(Vx, xi, yi);
      Vyi = interp2(Vy, xi, yi);      
      hold on;            
      quiver(xgrid, ygrid, Vxi, Vyi, 2, 'r');                              
      hold off;
      drawnow;
            
      set(0,'CurrentFigure',3)      
      a1 = [min(Vx(:)), max(Vx(:)), min(Vy(:)), max(Vy(:)), mean(Vx(:)), ...
        mean(Vy(:))];           
      bar(a1, 0.1, 'r');
      %%colormap summer;
      set(gca,'ylim',[-1 1]);      
      %%fprintf('frame: %04d  %4.0f  %4.0f  %4.0f  %4.0f\n', frame, ...
      %% min(min(Vx)), max(max(Vx)), min(min(Vy)), max(max(Vy)) );
      drawnow;        
      fprintf('frame: %04d\n', frame);                                 
    end 
        
    images(:,:,2) = images(:,:,1);    
    frame = frame + 1;        
  end

catch exception
  exception.getReport
end


if exist('vid')
  delete(vid);
  clear vid;    
end  

