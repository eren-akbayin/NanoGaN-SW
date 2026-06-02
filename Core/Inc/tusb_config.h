#pragma once

// ── Debug ─────────────────────────────────────────────────────────────────
#define CFG_TUSB_DEBUG 0 // set to 1 to get uart debug logs

// ── Memory ────────────────────────────────────────────────────────────────
#define CFG_TUSB_MEM_SECTION // no special linker section needed
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

// ── USB Device Configuration ─────────────────────────────────────────────
#define CFG_TUD_MAX_SPEED OPT_MODE_FULL_SPEED
#define CFG_TUSB_RHPORT0_MODE (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// ── Device stack ──────────────────────────────────────────────────────────
#define CFG_TUD_ENABLED 1

// ── Endpoint 0 ────────────────────────────────────────────────────────────
#define CFG_TUD_ENDPOINT0_SIZE 64

// ── CDC ───────────────────────────────────────────────────────────────────
#define CFG_TUD_CDC 1
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 512
#define CFG_TUD_CDC_EP_BUFSIZE 64 // must match descriptor (FS max = 64)

// ── All other classes off ─────────────────────────────────────────────────
#define CFG_TUD_HID 0
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0
#define CFG_TUD_DFU_RUNTIME 0