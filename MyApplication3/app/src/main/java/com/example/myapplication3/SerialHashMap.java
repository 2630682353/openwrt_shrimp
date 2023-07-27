package com.example.myapplication3;

import java.io.Serializable;
import java.util.HashMap;

public class SerialHashMap implements Serializable {
    private HashMap<String, SensorData> map;

    public HashMap<String, SensorData> getMap() {
        return map;
    }

    public void setMap(HashMap<String, SensorData> map) {
        this.map = map;
    }
}
