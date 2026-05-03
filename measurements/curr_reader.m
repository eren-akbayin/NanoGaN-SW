clc

clear

time = (0:1:1999)*1e-3;

fid = fopen('angle_meas17.bin', 'rb');

angle1  = fread(fid, 2000, 'float');

fid1 = fopen('curr_meas17.bin', 'rb');

U = fread(fid1, 2000, 'float');
V = fread(fid1, 2000, 'float');
W = fread(fid1, 2000, 'float');

d = (U .* cos(angle1) + V .* cos(angle1 - 2*pi/3 ) + W .* cos( angle1 + 2*pi/3))*2/3;

q = (- U .* sin(angle1) - V .* sin(angle1 - 2*pi/3 ) - W .* sin( angle1 + 2*pi/3))*2/3;

subplot(3,1,1)
plot(angle1)

subplot(3,1,2)
plot(U)
hold on;
plot(V)
plot(W)

subplot(3,1,3)

plot(d)
hold on
plot(q)


