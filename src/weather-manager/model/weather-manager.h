#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include "ns3/object.h"
#include "ns3/mobility-model.h"
#include "ns3/ptr.h"
#include "ns3/simulator.h"
#include "ns3/log.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <cmath>
#include "ns3/constant-velocity-mobility-model.h"
#include "ns3/random-waypoint-mobility-model.h"
#include "ns3/gauss-markov-mobility-model.h"
#include "ns3/waypoint-mobility-model.h"
#include "ns3/constant-position-mobility-model.h"
#include "ns3/node.h"
#include "ns3/double.h"

namespace ns3 {

// Struct for thresholds
struct WeatherThresholds
{
  double rainLight = 2.5;
  double rainModerate = 7.6;
  double rainHeavy = 20.0;
  double rainSevere = 50.0;

  double fogLight = 0.2;
  double fogModerate = 0.5;
  double fogDense = 0.8;
  double fogCritical = 1.2;

  double snowLight = 5.0;
  double snowModerate = 10.0;
  double snowHeavy = 25.0;
  double snowBlizzard = 50.0;

  double windNoticeable = 5.0;
  double windDrift = 10.0;
  double windUnstable = 15.0;
  double windDangerous = 20.0;

  double tempCold = -5.0;
  double tempFreeze = -10.0;
  double tempHot = 35.0;
  double tempCritical = 45.0;
};

struct WeatherLogEntry
{
  double startTime;
  double intensity;
};

/**
 * \brief WeatherManager class
 * Manages dynamic weather conditions and provides signal attenuation values.
 */
class WeatherManager : public Object {
public:
  static TypeId GetTypeId();

  WeatherManager();
  virtual ~WeatherManager();

  void SetHistoryFilename(const std::string &filename);
  // Set and get weather conditions
  void SetWeatherCondition(const std::string &conditionType, double value);
  double GetWeatherCondition(const std::string &conditionType) const;

  // Thresholds
  WeatherThresholds GetThresholds() const;
  void SetThresholds(const WeatherThresholds& thresholds);

  // Attenuation calculations
  double GetRainAttenuation(Ptr<MobilityModel> a, Ptr<MobilityModel> b,
                            double frequency, std::string polarization) const;
  double GetFogAttenuation(Ptr<MobilityModel> a, Ptr<MobilityModel> b,
                           double frequency, double temperature) const;
  double GetSnowAttenuation(Ptr<MobilityModel> a, Ptr<MobilityModel> b,
                            double frequency) const;
  double GetGasAttenuation(double pathLength, double frequency,
                           double temperature, double humidity) const;

  // Effective LOS
  double GetEffectiveLOSRange(double nominalRange) const;

  // Debug
  void DebugRainCoefficients() const;
  void LogWeatherConditions() const;

  // Weather change scheduling
  void ScheduleWeatherChange(double time, const std::string &conditionType, double value);

  // Mobility impact
  void ScheduleMobilityReduction(Ptr<MobilityModel> model, const std::string& nodeType, double interval);
  void EvaluateAndApplyMobilityReduction(Ptr<MobilityModel> model, const std::string& nodeType);

  // Logging helpers
  void SetSpeedLogFile(const std::string &filename);
  void SetMetadata(const std::string &routing, uint32_t scenario, uint32_t run);

  // Weather history and averages
  std::map<std::string, double> GetAverageWeatherConditions() const;
std::map<std::string, double> GetAverageWeatherConditions(double startTime,
                                                              double endTime) const;
  void WriteWeatherHistoryToFile(const std::string& historyFilename) const;
protected:                     // DoDispose should be protected in ns-3
  void DoDispose() override;   // ensure exact override
private:
  std::map<std::string, double> m_weatherConditions; 
  std::map<std::string, std::vector<WeatherLogEntry>> m_weatherHistory;
  std::string m_historyFilename;   // e.g., "weather_history.csv"

  WeatherThresholds m_thresholds;

  std::map<uint32_t, double> m_originalSpeed;
  std::string m_speedLogFileName;
  std::string m_routingProtocol;
  uint32_t m_scenarioId = 0;
  uint32_t m_runNumber = 0;

  // Attenuation helpers
  double CalculateRainAttenuation(double rainRate, double pathLength, double frequency, std::string polarization) const;
  double CalculateFogAttenuation(double fogDensity, double pathLength, double frequency, double temperature) const;
  double CalculateSnowAttenuation(double snowRate, double pathLength, double frequency, bool isWetSnow) const;
  double CalculateGasAttenuation(double frequency, double temperature, double humidity) const;

  void GetRainCoefficients(double frequency, std::string polarization, double &k, double &alpha) const;
  void UpdateWeatherCondition(const std::string &conditionType, double value);
};

} // namespace ns3

#endif // WEATHER_MANAGER_H
