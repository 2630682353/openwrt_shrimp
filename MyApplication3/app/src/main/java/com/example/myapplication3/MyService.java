package com.example.myapplication3;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Color;
import android.net.ConnectivityManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Vibrator;
import android.util.Log;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.core.app.NotificationCompat;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;

public class MyService extends Service {
    public MyService() {
    }
    private BroadcastReceiver broadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            int msg_type = intent.getIntExtra("msg_type", 0);
            if (msg_type == 1) {
                Bundle bundle = intent.getExtras();
                SerialHashMap serializableHashMap = (SerialHashMap) bundle.get("map");
                hashMap = serializableHashMap.getMap();
                Log.d("zc", "onReceive: ");
            }
            else if(msg_type == 2) {
                response_success = 0;
                response_error = 0;
                alert_num = 0;
                alert_string="";
            }else if(msg_type == 3) {
                request_enable = 0;
            }else if(msg_type == 4) {
                request_enable = 1;
            }
        }
    };

    private HashMap<String, SensorData> hashMap = new HashMap<>();
    private Notification notification;
    private NotificationCompat.Builder builder;
    private NotificationChannel channel;
    private int response_success = 0;
    private int response_error = 0;
    private int alert_num = 0;
    private int temp_alert_num = 0;
    private int request_enable = 1;
    private String alert_string;
    private Thread thread;
    private String request_url;
    private Vibrator vibrator;
    private ConnectivityManager connectivityManager;

    public void do_vibrate()
    {
        vibrator = (Vibrator)getSystemService(Context.VIBRATOR_SERVICE);
        long [] pattern = {100,1000,100,1000}; // 停止 开启 停止 开启
        vibrator.vibrate(pattern, -1);
        alert_num++;
    }

    public boolean do_ping()
    {
        try {
            Process p = Runtime.getRuntime().exec("ping -c 2 -w 1 223.5.5.5");
            // PING的状态
            int status = p.waitFor();
            if (status == 0) {
                return  true;
            } else {
                return false;
            }
        } catch (Exception e) {
            return false;
        }

    }
    private Handler mHandler = new Handler(Looper.myLooper()){
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            if (msg.what == 0){
                String response_json = msg.obj.toString();
                try {
                    JSONObject jsonObject=new JSONObject(response_json);
                    if (jsonObject.getInt("code") == 0)
                    {
                        JSONArray jsonArray = jsonObject.getJSONArray("last_data");
                        for(int i=0;i<jsonArray.length();i++){
                            JSONObject item = jsonArray.getJSONObject(i);
                            float value =0f;
                            SensorData sensorData = hashMap.get(item.getString("client_mac")+
                                    item.getString("sensor_pin"));
                            if (sensorData == null) {
                                return;
                            }
                            if (sensorData.timeout_minute > 0) {
                                long start_time =  new SimpleDateFormat("yyyy-MM-dd HH:mm:ss").parse(item.getString("capture_time")).getTime()/1000;
                                long end_time = System.currentTimeMillis() / 1000;
                                if (end_time-start_time > sensorData.timeout_minute*60) {
                                    do_vibrate();
                                    alert_string = msg.obj.toString()+new Date().toString();
                                }
                            }
                            if (item.has("temper")) {
                                value = Float.parseFloat(item.getString("temper"));
                            }else if(item.has("air_pressure")) {
                                value = Float.parseFloat(item.getString("air_pressure"));
                            }
                            if (sensorData.min_value > value || sensorData.max_value < value){
                                do_vibrate();
                                alert_string = msg.obj.toString()+new Date().toString();
                            }
                        }

                    }
                    else{
                        response_error++;
                        do_vibrate();
                        alert_string = msg.obj.toString()+new Date().toString();
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    response_error++;
                    do_vibrate();
                    alert_string = msg.obj.toString()+new Date().toString();
                }
            }
            String show_text = "success:"+response_success+" error:"+response_error+" alert:"+alert_num;
            notification = builder.setContentText(show_text).build();
            NotificationManager mNotificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            mNotificationManager.notify(1, notification);
            Intent intent = new Intent("com.example.http_response.broadcast");
            intent.putExtra("type", msg.what);
            intent.putExtra("response_success", response_success);
            intent.putExtra("response_error", response_error);
            intent.putExtra("alert_num", alert_num);

            intent.putExtra("data", msg.obj.toString());
            intent.putExtra("url", request_url);
            intent.putExtra("alert_string", alert_string);
            sendBroadcast(intent);
        }
    };

    @Override
    public IBinder onBind(Intent intent) {
        // TODO: Return the communication channel to the service.
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public void onCreate() {
        super.onCreate();

        //创建通知
        NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            channel = new NotificationChannel("channel_id", "通知", NotificationManager.IMPORTANCE_DEFAULT);
            notificationManager.createNotificationChannel(channel);
        }
        Intent intent = new Intent(this, MainActivity.class);

        PendingIntent intentPend = PendingIntent.getActivity(this, 0, intent,PendingIntent.FLAG_CANCEL_CURRENT);
        builder = new NotificationCompat.Builder(this, "channel_id");
        notification = builder.setSmallIcon(R.drawable.ic_launcher_background)
                .setContentTitle("这是通知标题")
                .setContentText("这是通知内容")
                .setContentIntent(intentPend)
                .build();
        startForeground(1, notification);
        registerReceiver(broadcastReceiver, new IntentFilter("com.example.zc.broadcast"));

        thread = new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    try {
                        Thread.sleep(10000);
                        if (request_enable == 0)
                            continue;
                        JSONArray table_array = new JSONArray();
                        for (SensorData value : hashMap.values()) {
                            if (value.enable.equals("enable")) {
                                JSONObject jsonObject = new JSONObject();
                                jsonObject.put("client_mac", value.client_mac);
                                jsonObject.put("sensor_pin", value.sensor_pin);
                                jsonObject.put("table", value.table);
                                table_array.put(jsonObject);
                            }
                        }
                        String url = "";
                        JSONObject jsonRoot = new JSONObject();
                        jsonRoot.put("table_array", table_array);
                        url = "http://124.222.150.248:32842/portal_cgi?opt=query_last_data&table_list=" + jsonRoot.toString();
                        request_url = url;
                        Log.d("zc", "thread: "+jsonRoot.toString());
                        String response = NetUtil.doGet(url);

                        Message msg = new Message();
                        msg.what = 0;
                        msg.obj = response;
                        mHandler.sendMessage(msg);
                        response_success++;
                        temp_alert_num = 0;

                    } catch (Exception e) {
                        e.printStackTrace();
                        Message msg = new Message();
                        msg.what = 1;

                        connectivityManager=(ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
                        if (connectivityManager.getActiveNetwork() == null)
                        {
                            msg.obj = "network is disabled";
                        }else {
                            if (connectivityManager.getActiveNetworkInfo().isAvailable() && do_ping()) {
                                temp_alert_num++;
                                msg.obj = "network is available but connect error";
                                if (temp_alert_num > 2) {
                                    do_vibrate();
                                    alert_string = msg.obj.toString()+new Date().toString();
                                }

                            } else {
                                msg.obj = "network is unavailable";
                            }
                        }
                        mHandler.sendMessage(msg);
                        response_error++;
                    }
                }
            }
        });
        thread.start();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }
}