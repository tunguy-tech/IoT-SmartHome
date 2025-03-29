package com.example.doan3;

import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import android.util.Log;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import retrofit2.Call;
import retrofit2.Callback;
import retrofit2.Response;

import android.speech.tts.TextToSpeech;

import java.util.Locale;

public class MainActivity extends AppCompatActivity {
    private TextView responseText;
    Button btnRefresh, btnRefresh2, btnSpeak, btnSpeak2;
    private TextView responseText2;
    private TextToSpeech textToSpeech;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        responseText = findViewById(R.id.responseText);
        responseText2 = findViewById(R.id.responseText2);
        btnSpeak = findViewById(R.id.btnSpeak);
        btnSpeak2 = findViewById(R.id.btnSpeak2);
        textToSpeech = new TextToSpeech(getApplicationContext(), status -> {
            if (status == TextToSpeech.SUCCESS) {
                textToSpeech.setLanguage(Locale.forLanguageTag("vi-VN")); // Ngôn ngữ Tiếng Viet
            } else {
                Log.e("TTS", "Khởi tạo thất bại!");
            }
        });

        // Gửi yêu cầu đến API
        callApi();

        Button btnRefresh = findViewById(R.id.btnRefresh);
        btnRefresh.setOnClickListener(v -> {
            Intent intent = getIntent();
            finish();
            startActivity(intent);
        });

        btnSpeak.setOnClickListener(v -> {
            String text = responseText.getText().toString();
            speakText(text);
        });

        btnSpeak2.setOnClickListener(v -> {
            String text = responseText2.getText().toString();
            speakText(text);
        });
    }


    private void callApi() {
        ApiService apiService = RetrofitClient.getApiService();
        RequestBody requestBody = new RequestBody("");

        apiService.askServer(requestBody).enqueue(new Callback<ApiResponse>() {
            @Override
            public void onResponse(Call<ApiResponse> call, Response<ApiResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    responseText.setText(response.body().getResponse());
                } else {
                    Log.e("API_ERROR", "Response failed");
                }
            }

            ApiService apiService = RetrofitClient.getApiService();
            @Override
            public void onFailure(Call<ApiResponse> call, Throwable t) {
                Log.e("API_ERROR", "Failed to call API", t);
            }
        });

        apiService.getEnergyAdvice().enqueue(new Callback<EnergyResponse>() {
            @Override
            public void onResponse(@NonNull Call<EnergyResponse> call, @NonNull Response<EnergyResponse> response) {
                if (response.isSuccessful() && response.body() != null) {
                    responseText2.setText("Advice: " + response.body().getResponse());
                } else {
                    Log.e("API_ERROR", "Response failed");
                    responseText2.setText("Failed to get advice.");
                }
            }

            @Override
            public void onFailure(@NonNull Call<EnergyResponse> call, Throwable t) {
                Log.e("API_ERROR", "Failed to call API", t);
                responseText2.setText("Error connecting to server.");
            }
        });
        Button btnRefresh2 = findViewById(R.id.btnRefresh2);
        btnRefresh2.setOnClickListener(v -> {
            Intent intent = getIntent();
            finish();
            startActivity(intent);
        });
        }
    private void speakText(String text) {
        if (!text.isEmpty()) {
            textToSpeech.speak(text, TextToSpeech.QUEUE_FLUSH, null, null);
        }
    }
    @Override
    protected void onDestroy() {
        if (textToSpeech != null) {
            textToSpeech.stop();
            textToSpeech.shutdown();
        }
        super.onDestroy();
    }
    }