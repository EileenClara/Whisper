#ifndef UI_CLOCK_H
#define UI_CLOCK_H

#include "config.h"
#include "display_screen.h"
#include "network_ntp.h"

class UIClock {
public:
    static void begin();
    static void update();
    static void draw();

private:
    static void _drawTime(int h, int m, int s);
    static void _drawSeconds(int s);
    static void _drawDate();

    static int _lastHour, _lastMin, _lastSec, _lastDay;
};

#endif
