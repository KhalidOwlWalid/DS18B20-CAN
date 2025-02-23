#ifndef INCLUDE_COMMON_DEFINES_HPP
#define INCLUDE_COMMON_DEFINES_HPP

#define CAN_2515

// For Arduino MCP2515 Hat:
// the cs pin of the version after v1.1 is default to D9
// v0.9b and v1.0 is default D10
const int SPI_CS_PIN = 9;
const int CAN_INT_PIN = 2;
const int ONE_WIRE_BUS = 4;
const int N_TEMPERATURE_SENSOR = 1;
const int TEMPERATURE_PRECISION = 12;

#endif // INCLUDE_COMMON_DEFINES_HPP