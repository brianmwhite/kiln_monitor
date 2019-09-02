#ifndef __KilnUtilities_H
#define __KilnUtilities_H

#include <string>
using namespace std;

class KilnUtilities {

    public:
        KilnUtilities() {}
        string LookupConeValue(float tempInF);
        float ConvertCelsiusToFahrenheit(float temperatureInCelsius);
    };

#endif