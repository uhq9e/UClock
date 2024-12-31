#pragma once

#include <time.h>
#include <stdint.h>
#include <math.h>

#include "ds3231_srv.h"

struct tm *get_current_time();
struct tm *set_current_time(struct tm *time);

float get_temperature();
void set_temperature(float temp);

uint8_t *tm_to_current_time(uint8_t *current_time, struct tm *timeinfo);
struct tm *current_time_to_tm(struct tm *dest, uint8_t *src);

uint8_t *encode_temperature(uint8_t *data, float temperature);