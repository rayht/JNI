package com.ffmpeg.player;

import org.junit.Test;

import static org.junit.Assert.*;

/**
 * Example local unit test, which will execute on the development machine (host).
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
public class ExampleUnitTest {
    @Test
    public void addition_isCorrect() {
        System.load("C:\\Users\\Leiht\\CMakeBuilds\\1576c1d5-de6f-1e37-b0be-3763f01ada6c\\build\\x64-Debug\\Lsn_07\\lsn07jni.dll");

        test(100, "java");
    }

    native void test(int i, String j);
}