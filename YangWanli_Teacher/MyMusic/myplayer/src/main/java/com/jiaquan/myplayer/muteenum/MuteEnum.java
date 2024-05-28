package com.jiaquan.myplayer.muteenum;

/**
 * 声道枚举:右声道、左声道、立体声
 */
public enum MuteEnum {
    MUTE_RIGHT("RIGHT", 0),
    MUTE_LEFT("LEFT", 1),
    MUTE_CENTER("CENTER", 2);

    private String name;
    private int value;

    MuteEnum(String name, int value) {
        this.name = name;
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
