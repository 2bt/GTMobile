package com.twobit.gtmobile;

import android.app.Activity;
import android.os.Bundle;


public class MainActivity extends Activity {
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        View mView = new View(getApplication());
        setContentView(mView);


        Native.init(getAssets());
    }
}