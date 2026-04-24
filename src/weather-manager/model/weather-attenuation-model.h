#ifndef WEATHER_ATTENUATION_MODEL_H
#define WEATHER_ATTENUATION_MODEL_H

#include "ns3/propagation-loss-model.h"
#include "ns3/mobility-model.h"
#include "weather-manager.h"
#include "ns3/node.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include <fstream>
#include <sstream>


namespace ns3 {

/**
 * @class WeatherAttenuationModel
 * @brief A propagation loss model that accounts for attenuation due to various weather effects.
 * 
 * This model integrates rain, fog, snow, and gas attenuation using ITU-R models.
 * It retrieves real-time weather conditions from `WeatherManager`.
 */
class WeatherAttenuationModel : public PropagationLossModel
{
public:
  static TypeId GetTypeId(void);

  WeatherAttenuationModel();
  ~WeatherAttenuationModel() override;

  // Wiring / config
  void SetChild(Ptr<PropagationLossModel> child);      // keep only if you don’t use SetNext()
  void SetWeatherManager(Ptr<WeatherManager> weatherManager);
  Ptr<PropagationLossModel> GetChild() const { return m_child; }


  void SetFrequency(double ghz);
  void SetPolarization(const std::string& polarization);
  void SetTemperature(double celsius);

  double GetFrequency() const;
  std::string GetPolarization() const;
  double GetTemperature() const;

  // Make this public ONLY if you want to call it from outside the class.
  void WriteCsvRecord(const std::string& filename,
                      double txPowerDbm,
                      Ptr<MobilityModel> a,
                      Ptr<MobilityModel> b) const;


  double DoCalcRxPower(double txPowerDbm,
                       Ptr<MobilityModel> a,
                       Ptr<MobilityModel> b) const override;
protected:
  int64_t DoAssignStreams(int64_t stream) override;

private:
  Ptr<PropagationLossModel> m_child;       ///< Optional inner model (if not using SetNext()).
  Ptr<WeatherManager>       m_weatherManager;

  std::string m_csvFilename ;
  double      m_frequency  ;         ///< GHz
  std::string m_polarization;
  double      m_temperature ;        ///< °C

  std::string GetDataset(double txPowerDbm,
                         Ptr<MobilityModel> a,
                         Ptr<MobilityModel> b) const;
};

} // namespace ns3
#endif // WEATHER_ATTENUATION_MODEL_H
