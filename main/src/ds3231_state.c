#include "common.h"
#include "ds3231_state.h"

static struct tm current_time;
static float temperature;

struct tm *get_current_time()
{
  return &current_time;
}

struct tm *set_current_time(struct tm *time)
{
  current_time.tm_year = time->tm_year;
  current_time.tm_mon = time->tm_mon;
  current_time.tm_mday = time->tm_mday;
  current_time.tm_hour = time->tm_hour;
  current_time.tm_min = time->tm_min;
  current_time.tm_sec = time->tm_sec;
  current_time.tm_wday = time->tm_wday;
  current_time.tm_isdst = time->tm_isdst;

  return &current_time;
}

float get_temperature()
{
  return temperature;
}

void set_temperature(float temp)
{
  temperature = temp;
}

uint8_t *tm_to_current_time(uint8_t *output, struct tm *value)
{
  // Year is Little Endian
  output[0] = (uint8_t)(value->tm_year + 1900);
  output[1] = (uint8_t)((value->tm_year + 1900) >> 8);

  output[2] = (uint8_t)(value->tm_mon + 1);
  output[3] = (uint8_t)(value->tm_mday);
  output[4] = (uint8_t)(value->tm_hour);
  output[5] = (uint8_t)(value->tm_min);
  output[6] = (uint8_t)(value->tm_sec);
  output[7] = (uint8_t)(value->tm_wday + 1);
  output[8] = 0x00;
  output[9] = 0x00;

  return output;
}

struct tm *current_time_to_tm(struct tm *output, uint8_t *value)
{
  output->tm_year = (value[0] | value[1] << 8) - 1900;
  output->tm_mon = value[2] - 1;
  output->tm_mday = value[3];
  output->tm_hour = value[4];
  output->tm_min = value[5];
  output->tm_sec = value[6];
  output->tm_wday = value[7] - 1;
  output->tm_yday = 0;
  output->tm_isdst = -1;

  return output;
}

uint8_t *encode_temperature(uint8_t *output, float value)
{
  if (value == 0.0f)
  {
    output[0] = 0x00;
    output[1] = 0x00;
    output[2] = 0x00;
    output[3] = 0x00;
    output[4] = 0x00;

    return output;
  }

  int sign = (value < 0.0f) ? -1 : 1;
  value = fabs(value);

  int8_t exponent = 0;

  while (value > 8388607.0f)
  {
    value /= 10.0f;
    exponent++;

    if (exponent > 127)
    {
      return output;
    }
  }
  while (value < 1.0f && value > 0.0f)
  {
    value *= 10.0f;
    exponent--;

    if (exponent < -128)
    {
      return output;
    }
  }

  int32_t mantissa = (int32_t)(value * sign);

  if (mantissa < -8388608 || mantissa > 8388607)
  {
    return output;
  }

  output[0] = 0x00;
  output[1] = (uint8_t)(mantissa & 0xFF);
  output[2] = (uint8_t)((mantissa >> 8) & 0xFF);
  output[3] = (uint8_t)((mantissa >> 16) & 0xFF);
  output[4] = (uint8_t)(exponent);

  return output;
}