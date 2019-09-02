#include <KilnUtilities.h>
#include <string>
using namespace std;

float KilnUtilities::ConvertCelsiusToFahrenheit(float temperatureInCelsius) {
  return (temperatureInCelsius * 9.0 / 5.0) + 32.0;
}

string KilnUtilities::LookupConeValue(float tempInF) {
  string cone = "";
  if (tempInF >= 2345) {cone = "10";}
  else if (tempInF >= 2300) {cone = "9";}
  else if (tempInF >= 2273) {cone = "8";}
  else if (tempInF >= 2262) {cone = "7";}
  else if (tempInF >= 2232) {cone = "6";}
  else if (tempInF >= 2167) {cone = "5";}
  else if (tempInF >= 2142) {cone = "4";}
  else if (tempInF >= 2106) {cone = "3";}
  else if (tempInF >= 2088) {cone = "2";}
  else if (tempInF >= 2079) {cone = "1";}
  else if (tempInF >= 2046) {cone = "01";}
  else if (tempInF >= 2016) {cone = "02";}
  else if (tempInF >= 1987) {cone = "03";}
  else if (tempInF >= 1945) {cone = "04";}
  else if (tempInF >= 1888) {cone = "05";}
  else if (tempInF >= 1828) {cone = "06";}
  else if (tempInF >= 1789) {cone = "07";}
  else if (tempInF >= 1728) {cone = "08";}
  else if (tempInF >= 1688) {cone = "09";}
  else if (tempInF >= 1657) {cone = "010";}
  else if (tempInF >= 1607) {cone = "011";}
  else if (tempInF >= 1582) {cone = "012";}
  else if (tempInF >= 1539) {cone = "013";}
  else if (tempInF >= 1485) {cone = "014";}
  else if (tempInF >= 1456) {cone = "015";}
  else if (tempInF >= 1422) {cone = "016";}
  else if (tempInF >= 1360) {cone = "017";}
  else if (tempInF >= 1252) {cone = "018";}
  else if (tempInF >= 1252) {cone = "019";}
  else if (tempInF >= 1159) {cone = "020";}
  else if (tempInF >= 1112) {cone = "021";}
  else if (tempInF >= 1087) {cone = "022";}
  else cone = "";
  return cone;
}