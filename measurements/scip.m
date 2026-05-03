clc

clear

fAngleEl = 2.564816/pi*180;

U = (3249-3437)/3437
V = (3281-3437)/3437
W = (3780-3437)/3437

alpha = U-V*cosd(60)-W*cosd(60);

beta = V*cosd(30) - W*cosd(30)

d = (U * cosd(fAngleEl) + V * cosd(fAngleEl - 120 ) + W * cosd( fAngleEl + 120))*2/3;

q = (- U * sind(fAngleEl) - V * sind(fAngleEl - 120 ) - W * sind( fAngleEl + 120))*2/3;
