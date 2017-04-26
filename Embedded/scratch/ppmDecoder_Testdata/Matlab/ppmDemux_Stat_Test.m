clc;
clear all;
close all;

pathTo = '../rpi_kernelOutput/';

%---------------------------------------------------------------------
% Threaded Driver + EMLID-based PREEMPT_RT-Kernel
%---------------------------------------------------------------------
% setValue = 608;
% M_NoLoad=csvread(strcat(pathTo, 'emlidRT.620us.woLoad.threaded.csv'));
% M_WLoad=csvread(strcat(pathTo, 'emlidRT.620us.withLoad.threaded.csv'));
% 
% setValue = 1216;
% M_NoLoad=csvread(strcat(pathTo, 'emlidRT.1220us.woLoad.threaded.csv'));
% M_WLoad=csvread(strcat(pathTo, 'emlidRT.1220us.withLoad.threaded.csv'));
%
%
% >>> BEST RESULT
% called with "chrt 50 sudo ./getInterruptTimes"
setValue = 1216;
M_NoLoad=csvread(strcat(pathTo, 'emlidRT.1220us.woLoad.threaded.prio50.csv'));
M_WLoad=csvread(strcat(pathTo, 'emlidRT.1220us.withLoad.threaded.prio50.csv'));
% <<< BEST RESULT
%
% 
% setValue = 2072;
% M_NoLoad=csvread(strcat(pathTo, 'emlidRT.2080us.woLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'emlidRT.2080us.withLoad.csv'));
% 
% setValue = 2192;
% M_NoLoad=csvread(strcat(pathTo, 'emlidRT.2200us.woLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'emlidRT.2200us.withLoad.csv'));

%---------------------------------------------------------------------
% EMLID-based PREEMPT_RT-Kernel
%---------------------------------------------------------------------
% setValue = 608;
% M_NoLoad=csvread(strcat(pathTo, 'emlidRT.620us.woLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'emlidRT.620us.withLoad.csv'));
% % 
% setValue = 1216;
% M_NoLoad=csvread(strcat(pathTo, 'emlidRT.1220us.woLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'emlidRT.1220us.withLoad.csv'));
% 
% setValue = 2072;
% M_NoLoad=csvread(strcat(pathTo, 'emlidRT.2080us.woLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'emlidRT.2080us.withLoad.csv'));

% setValue = 2192;
% M_NoLoad=csvread(strcat(pathTo, 'emlidRT.2200us.woLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'emlidRT.2200us.withLoad.csv'));

%---------------------------------------------------------------------
% Custom PREEMPT_RT-Kernel
%---------------------------------------------------------------------
% setValue = 1216;
% M_NoLoad=csvread(strcat(pathTo, 'rtTest.1220.csv'));
% M_WLoad=csvread(strcat(pathTo, 'rtTest.withLoad.1220.csv'));

%---------------------------------------------------------------------
% Custom PREEMPT-Kernel
%---------------------------------------------------------------------
% setValue = 608;
% M_NoLoad=csvread(strcat(pathTo, 'shortPulse_620us_NoLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'shortPulse_620us_WithLoad.csv'));

% setValue = 1216;
% M_NoLoad=csvread(strcat(pathTo, 'shortPulse_1220us_NoLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'shortPulse_1220us_WithLoad.csv'));

% setValue = 2000;
% M_NoLoad=csvread(strcat(pathTo, 'shortPulse_2000us_NoLoad.csv'));
% M_WLoad=csvread(strcat(pathTo, 'shortPulse_2000us_WithLoad.csv'));

%filter outliers due to wrap-arounds etc.
M_NoLoad(M_NoLoad>setValue.*3.5) = NaN;
M_WLoad(M_WLoad>setValue.*3.5) = NaN;

M_NoLoad(isnan(M_NoLoad)) = [];
M_WLoad(isnan(M_WLoad)) = [];

[NwoLoad,XwoLoad]=hist(M_NoLoad,100);
[NwLoad,XwLoad]=hist(M_WLoad,100);
maxBinWo = 100;
maxBinW = 100;
NwoLoad_norm=NwoLoad(1:maxBinWo)./sum(NwoLoad(1:maxBinWo));
figure; bar(XwoLoad(1:maxBinWo),NwoLoad_norm);
title({'Custom Kernel-module driver (system on idle)' ['(stimulus: ',num2str(setValue),'us)']})
M=mean(M_NoLoad);
V=var(M_NoLoad);
hold on
plot(XwoLoad(1:maxBinWo), max(NwoLoad_norm).*exp(-0.5*( (XwoLoad(1:maxBinWo)-M)./sqrt(V) ).^2)  ,'r');
xlabel({'measured value (us)' '' ['Mean: ' num2str(M)] ['Variance: ' num2str(V)]}) % x-axis label
legend('Histogram on measured data','Fitted normal distribution')
ylabel('probability')
ylim(gca,[0 1])
grid on 

NwLoad_norm=NwLoad(1:maxBinW)./sum(NwLoad(1:maxBinW));
figure; bar(XwLoad(1:maxBinW),NwLoad_norm)
title({'Custom Kernel-module driver (with system load)' ['(stimulus: ',num2str(setValue),'us)']})
ylabel('probability')
M=mean(M_WLoad);
V=var(M_WLoad);
hold on
plot(XwLoad(1:maxBinW), max(NwLoad_norm).*exp(-0.5*( (XwLoad(1:maxBinW)-M)./sqrt(V) ).^2)  ,'r');
xlabel({'measured value (us)' '' ['Mean: ' num2str(M)] ['Variance: ' num2str(V)]}) % x-axis label
legend('Histogram on measured data','Fitted normal distribution')
ylim(gca,[0 1])
grid on 