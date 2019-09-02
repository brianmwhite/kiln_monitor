#ifndef __KilnUtilities_H
#define __KilnUtilities_H

#include <string>
using namespace std;

class KilnUtilities {

    public:
        KilnUtilities() {}
        float ConvertCelsiusToFahrenheit(float temperatureInCelsius);
        float ConvertFahrenheitToCelsius(float temperatureInFahrenheit);
        string LookUpConeValueFromFahrenheit(float tempInF);
        float LookUpTemperatureValueFromCone(string cone);

};

#endif