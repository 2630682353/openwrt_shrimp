package com.example.myapplication3;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;

public class NetUtil {
    public static String doGet(String url) throws Exception {
        String result = "";
        BufferedReader reader = null;
        String jsonstr = null;
        HttpURLConnection httpURLConnection = null;
        URL request_url;
        request_url = new URL(url);
        httpURLConnection = (HttpURLConnection) request_url.openConnection();
        httpURLConnection.setRequestMethod("GET");
        httpURLConnection.setConnectTimeout(5000);
        httpURLConnection.connect();
        //获取二进制流
        InputStream inputStream = httpURLConnection.getInputStream();

        reader = new BufferedReader((new InputStreamReader(inputStream)));
        String line;
        StringBuilder builder = new StringBuilder();
        while ((line = reader.readLine()) != null) {
            builder.append(line);
            builder.append("\n");
        }
        if (builder.length() == 0)
            return null;
        return builder.toString();
    }
}
