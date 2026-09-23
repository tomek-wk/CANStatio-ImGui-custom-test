#pragma once

class CursorModel;
struct Dataset;

class CursorOverlay {
public:
    // Draws and handles A/B cursors inside the current ImPlot plot.
    // Returns true while a cursor drag owns the current LMB gesture.
    static bool draw(const Dataset& dataset, CursorModel& cursors);
};
