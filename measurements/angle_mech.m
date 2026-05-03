clc

clear

fid = fopen('angle_mech.bin', 'rb');

angle1  = fread(fid, 2000, 'uint32');

