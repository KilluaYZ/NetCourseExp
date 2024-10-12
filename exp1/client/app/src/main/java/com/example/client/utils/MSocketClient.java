package com.example.client.utils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class MSocketClient {
    private Socket socket;

    public MSocketClient(String ip, int port) throws IOException {
        socket = new Socket(ip, port);
        System.out.println("Connect to Server "+ip+":"+port);
    }

    public void receive() throws NullPointerException, IOException {
        if(socket == null) throw new NullPointerException();
        BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream()));

    }
}
