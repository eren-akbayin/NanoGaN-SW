clc

clear

fid = fopen('measurement15.bin', 'rb');

%% Read header fields (all uint32 or float, 4 bytes each)
bufferSize      = fread(fid, 1, 'uint32');   % bufferSize (= MEASUREMENT_SIZE)
dmaIndex        = fread(fid, 1, 'uint32');   % dmaIndex
faultIndex      = fread(fid, 1, 'uint32');   % faultIndex
uTimeStepUs     = fread(fid, 1, 'uint32');   % uTimeStepUs
fVoltPerBit     = fread(fid, 1, 'float32');  % fVoltPerBit
fAmperePerBit   = fread(fid, 1, 'float32');  % fAmperePerBit

%% Current offsets - volatile uint16_t (2 bytes each)
% Three uint16 values packed into 6 bytes - likely padded to 8 bytes (alignment)
uCurrOffsetU    = fread(fid, 1, 'uint16');
uCurrOffsetV    = fread(fid, 1, 'uint16');
uCurrOffsetW    = fread(fid, 1, 'uint16');
padding         = fread(fid, 1, 'uint16');   % 2-byte padding to reach uint32 alignment

%% Arrays - uint32[MEASUREMENT_SIZE]
uDcLinkVoltage  = fread(fid, 3*bufferSize, 'uint32');
uPhaseSens      = fread(fid, 3*bufferSize, 'uint32');
uCurrSens       = fread(fid, 3*bufferSize, 'uint32');

fid = fclose(fid);

dmaIndexShift = dmaIndex - mod(dmaIndex,3);

uDcLinkVoltage = circshift(uDcLinkVoltage, -dmaIndexShift);
uPhaseSens = circshift(uPhaseSens, -dmaIndexShift);
uCurrSens = circshift(uCurrSens, -dmaIndexShift);

%% Converting to floating point

fVoltageDC = uDcLinkVoltage * fVoltPerBit;

fVoltageU = uPhaseSens(1:3:end) * fVoltPerBit;
fVoltageV = uPhaseSens(2:3:end) * fVoltPerBit;
fVoltageW = uPhaseSens(3:3:end) * fVoltPerBit;

fCurrU = -(uCurrSens(1:3:end) - uCurrOffsetU) * fAmperePerBit;   % indices 1, 4, 7, ... → 100 samples
fCurrV = (uCurrSens(2:3:end) - uCurrOffsetV) * fAmperePerBit;   % indices 2, 5, 8, ... → 100 samples
fCurrW = (uCurrSens(3:3:end) - uCurrOffsetW) * fAmperePerBit;   % indices 3, 6, 9, ... → 100 samples

%% Time axis
N  = bufferSize;
t_us = (0:N-1) * uTimeStepUs;

t_us3 = (0:3*N-1) * uTimeStepUs/3;

tiledlayout(3,1)

ax1 = nexttile;

plot(t_us, fCurrU);

hold on;

plot(t_us, fCurrV);

plot(t_us, fCurrW);

grid on;

grid minor;

ylabel("Phase Currents")

legend("Phase U", "Phase V", "Phase W")

ax2 = nexttile;

stairs(t_us, fVoltageU)

hold on;

stairs(t_us, fVoltageV)

stairs(t_us, fVoltageW)

grid on;

grid minor;

ylabel("Phase Voltages")

legend("Phase U", "Phase V", "Phase W")

ax3 = nexttile;

plot(t_us3, fVoltageDC)

grid on;

grid minor;

xlabel("Time in us")

ylabel("DC Link Voltage")

linkaxes([ax1,ax2,ax3],'x');