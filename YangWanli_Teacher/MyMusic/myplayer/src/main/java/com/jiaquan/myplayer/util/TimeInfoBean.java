package com.jiaquan.myplayer.util;

/**
 * 时间信息的封装类
 */
public class TimeInfoBean {
    private int mCurrentTime_ = 0;
    private int mTotalTime_ = 0;

    public int getCurrentTime() {
        return mCurrentTime_;
    }

    public void setCurrentTime(int currentTime) {
        this.mCurrentTime_ = currentTime;
    }

    public int getTotalTime() {
        return mTotalTime_;
    }

    public void setTotalTime(int totalTime) {
        this.mTotalTime_ = totalTime;
    }

    @Override
    public String toString() {
        return "TimeInfoBean{" +
                "mCurrentTime_=" + mCurrentTime_ +
                ", totalTime=" + mTotalTime_ +
                '}';
    }
}
