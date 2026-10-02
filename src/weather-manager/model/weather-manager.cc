
#include "weather-manager.h"
#include <iostream> // For debug output
#include <cmath>    // For log10 and pow functions
#include "ns3/weather-waypoint-mobility-model.h"
#include <iomanip>
#include "ns3/rng-seed-manager.h"



namespace ns3 {

NS_LOG_COMPONENT_DEFINE("WeatherManager");
NS_OBJECT_ENSURE_REGISTERED(WeatherManager);

TypeId WeatherManager::GetTypeId() {
  static TypeId tid = TypeId("ns3::WeatherManager")
    .SetParent<Object>()
    .SetGroupName("Weather")
    .AddConstructor<WeatherManager>();
  return tid;
}

WeatherManager::WeatherManager()
:// m_averagesFilename("weather_averages.csv"),
m_historyFilename("weather_history.csv")
{
  
  NS_LOG_INFO("WeatherManager initialized");
}

WeatherManager::~WeatherManager() {
  NS_LOG_INFO("WeatherManager destroyed");
}


/**
 * @brief Sets a weather condition dynamically.
 * @param conditionType The type of weather condition (e.g., "RainRate", "FogDensity").
 * @param value The numerical value for the condition (e.g., rain rate in mm/h).
 */


void WeatherManager::SetWeatherCondition(const std::string &conditionType, double value) {
    double now = Simulator::Now().GetSeconds();
    m_weatherConditions[conditionType] = value;

    // For the first entry, use t=0; for all others, use current simulation time.
    if (m_weatherHistory[conditionType].empty()) {
        m_weatherHistory[conditionType].push_back({0.0, value});
        NS_LOG_INFO(" Initial Weather Condition Set: " << conditionType << " = " << value << " (Time 0s)");
    } else {
        m_weatherHistory[conditionType].push_back({now, value});
        NS_LOG_INFO(" Weather Condition Updated: " << conditionType << " = " << value << " (Time " << now << "s)");
    }
}


/**
 * @brief Compute the ITU-R P.838-3 coefficients (k and alpha) for rain attenuation.
 * @param frequency The frequency in GHz.
 * @param polarization "horizontal" or "vertical".
 * @param k Output parameter for the attenuation coefficient.
 * @param alpha Output parameter for the exponent.
 */
void
WeatherManager::GetRainCoefficients(double frequency,
                                    std::string polarization,
                                    double &k,
                                    double &alpha) const
{
    // ITU-R P.838-3 coefficients for kH (horizontal polarization)
    // Static local arrays: initialized once, no per-call allocations.
    static const double kH_coeff[4][3] = {
        {-5.33980, -0.10008, 1.13098},
        {-0.35351,  1.26970, 0.45400},
        {-0.23789,  0.86036, 0.15354},
        {-0.94158,  0.64552, 0.16817}
    };

    // ITU-R P.838-3 coefficients for kV (vertical polarization)
    static const double kV_coeff[4][3] = {
        {-3.80595, 0.56934, 0.81061},
        {-3.44965,-0.22911, 0.51059},
        {-0.39902, 0.73042, 0.11899},
        { 0.50167, 1.07319, 0.27195}
    };

    // ITU-R P.838-3 coefficients for alphaH (horizontal polarization)
    static const double alphaH_coeff[5][3] = {
        {-0.14318,  1.82442, -0.55187},
        { 0.29591,  0.77564,  0.19822},
        { 0.32177,  0.63773,  0.13164},
        {-5.37610, -0.96230,  1.47828},
        {16.17210, -3.29980,  3.43990}
    };

    // ITU-R P.838-3 coefficients for alphaV (vertical polarization)
    static const double alphaV_coeff[5][3] = {
        {-0.07771,   2.33840, -0.76284},
        { 0.56727,   0.95545,  0.54039},
        {-0.20238,   1.14520,  0.26809},
        {-48.29910,  0.791669, 0.116226},
        { 48.58330,  0.791459, 0.116479}
    };

    // coefficients from ITU-R Table
    const bool horizontal = (polarization == "horizontal");

    double m_k     = horizontal ? -0.18961   : -0.16398;
    double c_k     = horizontal ?  0.71147   :  0.63297;
    double m_alpha = horizontal ?  0.67849   : -0.053739;
    double c_alpha = horizontal ? -1.95537   :  0.83433;

    // Select appropriate coefficient tables
    const double (*k_coeff)[3]     = horizontal ? kH_coeff     : kV_coeff;
    const double (*alpha_coeff)[3] = horizontal ? alphaH_coeff : alphaV_coeff;

    double logF      = std::log10(frequency);
    double sum_k     = 0.0;
    double sum_alpha = 0.0;

    // Compute k (4 terms)
    for (int i = 0; i < 4; ++i)
    {
        const double *c = k_coeff[i];
        double a = c[0];
        double b = c[1];
        double cval = c[2];
        sum_k += a * std::exp(-std::pow((logF - b) / cval, 2.0));
    }

    // Compute alpha (5 terms)
    for (int i = 0; i < 5; ++i)
    {
        const double *c = alpha_coeff[i];
        double a = c[0];
        double b = c[1];
        double cval = c[2];
        sum_alpha += a * std::exp(-std::pow((logF - b) / cval, 2.0));
    }

    // Apply correction coefficients
    sum_k     += m_k * logF + c_k;
    sum_alpha += m_alpha * logF + c_alpha;

    // Final values (same semantics as your original)
    k     = std::pow(10.0, sum_k);
    alpha = sum_alpha;
}
/**
 * @brief Calculate Rain Attenuation using ITU-R P.838-3 model.
 * @param rainRate Rain rate in mm/h.
 * @param pathLength Path length in km.
 * @param frequency Frequency in GHz.
 * @param polarization Polarization type ("horizontal" or "vertical").
 * @return Attenuation in dB.
 */
double WeatherManager::CalculateRainAttenuation(double rainRate, double pathLength, double frequency, std::string polarization) const {
  double k, alpha;
  GetRainCoefficients(frequency, polarization, k, alpha);
  return k * pow(rainRate, alpha) * pathLength;
}

/**
 * @brief Retrieves rain attenuation for a given link.
 */
double WeatherManager::GetRainAttenuation(Ptr<MobilityModel> a, Ptr<MobilityModel> b, double frequency, std::string polarization) const {
  auto it = m_weatherConditions.find("RainRate");
  if (it == m_weatherConditions.end()) return 0.0;
  double rainRate = it->second;
  double pathLength = a->GetDistanceFrom(b) / 1000.0;
  return CalculateRainAttenuation(rainRate, pathLength, frequency, polarization);
}

/**
 * @brief Debug function to print rain attenuation coefficients for different frequencies based on ITU-R values.
 */
void WeatherManager::DebugRainCoefficients() const {
  std::cout << "=== Debugging Rain Attenuation Coefficients (ITU-R P.838-3) ===" << std::endl;
  std::cout << "Frequency (GHz) | kH  | alphaH | kV  | alphaV" << std::endl;
  std::cout << "------------------------------------------------------" << std::endl;

  for (double frequency = 1.0; frequency <= 100.0; frequency += 1.0) {
    double kH, alphaH, kV, alphaV;
    GetRainCoefficients(frequency, "horizontal", kH, alphaH);
    GetRainCoefficients(frequency, "vertical", kV, alphaV);

    std::cout << frequency << " GHz | " 
              << kH << " | " << alphaH << " | " 
              << kV << " | " << alphaV << std::endl;
  }
  std::cout << "=== End of Debugging ===" << std::endl;
}




double WeatherManager::GetEffectiveLOSRange(double nominalRange) const {
    // Retrieve weather conditions
    double rainRate = GetWeatherCondition("RainRate"); // mm/h
    double fogDensity = GetWeatherCondition("FogDensity"); // g/m³
    double fogSeverity = std::clamp(fogDensity, 0.0, 1.0); // dimensionless


    // Default LOS range (no attenuation)
    double effectiveRange = nominalRange;

    
    // Check for rain impact on LOS
    if (rainRate > 0.0) {
        const double rainRetentionCap = 0.5;  // original setting

        double rainAttenuationFactor =
            1.0 - (0.02 * rainRate);

        effectiveRange *=
            std::max(rainRetentionCap, rainAttenuationFactor);
    }

    // Check for fog impact on LOS
    // Check for fog impact on LOS
    if (fogDensity > 0.0) {

        const double fogRetentionCap = 0.3;  // 30% original setting

        double fogAttenuationFactor =
            1.0 - ((1.0 - fogRetentionCap) * fogSeverity);

        effectiveRange *=
            std::max(fogRetentionCap, fogAttenuationFactor);
    }
    //  Log to CSV whenever LOS is affected by weather
    std::ofstream losLogFile;
    losLogFile.open("LOS_changes.csv", std::ios_base::app); // Append mode
          losLogFile << "routing,Seed, RunNum ,Time,nominalRange,rainRate,fogDensity,effectiveRange\n";

    if (losLogFile.is_open()) {
        losLogFile  << m_routingProtocol << ","
                 << m_scenarioId << ","
                 << m_runNumber << ","       
        
        <<Simulator::Now().GetSeconds() << "," // Simulation time
                   << nominalRange << ","                 // Original LOS
                   << rainRate << ","                     // Rain intensity
                   << fogDensity << ","                   // Fog density
                   << effectiveRange << "\n";             // Adjusted LOS
        losLogFile.close();
    } else {
        std::cerr << " [WeatherManager] Could not open LOS_changes.csv for writing!" << std::endl;
    }

    return effectiveRange;
}




double WeatherManager::CalculateFogAttenuation(double fogDensity, double pathLength, double frequency, double temperature) const {
  
    // Convert temperature to Kelvin
    double T = temperature + 273.15;
   // std::cout << "Temperature (Kelvin): " << T << "\n";

    // Compute theta
    double theta = 300.0 / T;
    //std::cout << "Theta: " << theta << "\n";

    // Compute dielectric permittivity parameters
    double epsilon_0 = 77.66 + 103.3 * (theta - 1);
    double epsilon_1 = 0.0671*epsilon_0;  // Fixed constant
    double epsilon_2 = 3.52;  // Fixed constant

  
    //  Relaxation Frequencies (GHz)
    double d = theta - 1.0;
    double f_p = 20.20 - 146.0 * d + 316.0 * d * d;

    //double f_p = 20.20 - (146.0 * (theta - 1)) + (316.0 * pow((theta - 1), 2));
    double f_s = 39.8 * f_p;

    //  Real Permittivity Calculation (ε')
    const double den_p = 1.0 + (frequency / f_p) * (frequency / f_p);
    const double den_s = 1.0 + (frequency / f_s) * (frequency / f_s);

    double epsilon_real =
        (epsilon_0 - epsilon_1) / den_p +
        (epsilon_1 - epsilon_2) / den_s +
        epsilon_2;

  

    double epsilon_imag =
    (frequency * (epsilon_0 - epsilon_1)) / (f_p * den_p) +
    (frequency * (epsilon_1 - epsilon_2)) / (f_s * den_s);

    if (epsilon_imag < 1e-6) {
      //  std::cout << " Warning: ε''(f) is too small! Adjusting...\n";
        epsilon_imag = std::max(epsilon_imag, 1e-6);
    }

    double eta = (2.0 + epsilon_real) / epsilon_imag;

   
    //  the Attenuation Coefficient Calculation
    double K_l = (0.819 * frequency) / (epsilon_imag * (1.0 + eta * eta));

  

    //Compute Specific Attenuation (dB/km)
    double specificAttenuation = K_l * fogDensity;
    //std::cout << "Specific Attenuation (dB/km): " << specificAttenuation << "\n";

    // Compute Total Attenuation over Path Length (dB)
    double totalAttenuation = specificAttenuation * pathLength;
    // std::cout << "Total Attenuation (dB) over " << pathLength << " km: " << totalAttenuation << "\n";
    // std::cout << "===== End Debugging Fog Attenuation =====\n\n";

    return totalAttenuation;
}


double WeatherManager::GetFogAttenuation(Ptr<MobilityModel> a, Ptr<MobilityModel> b, double frequency, double temperature) const {
    /**
     * Retrieves the **fog attenuation** between two nodes based on current weather conditions.
     * - Fetches fog density from `m_weatherConditions`.
     * - Uses **distance** between nodes to compute **path length** (km).
     * - Calls `CalculateFogAttenuation()` to compute final **attenuation in dB**.
     */

    // Fetch fog density from weather conditions
    auto it = m_weatherConditions.find("FogDensity");
    if (it == m_weatherConditions.end()) {
        NS_LOG_WARN("Fog density not set. Returning 0 attenuation.");
        return 0.0;
    }

    double fogDensity = it->second; // g/m³ (user input)
    double pathLength = a->GetDistanceFrom(b) / 1000.0; // Convert meters to km

    // Compute and return fog attenuation in dB
    return CalculateFogAttenuation(fogDensity, pathLength, frequency, temperature);
}
double WeatherManager::CalculateSnowAttenuation(double snowRate,
                                                double pathLengthKm,
                                                double frequency,
                                                bool isWetSnow) const
{
    if (snowRate <= 0.0 || pathLengthKm <= 0.0 || frequency <= 0.0)
        return 0.0;

    const double eta = isWetSnow ? 0.6 : 0.3;   // always < 1
    const std::string pol = "horizontal";

    // Use the same numeric rate as rain, but downscale the resulting loss
    const double rainDbSameRate =
        CalculateRainAttenuation(snowRate, pathLengthKm, frequency, pol);

    return eta * rainDbSameRate;
}
double WeatherManager::GetSnowAttenuation(Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b,
                                          double frequency) const
{
    /**
     * Retrieves snow attenuation (dB) between two nodes.
     *
     * Steps:
     * 1. Read snow intensity from WeatherManager state
     * 2. Measure node-to-node distance via MobilityModel
     * 3. Convert distance from meters to kilometers
     * 4. Determine wet/dry snow condition
     * 5. Call CalculateSnowAttenuation() to compute total loss
     */

    // Step 1: Get snow rate (mm/h)
    auto it = m_weatherConditions.find("SnowRate");
    if (it == m_weatherConditions.end()) {
        return 0.0;
    }
    double snowRate = std::max(0.0, it->second);

    // Step 2: Get distance between nodes (meters -> km)
    double pathLengthKm = a->GetDistanceFrom(b) / 1000.0;
    if (pathLengthKm <= 0.0)
        return 0.0;

    // Step 3: Determine wet/dry snow
    bool isWetSnow = false;
    auto itWet = m_weatherConditions.find("WetSnow");
    if (itWet != m_weatherConditions.end()) {
        isWetSnow = (itWet->second > 0.5);
    }

    // Step 4: Compute total snow attenuation (dB)
    return CalculateSnowAttenuation(snowRate,
                                    pathLengthKm,
                                    frequency,
                                    isWetSnow);
}
double WeatherManager::CalculateGasAttenuation(double frequency,
                                               double temperature,
                                               double humidity) const
{
    /**
     * ITU-R P.676 (Approximate, MANET-safe implementation)
     *
     * Computes atmospheric gaseous attenuation (dB/km) due to:
     * - Oxygen absorption
     * - Water vapour absorption
     *
     * Inputs:
     * - frequency  : GHz
     * - temperature: °C
     * - humidity   : water vapour density (g/m^3)
     *
     * Returns:
     * - Total gaseous attenuation (dB/km)
     *
     * NOTE:
     * - This follows the STRUCTURE of ITU-R P.676 but uses a simplified
     *   approximation suitable for sub-6 GHz MANET simulations.
     */

    // Guard conditions
    if (frequency <= 0.0) return 0.0;

    // Convert temperature to Kelvin
    const double T = temperature + 273.15;
    const double theta = 300.0 / T;

    // Standard sea-level pressure (hPa)
    const double P = 1013.25;

    // Convert water vapour density (g/m^3) → partial pressure e (hPa)
    // ITU-R P.676 relation
    const double e = (humidity * T) / 216.7;

    // Dry air pressure
    const double p = P - e;
    if (p <= 0.0) return 0.0;

    /* ---------------------------------------------------------
     * Oxygen specific attenuation γ_O2 (dB/km)
     * Approximate form capturing low-frequency behaviour
     * --------------------------------------------------------- */
    double gamma_O2 =
        0.1820 * frequency *
        (p * theta * theta) /
        (1.0 + 0.0001 * frequency * frequency);

    /* ---------------------------------------------------------
     * Water vapour specific attenuation γ_H2O (dB/km)
     * Includes humidity dependence and 22 GHz resonance shape
     * --------------------------------------------------------- */
    double gamma_H2O =
        0.1820 * frequency *
        (e * theta * theta * theta) /
        (1.0 + std::pow(frequency / 22.235, 2));

    // Total gaseous attenuation (dB/km)
    return gamma_O2 + gamma_H2O;
}

double WeatherManager::GetGasAttenuation(double pathLength,
                                         double frequency,
                                         double temperature,
                                         double humidity) const
{
    /**
     * Retrieves the gas attenuation based on atmospheric conditions.
     *
     * Inputs:
     * - pathLength (meters)   <-- IMPORTANT
     * - frequency (GHz)
     * - temperature (°C)
     * - humidity (g/m³)
     *
     * Returns:
     * - Total attenuation (dB)
     */

    if (pathLength <= 0.0) return 0.0;

    // Convert meters → km
    double pathLengthKm = pathLength / 1000.0;

    // Attenuation per km (dB/km)
    double attenuationPerKm =
        CalculateGasAttenuation(frequency, temperature, humidity);

    // Total attenuation (dB)
    return attenuationPerKm * pathLengthKm;
}

std::map<Ptr<Node>, Ptr<MobilityModel>> m_nodeMobilityMap;

void WeatherManager::ScheduleMobilityReduction(Ptr<MobilityModel> model, const std::string& nodeType, double interval)
{
  // 1. Apply the reduction **now**
  EvaluateAndApplyMobilityReduction(model, nodeType);

  // 2. Schedule the next round after `interval` seconds
  Simulator::Schedule(Seconds(interval), &WeatherManager::ScheduleMobilityReduction, this, model, nodeType, interval);
}
void WeatherManager::SetSpeedLogFile(const std::string &filename) {
  m_speedLogFileName = filename;
}
void
WeatherManager::SetMobilitySensitivity(double sensitivity)
{
    if (sensitivity < 0.0)
    {
        sensitivity = 0.0;
    }
    else if (sensitivity > 1.0)
    {
        sensitivity = 1.0;
    }

    m_mobilitySensitivity = sensitivity;

    NS_LOG_INFO(
        "WeatherManager mobility sensitivity set to "
        << m_mobilitySensitivity);
}


double
WeatherManager::GetMobilitySensitivity() const
{
    return m_mobilitySensitivity;
}

void
WeatherManager::EvaluateAndApplyMobilityReduction(
    Ptr<MobilityModel> model,
    const std::string& nodeType)
{
    // ---------------------------------------------------------------------
    // 0. Basic validation
    // ---------------------------------------------------------------------

    if (!model)
    {
        NS_LOG_WARN("EvaluateAndApplyMobilityReduction called with null model.");
        return;
    }

    Ptr<Node> node = model->GetObject<Node>();

    if (!node)
    {
        NS_LOG_WARN("MobilityModel is not attached to a valid Node.");
        return;
    }

    const uint32_t nodeId = node->GetId();


    // ---------------------------------------------------------------------
    // 1. Retrieve current environmental state
    // ---------------------------------------------------------------------

    const double rain =
        GetWeatherCondition("RainRate");       // mm/h

    const double fog =
        GetWeatherCondition("FogDensity");     // g/m^3

    const double snow =
        GetWeatherCondition("SnowRate");       // mm/h

    const double wind =
        GetWeatherCondition("WindSpeed");      // m/s

    const WeatherThresholds& t = m_thresholds;


    // ---------------------------------------------------------------------
    // 2. Determine current node speed
    // ---------------------------------------------------------------------

    Vector velocity = model->GetVelocity();

    const double currentSpeed =
        std::sqrt(
            velocity.x * velocity.x +
            velocity.y * velocity.y +
            velocity.z * velocity.z);

    // Do not attempt to rescale a stationary/paused node here.
    if (currentSpeed == 0.0)
    {
        return;
    }


    // ---------------------------------------------------------------------
    // 3. Store reference/original speed once
    //
    // This preserves the behaviour of the original implementation.
    // ---------------------------------------------------------------------

    if (m_originalSpeed.find(nodeId) == m_originalSpeed.end())
    {
        m_originalSpeed[nodeId] = currentSpeed;

        NS_LOG_INFO(
            "Stored original speed for Node "
            << nodeId
            << ": "
            << currentSpeed
            << " m/s");
    }

    const double baseSpeed = m_originalSpeed[nodeId];


    // ---------------------------------------------------------------------
    // 4. Calculate ORIGINAL heuristic mobility factor
    //
    // IMPORTANT:
    // These are the SAME heuristic values used in the original model.
    // Nothing in this section is changed by the sensitivity experiment.
    //
    // Multiple active weather stressors are composed multiplicatively:
    //
    // F_original =
    //     F_fog *
    //     F_rain *
    //     F_snow *
    //     F_wind
    // ---------------------------------------------------------------------

    double originalReductionFactor = 1.0;


    // =====================================================================
    // FOG
    // =====================================================================

    if (fog >= t.fogCritical)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.6;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.7;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.4;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in fogCritical: "
                << nodeType);
        }
    }
    else if (fog >= t.fogDense)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.7;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.8;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.7;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in fogDense: "
                << nodeType);
        }
    }
    else if (fog >= t.fogModerate)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.85;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.9;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.8;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in fogModerate: "
                << nodeType);
        }
    }
    else if (fog >= t.fogLight)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.95;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.98;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.95;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in fogLight: "
                << nodeType);
        }
    }


    // =====================================================================
    // RAIN
    // =====================================================================

    if (rain >= t.rainSevere)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.05;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.20;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.40;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in rainSevere: "
                << nodeType);
        }
    }
    else if (rain >= t.rainHeavy)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.20;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.80;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.80;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in rainHeavy: "
                << nodeType);
        }
    }
    else if (rain >= t.rainModerate)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.60;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.90;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.90;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in rainModerate: "
                << nodeType);
        }
    }
    else if (rain >= t.rainLight)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.90;
        }
        else if (
            nodeType == "vehicle" ||
            nodeType == "drone")
        {
            originalReductionFactor *= 1.00;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in rainLight: "
                << nodeType);
        }
    }


    // =====================================================================
    // SNOW
    // =====================================================================

    if (snow >= t.snowBlizzard)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.10;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.40;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.20;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in snowBlizzard: "
                << nodeType);
        }
    }
    else if (snow >= t.snowHeavy)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.50;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.60;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.50;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in snowHeavy: "
                << nodeType);
        }
    }
    else if (snow >= t.snowModerate)
    {
        if (nodeType == "foot")
        {
            originalReductionFactor *= 0.70;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.85;
        }
        else if (nodeType == "drone")
        {
            originalReductionFactor *= 0.70;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in snowModerate: "
                << nodeType);
        }
    }
    else if (snow >= t.snowLight)
    {
        if (
            nodeType == "foot" ||
            nodeType == "vehicle" ||
            nodeType == "drone")
        {
            originalReductionFactor *= 0.95;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in snowLight: "
                << nodeType);
        }
    }


    // =====================================================================
    // WIND
    // =====================================================================

    if (wind >= t.windDangerous)
    {
        if (nodeType == "drone")
        {
            originalReductionFactor *= 0.20;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.50;
        }
        else if (nodeType == "foot")
        {
            originalReductionFactor *= 0.40;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in windDangerous: "
                << nodeType);
        }
    }
    else if (wind >= t.windUnstable)
    {
        if (nodeType == "drone")
        {
            originalReductionFactor *= 0.40;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.70;
        }
        else if (nodeType == "foot")
        {
            originalReductionFactor *= 0.70;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in windUnstable: "
                << nodeType);
        }
    }
    else if (wind >= t.windDrift)
    {
        if (nodeType == "drone")
        {
            originalReductionFactor *= 0.70;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.85;
        }
        else if (nodeType == "foot")
        {
            originalReductionFactor *= 0.85;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in windDrift: "
                << nodeType);
        }
    }
    else if (wind >= t.windNoticeable)
    {
        if (nodeType == "drone")
        {
            originalReductionFactor *= 0.90;
        }
        else if (nodeType == "vehicle")
        {
            originalReductionFactor *= 0.95;
        }
        else if (nodeType == "foot")
        {
            originalReductionFactor *= 0.95;
        }
        else
        {
            NS_LOG_WARN(
                "Unknown node type in windNoticeable: "
                << nodeType);
        }
    }


    // ---------------------------------------------------------------------
    // 5. Apply sensitivity coefficient
    //
    // ORIGINAL MODEL:
    //
    //     lambda = 1
    //
    // gives:
    //
    //     appliedFactor = originalReductionFactor
    //
    //
    // NO MOBILITY EFFECT:
    //
    //     lambda = 0
    //
    // gives:
    //
    //     appliedFactor = 1
    //
    //
    // Intermediate values smoothly interpolate between the two.
    // ---------------------------------------------------------------------

    double lambda = m_mobilitySensitivity;

    if (lambda < 0.0)
    {
        lambda = 0.0;
    }
    else if (lambda > 1.0)
    {
        lambda = 1.0;
    }

    const double appliedReductionFactor =
        1.0 -
        lambda *
        (1.0 - originalReductionFactor);


    // ---------------------------------------------------------------------
    // 6. Compute reference new speed
    // ---------------------------------------------------------------------

    const double requestedNewSpeed =
        baseSpeed * appliedReductionFactor;


    // ---------------------------------------------------------------------
    // 7. Apply to the actual mobility model
    // ---------------------------------------------------------------------

    if (
        Ptr<ConstantVelocityMobilityModel> cvm =
            DynamicCast<ConstantVelocityMobilityModel>(model))
    {
        Vector direction(
            velocity.x / currentSpeed,
            velocity.y / currentSpeed,
            velocity.z / currentSpeed);

        cvm->SetVelocity(
            direction * requestedNewSpeed);

        NS_LOG_INFO(
            "Node "
            << nodeId
            << " (ConstantVelocity)"
            << " OriginalFactor="
            << originalReductionFactor
            << " Sensitivity="
            << lambda
            << " AppliedFactor="
            << appliedReductionFactor
            << " NewSpeed="
            << requestedNewSpeed);
    }

    else if (
        Ptr<WeatherWaypointMobilityModel> wwm =
            DynamicCast<WeatherWaypointMobilityModel>(model))
    {
        /*
         * WeatherWaypointMobilityModel already knows the base speed
         * for its active movement leg.
         *
         * Therefore pass the SCALE, not requestedNewSpeed.
         */
        wwm->SetSpeedScale(
            appliedReductionFactor);

        NS_LOG_INFO(
            "Node "
            << nodeId
            << " (WeatherWaypoint)"
            << " OriginalFactor="
            << originalReductionFactor
            << " Sensitivity="
            << lambda
            << " AppliedFactor="
            << appliedReductionFactor);
    }

    else if (
        Ptr<RandomWaypointMobilityModel> rwm =
            DynamicCast<RandomWaypointMobilityModel>(model))
    {
        rwm->SetAttribute(
            "Speed",
            DoubleValue(requestedNewSpeed));

        NS_LOG_INFO(
            "Node "
            << nodeId
            << " (RandomWaypoint)"
            << " OriginalFactor="
            << originalReductionFactor
            << " Sensitivity="
            << lambda
            << " AppliedFactor="
            << appliedReductionFactor
            << " RequestedSpeed="
            << requestedNewSpeed);
    }

    else if (
        Ptr<GaussMarkovMobilityModel> gmm =
            DynamicCast<GaussMarkovMobilityModel>(model))
    {
        gmm->SetAttribute(
            "MeanVelocity",
            DoubleValue(requestedNewSpeed));

        NS_LOG_INFO(
            "Node "
            << nodeId
            << " (GaussMarkov)"
            << " OriginalFactor="
            << originalReductionFactor
            << " Sensitivity="
            << lambda
            << " AppliedFactor="
            << appliedReductionFactor
            << " RequestedSpeed="
            << requestedNewSpeed);
    }

    else
    {
        NS_LOG_WARN(
            "Node "
            << nodeId
            << " uses an unsupported mobility model.");
    }


    // ---------------------------------------------------------------------
    // 8. Query resulting velocity for diagnostic logging
    // ---------------------------------------------------------------------

    Vector updatedVelocity =
        model->GetVelocity();

    const double observedSpeed =
        std::sqrt(
            updatedVelocity.x * updatedVelocity.x +
            updatedVelocity.y * updatedVelocity.y +
            updatedVelocity.z * updatedVelocity.z);


    // ---------------------------------------------------------------------
    // 9. Write detailed sensitivity-aware mobility log
    // ---------------------------------------------------------------------

    std::ofstream speedLogFile(
        m_speedLogFileName,
        std::ios_base::app);

    if (speedLogFile.is_open())
    {
        if (speedLogFile.tellp() == 0)
        {
            speedLogFile
                << "routing,"
                << "Seed,"
                << "RunNum,"
                << "Time,"
                << "NodeId,"
                << "NodeType,"
                << "ReferenceBaseSpeed,"
                << "OriginalHeuristicFactor,"
                << "MobilitySensitivity,"
                << "AppliedReductionFactor,"
                << "RequestedNewSpeed,"
                << "ObservedSpeed,"
                << "RainRate,"
                << "FogDensity,"
                << "SnowRate,"
                << "WindSpeed"
                << "\n";
        }

        speedLogFile
            << m_routingProtocol << ","
            << m_scenarioId << ","
            << m_runNumber << ","
            << Simulator::Now().GetSeconds() << ","
            << nodeId << ","
            << nodeType << ","
            << baseSpeed << ","
            << originalReductionFactor << ","
            << lambda << ","
            << appliedReductionFactor << ","
            << requestedNewSpeed << ","
            << observedSpeed << ","
            << rain << ","
            << fog << ","
            << snow << ","
            << wind
            << "\n";

        speedLogFile.close();
    }
    else
    {
        NS_LOG_ERROR(
            "Unable to open "
            << m_speedLogFileName
            << " for writing.");
    }
}

void WeatherManager::SetMetadata(const std::string &routing, uint32_t scenario, uint32_t run) {
  m_routingProtocol = routing;
  m_scenarioId = scenario;
  m_runNumber = run;
}


void WeatherManager::LogWeatherConditions() const {
  for (const auto& condition : m_weatherConditions) {
    NS_LOG_INFO("Condition: " << condition.first << " = " << condition.second);
  }
}

void WeatherManager::ScheduleWeatherChange(double time, const std::string &conditionType, double value) {
  Simulator::Schedule(Seconds(time), &WeatherManager::UpdateWeatherCondition, this, conditionType, value);
}



void WeatherManager::UpdateWeatherCondition(const std::string &conditionType, double value) {
  double currentTime = Simulator::Now().GetSeconds();

  //  Preserve the first assigned value (if it exists)
  if (m_weatherHistory.find(conditionType) == m_weatherHistory.end() || m_weatherHistory[conditionType].empty()) {
      NS_LOG_WARN(" WARNING: No initial value recorded for " << conditionType << "! Assigning now.");
      m_weatherHistory[conditionType].push_back({ currentTime, value});
  }

  //  Log the previous duration before updating
  if (!m_weatherHistory[conditionType].empty()) {
      auto &history = m_weatherHistory[conditionType];
      history.back().startTime = currentTime - history.back().startTime;
  }

  //  Add the new weather entry with its update time
  m_weatherHistory[conditionType].push_back({currentTime, value});
  m_weatherConditions[conditionType] = value;

  NS_LOG_INFO(" Weather condition updated: " << conditionType << " = " << value << " at time " << currentTime);


}


//GET AVERG AS A METHOD start to end
std::map<std::string, double>
WeatherManager::GetAverageWeatherConditions() const
{
    double now = Simulator::Now().GetSeconds();
    // Global average from t = 0 to now
    return GetAverageWeatherConditions(0.0, now);
}


std::map<std::string, double>
WeatherManager::GetAverageWeatherConditions(double startTime, double endTime) const
{
    std::map<std::string, double> result;

    if (startTime > endTime) {
        std::swap(startTime, endTime);
    }

    for (const auto& entry : m_weatherHistory) {
        const std::string& conditionType = entry.first;
        const std::vector<WeatherLogEntry>& history = entry.second;

        if (history.empty()) {
            result[conditionType] = 0.0;
            continue;
        }

        double weightedSum   = 0.0;
        double totalDuration = 0.0;

        // We assume 'history' is sorted by startTime
        for (size_t i = 0; i < history.size(); ++i) {
            double segStart = history[i].startTime;
            double segEnd   = (i + 1 < history.size())
                              ? history[i + 1].startTime
                              : Simulator::Now().GetSeconds();
            double intensity = history[i].intensity;

            // clip interval to [startTime, endTime]
            double overlapStart = std::max(segStart, startTime);
            double overlapEnd   = std::min(segEnd,   endTime);
            double duration     = overlapEnd - overlapStart;

            if (duration > 0.0) {
                weightedSum   += intensity * duration;
                totalDuration += duration;
            }

            // small optimisation: if segStart > endTime we can break early
            if (segStart > endTime) {
                break;
            }
        }

        if (totalDuration > 0.0) {
            result[conditionType] = weightedSum / totalDuration;
        } else {
            result[conditionType] = 0.0;
        }
    }

    return result;
}


void WeatherManager::WriteWeatherHistoryToFile(const std::string& historyFilename) const
{
    uint32_t runNumber = ns3::RngSeedManager::GetRun();
    NS_LOG_INFO(" Dumping full weather history to: " << historyFilename);

    std::ofstream historyFile(historyFilename, std::ios::out | std::ios::app);
    if (!historyFile.is_open()) {
        NS_LOG_ERROR(" Unable to open " << historyFilename << " for writing.");
        return;
    }

    // Header for each run
    historyFile << "\n===== Full Weather History - Run " << runNumber << " =====\n";
    historyFile << "Time (s),Condition,Value\n"; // CSV header

    if (m_weatherHistory.empty()) {
        NS_LOG_WARN(" Weather history is EMPTY!");
        historyFile << " No weather history recorded.\n";
        historyFile.close();
        return;
    }

    // Gather and sort all entries by time
    std::vector<std::tuple<double, std::string, double>> allEntries;
    for (const auto& entry : m_weatherHistory) {
        const std::string& conditionType = entry.first;
        const std::vector<WeatherLogEntry>& history = entry.second;
        for (const auto& log : history) {
            // Correct order: (time, condition, value)
            allEntries.emplace_back(log.startTime, conditionType, log.intensity);
        }
    }
    std::sort(allEntries.begin(), allEntries.end());

    // Write in CSV
    for (const auto& event : allEntries) {
        double time = std::get<0>(event);
        const std::string& type = std::get<1>(event);
        double value = std::get<2>(event);

        historyFile << std::fixed << std::setprecision(2)
                    << time << "," << type << "," << value << "\n";
        NS_LOG_INFO("⏳ " << type << " changed to " << value << " at " << time << "s");
    }

    historyFile.close();
    NS_LOG_INFO(" Weather history successfully written.");
}

void
WeatherManager::SetHistoryFilename(const std::string &filename)
{
  m_historyFilename = filename;
}

double WeatherManager::GetWeatherCondition(const std::string &conditionType) const {
    auto it = m_weatherConditions.find(conditionType);
    if (it != m_weatherConditions.end()) {
        return it->second; // Return the weather condition value if found
    }
    NS_LOG_WARN("Weather condition '" << conditionType << "' not found. Returning default value.");
    return 0.0; // Return default value if the condition is not set
}

void
WeatherManager::DoDispose()
{
  NS_LOG_INFO("🚀 Simulation ending, computing weather averages...");

  WriteWeatherHistoryToFile( m_historyFilename);

  // Finally, call the parent class's Dispose
  Object::DoDispose();
}

WeatherThresholds
WeatherManager::GetThresholds() const
{
    return m_thresholds;
}

void
WeatherManager::SetThresholds(const WeatherThresholds& thresholds)
{
    m_thresholds = thresholds;
}



} // namespace ns3
