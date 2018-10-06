package com.ffmpeg.player;

import android.app.Activity;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;
import android.widget.SeekBar;
import android.widget.Toast;


public class MainActivity_ extends Activity implements SeekBar.OnSeekBarChangeListener {

    private FfmpegPlayer player;

    private String url = "http://pl3.live.panda.tv/live_panda/5522faa581df75a80e04efeb977c3067_mid.flv?sign=f55d1e54c7036778a96af9fc99470689&time=&ts=5bb5cc2d&rid=-27466261";
    private String url1 = "/sdcard/test.mp4";
    private SeekBar seekBar;

    private boolean isTouch;

    private boolean isSeek;

    private int progress;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d("HONGTAI", "onCreate...");
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main_);
        seekBar = findViewById(R.id.seekBar);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        player = new FfmpegPlayer();
        player.setSurfaceView(surfaceView);

        //player.setDataSource("/sdcard/b.mp4");
        player.setDataSource(url);

        player.setOnPrepareListener(new FfmpegPlayer.OnPrepareListener() {
            /**
             * 视频信息获取完成 随时可以播放的时候回调
             */
            @Override
            public void onPrepared() {
//                //获得时间
//                int duration = player.getDuration();
//                //直播： 时间就是0
//                if (duration != 0) {
//                    runOnUiThread(new Runnable() {
//                        @Override
//                        public void run() {
//                            //显示进度条
//                            seekBar.setVisibility(View.VISIBLE);
//                        }
//                    });
//                }
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity_.this, "视频准备好，可以播放", Toast.LENGTH_LONG).show();
                    }
                });
                player.start();
            }
        });
        player.setOnErrorListener(new FfmpegPlayer.OnErrorListener() {
            @Override
            public void onError(final int error) {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(MainActivity_.this,"出错了，error=" + error, Toast.LENGTH_LONG).show();
                    }
                });
            }
        });
        player.setOnProgressListener(new FfmpegPlayer.OnProgressListener() {

            @Override
            public void onProgress(final int progress2) {
                if (!isTouch) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            int duration = player.getDuration();
                            //如果是直播
                            if (duration != 0) {
                                if (isSeek) {
                                    isSeek = false;
                                    return;
                                }
                                //更新进度 计算比例
                                seekBar.setProgress(progress2 * 100 / duration);
                            }
                        }
                    });
                }
            }
        });
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        Log.d("HONGTAI", "onConfigurationChanged...");
        super.onConfigurationChanged(newConfig);
        setContentView(R.layout.activity_main_);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        player.setSurfaceView(surfaceView);
        player.setDataSource(url);

        seekBar = findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(this);
        seekBar.setProgress(progress);
    }

    @Override
    protected void onResume() {
        Log.d("HONGTAI", "onResume...");
        super.onResume();
        player.prepare();
    }

    @Override
    protected void onStop() {
        Log.d("HONGTAI", "onStop...");
        super.onStop();
        player.stop();
    }

    @Override
    protected void onDestroy() {
        Log.d("HONGTAI", "onDestroy...");
        super.onDestroy();
        player.release();
    }

    @Override
    public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {

    }

    @Override
    public void onStartTrackingTouch(SeekBar seekBar) {
        isTouch = true;
    }

    /**
     * 停止拖动的时候回调
     *
     * @param seekBar
     */
    @Override
    public void onStopTrackingTouch(SeekBar seekBar) {
        isSeek = true;
        isTouch = false;
        progress = player.getDuration() * seekBar.getProgress() / 100;
        //进度调整
        player.seek(progress);
    }

}
