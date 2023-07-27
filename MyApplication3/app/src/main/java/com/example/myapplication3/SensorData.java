package com.example.myapplication3;

import java.io.Serializable;

public class SensorData implements Serializable {
    public String client_mac;
    public String sensor_pin;
    public String table;
    public int min_value;
    public int max_value;
    public String enable;
    public int alert_id;
    public int show_value_id;
}
