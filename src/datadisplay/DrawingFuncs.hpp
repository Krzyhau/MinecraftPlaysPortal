#pragma once

#include "glut.h"

static int g_fontBase = 0;

struct Color {
    byte r, g, b, a;
    Color() :r(0), g(0), b(0), a(255) {};
    Color(byte r, byte g, byte b) : r(r), g(g), b(b), a(255) {};
    Color(byte r, byte g, byte b, byte a) : Color(r, g, b) { this->a = a; };
};

void BuildFont(int height, const char* fontName) {
    HDC hdc = wglGetCurrentDC();
    g_fontBase = glGenLists(96);
    HFONT hFont = CreateFontA(-(height), 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE | DEFAULT_PITCH, fontName);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    wglUseFontBitmaps(hdc, 32, 96, g_fontBase);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void Print(float x, float y, Color c, const char* format, ...) {
    glEnable(GL_LINE_SMOOTH);

    glColor4ub(c.r, c.g, c.b, c.a);
    glRasterPos2f(x, y);

    char text[1024];
    va_list args;
    va_start(args, format);
    vsprintf_s(text, 1024, format, args);
    va_end(args);

    glPushAttrib(GL_LIST_BIT);
    glListBase(g_fontBase - 32);
    glCallLists(strlen(text), GL_UNSIGNED_BYTE, text);
    glPopAttrib();
}

void DrawRectangle(float x, float y, float width, float height, Color c) {
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x, y + height);
    glVertex2f(x + width, y + height);
    glVertex2f(x + width, y);
    glEnd();
}

void DrawLine(float x1, float y1, float x2, float y2, Color c) {
    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_LINES);
    glVertex2f(x1, y1);
    glVertex2f(x2, y2);
    glEnd();
}

void DrawCircle(float x, float y, float r, Color c, int lod = 24) {
    int i;
    float twopi = 2.0f * 3.1415926535f;

    glColor4ub(c.r, c.g, c.b, c.a);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (i = 0; i <= lod; i++) {
        glVertex2f(
            x + r * cos(i * twopi / lod),
            y + r * sin(i * twopi / lod)
        );
    }
    glEnd();
}