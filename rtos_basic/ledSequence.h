#include "mbed.h"
#include <string>
class ledSequence
{
    public:
        string ledSelect();
        void ledRepeatSequence(string);
        void turnOffColor();
};
