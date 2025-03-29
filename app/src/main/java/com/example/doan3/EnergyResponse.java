package com.example.doan3;

import com.google.gson.annotations.SerializedName;

public class EnergyResponse {

    public String getResponse() {
        return response;
    }

    @SerializedName("response")
    public String response;
}

