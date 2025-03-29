package com.example.doan3;

import retrofit2.Call;
import retrofit2.http.Body;
import retrofit2.http.GET;
import retrofit2.http.Headers;
import retrofit2.http.POST;

public interface ApiService {
    @POST("/ask")
    Call<ApiResponse> askServer(@Body RequestBody requestBody);

    @POST("/ask-energy")
    Call<EnergyResponse> getEnergyAdvice();
}
