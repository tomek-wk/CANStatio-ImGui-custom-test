#pragma once

struct TimeCursor {
    bool visible = false;
    double timeSeconds = 0.0;
};

class CursorModel {
public:
    const TimeCursor& a() const { return a_; }
    const TimeCursor& b() const { return b_; }

    TimeCursor& a() { return a_; }
    TimeCursor& b() { return b_; }

    void setA(double timeSeconds, double datasetEndSeconds);
    void setB(double timeSeconds, double datasetEndSeconds);
    void hideA();
    void hideB();
    void reset();

    static double clampTime(double timeSeconds, double datasetEndSeconds);

private:
    TimeCursor a_;
    TimeCursor b_;
};
