package com.twobit.gtmobile;

import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.core.content.FileProvider;

public class ExportFileProvider extends FileProvider {
    @Override
    public String getType(@NonNull Uri uri) {
        String name = uri.getLastPathSegment();
        if (name != null) return MainActivity.getMimeFromName(name);
        return super.getType(uri);
    }
}
