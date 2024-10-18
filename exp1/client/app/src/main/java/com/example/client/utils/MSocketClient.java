package com.example.client.utils;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.net.Socket;
import com.example.client.utils.MBinaryStream;

public class MSocketClient {
    private Socket socket;

    public MSocketClient(String ip, int port) throws IOException {
        socket = new Socket(ip, port);
        System.out.println("Connect to Server "+ip+":"+port);
    }

    private void ack() throws IOException {
        if(socket == null) throw new NullPointerException();
        OutputStream out = socket.getOutputStream();
        MFrame ack_frame = new MFrame();
        ack_frame.set_type(MFrame.FRAME_TYPE_ACK);
        out.write(ack_frame.toBytes());
    }

    public void receive(String file_path) throws NullPointerException, IOException {
        if(socket == null) throw new NullPointerException();
        OutputStream out = socket.getOutputStream();
        MFrame mFrame = new MFrame();
        mFrame.set_type(MFrame.FRAME_TYPE_REQUEST_DATA);
        out.write(mFrame.toBytes());

        byte[] buffer = new byte[1024];
        InputStream in = socket.getInputStream();
        MBinaryStream mbs = new MBinaryStream(file_path);
        while(mbs.has_next()) {
            int readSize = in.read(buffer);
            mbs.next(buffer);
            ack();
        }
    }

    public void send(String file_path) throws IOException {
        if(socket == null) throw new NullPointerException();
        OutputStream out = socket.getOutputStream();
        MBinaryStream mbs = new MBinaryStream(file_path);
        while(mbs.has_next()){
            byte[] resp = mbs.next();
            out.write(resp);
        }
    }
}
