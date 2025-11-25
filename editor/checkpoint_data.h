#ifndef CHECKPOINT_DATA_H
#define CHECKPOINT_DATA_H

struct CheckpointData {
    int id;
    float x;
    float y;

    CheckpointData(int id = 0, float x = 0.0f, float y = 0.0f)
        : id(id), x(x), y(y) {}
};

#endif 
