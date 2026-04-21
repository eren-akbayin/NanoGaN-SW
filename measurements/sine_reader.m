clc

clear

time = (0:1:999)*1e-3;

fid = fopen(['sine12.bin'], 'rb');

sine0  = fread(fid, 1000, 'uint32');

sine1  = fread(fid, 1000, 'uint32');

sine2  = fread(fid, 1000, 'uint32');

plot(time,sine0)

hold on;

plot(time,sine1)

plot(time,sine2)