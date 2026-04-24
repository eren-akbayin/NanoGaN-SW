clc

clear

time = (0:1:999)*1e-3;

fid = fopen(['sine14.bin'], 'rb');

sine0  = fread(fid, 1000, 'uint32');

sine1  = fread(fid, 1000, 'uint32');

sine2  = fread(fid, 1000, 'uint32');

Va = (sine0 - 3437)/3437;

Vb = (sine1 - 3437)/3437;

Vc = (sine2 - 3437)/3437;

Valpha = (2/3) * (Va - 0.5*Vb - 0.5*Vc);
Vbeta  = (2/3) * (sqrt(3)/2 * Vb - sqrt(3)/2 * Vc);

theta = 6.12556887;

Vd =  Valpha * cos(theta) + Vbeta * sin(theta);
Vq = -Valpha * sin(theta) + Vbeta * cos(theta);

plot(time,sine0)

hold on;

plot(time,sine1)

plot(time,sine2)