/*
 * parameters.h
 *
 *  Created on: Apr 13, 2026
 *      Author: locomotive
 */

#pragma once
#include "stdint.h"

typedef struct{
	uint16_t id;
	int32_t min;
	int32_t max;
	int32_t def;
	void (*callback)(void *argx);
}parameters_t;
