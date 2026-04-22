clc

clear

time = (0:1:1999)*1e-3;

fid = fopen(['angle_meas8.bin'], 'rb');

angle  = fread(fid, 2000, 'float');

plot(time,angle)