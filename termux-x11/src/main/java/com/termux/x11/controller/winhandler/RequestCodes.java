package com.termux.x11.controller.winhandler;

abstract class RequestCodes {
    public static final byte EXIT = 0;
    public static final byte INIT = 1;
    public static final byte EXEC = 2;
    public static final byte KILL_PROCESS = 3;
    public static final byte LIST_PROCESSES = 4;
    public static final byte GET_PROCESS = 5;
    public static final byte SET_PROCESS_AFFINITY = 6;
    public static final byte MOUSE_EVENT = 7;
    public static final byte GET_GAMEPAD = 8;
    public static final byte GET_GAMEPAD_STATE = 9;
    public static final byte RELEASE_GAMEPAD = 10;
    public static final byte KEYBOARD_EVENT = 11;
    public static final byte BRING_TO_FRONT = 12;
    public static final byte CURSOR_POS_FEEDBACK = 13;
}