#include <iostream>

#include "DataReceiver.hpp"
#include "DrawingFuncs.hpp"
#include <string>
#include <chrono>
#include <thread>
#include <fstream>

using namespace std;

#define WINDOW_WIDTH 19
#define WINDOW_HEIGHT 15

static DumbControllerData drawData;
static float saveInterp = 0;
static float loadInterp = 0;
static bool ending = false;
static string serverIP;
static const float saveLoadDiv = 3.0f;

void WindowDisplay() {

    glClear(GL_COLOR_BUFFER_BIT);
    glLineWidth(5);

    //creating data interpolation for smooth displaying
    DumbControllerData data = g_dataReceiver->GetData();
    float interp = 0.4;
    drawData.movementX += (data.movementX - drawData.movementX) * interp;
    drawData.movementY += (data.movementY - drawData.movementY) * interp;
    drawData.angleX += (data.angleX - drawData.angleX) * interp;
    drawData.angleY += (data.angleY - drawData.angleY) * interp;
    for (int i = 0; i < 5; i++) {
        drawData.digitalAnalogs[i] += (data.digitalAnalogs[i] - drawData.digitalAnalogs[i]) * interp;
    }
    // smooth displaying for save and load bars
    float saveGoal = (data.playerCount == 0) ? 0 : fmin(1, (data.inputCounts[7] / (float)data.playerCount) * saveLoadDiv);
    float loadGoal = (data.playerCount == 0) ? 0 : fmin(1, (data.inputCounts[8] / (float)data.playerCount) * saveLoadDiv);
    saveInterp += (saveGoal - saveInterp) * interp;
    loadInterp += (loadGoal - loadInterp) * interp;

    // draw movement inputs
    DrawRectangle(2, 2, 5, 5, Color(0, 20, 100));
    DrawCircle(4.5, 4.5, 2.5, Color(0, 40, 200), 40);
    DrawLine(4.5, 2, 4.5, 7, Color(0, 20, 100));
    DrawLine(2, 4.5, 7, 4.5, Color(0, 20, 100));

    float movXPos = 4.5 + drawData.movementX * 2.5, movYPos = 4.5 + drawData.movementY * 2.5;
    DrawLine(4.5, 4.5, movXPos, movYPos, Color(255, 255, 255));
    DrawCircle(movXPos, movYPos, 0.3, Color(255, 200, 0), 10);

    Print(2, 7.2, Color(0, 40, 200), "Movement");
    Print(2, 1.3, Color(255, 255, 255), "x:%.03f", data.movementX);
    Print(4.6, 1.3, Color(255, 255, 255), "y:%.03f", data.movementY);

    // draw angles inputs
    DrawRectangle(12, 2, 5, 5, Color(100, 20, 0));
    DrawCircle(14.5, 4.5, 2.5, Color(200, 40, 0), 40);
    DrawLine(14.5, 2, 14.5, 7, Color(100, 20, 0));
    DrawLine(12, 4.5, 17, 4.5, Color(100, 20, 0));

    float angXPos = 14.5 + drawData.angleX * 2.5, angYPos = 4.5 + drawData.angleY * 2.5;
    DrawLine(14.5, 4.5, angXPos, angYPos, Color(255, 255, 255));
    DrawCircle(angXPos, angYPos, 0.3, Color(255, 200, 0), 10);

    Print(12, 7.2, Color(200, 40, 0), "Angles");
    Print(12, 1.3, Color(255, 255, 255), "x:%.03f", data.angleX);
    Print(14.6, 1.3, Color(255, 255, 255), "y:%.03f", data.angleY);

    // draw digital inputs

    Color digitalColors[5] = {
        Color(0,180,255),Color(255,180,0),Color(20,200,20),Color(100,0,200),Color(255,255,0)
    };
    const char* digitalNames[5] = {
        "Blue", "Orange", "Jump", "Duck", "Use"
    };

    for (int i = 0; i < 5; i++) {
        int x = 3 * i + (i > 1 ? 3 : 2);
        DrawRectangle(x, 9, 2, 4, digitalColors[i]);
        DrawRectangle(x + 0.1, 9.1, 1.8, 3.8, Color(40, 10, 10));
        DrawRectangle(x + 0.1, 10.95, 1.8, 0.1, Color(100, 50, 0));

        float p = drawData.digitalAnalogs[i];
        float realP = data.digitalAnalogs[i];
        Color c = (realP <= 0) ? Color(50, 100, 50) : Color(20, 200, 20);
        DrawRectangle(x + 0.1, 9.1, 1.8, 3.8f*(p+1)*0.5f, c);

        Print(x, 13.2, digitalColors[i], digitalNames[i]);

        Print(x, 8.3, Color(255,255,255), "%.03f", realP);
    }

    //save input
    int saveCount = data.inputCounts[7];
    int saveReq = (data.playerCount + 1) / saveLoadDiv;
    Color saveColor = (saveCount >= saveReq) ? Color(255, 255, 255) : Color(128, 128, 128);
    Print(7.5, 5, saveColor, "Save: %d/%d", saveCount, saveReq);
    DrawRectangle(7.5, 4, 4, 0.8, Color(255, 255, 255));
    DrawRectangle(7.6, 4.1, 3.8, 0.6, Color(40, 10, 10));
    DrawRectangle(7.6, 4.1, 3.8 * saveInterp, 0.6, Color(20, 200, 20));
    //load input
    int loadCount = data.inputCounts[8];
    int loadReq = (data.playerCount + 1) / saveLoadDiv;
    Color loadColor = (loadCount >= loadReq) ? Color(255, 255, 255) : Color(128, 128, 128);
    Print(7.5, 3, loadColor, "Load: %d/%d", loadCount, loadReq);
    DrawRectangle(7.5, 2, 4, 0.8, Color(40,40,40));
    DrawRectangle(7.6, 2.1, 3.8, 0.6, Color(40, 10, 10));
    DrawRectangle(7.6, 2.1, 3.8 * loadInterp, 0.6, Color(20, 200, 20));

    if (!g_dataReceiver->IsActive()) {
        DrawRectangle(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, Color(50, 50, 50, 200));
        Print(6, 8.1, Color(25, 0, 0), "Connecting to server...");
        Print(6, 8.2, Color(255, 0, 0), "Connecting to server...");
    }

    glutSwapBuffers();
    glutPostRedisplay();
}

void WindowInit() {
    glClearColor(0,0,0, 1.0);
    glColor3f(1.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void WindowResize(int width, int height) {
    glutReshapeWindow(WINDOW_WIDTH * 40, WINDOW_HEIGHT * 40);
}


void DataReceiveLoop() {
    while (!ending) {
        try {
            g_dataReceiver->Initialize(serverIP);
            cout << "connected" << endl;
            while (g_dataReceiver->IsActive()) {
                auto prevTime = chrono::system_clock::now();
                g_dataReceiver->ReceiveData();
                auto currTime = chrono::system_clock::now();

                auto sleepTime = 50ms - (currTime - prevTime);
                if (sleepTime < 5ms)sleepTime = 5ms;
                this_thread::sleep_for(sleepTime);
            }
            g_dataReceiver->Disable();
        }
        catch (string err) {
            cout << err << endl;
            g_dataReceiver->Disable();
        }
        this_thread::sleep_for(1000ms);
    }
}

void LoadServerIPFile() {
    ifstream ipfile("serverip.txt");
    if (!ipfile.good()) {
        serverIP = "127.0.0.1";
    }
    else {
        ipfile >> serverIP;
    }
    cout << serverIP;
    ipfile.close();
}

int __stdcall WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd
) {
    int argc = 1;
    const char* argv[1] = {" "};
    glutInit(&argc, (char**)argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_MULTISAMPLE);
    glutInitWindowSize(WINDOW_WIDTH * 40, WINDOW_HEIGHT * 40);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("MinecraftKrzyServer Data Display");
    glutDisplayFunc(WindowDisplay);
    WindowInit();
    glutReshapeFunc(WindowResize);

    LoadServerIPFile();
    thread dataRecv(DataReceiveLoop);

    BuildFont(30, "D-DIN");
    glutMainLoop();

    g_dataReceiver->Disable();
    ending = false;
    return 0;
}