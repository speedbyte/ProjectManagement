clc;
clear all;
close all;

pathTo = '../rpi_kernelOutput/';
setValue = 1216;

M_NoLoad=csvread(strcat(pathTo, 'stdGpio_test.woLoad.csv'));
M_WLoad=csvread(strcat(pathTo, 'stdGpio_test.wLoad.csv'));

M_NoLoad(M_NoLoad>setValue.*1.5) = NaN;
M_WLoad(M_WLoad>setValue.*1.5) = NaN;

M_NoLoad(isnan(M_NoLoad)) = [];
M_WLoad(isnan(M_WLoad)) = [];

[NwoLoad,XwoLoad]=hist(M_NoLoad,1000);
[NwLoad,XwLoad]=hist(M_WLoad,1000);
maxBinWo = min(find( XwoLoad >= 4 ));
maxBinW = min(find( XwLoad >= 4 ));
figure; bar(XwoLoad(1:maxBinWo),NwoLoad(1:maxBinWo)./sum(NwoLoad(1:maxBinWo)))
title({'Standard Kernel-Interface GPIO, without system load' '(system stimulus: 1,22ms)'})
ylabel('probability')
axis([0 4 0 1])
M=mean(M_WLoad.*1000);
V=var(M_WLoad.*1000);
xlabel({'measured value (milliseconds)' '' ['Mean: ' num2str(M)] ['Variance: ' num2str(V)]}) % x-axis label
grid on
figure; bar(XwLoad(1:maxBinW),NwLoad(1:maxBinW)./sum(NwLoad(1:maxBinW)))
title({'Standard Kernel-Interface GPIO, with system load' '(system stimulus: 1,22ms)'})
xlabel('measured value [milliseconds]')
ylabel('probability')
axis([0 4 0 1])
M=mean(M_WLoad.*1000);
V=var(M_WLoad.*1000);
xlabel({'measured value (milliseconds)' '' ['Mean: ' num2str(M)] ['Variance: ' num2str(V)]}) % x-axis label
grid on