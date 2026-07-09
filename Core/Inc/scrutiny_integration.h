#pragma once
#include <stddef.h>
#include <stdint.h>

void scrutiny_integration_init(void);
void scrutiny_receive_data(const uint8_t *data, uint16_t len);
uint16_t scrutiny_process_and_collect(uint8_t *out_buf, uint16_t out_buf_size);
void scrutiny_loop_process(uint32_t timestep_100ns);
void scrutiny_daq_loop_process(uint32_t timestep_100ns);