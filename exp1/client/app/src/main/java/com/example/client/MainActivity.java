package com.example.client;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.example.client.utils.MSocketClient;

import java.io.IOException;

public class MainActivity extends AppCompatActivity {
    // 请求码
    private static final int REQUEST_CODE_SAVE_FILE = 1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        EdgeToEdge.enable(this);
        setContentView(R.layout.activity_main);
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main), (v, insets) -> {
            Insets systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars());
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom);
            return insets;
        });

        Button recvBtn = findViewById(R.id.recvBtn);
        recvBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                // 启动文件选择器
                Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
                intent.addCategory(Intent.CATEGORY_OPENABLE);
                intent.setType("application/octet-stream"); // 例如，这里设置为二进制流类型
                intent.putExtra(Intent.EXTRA_TITLE, "MyFile.txt"); // 指定文件名
                startActivityForResult(intent, REQUEST_CODE_SAVE_FILE);
            }
        });
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent resultData) {
        Log.d("MainActivity", "enter onActivityResult");
        super.onActivityResult(requestCode, resultCode, resultData);

        if (requestCode == REQUEST_CODE_SAVE_FILE && resultCode == RESULT_OK) {
            if (resultData != null) {
                Uri uri = resultData.getData();
                Log.d("MainActivity", "file uri : " + uri.getPath());
                Uri finalUri = uri;
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        MSocketClient msc = null;
                        try {
                            msc = new MSocketClient("station.killuayz.top", 20000);
                            msc.receive(finalUri.getPath());
                        } catch (IOException e) {
                            e.printStackTrace();
                        }
                    }
                }).start();
            }
        }
    }}