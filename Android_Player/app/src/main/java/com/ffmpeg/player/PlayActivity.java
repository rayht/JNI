package com.ffmpeg.player;

import android.content.res.Configuration;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;
import android.widget.SeekBar;
import android.widget.Toast;

import com.trello.rxlifecycle2.components.support.RxAppCompatActivity;

public class PlayActivity extends RxAppCompatActivity implements SeekBar.OnSeekBarChangeListener {
    private FfmpegPlayer player;
    public String url;

    private SeekBar seekBar;

    private int progress;
    private boolean isTouch;
    private boolean isSeek;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager
                .LayoutParams.FLAG_KEEP_SCREEN_ON);
        setContentView(R.layout.activity_play);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        player = new FfmpegPlayer();
        player.setSurfaceView(surfaceView);
        player.setOnPrepareListener(new FfmpegPlayer.OnPrepareListener() {
            @Override
            public void onPrepared() {
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Toast.makeText(PlayActivity.this, "开始播放", Toast.LENGTH_SHORT).show();
                    }
                });

                //获得时间
                int duration = player.getDuration();
                //直播： 时间就是0
                if (duration != 0){
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            //显示进度条
                            seekBar.setVisibility(View.VISIBLE);
                        }
                    });
                }
                player.start();
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
                                if (isSeek){
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


        seekBar = findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(this);
//        url = getIntent().getStringExtra("url");
        player.setDataSource("/sdcard/b.mp4");
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (newConfig.orientation == Configuration.ORIENTATION_LANDSCAPE) {
            getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager
                    .LayoutParams.FLAG_FULLSCREEN);
        } else {
            getWindow().clearFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        }
        setContentView(R.layout.activity_play);
        SurfaceView surfaceView = findViewById(R.id.surfaceView);
        player.setSurfaceView(surfaceView);

        seekBar = findViewById(R.id.seekBar);
        seekBar.setOnSeekBarChangeListener(this);
        seekBar.setProgress(progress);
    }

    @Override
    protected void onResume() {
        super.onResume();
        player.prepare();
    }

    @Override
    protected void onStop() {
        super.onStop();
        player.stop();
    }

    @Override
    protected void onDestroy() {
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
