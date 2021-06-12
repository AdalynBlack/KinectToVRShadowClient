#pragma comment(lib, "wsock32.lib")

#include <Windows.h> //MUST BE BEFORE NUI
#include <ole2.h>

#include <NuiApi.h>
#include <NuiImageCamera.h>
#include <NuiSensor.h>
#include <NuiSkeleton.h>

#include <stdio.h>
#include <string.h>

#include "KinectV1Handler.h"

#define PORT 7063
#define PACKETSIZE sizeof(NUI_SKELETON_FRAME)

class Networking {
public:
    Networking()
    {
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;

        wVersionRequested = MAKEWORD(2, 2);

        err = WSAStartup(wVersionRequested, &wsaData);
        if (err != 0) {
            printf("\nWSAStartup failed with error: %d\n", err);
            return;
        }

        //const char* hello = "Hello from client";
        //char buffer[1024] = { 0 };

        printf("Please input the ip you wish to connect to (i.e. 192.168.2.2): ");
        std::cin >> ip;

        if (connectToIP())
        {
            printf("Connection Successful\n");
        }
        else {
            printf("Connection Failed\n");
        }

        // Convert IPv4 and IPv6 addresses from text to binary form
        /*if (InetPton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
        {
            printf("\nInvalid address/ Address not supported \n");
            return -1;
        }*/
    }

    void sendSkeleton(NUI_SKELETON_FRAME frame)
    {

        //NUI_SKELETON_FRAME defaultSkeleton;
        char skeleBuf[PACKETSIZE];

        serializeFrame(&frame, skeleBuf);

        int i = 0;
        while (i < PACKETSIZE)
        {
            const int l = send(sock, &skeleBuf[i], __min(1024, PACKETSIZE - i), 0);
            if (l < 0) { connected = false; printf("\nDisconnected\n"); closesocket(sock); return; }
            i += l;
        }
    }

    void closeSocket()
    {
        closesocket(sock);
        WSACleanup();
    }

    bool connectToIP()
    {
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            printf("\n Socket creation error: %i \n", sock);
            closesocket(sock);
            return false;
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        serv_addr.sin_addr.s_addr = inet_addr(ip.c_str());

        u_long block = 1;
        if (ioctlsocket(sock, FIONBIO, &block) == SOCKET_ERROR)
        {
            return false;
        }

        if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
        {
            if (WSAGetLastError() != WSAEWOULDBLOCK)
            {
                closesocket(sock);
                return false;
            }

            fd_set setW, setE;

            FD_ZERO(&setW);
            FD_SET(sock, &setW);
            FD_ZERO(&setE);
            FD_SET(sock, &setE);

            timeval time_out = { 0 };
            time_out.tv_sec = 2;
            time_out.tv_usec = 0;

            int ret = select(0, NULL, &setW, &setE, &time_out);
            if (ret <= 0)
            {
                // select() failed or connection timed out
                closesocket(sock);
                if (ret == 0)
                    WSASetLastError(WSAETIMEDOUT);
                return false;
            }

            if (FD_ISSET(sock, &setE))
            {
                // connection failed
                int err = 0;
                char buf[1024] = { 0 };
                getsockopt(sock, SOL_SOCKET, SO_ERROR, buf, &err);
                closesocket(sock);
                WSASetLastError(err);
                return false;
            }
        }

        connected = true;
        return true;
    }

    bool connected = false;
    std::string ip;
private:
    void serializeFrame(NUI_SKELETON_FRAME* frame, char *skeleBuf)
    {
        int *q = (int*)skeleBuf;
        *q = frame->dwFlags; q++;
        *q = frame->dwFrameNumber; q++;
        *q = frame->liTimeStamp.HighPart; q++;
        *q = frame->liTimeStamp.LowPart; q++;

        serializeVec(&q, &frame->vFloorClipPlane);
        serializeVec(&q, &frame->vNormalToGravity);

        for (NUI_SKELETON_DATA data : frame->SkeletonData)
        {
            serializeData(&q, &data);
        }
    }

    void serializeVec(int** q, Vector4* vec)
    {
        **q = reinterpret_cast<int &>(vec->w); (*q)++;
        **q = reinterpret_cast<int &>(vec->x); (*q)++;
        **q = reinterpret_cast<int &>(vec->y); (*q)++;
        **q = reinterpret_cast<int &>(vec->z); (*q)++;
    }

    void serializeData(int** q, NUI_SKELETON_DATA* data)
    {
        **q = data->dwEnrollmentIndex; (*q)++;
        **q = data->dwQualityFlags; (*q)++;
        **q = data->dwTrackingID; (*q)++;
        **q = data->dwUserIndex; (*q)++;
        **q = data->eTrackingState; (*q)++;
        
        serializeVec(&*q, &data->Position);

        for (int i=0; i < 20; i++)
        {
            **q = data->eSkeletonPositionTrackingState[i]; (*q)++;
            serializeVec(&*q, &data->SkeletonPositions[i]);
        }
    }

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
};

inline void netLoop(Networking& networking, KinectV1Handler& kinect)
{
    kinect.update();
    kinect.initialiseSkeleton();

    while (1) //I don't like this but it is the way that was recommended so why not
    {
        if (networking.connected)
        {
            networking.sendSkeleton(kinect.skeletonFrame);
            Sleep(34);
            kinect.update();
        }
        else {
            printf("\nThe connection has been interrupted, attempting to reconnect.\n\n");
            bool tempCon = false;
            Sleep(5);
            for (int i = 0; i < 10; i++)
            {
                printf("Connection Attempt %d/10: ", i + 1);
                if (!networking.connectToIP())
                {
                    printf("Failed\n");
                }
                else
                {
                    printf("Success\n");
                    tempCon = true;
                    break;
                }
            }

            if (!tempCon) {
                printf("\nUnable to reestablish connection automatically.\nPlease enter the IP you want to connect to: ");
                std::cin >> networking.ip;
                networking.connectToIP();
            }
        }
    }
}