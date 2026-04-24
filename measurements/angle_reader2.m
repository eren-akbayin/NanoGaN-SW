clc

clear

time = (0:1:1999)*1e-3;

fid = fopen(['angle_meas11.bin'], 'rb');

angle1  = fread(fid, 2000, 'uint16');

angle2  = fread(fid, 2000, 'uint16');

plot(time,angle1)

hold on;

plot(time, angle2)