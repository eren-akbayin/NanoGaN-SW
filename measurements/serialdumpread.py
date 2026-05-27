"""
parse_inverter.py
Reads and parses a binary dump of inverterMeasurementsTypeDef_t over serial.

Install dependencies:
    pip install pyserial matplotlib numpy

Usage:
    python parse_inverter.py              # uses defaults below
    python parse_inverter.py --port COM3 --baud 115200 --size 100
"""

import argparse
import struct
import numpy as np
import matplotlib.pyplot as plt
import serial
import serial.tools.list_ports


# ── CONFIG ────────────────────────────────────────────────────────────────────
DEFAULT_PORT             = "COM5"
DEFAULT_BAUD             = 115200
DEFAULT_MEASUREMENT_SIZE = 8000
TRIGGER_BYTE             = b"1"
# ─────────────────────────────────────────────────────────────────────────────


def list_ports():
    ports = serial.tools.list_ports.comports()
    if not ports:
        print("No serial ports found.")
    else:
        print("Available serial ports:")
        for p in ports:
            print(f"  {p.device}  —  {p.description}")


def _expected_size(N: int) -> int:
    """
    Struct layout (little-endian, ARM default alignment):

      Offset   Size   Field
      0        4      bufferSize             uint32
      4        4      dmaIndexCurrent        uint32
      8        4      dmaIndexPhaseVoltage   uint32
      12       4      dmaIndexDCVoltage      uint32
      16       4      dmaIndexAngle          uint32
      20       4      faultIndex             uint32
      24       4      uTimeStepUs            uint32
      28       4      fVoltPerBit            float32
      32       4      fAmperePerbit          float32
      36       2      uCurrOffsetU           uint16
      38       2      uCurrOffsetV           uint16
      40       2      uCurrOffsetW           uint16
      42       2      <padding>              —
      ─── arrays ─────────────────────────────────────────────────────────────
      44       4*N    uDcLinkVoltage         uint32[N]
      44+4N    4*3N   uPhaseSens             uint32[3*N]
      44+16N   4*3N   uCurrSens              uint32[3*N]
      44+28N   2*(N/2) uElectricalPosition   uint16[N/2]
      44+29N   2      uAngleoffset           uint16
      44+29N+2 1      uPolePair              uint8
      44+29N+3 1      <struct tail padding>  —
    """
    # 7×uint32 + 2×float + 3×uint16 + 2-byte pad = 44 bytes header
    # arrays: 4N + 12N + 12N + N = 29N bytes
    # tail: uint16 + uint8 + 1-byte pad = 4 bytes
    return 44 + 29 * N + 4


def read_dump(port: str, baud: int, measurement_size: int) -> bytes:
    N             = measurement_size
    expected_size = _expected_size(N)

    print(f"Connecting to {port} @ {baud} baud ...")
    with serial.Serial(port, baud, bytesize=8, parity="N",
                       stopbits=1, timeout=5) as ser:
        ser.reset_input_buffer()

        print(f"Sending trigger '{TRIGGER_BYTE.decode()}' ...")
        ser.write(TRIGGER_BYTE)

        print(f"Waiting for {expected_size} bytes ...")
        raw = ser.read(expected_size)

    if len(raw) < expected_size:
        raise RuntimeError(
            f"Only received {len(raw)}/{expected_size} bytes. "
            "Check MEASUREMENT_SIZE or increase serial timeout."
        )

    print(f"Received {len(raw)} bytes.\n")
    return raw


def parse(raw: bytes, measurement_size: int) -> dict:
    N   = measurement_size
    ARR = 3 * N

    # ── scalars (44-byte header) ──────────────────────────────────────────────
    # 7 × uint32, 2 × float, 3 × uint16, 2-byte pad
    scalars = struct.unpack_from("<IIIIIIIffHHH2x", raw, offset=0)

    (buffer_size,
     dma_index_current,
     dma_index_phase_voltage,
     dma_index_dc_voltage,
     dma_index_angle,
     fault_index,
     time_step_us,
     volt_per_bit,
     amp_per_bit,
     offset_u, offset_v, offset_w) = scalars

    # ── arrays ────────────────────────────────────────────────────────────────
    base    = 44
    fmt_n   = f"<{N}I"          # N   × uint32
    fmt_arr = f"<{ARR}I"        # 3N  × uint32
    fmt_pos = f"<{N // 2}H"     # N/2 × uint16

    dc_link_raw  = np.array(struct.unpack_from(fmt_n,   raw, base),                    dtype=np.uint32)   # (N,)
    phase_raw    = np.array(struct.unpack_from(fmt_arr, raw, base + N*4),              dtype=np.uint32)
    curr_raw     = np.array(struct.unpack_from(fmt_arr, raw, base + N*4 + ARR*4),      dtype=np.uint32)
    elec_pos_raw = np.array(struct.unpack_from(fmt_pos, raw, base + N*4 + 2*ARR*4),   dtype=np.uint16)   # (N/2,)

    tail_offset  = base + N*4 + 2*ARR*4 + (N // 2)*2
    (angle_offset, pole_pair) = struct.unpack_from("<HB", raw, tail_offset)

    # reshape phase / current arrays: (3, N)
    phase_raw = phase_raw.reshape(N, 3).T
    curr_raw  = curr_raw.reshape(N, 3).T

    # ── physical unit conversion ──────────────────────────────────────────────
    offsets   = np.array([[offset_u], [offset_v], [offset_w]], dtype=np.float32)
    voltage_V = dc_link_raw.astype(np.float32) * volt_per_bit          # (N,)
    phase_V   = phase_raw.astype(np.float32) * volt_per_bit            # (3, N)
    current_A = (curr_raw.astype(np.float32) - offsets) * amp_per_bit  # (3, N)
    current_A[0] = -current_A[0]   # invert phase U

    # electrical position: convert raw uint16 count → degrees (0–360)
    raw = (elec_pos_raw & 0x3FFF).astype(np.int32) * 4  # strip top 2 bits
    elec_angle_deg = ((raw + angle_offset) % 65535) / 65535.0 * 360.0 * pole_pair % 360.0

    return dict(
        buffer_size              = buffer_size,
        dma_index_current        = dma_index_current,
        dma_index_phase_voltage  = dma_index_phase_voltage,
        dma_index_dc_voltage     = dma_index_dc_voltage,
        dma_index_angle          = dma_index_angle,
        fault_index              = fault_index,
        time_step_us             = time_step_us,
        volt_per_bit             = volt_per_bit,
        amp_per_bit              = amp_per_bit,
        offset_u                 = offset_u,
        offset_v                 = offset_v,
        offset_w                 = offset_w,
        angle_offset             = angle_offset,
        pole_pair                = pole_pair,
        dc_link_raw              = dc_link_raw,
        phase_raw                = phase_raw,
        curr_raw                 = curr_raw,
        elec_pos_raw             = elec_pos_raw,
        voltage_V                = voltage_V,
        phase_V                  = phase_V,
        current_A                = current_A,
        elec_angle_deg           = elec_angle_deg,
    )


def print_summary(d: dict):
    print("=== Parsed Struct ===")
    print(f"  bufferSize             : {d['buffer_size']}")
    print(f"  dmaIndexCurrent        : {d['dma_index_current']}")
    print(f"  dmaIndexPhaseVoltage   : {d['dma_index_phase_voltage']}")
    print(f"  dmaIndexDCVoltage      : {d['dma_index_dc_voltage']}")
    print(f"  dmaIndexAngle          : {d['dma_index_angle']}")
    print(f"  faultIndex             : {d['fault_index']}")
    print(f"  uTimeStepUs            : {d['time_step_us']} µs")
    print(f"  fVoltPerBit            : {d['volt_per_bit']:.6f} V/bit")
    print(f"  fAmperePerbit          : {d['amp_per_bit']:.6f} A/bit")
    print(f"  CurrOffset U/V/W       : {d['offset_u']}  {d['offset_v']}  {d['offset_w']}")
    print(f"  uAngleoffset           : {d['angle_offset']}")
    print(f"  uPolePair              : {d['pole_pair']}")


def plot(d: dict):
    N      = d["voltage_V"].shape[0]           # full-rate sample count
    N_pos  = d["elec_angle_deg"].shape[0]      # N/2 samples
    t      = np.arange(N)     * d["time_step_us"] * 1e-6
    t_pos  = np.arange(N_pos) * d["time_step_us"] * 1e-6 * 2   # half rate
    phases = ["U", "V", "W"]
    colors = ["tab:blue", "tab:orange", "tab:green"]

    fig, axes = plt.subplots(8, 1, figsize=(12, 16), sharex=True)
    fig.suptitle("Inverter Measurements", fontsize=14)

    # DC Link Voltage
    axes[0].plot(t, d["voltage_V"], color="tab:blue", linewidth=0.8)
    axes[0].set_ylabel("V_DC (V)")
    axes[0].grid(True, alpha=0.4)

    # Phase Currents
    for i in range(3):
        axes[1 + i].plot(t, d["current_A"][i], color=colors[i], linewidth=0.8)
        axes[1 + i].set_ylabel(f"I_{phases[i]} (A)")
        axes[1 + i].grid(True, alpha=0.4)

    # Phase Voltages (sensor counts, step-plot)
    for i in range(3):
        axes[4 + i].plot(t, d["phase_V"][i], color=colors[i],
                         linewidth=0.8, drawstyle="steps-post")
        axes[4 + i].set_ylabel(f"PhaseSens {phases[i]}")
        axes[4 + i].grid(True, alpha=0.4)

    # Electrical Position (half rate)
    axes[7].plot(t_pos, d["elec_angle_deg"], color="tab:purple", linewidth=0.8)
    axes[7].set_ylabel("Elec. Angle (°)")
    axes[7].set_xlabel("Time (s)")
    axes[7].grid(True, alpha=0.4)
    
    plt.tight_layout()
    plt.show()


def main():
    parser = argparse.ArgumentParser(description="Inverter serial dump parser")
    parser.add_argument("--port",  default=DEFAULT_PORT,             help="Serial port")
    parser.add_argument("--baud",  default=DEFAULT_BAUD,  type=int,  help="Baud rate")
    parser.add_argument("--size",  default=DEFAULT_MEASUREMENT_SIZE, type=int,
                        help="MEASUREMENT_SIZE firmware define (must be even)")
    parser.add_argument("--list-ports", action="store_true",         help="List available ports and exit")
    parser.add_argument("--file",  default=None,                     help="Parse from binary file instead of serial")
    args = parser.parse_args()

    if args.list_ports:
        list_ports()
        return

    if args.size % 2 != 0:
        raise ValueError("MEASUREMENT_SIZE must be even (uElectricalPosition has N/2 elements).")

    if args.file:
        print(f"Reading from file: {args.file}")
        with open(args.file, "rb") as f:
            raw = f.read()
    else:
        raw = read_dump(args.port, args.baud, args.size)

    data = parse(raw, args.size)
    print_summary(data)
    plot(data)


if __name__ == "__main__":
    main()