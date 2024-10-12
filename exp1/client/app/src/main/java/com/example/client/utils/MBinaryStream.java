package com.example.client.utils;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

public class MBinaryStream {
    private String filePath;
    private int _cur;
    private int _ext_size;
    private int _size;
    private int _byte_size;
    private FileOutputStream outfile;
    private FileInputStream infile;

    public MBinaryStream(String filePath){
        this.filePath = filePath;
        this._cur = 0;
        this._ext_size = 0;
        this._size = 0;
        this._byte_size = 0;
    }

    private void first(byte[] data) throws FileNotFoundException {
        MFrame mframe = new MFrame().fromBytes(data);
        if(mframe.get_type() != MFrame.FRAME_TYPE_START) throw new RuntimeException();
        if(mframe.get_id() != 0) throw new RuntimeException();
        this._cur = 0;
        this._size = mframe.get_length();
        this._ext_size = this._size + 2;
        this.outfile = new FileOutputStream(this.filePath);
    }

    private void mid (byte[] data) throws IOException {
        MFrame mframe = new MFrame().fromBytes(data);
        if(mframe.get_type() != MFrame.FRAME_TYPE_DATA) throw new RuntimeException();
        outfile.write(mframe.get_buf(), 0, mframe.get_length());
    }

    private void end(byte[] data) throws IOException {
        MFrame mframe = new MFrame().fromBytes(data);
        if(mframe.get_type() != MFrame.FRAME_TYPE_END) throw new RuntimeException();
        if(mframe.get_id() != this._ext_size - 1) throw new RuntimeException();
        this.outfile.close();
    }

    private byte[] first() throws FileNotFoundException {
        File file = new File(this.filePath);
        long fileSize = file.length();
        this._cur = 0;
        this._byte_size = (int) fileSize;
        this._size = (int) Math.ceil((double) fileSize / MFrame.FRAME_BUF_SIZE);
        this._ext_size = this._size + 2;
        this.infile = new FileInputStream(this.filePath);

        MFrame mframe = new MFrame()
                .set_id(0)
                .set_type(MFrame.FRAME_TYPE_START)
                .set_length(this._size);
        return mframe.toBytes();
    }

    private byte[] mid() throws IOException {
        byte[] _bytes = new byte[MFrame.FRAME_BUF_SIZE];
        int readSize = this.infile.read(_bytes, 0, MFrame.FRAME_BUF_SIZE);

        MFrame mframe = new MFrame()
                .set_id(this._cur)
                .set_type(MFrame.FRAME_TYPE_DATA)
                .set_length(readSize)
                .set_buf(_bytes);

        return mframe.toBytes();
    }

    private byte[] end() throws IOException {
        infile.close();
        MFrame mframe = new MFrame()
                .set_id(this._ext_size - 1)
                .set_type(MFrame.FRAME_TYPE_END)
                .set_length(0);
        return mframe.toBytes();
    }

    public byte[] next() throws IOException {
        byte[] ret;
        if(this._cur == 0){
           ret = first();
        }else if(this._cur == _ext_size - 1){
            ret = end();
        }else{
            ret = mid();
        }
        this._cur++;
        return ret;
    }

    public void next(byte[] _bytes) throws IOException {
        if(this._cur == 0){
            first(_bytes);
        }else if(this._cur == _ext_size - 1){
            end(_bytes);
        }else{
            mid(_bytes);
        }
        this._cur++;
    }
}
