clc

clear

buf_len = 600;

dmaIndex = 471;

fid = fopen('currents.bin', 'rb');
current_raw = fread(fid, buf_len, 'uint32');  % reads 300 x uint32 elements
fclose(fid);

current_raw = circshift(current_raw, -dmaIndex);

fid = fopen('voltages.bin', 'rb');
voltage_raw = fread(fid, buf_len, 'uint32');  % reads 300 x uint32 elements
fclose(fid);

fid = fopen('dcvoltage.bin', 'rb');
dc_voltage_raw = fread(fid, buf_len, 'uint32');  % reads 300 x uint32 elements
fclose(fid);

voltage_raw = circshift(voltage_raw, -dmaIndex);

CURRENT_PER_BITS = 80.0/4096.0;

VOLTAGE_PER_BITS = 12.0/790.0;

uCurrOffsetU = 2076;
uCurrOffsetV = 2079;
uCurrOffsetW = 2078;

currents_phaseU = -(current_raw(1:3:end) - uCurrOffsetU) * CURRENT_PER_BITS;   % indices 1, 4, 7, ... → 100 samples
currents_phaseV = (current_raw(2:3:end) - uCurrOffsetV) * CURRENT_PER_BITS;   % indices 2, 5, 8, ... → 100 samples
currents_phaseW = (current_raw(3:3:end) - uCurrOffsetW) * CURRENT_PER_BITS;   % indices 3, 6, 9, ... → 100 samples

voltage_phaseU = voltage_raw(1:3:end) * VOLTAGE_PER_BITS;
voltage_phaseV = voltage_raw(2:3:end) * VOLTAGE_PER_BITS;
voltage_phaseW = voltage_raw(3:3:end) * VOLTAGE_PER_BITS;

dc_voltage = dc_voltage_raw(1:3:end) * VOLTAGE_PER_BITS;


tiledlayout(3,1)

ax1 = nexttile;

plot(currents_phaseU);

hold on;

plot(currents_phaseV);

plot(currents_phaseW);

legend("Phase U", "Phase V", "Phase W")

ax2 = nexttile;

stairs(voltage_phaseU)

hold on;

stairs(voltage_phaseV)

stairs(voltage_phaseW)

legend("Phase U", "Phase V", "Phase W")

ax3 = nexttile;

plot(dc_voltage)

linkaxes([ax1,ax2,ax3],'x');