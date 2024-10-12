package com.example.client.utils;

public class MFrame {
    public int _id;
    public int _type;
    public int _length;
    public byte[] _buf;

    public static int FRAME_HEADER_SIZE = 8;
    public static int FRAME_BUF_SIZE = 1016;
    public static int FRAME_SIZE = 1024;
    public static int FRAME_TYPE_START = 1;
    public static int FRAME_TYPE_END = 2;
    public static int FRAME_TYPE_DATA = 3;
    public static int FRAME_TYPE_ACK = 4;

    public MFrame() {

    }

    public MFrame fromBytes(byte[] _data) {
        if (_data == null) throw new RuntimeException();
        if (_data.length < FRAME_HEADER_SIZE) throw new RuntimeException();
        this._id = bytesToInt32LittleEndian(_data, 0);
        this._type = bytesToInt16LittleEndian(_data, 4);
        this._length = bytesToInt32LittleEndian(_data, 6);
        this._buf = new byte[FRAME_SIZE];
        System.arraycopy(_data, FRAME_HEADER_SIZE, this._buf, 0, this._length);
        return this;
    }

    public static int bytesToInt16LittleEndian(byte[] bytes, int offset) {
        return ((bytes[offset + 1] << 8) | (bytes[offset]));
    }

    public static int bytesToInt32LittleEndian(byte[] bytes, int offset) {
        return ((bytes[offset + 3] << 24) | (bytes[offset + 2] << 16) | (bytes[offset + 1] << 8) | (bytes[offset]));
    }

    public static void int16ToBytesLittleEndian(byte[] _data, int offset, int val){
        _data[offset] = (byte) ((val) & 0xff);
        _data[offset+1] = (byte) ((val >>> 8) & 0xff);
    }

    public static void int32ToBytesLittleEndian(byte[] _data, int offset, int val){
        _data[offset] = (byte) ((val) & 0xff);
        _data[offset+1] = (byte) ((val >>> 8) & 0xff);
        _data[offset+2] = (byte) ((val >>> 16) & 0xff);
        _data[offset+3] = (byte) ((val >>> 24) & 0xff);
    }

    public byte[] toBytes() {
        byte[] ret = new byte[FRAME_SIZE];
        int32ToBytesLittleEndian(ret, 0, this._id);
        int16ToBytesLittleEndian(ret, 4, this._type);
        int16ToBytesLittleEndian(ret, 6, this._length);
        System.arraycopy(this._buf, 0, ret, FRAME_HEADER_SIZE, this._length);
        return ret;
    }

    public int get_id() {
        return _id;
    }

    public int get_length() {
        return _length;
    }

    public byte[] get_buf() {
        return _buf;
    }

    public int get_type() {
        return _type;
    }

    public MFrame set_id(int _id){
        this._id = _id;
        return this;
    }

    public MFrame set_length(int _length){
        this._length = _length;
        return this;
    }

    public MFrame set_type(int _type){
        this._type = _type;
        return this;
    }

    public MFrame set_buf(byte[] _buf){
        this._buf = _buf;
        return this;
    }
}
