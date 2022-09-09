namespace sensor
{
    void setup(char *sensorID);
    void startsgp(boolean full = false);
    void publish(float t, float h, uint16_t co2);
    void read();
    void reset();
}