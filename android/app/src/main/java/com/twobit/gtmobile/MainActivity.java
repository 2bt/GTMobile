package com.twobit.gtmobile;

import android.app.Activity;
import android.os.Bundle;


public class MainActivity extends Activity {
    View mView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mView = new View(getApplication());
        setContentView(mView);
    }
    @Override
    protected void onResume() {
        super.onResume();
        mView.onResume();
        Native.startAudio();
    }
    @Override
    protected void onPause() {
        super.onPause();
        mView.onPause();
        Native.stopAudio();
    }

}