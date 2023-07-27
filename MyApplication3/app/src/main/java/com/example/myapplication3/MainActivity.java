package com.example.myapplication3;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.widget.AutoCompleteTextView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.GridLayout;
import android.widget.TextView;
import android.widget.Toast;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;

public class MainActivity extends AppCompatActivity {
    private String zc_test = "";
    private HashMap<String, SensorData> hashMap = new HashMap<>();
    private int dataNum = 3;
    private int id_offset = 3;
    private GridLayout gridLayout;
    private SharedPreferences sp;
	private int response_success = 0;
	private int response_error = 0;
	private String request_url;
    private BroadcastReceiver broadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Message msg = new Message();
            msg.what = intent.getIntExtra("type", 1);
            msg.obj = intent.getStringExtra("data");

            mHandler.sendMessage(msg);
            response_success = intent.getIntExtra("response_success", response_success);
            response_error = intent.getIntExtra("response_error", response_error);
            request_url = intent.getStringExtra("url");
        }
    };


    private Handler mHandler = new Handler(Looper.myLooper()){
        @Override
        public void handleMessage(@NonNull Message msg) {
            super.handleMessage(msg);
            TextView tvHttpResponse=findViewById(R.id.tvHttpResponse);

            tvHttpResponse.setText(response_success+":"+response_error+" "+msg.obj.toString());
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
                            if (item.has("temper")) {
                                value = Float.parseFloat(item.getString("temper"));
                            }else if(item.has("air_pressure")){
                                value = Float.parseFloat(item.getString("air_pressure"));
                            }
                            TextView tv=findViewById(sensorData.show_value_id);
                            tv.setText(value+"");
                            if (sensorData.min_value > value || sensorData.max_value < value){
                                findViewById(sensorData.alert_id).setBackgroundColor(Color.RED);
                            }
                        }

                    }
                    else{
                        response_error++;
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                    response_error++;
                }
            }
        }
    };
    public void freshMap(){
        hashMap.clear();
        SharedPreferences sp = getPreferences(MODE_PRIVATE);
        for(int i=0;i<dataNum;i++) {
            SensorData sensorData = new SensorData();
            sensorData.client_mac = sp.getString(new Integer(i*100*2+0+id_offset).toString(), "null");
            sensorData.sensor_pin = sp.getString(new Integer(i*100*2+1+id_offset).toString(), "null");
            sensorData.table = sp.getString(new Integer(i*100*2+2+id_offset).toString(), "null");
            sensorData.min_value = Integer.parseInt(sp.getString(new Integer(i*100*2+3+id_offset).toString(), "0"));
            sensorData.max_value = Integer.parseInt(sp.getString(new Integer(i*100*2+4+id_offset).toString(), "0"));
            sensorData.enable = sp.getString(new Integer((i*2+1)*100+3+id_offset).toString(), "disable");
            sensorData.alert_id = (i*2+1)*100+1+id_offset;
            sensorData.show_value_id = (i*2+1)*100+4+id_offset;
            hashMap.put(sensorData.client_mac+sensorData.sensor_pin, sensorData);
        }
        Intent intent = new Intent("com.example.zc.broadcast");
        SerialHashMap serialHashMap = new SerialHashMap();
        serialHashMap.setMap(hashMap);
        Bundle bundle = new Bundle();
        bundle.putSerializable("map", serialHashMap);
        intent.putExtras(bundle);
        sendBroadcast(intent);
    }
    public GridLayout.LayoutParams getGridSpec(int row, int col){
        //使用Spec定义子控件的位置和比重
        GridLayout.Spec rowSpec = GridLayout.spec(row,1f);
        GridLayout.Spec columnSpec = GridLayout.spec(col,1f);
        //将Spec传入GridLayout.LayoutParams并设置宽高为0，必须设置宽高，否则视图异常
        GridLayout.LayoutParams layoutParams = new GridLayout.LayoutParams(rowSpec, columnSpec);
        layoutParams.height = 0;
        layoutParams.width = 0;
        return layoutParams;
    }
    public void createTextView(int controlId, String text, int row, int col) {
        AutoCompleteTextView editText = new AutoCompleteTextView(this);
        editText.setId(controlId);
        editText.setLayoutParams(getGridSpec(row, col));
        editText.setText(text);
        gridLayout.addView(editText);
    }
    public void createLineButton(int row) {
        Button saveButton = new Button(this);
        saveButton.setText("保存");
        saveButton.setId(row*100+id_offset);
        saveButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //提交数据
                commitData(v);
            }
        });
        saveButton.setLayoutParams(getGridSpec(row, 0));
        gridLayout.addView(saveButton);
        Button alertButton = new Button(this);
        alertButton.setText("告警");
        alertButton.setId(row*100+1+id_offset);
        alertButton.setLayoutParams(getGridSpec(row, 1));
        gridLayout.addView(alertButton);
        Button resetButton = new Button(this);
        resetButton.setText("重置");
        resetButton.setId(row*100+2+id_offset);
        resetButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //提交数据
                resetData(v);
            }
        });
        resetButton.setLayoutParams(getGridSpec(row, 2));
        gridLayout.addView(resetButton);
        Button enableButton = new Button(this);
        enableButton.setText(sp.getString(new Integer(row*100+3+id_offset).toString(), "disable"));
        enableButton.setId(row*100+3+id_offset);
        enableButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //提交数据
                enableData(v);
            }
        });
        enableButton.setLayoutParams(getGridSpec(row, 3));
        gridLayout.addView(enableButton);
        TextView tv = new TextView(this);
        tv.setText("0");
        tv.setId(row*100+4+id_offset);
        tv.setLayoutParams(getGridSpec(row, 4));
        gridLayout.addView(tv);
    }

    @SuppressLint("ResourceType")
    public void createMainButton(int row) {
        Button saveButton = new Button(this);
        saveButton.setText("URL");
        saveButton.setId(9000);
        saveButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //提交数据
                showURL(v);
            }
        });
        saveButton.setLayoutParams(getGridSpec(row, 0));
        gridLayout.addView(saveButton);
        Button alertButton = new Button(this);
        alertButton.setText("告警");
        alertButton.setId(9001);
        alertButton.setLayoutParams(getGridSpec(row, 1));
        gridLayout.addView(alertButton);
        Button resetButton = new Button(this);
        resetButton.setText("重置");
        resetButton.setId(9002);
        resetButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //提交数据
                resetData(v);
            }
        });
        resetButton.setLayoutParams(getGridSpec(row, 2));
        gridLayout.addView(resetButton);
        Button enableButton = new Button(this);
        enableButton.setText(sp.getString("9003", "disable"));
        enableButton.setId(9003);
        enableButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                //提交数据
                enableData(v);
            }
        });
        enableButton.setLayoutParams(getGridSpec(row, 3));
        gridLayout.addView(enableButton);
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.gridlayout);
        sp = getPreferences(MODE_PRIVATE);
        gridLayout = findViewById(R.id.gridlayout);
        for(int i=0; i< dataNum; i++) {
            for(int j=0;j < 5; j++) {
                //存储的key就是view的id
                String text = sp.getString(new Integer(i*100*2+j+id_offset).toString(), "0");
                createTextView(i*2*100+j+id_offset, text, i*2, j);
            }
            createLineButton(i*2+1);
        }
        registerReceiver(broadcastReceiver, new IntentFilter("com.example.http_response.broadcast"));
        createMainButton(10);
        Intent intent = new Intent(this, MyService.class);
        startService(intent);
/*
        Thread thread = new Thread(new Runnable() {
            @Override
            public void run() {
                while (true) {
                    try {
                        Thread.sleep(5000);
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
                        url = "http://192.168.10.105/portal_cgi?opt=query_last_data&table_list=" + jsonRoot.toString();

                        String response = NetUtil.doGet(url);
						request_url = url;
                        Message msg = new Message();
                        msg.what = 0;
                        msg.obj = response;
                        mHandler.sendMessage(msg);
						response_success++;

                    } catch (Exception e) {
                        e.printStackTrace();
                        String test = "get data error";
                        Message msg = new Message();
                        msg.what = 1;
                        msg.obj = test;
                        mHandler.sendMessage(msg);
						response_error++;
                    }
                }
            }
        });
       thread.start();
 */
        new Handler().postDelayed(new Runnable() {
            public void run() {
                freshMap();
            }}, 5000);
    }
    public void commitData(View view) {
        //Toast.makeText(this, "in displa"+view.getId(), Toast.LENGTH_SHORT).show();
        int baseId = (int) (((view.getId()-id_offset)*0.01-1)*100);
        SharedPreferences.Editor editor = sp.edit();
        TextView txtData;
        for (int j=0;j<5;j++){
            txtData = findViewById(baseId+j+id_offset);
            editor.putString(new Integer(baseId+j+id_offset).toString(), txtData.getText().toString());
        }
        editor.commit();
        freshMap();
        Toast.makeText(this, "save success", Toast.LENGTH_SHORT).show();
    }
    public void resetData(View view) {
        @SuppressLint("ResourceType") int alert_id = view.getId() - 1;
        findViewById(alert_id).setBackgroundColor(Color.GREEN);
    }
	public void showURL(View view) {
        AlertDialog.Builder builder = new AlertDialog.Builder(MainActivity.this);
        builder.setMessage(request_url);
        builder.show();
    }
    public void enableData(View view) {
        Button button = (Button) view;
        if(button.getText().toString().equals("disable"))
            button.setText("enable");
        else if(button.getText().toString().equals("enable"))
            button.setText("disable");
        SharedPreferences.Editor editor = sp.edit();
        editor.putString(new Integer(button.getId()).toString(), button.getText().toString());
        editor.commit();
        freshMap();
        Toast.makeText(this, "save success "+new Integer(button.getId()).toString()+button.getText().toString(), Toast.LENGTH_SHORT).show();
    }
}