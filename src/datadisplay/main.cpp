#include <iostream>

#include "DataReceiver.hpp"
#include "DrawingFuncs.hpp"
#include <string>
#include <chrono>
#include <thread>

using namespace std;

#define WINDOW_WIDTH 19
#define WINDOW_HEIGHT 16

static DumbControllerData drawData;
static float saveInterp = 0;
static float loadInterp = 0;

void WindowDisplay() {

    glClear(GL_COLOR_BUFFER_BIT);
    glLineWidth(5);

    DumbControllerData data = g_dataReceiver->GetData();
    float interp = 0.4;
    drawData.movementX += (data.movementX - drawData.movementX) * interp;
    drawData.movementY += (data.movementY - drawData.movementY) * interp;
    drawData.angleX += (data.angleX - drawData.angleX) * interp;
    drawData.angleY += (data.angleY - drawData.angleY) * interp;
    for (int i = 0; i < 5; i++) {
        drawData.digitalAnalogs[i] += (data.digitalAnalogs[i] - drawData.digitalAnalogs[i]) * interp;
    }
    saveInterp += (fmin(1,(data.inputCounts[7] / (float)data.playerCount) * 2.0) - saveInterp) * interp;
    loadInterp += (fmin(1,(data.inputCounts[8] / (float)data.playerCount) * 2.0) - loadInterp) * interp;

    // draw movement inputs
    DrawRectangle(2, 2, 5, 5, Color(0, 20, 100));
    DrawCircle(4.5, 4.5, 2.5, Color(0, 40, 200));
    DrawLine(4.5, 2, 4.5, 7, Color(0, 20, 100));
    DrawLine(2, 4.5, 7, 4.5, Color(0, 20, 100));

    float movXPos = 4.5 + drawData.movementX * 2.5, movYPos = 4.5 + drawData.movementY * 2.5;
    DrawLine(4.5, 4.5, movXPos, movYPos, Color(255, 255, 255));
    DrawCircle(movXPos, movYPos, 0.3, Color(255, 200, 0), 10);

    Print(2, 7.5, Color(0, 40, 200), "Movement");
    Print(2, 1, Color(255, 255, 255), "x:%.03f", data.movementX);
    Print(4.6, 1, Color(255, 255, 255), "y:%.03f", data.movementY);

    // draw angles inputs
    DrawRectangle(12, 2, 5, 5, Color(100, 20, 0));
    DrawCircle(14.5, 4.5, 2.5, Color(200, 40, 0));
    DrawLine(14.5, 2, 14.5, 7, Color(100, 20, 0));
    DrawLine(12, 4.5, 17, 4.5, Color(100, 20, 0));

    float angXPos = 14.5 + drawData.angleX * 2.5, angYPos = 4.5 + drawData.angleY * 2.5;
    DrawLine(14.5, 4.5, angXPos, angYPos, Color(255, 255, 255));
    DrawCircle(angXPos, angYPos, 0.3, Color(255, 200, 0), 10);

    Print(12, 7.5, Color(200, 40, 0), "Angles");
    Print(12, 1, Color(255, 255, 255), "x:%.03f", data.angleX);
    Print(14.6, 1, Color(255, 255, 255), "y:%.03f", data.angleY);

    // draw digital inputs

    Color digitalColors[5] = {
        Color(0,180,255),Color(255,180,0),Color(20,200,20),Color(100,0,200),Color(255,255,0)
    };
    const char* digitalNames[5] = {
        "Blue", "Orange", "Jump", "Duck", "Use"
    };

    for (int i = 0; i < 5; i++) {
        int x = 3 * i + (i > 1 ? 3 : 2);
        DrawRectangle(x, 10, 2, 4, digitalColors[i]);
        DrawRectangle(x + 0.1, 10.1, 1.8, 3.8, Color(40, 10, 10));
        DrawRectangle(x + 0.1, 11.95, 1.8, 0.1, Color(100, 50, 0));

        float p = drawData.digitalAnalogs[i];
        float realP = data.digitalAnalogs[i];
        Color c = (realP <= 0) ? Color(50, 100, 50) : Color(20, 200, 20);
        DrawRectangle(x + 0.1, 10.1, 1.8, 3.8f*(p+1)*0.5f, c);

        Print(x, 14.5, digitalColors[i], digitalNames[i]);

        Print(x, 9, Color(255,255,255), "%.03f", realP);
    }
    //save input
    int saveCount = data.inputCounts[7];
    int saveReq = (data.playerCount + 1) / 2;
    Color saveColor = (saveCount >= saveReq) ? Color(255, 255, 255) : Color(128, 128, 128);
    Print(7.5, 6, saveColor, "Save: %d/%d", saveCount, saveReq);
    DrawRectangle(7.5, 5, 4, 0.8, Color(255, 255, 255));
    DrawRectangle(7.6, 5.1, 3.8, 0.6, Color(40, 10, 10));
    DrawRectangle(7.6, 5.1, 3.8 * saveInterp, 0.6, Color(20, 200, 20));
    //load input
    int loadCount = data.inputCounts[8];
    int loadReq = (data.playerCount + 1) / 2;
    Color loadColor = (loadCount >= loadReq) ? Color(255, 255, 255) : Color(128, 128, 128);
    Print(7.5, 4, loadColor, "Load: %d/%d", loadCount, loadReq);
    DrawRectangle(7.5, 3, 4, 0.8, Color(40,40,40));
    DrawRectangle(7.6, 3.1, 3.8, 0.6, Color(40, 10, 10));
    DrawRectangle(7.6, 3.1, 3.8 * loadInterp, 0.6, Color(20, 200, 20));

    glFlush();
}

void WindowInit() {
    glClearColor(0,0,0, 1.0);
    glColor3f(1.0, 0.0, 0.0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, WINDOW_WIDTH, 0.0, WINDOW_HEIGHT);
}

void WindowResize(int width, int height) {
    glutReshapeWindow(WINDOW_WIDTH * 40, WINDOW_HEIGHT * 40);
}

void WindowTimer(int value)
{
    glutTimerFunc(16, WindowTimer, 0);
    glutPostRedisplay();
}


void DataReceiveLoop() {
    try {
        g_dataReceiver->Initialize();
        cout << "connected" << endl;
        while (g_dataReceiver->IsActive()) {
            g_dataReceiver->ReceiveData();
            this_thread::sleep_for(50ms);
        }
    }
    catch (string s) {
        cout << s;
        g_dataReceiver->Disable();
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_MULTISAMPLE);
    glutInitWindowSize(WINDOW_WIDTH * 40, WINDOW_HEIGHT * 40);
    glutInitWindowPosition(0, 0);
    glutCreateWindow("MinecraftKrzyServer Data Display");
    glutTimerFunc(0, WindowTimer, 0);
    glutDisplayFunc(WindowDisplay);
    WindowInit();
    glutReshapeFunc(WindowResize);

    thread dataRecv(DataReceiveLoop);

    BuildFont(30, "D-DIN");
    glutMainLoop();
}