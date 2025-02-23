#include <SPI.h>
#include <array>
#include "common-defines.hpp"

OneWire one_wire(ONE_WIRE_BUS);
DallasTemperature sensors(&one_wire);
int device_count;
mcp2515_can CAN(SPI_CS_PIN); // Set CS pin



class SensorData {

    // Wrapper required since we can't directly return array
    // in C
    typedef struct {
        DeviceAddress dev_addr;
    } DeviceAddressWrapper;

    public:
        SensorData() = default;
        ~SensorData() = default;

        int set_idx(const int idx) {
            _idx = idx;
        }

        int get_idx() const {
            return _idx;
        }
        
        void set_temperature_celcius(DallasTemperature &sensors) {
            float tmp = sensors.getTempC(_dev_address);
            if (tmp == DEVICE_DISCONNECTED_C) {
                Serial.println("Error: Could not read temperature data");
                return;
            }
            _temperature_C = tmp;
            Serial.println("I am in set: ");
            Serial.print(_temperature_C);
        }

        float get_temperature_celcius() const {
            Serial.println("I am in here: ");
            // Serial.print(_temperature_C);
            Serial.print(_temperature_C, 3);
            return _temperature_C;
        }

        // function to print the temperature for a device
        void print_temperature() {
            Serial.print("Temp C: ");
            Serial.print(_temperature_C, 3);
        }

        // function to print a device's resolution
        void print_resolution() {
        Serial.print("Resolution: ");
        Serial.print(sensors.getResolution(_dev_address));
        Serial.println();
        }

        // function to print a device address
        void print_address() {
            for (uint8_t i = 0; i < 8; i++) {
                // zero pad the address if necessary
                if (_dev_address[i] < 16) Serial.print("0");
                Serial.print(_dev_address[i], HEX);
            }
        }

        // main function to print information about a device
        void print_data() {
            Serial.println("Device Address: ");
            print_address();
            Serial.print(" ");
            print_temperature();
            Serial.println();
        }

        DeviceAddress _dev_address;

    private:
        float _temperature_C;
        int _idx;

};

std::array<SensorData, N_TEMPERATURE_SENSOR> sensor_data_array;

static void init_can() {
    while (CAN_OK != CAN.begin(CONFIG_BITRATE)) {             // init can bus : baudrate = 500k
        Serial.println("CAN init fail, retry...");
        delay(100);
    }
    Serial.println("CAN init ok!");
}

// There are 2 methods in obtaining the readings from the sensors

// 1st method (current implementation): Use indexing
// I use the first method since we can dynamically obtain the sensors
// and if the sensors were to swap, we would not have any issues with the
// address

// 2nd method: Determine the address of each sensors used
// and then request the temperature reading with a statically assigned
// address
// (e.g. sensor1[8] = {0x12, 0x11, ......})
static bool init_sensors() {
    Serial.println("Locating devices...");
    
    // Begin searching for sensors on the bus by pulling the bus low
    // for a certain duration (see datasheet) to trigger the sensors
    // to send signals indicating its presence
    sensors.begin();
    
    // Once the above is done, we are able to detect the presence
    // of n sensors
    device_count = sensors.getDeviceCount();

    // Check if the device count matches the expected number of sensors or not
    if (device_count > N_TEMPERATURE_SENSOR) {
       Serial.println("Expected ");
        Serial.print(N_TEMPERATURE_SENSOR);
        Serial.print(" sensors. However, ");
        Serial.print(device_count);
        Serial.print(" is detected. Please update the N_TEMPERATURE_SENSOR definition to ");
        Serial.print(device_count);
        Serial.println(" or higher.");
        delay(1000);
        return false;
    } else {
        Serial.println(device_count);
        Serial.println(" device(s) found.");
        // We will only be looping with the amount of device count found from before
        if (!(device_count > 0)) {
            Serial.println("Fail to find any device.");
            return false;
        }
        for (size_t i = 0; i < device_count; i++) {
            if (!sensors.getAddress(sensor_data_array[i]._dev_address, i)) {
                Serial.println("Unable to find address for Device ");
                Serial.print(i);
            } else {
                Serial.println("Sensor found, temperature precision set to ");
                Serial.print(TEMPERATURE_PRECISION);
                sensors.setResolution(sensor_data_array[i]._dev_address, TEMPERATURE_PRECISION);
                sensor_data_array[i].print_address();
            }
        }
    }
    return true;
}

void setup() {
    Serial.begin(9600);
    Serial.println("Initializing Sensors...");

    while (true) {
        if (init_sensors()) {
            Serial.println("Sensors successfully initialized!");
            break;
        };
        Serial.println("Fail to init. Retrying...");
    }

    Serial.println("Initializing CAN interface...");
    init_can();
    Serial.println("Initialization successful!");
}


void loop() {

    // This will issue a global temperature request to all sensors on the
    // bus. Each available sensors connected to the bus will attempt to
    // convert the temperature reading
    sensors.requestTemperatures();

    unsigned char can_data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    for (size_t i = 0; i < device_count; i++) {
        SensorData &curr_sensor = sensor_data_array[i];
        curr_sensor.set_temperature_celcius(sensors);
        curr_sensor.print_data();
    }

    // TODO(Khalid): Ensure CAN is properly initialized
    int tmp = static_cast<int>(sensor_data_array[0].get_temperature_celcius() * 100);
    can_data[0] = (tmp & 0xFF); 
    can_data[1] = (tmp >> 8) & 0xFF;
    CAN.sendMsgBuf(0x00, 0, 8, can_data);
    delay(100);                       // send data per 100ms
    Serial.println("CAN BUS sendMsgBuf ok!");
}



// END FILE
