clc

clear

time = (0:1:1999)*1e-3;

fid = fopen('angle_meas16.bin', 'rb');

angle1  = fread(fid, 2000, 'float');

angle2  = fread(fid, 2000, 'float');

angle3  = fread(fid, 2000, 'float');

plot(time,angle1)

hold on;

plot(time, angle2)
plot(time, angle3)

