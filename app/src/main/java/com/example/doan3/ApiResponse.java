package com.example.doan3;

import com.google.gson.annotations.SerializedName;

public class ApiResponse {
    @SerializedName("response")
    private String response;

    public String getResponse() {
        return response;
    }
}
