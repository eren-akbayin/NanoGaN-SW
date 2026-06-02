#pragma once
#include <stddef.h>
#include <stdint.h>

void scrutiny_integration_init(void);
void scrutiny_receive_data(const uint8_t *data, uint16_t len);
void scrutiny_process_and_send(void);
void scrutiny_loop_process(uint32_t timestep_100ns);