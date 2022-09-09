namespace sensor
{
    void setup(char *sensorID);
    void publish(float t, float h, uint16_t co2);
    void read();
    void reset();
}