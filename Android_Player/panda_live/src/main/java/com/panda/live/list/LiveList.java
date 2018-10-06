package com.panda.live.list;

public class LiveList {

    private int errno;
    private String errmsg;
    private Data data;

    public void setErrno(int errno) {
        this.errno = errno;
    }

    public int getErrno() {
        return errno;
    }

    public void setErrmsg(String errmsg) {
        this.errmsg = errmsg;
    }

    public String getErrmsg() {
        return errmsg;
    }

    public void setData(Data data) {
        this.data = data;
    }

    public Data getData() {
        return data;
    }


}