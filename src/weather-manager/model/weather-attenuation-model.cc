#include "weather-attenuation-model.h"
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/string.h"

#include <fstream>
#include <string>
#if __has_include(<filesystem>)
  #include <filesystem>
  namespace fs = std::filesystem;
#endif


namespace ns3 {

NS_LOG_COMPONENT_DEFINE("WeatherAttenuationModel");

NS_OBJECT_ENSURE_REGISTERED(WeatherAttenuationModel);

TypeId WeatherAttenuationModel::GetTypeId(void)
{
  static TypeId tid = TypeId("ns3::WeatherAttenuationModel")
    .SetParent<PropagationLossModel>()
    .SetGroupName("Propagation")
    .AddConstructor<WeatherAttenuationModel>()
    .AddAttribute("Frequency",
                  "Operating frequency in GHz",
                  DoubleValue(2.4),
                  MakeDoubleAccessor(&WeatherAttenuationModel::SetFrequency,
                                     &WeatherAttenuationModel::GetFrequency),
                  MakeDoubleChecker<double>())
    .AddAttribute("Polarization",
                  "Polarization type (e.g., 'horizontal', 'vertical')",
                  StringValue("horizontal"),
                  MakeStringAccessor(&WeatherAttenuationModel::SetPolarization,
                                     &WeatherAttenuationModel::GetPolarization),
                  MakeStringChecker())
    .AddAttribute("Temperature",
                  "Temperature in Celsius",
                  DoubleValue(15.0),
                  MakeDoubleAccessor(&WeatherAttenuationModel::SetTemperature,
                                     &WeatherAttenuationModel::GetTemperature),
                  MakeDoubleChecker<double>());
   
  return tid;
}

      std::ofstream g_weatherCsv;
      bool g_weatherHeaderWritten = false;

      void EnsureWeatherCsvOpen(const std::string& filename)
      {
          if (!g_weatherCsv.is_open()) {
              g_weatherCsv.open(filename, std::ios::out | std::ios::app);
          }

          if (!g_weatherCsv.is_open()) {
              NS_LOG_ERROR("Unable to open " << filename << " for writing.");
          }
      }

      void EnsureWeatherHeader(const std::string& filename)
      {
          if (g_weatherHeaderWritten) {
              return;
          }

          EnsureWeatherCsvOpen(filename);
          if (!g_weatherCsv.is_open()) {
              return;
          }

          // If file is empty, write header; if not, just mark as written.
          g_weatherCsv.seekp(0, std::ios::end);
          if (g_weatherCsv.tellp() == 0) {
              g_weatherCsv
                  << "Time,NodeA,NodeB,TxPower_dBm,PathLength_km,Frequency_GHz,Polarization,"
                    "Temperature_C,BaseLoss_dB,RainRate_mm_per_h,RainLoss_dB,"
                    "FogDensity_g_per_m3,FogLoss_dB,SnowRate_mm_per_h,SnowLoss_dB,"
                    "Humidity_pct,GasLoss_dB,WindSpeed_m_per_s,WindDirection_deg,"
                    "WeatherAttenuation_dB,TotalLoss_dB,RxPowerNoWeather_dBm,"
                    "RxPowerWithWeather_dBm\n";
          }

          g_weatherHeaderWritten = true;
      }
  

WeatherAttenuationModel::WeatherAttenuationModel()
  : m_frequency(2.4),
    m_polarization("horizontal"),
    m_temperature(15.0)
   
{
  NS_LOG_FUNCTION(this);
}

WeatherAttenuationModel::~WeatherAttenuationModel()
{
  NS_LOG_FUNCTION(this);
}

void WeatherAttenuationModel::SetChild(Ptr<PropagationLossModel> child)
{
  m_child = child;
 // SetNext(child);
}

void WeatherAttenuationModel::SetWeatherManager(Ptr<WeatherManager> weatherManager)
{
  m_weatherManager = weatherManager;
}

// Setters
void WeatherAttenuationModel::SetFrequency(double freq)
{
  m_frequency = freq;
}

void WeatherAttenuationModel::SetPolarization(const std::string &polarization)
{
  m_polarization = polarization;
}

void WeatherAttenuationModel::SetTemperature(double temperature)
{
  m_temperature = temperature;
}


// Getters
double WeatherAttenuationModel::GetFrequency() const
{
  return m_frequency;
}

std::string WeatherAttenuationModel::GetPolarization() const
{
  return m_polarization;
}

double WeatherAttenuationModel::GetTemperature() const
{
  return m_temperature;
}

/**
 * @brief Computes and returns a CSV-formatted string of the dataset.
 *
 * The CSV record contains the following fields:
 *
 * TxPower (dBm), PathLength (km), BaseLoss (dB),
 * RainLoss (dB), FogLoss (dB), SnowLoss (dB), GasLoss (dB),
 * WeatherAttenuation (dB), TotalLoss (dB),
 * RxPowerNoWeather (dBm), RxPowerWithWeather (dBm),
 * TxNodeId, RxNodeId,
 * RainRate, FogDensity, SnowRate, Humidity, WindSpeed, WindDirection
 */
std::string WeatherAttenuationModel::GetDataset(double txPowerDbm,
  Ptr<MobilityModel> a,
  Ptr<MobilityModel> b) const
{

 double t = Simulator::Now().GetSeconds();
 
// Compute base propagation loss.
double baseRxPowerDbm = m_child->CalcRxPower(txPowerDbm, a, b);
double baseLoss = txPowerDbm - baseRxPowerDbm;

// Compute weather-induced losses.
double rainLoss = 0.0, fogLoss = 0.0, snowLoss = 0.0, gasLoss = 0.0;
if (m_weatherManager)
{
rainLoss = m_weatherManager->GetRainAttenuation(a, b, m_frequency, m_polarization);
fogLoss = m_weatherManager->GetFogAttenuation(a, b, m_frequency, m_temperature);
snowLoss = m_weatherManager->GetSnowAttenuation(a, b, m_frequency); 
double humidity = m_weatherManager->GetWeatherCondition("Humidity");
gasLoss = m_weatherManager->GetGasAttenuation(a->GetDistanceFrom(b)/1000.0, m_frequency, m_temperature, humidity);
}
double weatherAttenuationDb = rainLoss + fogLoss + snowLoss + gasLoss;
double totalLoss = baseLoss + weatherAttenuationDb;
double finalRxPowerDbm = baseRxPowerDbm - weatherAttenuationDb;

// Retrieve node IDs.
uint32_t nodeIdTx = 0, nodeIdRx = 0;
Ptr<Node> nodeTx = a->GetObject<Node>();
Ptr<Node> nodeRx = b->GetObject<Node>();
if (nodeTx) { nodeIdTx = nodeTx->GetId(); }
if (nodeRx) { nodeIdRx = nodeRx->GetId(); }

// Retrieve additional weather conditions.
double rainRate = m_weatherManager ? m_weatherManager->GetWeatherCondition("RainRate") : 0.0;
double fogDensity = m_weatherManager ? m_weatherManager->GetWeatherCondition("FogDensity") : 0.0;
double snowRate = m_weatherManager ? m_weatherManager->GetWeatherCondition("SnowRate") : 0.0;
double humidity = m_weatherManager ? m_weatherManager->GetWeatherCondition("Humidity") : 0.0;
double windSpeed = m_weatherManager ? m_weatherManager->GetWeatherCondition("WindSpeed") : 0.0;
double windDirection = m_weatherManager ? m_weatherManager->GetWeatherCondition("WindDirection") : 0.0;

// Create a CSV record.
std::ostringstream oss;

oss << t << ","
<< nodeIdTx << ","
<< nodeIdRx << ","
<< txPowerDbm << ","
<< a->GetDistanceFrom(b)/1000.0 << ","
<<m_frequency<<","
<<m_polarization<<","
<<m_temperature<<","
<< baseLoss << ","
<< rainRate << ","
<< rainLoss << ","
<< fogDensity << ","
<< fogLoss << ","
<< snowRate << ","
<< snowLoss << ","
<< humidity << ","
<< gasLoss << ","
<< windSpeed << ","
<< windDirection<<","
<< weatherAttenuationDb << ","
<< totalLoss << ","
<< baseRxPowerDbm << ","
<< finalRxPowerDbm ;

return oss.str();
}

/**
* @brief Writes the CSV-formatted dataset record to a file.
*
* The file "weather_data.csv" is opened in append mode, the record is written, then the file is closed.
*/
void
WeatherAttenuationModel::WriteCsvRecord(const std::string& filename,
                                        double txPowerDbm,
                                        Ptr<MobilityModel> a,
                                        Ptr<MobilityModel> b) const
{
    if (filename.empty()) {
        NS_LOG_WARN("CSV filename is empty; skipping write.");
        return;
    }

    std::string record = GetDataset(txPowerDbm, a, b);

    EnsureWeatherHeader(filename);
    if (!g_weatherCsv.is_open()) {
        return;
    }

    g_weatherCsv << record << '\n';
}

// void
// WeatherAttenuationModel::WriteCsvRecord(const std::string& filename,
//                                         double txPowerDbm,
//                                         Ptr<MobilityModel> a,
//                                         Ptr<MobilityModel> b) const
// {
//    if (filename.empty())
//   {
//     NS_LOG_WARN("CSV filename is empty; skipping write.");
//     return;
//   }
//   std::string record = GetDataset(txPowerDbm, a, b);

//   bool needHeader = false;

// #if __has_include(<filesystem>)
//   std::error_code ec;
//   if (!fs::exists(filename, ec))
//   {
//     needHeader = true;
//   }
//   else if (fs::is_regular_file(filename, ec) && fs::file_size(filename, ec) == 0)
//   {
//     needHeader = true;
//   }
// #else
//   // Fallback: check emptiness by opening for read and peeking
//   {
//     std::ifstream ifs(filename, std::ios::binary);
//     needHeader = !ifs.good() || ifs.peek() == std::ifstream::traits_type::eof();
//   }
// #endif

//   std::ofstream ofs(filename, std::ios_base::app);
//   if (!ofs.is_open())
//   {
//     NS_LOG_ERROR("Unable to open " << filename << " for writing.");
//     return;
//   }

//   if (needHeader)
//   {
//     ofs << "NodeA,NodeB,TxPower_dBm,PathLength_m,Frequency_GHz,Polarization,"
//            "Temperature_C,BaseLoss_dB,RainRate_mm_per_h,RainLoss_dB,"
//            "FogDensity_g_per_m3,FogLoss_dB,SnowRate_mm_per_h,SnowLoss_dB,"
//            "Humidity_pct,GasLoss_dB,WindSpeed_m_per_s,WindDirection_deg,"
//            "WeatherAttenuation_dB,TotalLoss_dB,RxPowerNoWeather_dBm,"
//            "RxPowerWithWeather_dBm\n";
//   }

//   ofs << record << '\n';
//   // ofs closes automatically on destruction
// }
// void WeatherAttenuationModel::WriteCsvRecord(double txPowerDbm,
// Ptr<MobilityModel> a,
// Ptr<MobilityModel> b) const
// {
// std::string record = GetDataset(txPowerDbm, a, b);
// // Open file in append mode.
// std::ofstream ofs("weathermetrics.csv", std::ios_base::app);
// if (ofs.is_open())
//   {
//     // Check if the file is empty (i.e., its current position is at 0).
//     ofs.seekp(0, std::ios::end);
//     if (ofs.tellp() == 0)
//       {
//         // Write the header only once.
//         ofs << "NodeA, Node B, TxPower,PathLength,Frequency GHZ,Polirazation,Temp,BaseLoss,RainRate, RainLoss,FogDensity,FogLoss,SnowRate,SnowLoss,Humidity,GasLoss windspeed, winddirection,WeatherAttenuation,TotalLoss,RxPowerNoWeather,RxPowerWithWeather" << "\n";

      
//       }
//       ofs << record << "\n";
//       ofs.close();
//     }

// else
// {
// NS_LOG_ERROR("Unable to open weather_data.csv for writing.");
// }
// }

double WeatherAttenuationModel::DoCalcRxPower(double txPowerDbm,
Ptr<MobilityModel> a,
Ptr<MobilityModel> b) const  {
NS_ABORT_MSG_IF(!m_child, "WeatherAttenuationModel has no child PropagationLossModel!");

// Step 1: Compute Base Loss (Standard Propagation)
double baseRxPowerDbm = m_child->CalcRxPower(txPowerDbm, a, b);
//double baseLoss = txPowerDbm - baseRxPowerDbm;

// Step 2: Compute Weather Attenuation
double rainLoss = 0.0, fogLoss = 0.0, snowLoss = 0.0, gasLoss = 0.0;
if (m_weatherManager) {
rainLoss = m_weatherManager->GetRainAttenuation(a, b, m_frequency, m_polarization);
fogLoss = m_weatherManager->GetFogAttenuation(a, b, m_frequency, m_temperature);
snowLoss = m_weatherManager->GetSnowAttenuation(a, b, m_frequency); 
double humidity = m_weatherManager->GetWeatherCondition("Humidity");
gasLoss = m_weatherManager->GetGasAttenuation(a->GetDistanceFrom(b) / 1000.0, m_frequency, m_temperature, humidity);
}
double weatherAttenuationDb = rainLoss + fogLoss + snowLoss + gasLoss;

// Step 3: Compute Total Loss and Final Rx Power
//double totalLoss = baseLoss + weatherAttenuationDb;
double finalRxPowerDbm = baseRxPowerDbm - weatherAttenuationDb;

// Retrieve Node IDs.
// Ptr<Node> txNode = a->GetObject<Node>();
// Ptr<Node> rxNode = b->GetObject<Node>();
// uint32_t txNodeId = txNode ? txNode->GetId() : 0;
// uint32_t rxNodeId = rxNode ? rxNode->GetId() : 0;

// Log Data to Console (optional)
// std::cout << "[Weather Data] Tx Node ID: " << txNodeId
// << ", Rx Node ID: " << rxNodeId
// << ", Tx Power: " << txPowerDbm << " dBm"
// << ", Path Length: " << a->GetDistanceFrom(b) / 1000.0 << " km"
// << ", Base Loss: " << baseLoss << " dB"
// << ", Rain Loss: " << rainLoss << " dB"
// << ", Fog Loss: " << fogLoss << " dB"
// << ", Snow Loss: " << snowLoss << " dB"
// << ", Gas Loss: " << gasLoss << " dB"
// << ", Weather Attenuation: " << weatherAttenuationDb << " dB"
// << ", Total Loss: " << totalLoss << " dB"
// << ", Rx Power No Weather: " << baseRxPowerDbm << " dBm"
// << ", Rx Power With Weather: " << finalRxPowerDbm << " dBm"
// << std::endl;


// static uint64_t calls = 0;
// calls++;
// if (calls == 1 || calls == 100000)
// {
//   std::cerr << "[WEATHER-ATTN] calls=" << calls
//             << " baseRx=" << baseRxPowerDbm
//             << " weatherDb=" << weatherAttenuationDb
//             << " finalRx=" << finalRxPowerDbm
//             << std::endl;
// }

// Write CSV Record (append to file)
WriteCsvRecord(m_csvFilename,txPowerDbm, a, b);



return finalRxPowerDbm;


}

// double WeatherAttenuationModel::DoCalcRxPower(double txPowerDbm,
//                                               Ptr<MobilityModel> a,
//                                               Ptr<MobilityModel> b) const {
//     NS_ABORT_MSG_IF(!m_child, "WeatherAttenuationModel has no child PropagationLossModel!");

//     // Step 1: Compute Base Loss (Standard Propagation)
//     double baseRxPowerDbm = m_child->CalcRxPower(txPowerDbm, a, b);
//     double baseLoss = txPowerDbm - baseRxPowerDbm; // L_propagation

//     // Step 2: Compute Weather Attenuation
//     double weatherAttenuationDb = 0.0;
//     double rainLoss = 0.0, fogLoss = 0.0, snowLoss = 0.0, gasLoss = 0.0;

//     if (m_weatherManager) {
//         rainLoss = m_weatherManager->GetRainAttenuation(a, b, m_frequency, m_polarization);
//         fogLoss = m_weatherManager->GetFogAttenuation(a, b, m_frequency, m_temperature);
//         snowLoss = m_weatherManager->GetSnowAttenuation(a, b, m_frequency, false); // Assume dry snow
//         double humidity = m_weatherManager->GetWeatherCondition("Humidity"); // Fetch from WeatherManager

//         gasLoss = m_weatherManager->GetGasAttenuation(a->GetDistanceFrom(b) / 1000.0, m_frequency, m_temperature, humidity);

//         weatherAttenuationDb = rainLoss + fogLoss + snowLoss + gasLoss;
//     }

//     // Step 3: Compute Total Loss
//     double totalLoss = baseLoss + weatherAttenuationDb;
//     double finalRxPowerDbm = baseRxPowerDbm - weatherAttenuationDb;


//     // Step 4: Log Data
//     std::cout << "[Weather Data] Tx: " << a   

//               << ", Rx: " << b  

//               << ", Tx Power: " << txPowerDbm
//               << ", Path Length: " << a->GetDistanceFrom(b) / 1000.0 << " km"
//               << ", Base Loss: " << baseLoss << " dB"
//               << ", Rain Loss: " << rainLoss << " dB"
//               << ", Fog Loss: " << fogLoss << " dB"
//               << ", Snow Loss: " << snowLoss << " dB"
//               << ", Gas Loss: " << gasLoss << " dB"
//               << ", weather attinuation : " <<weatherAttenuationDb << " dB"
//               << ", Total Loss: " << totalLoss << " dB"
//               << ", Rx Power No Weather: " << baseRxPowerDbm
//               << ", Rx Power With Weather: " << finalRxPowerDbm
//               << std::endl;

//     return finalRxPowerDbm;
// }

int64_t WeatherAttenuationModel::DoAssignStreams(int64_t stream)
{
  if (m_child)
  {
    stream = m_child->AssignStreams(stream);
  }
  return stream;
}

} // namespace ns3
