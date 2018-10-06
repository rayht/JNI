package com.ffmpeg.player;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class FfmpegPlayer implements SurfaceHolder.Callback {

    static {
        System.loadLibrary("native-lib");
    }

    /**
     * 显示视频的SurfaceView
     */
    private SurfaceView surfaceView;

    /**
     * 数据源(本地地址或者网络直播流)
     */
    private String dataSource;

    /**
     * SurfaceView对应的SurfaceHolder
     */
    private SurfaceHolder surfaceHolder;

    private OnPrepareListener onPrepareListener;

    private OnErrorListener onErrorListener;

    private OnProgressListener onProgressListener;

    public void prepare() {
        native_prepare(dataSource);
    }

    public void onError(int errorCode) {
//        stop();
        if (null != onErrorListener) {
            onErrorListener.onError(errorCode);
        }
    }

    public void onPrepare() {
        Log.d("HONGTAI", "onPrepare...");
        if (null != onPrepareListener) {
            onPrepareListener.onPrepared();
        }
    }

    /**
     * native 回调给java 播放进去的
     *
     * @param progress
     */
    public void onProgress(int progress) {
        if (null != onProgressListener) {
            onProgressListener.onProgress(progress);
        }
    }

    public void setSurfaceView(SurfaceView surfaceView) {
        if (this.surfaceHolder != null) {
            this.surfaceHolder.removeCallback(this);
        }
        this.surfaceHolder = surfaceView.getHolder();
        native_set_surface(surfaceHolder.getSurface());
        this.surfaceHolder.addCallback(this);
    }

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    public void start() {
        native_start();
    }

    public void stop() {
        native_stop();
    }

    public int getDuration() {
        return native_getDuration();
    }

    public void seek(final int progress) {
        new Thread() {
            @Override
            public void run() {
                native_seek(progress);
            }
        }.start();
    }

    public void release() {
        if (null != this.surfaceHolder) {
            this.surfaceHolder.removeCallback(this);
        }
        native_release();
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {
        native_set_surface(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
    }

    private native void native_start();

    private native void native_prepare(String dataSource);

    private native void native_set_surface(Surface surface);

    private native void native_stop();

    private native void native_release();

    private native int native_getDuration();

    private native void native_seek(int progress);

    public void setOnPrepareListener(OnPrepareListener onPrepareListener) {
        this.onPrepareListener = onPrepareListener;
    }

    public void setOnErrorListener(OnErrorListener onErrorListener) {
        this.onErrorListener = onErrorListener;
    }

    public void setOnProgressListener(OnProgressListener onProgressListener) {
        this.onProgressListener = onProgressListener;
    }

    public interface OnPrepareListener {
        void onPrepared();
    }

    public interface OnErrorListener {
        void onError(int error);
    }

    public interface OnProgressListener {
        void onProgress(int progress);
    }
}
