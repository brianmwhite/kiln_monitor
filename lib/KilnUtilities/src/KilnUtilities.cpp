#include <KilnUtilities.h>
#include <string>
using namespace std;

float KilnUtilities::ConvertCelsiusToFahrenheit(float temperatureInCelsius)
{
  return (temperatureInCelsius * 9.0 / 5.0) + 32.0;
}

float KilnUtilities::ConvertFahrenheitToCelsius(float temperatureInFahrenheit)
{
  return (temperatureInFahrenheit - 32.0) * 5.0 / 9.0;
}

string KilnUtilities::LookUpConeValueFromFahrenheit(float tempInF) 
{
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
  return cone;
}

float KilnUtilities::LookUpTemperatureValueFromCone(string cone) {
  float tempInF = 0.0;
  if (cone == "-1") {tempInF=82;} //fake cone for testing
  else if (cone == "10") {tempInF=2345;}
  else if (cone == "9") {tempInF=2300;}
  else if (cone == "8") {tempInF=2273;}
  else if (cone == "7") {tempInF=2262;}
  else if (cone == "6") {tempInF=2232;}
  else if (cone == "5") {tempInF=2167;}
  else if (cone == "4") {tempInF=2142;}
  else if (cone == "3") {tempInF=2106;}
  else if (cone == "2") {tempInF=2088;}
  else if (cone == "1") {tempInF=2079;}
  else if (cone == "01") {tempInF=2046;}
  else if (cone == "02") {tempInF=2016;}
  else if (cone == "03") {tempInF=1987;}
  else if (cone == "04") {tempInF=1945;}
  else if (cone == "05") {tempInF=1888;}
  else if (cone == "06") {tempInF=1828;}
  else if (cone == "07") {tempInF=1789;}
  else if (cone == "08") {tempInF=1728;}
  else if (cone == "09") {tempInF=1688;}
  else if (cone == "010") {tempInF=1657;}
  else if (cone == "011") {tempInF=1607;}
  else if (cone == "012") {tempInF=1582;}
  else if (cone == "013") {tempInF=1539;}
  else if (cone == "014") {tempInF=1485;}
  else if (cone == "015") {tempInF=1456;}
  else if (cone == "016") {tempInF=1422;}
  else if (cone == "017") {tempInF=1360;}
  else if (cone == "018") {tempInF=1252;}
  else if (cone == "019") {tempInF=1252;}
  else if (cone == "020") {tempInF=1159;}
  else if (cone == "021") {tempInF=1112;}
  else if (cone == "022") {tempInF=1087;}
  return tempInF;
}