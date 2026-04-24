
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
// void WeatherManager::SetWeatherCondition(const std::string &conditionType, double value) {
//   m_weatherConditions[conditionType] = value;
//   NS_LOG_INFO("Weather condition set: " << conditionType << " = " << value);

//    // ✅ Ensure first values are stored in history before updates begin
//    if (m_weatherHistory.find(conditionType) == m_weatherHistory.end() || m_weatherHistory[conditionType].empty()) {
//     double initialTime = 0.0; // Assume first record starts at time 0
//     m_weatherHistory[conditionType].push_back({value, initialTime});
//     NS_LOG_INFO("✅ Initial Weather Condition Set: " << conditionType << " = " << value << " (Time 0s)");
// } else {
//     NS_LOG_INFO("✅ Weather Condition Updated: " << conditionType << " = " << value);
// }
// }

void WeatherManager::SetWeatherCondition(const std::string &conditionType, double value) {
    double now = Simulator::Now().GetSeconds();
    m_weatherConditions[conditionType] = value;

    // For the first entry, use t=0; for all others, use current simulation time.
    if (m_weatherHistory[conditionType].empty()) {
        m_weatherHistory[conditionType].push_back({0.0, value});
        NS_LOG_INFO("✅ Initial Weather Condition Set: " << conditionType << " = " << value << " (Time 0s)");
    } else {
        m_weatherHistory[conditionType].push_back({now, value});
        NS_LOG_INFO("✅ Weather Condition Updated: " << conditionType << " = " << value << " (Time " << now << "s)");
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

// void WeatherManager::GetRainCoefficients(double frequency, std::string polarization, double &k, double &alpha) const {
//     // ITU-R P.838-3 coefficients for kH (horizontal polarization)
//     const std::vector<std::vector<double>> kH_coeff = {
//         {-5.33980, -0.10008, 1.13098},
//         {-0.35351, 1.26970, 0.45400},
//         {-0.23789, 0.86036, 0.15354},
//         {-0.94158, 0.64552, 0.16817}
//     };

//     // ITU-R P.838-3 coefficients for kV (vertical polarization)
//     const std::vector<std::vector<double>> kV_coeff = {
//         {-3.80595, 0.56934, 0.81061},
//         {-3.44965, -0.22911, 0.51059},
//         {-0.39902, 0.73042, 0.11899},
//         { 0.50167, 1.07319, 0.27195}
//     };

//     // ITU-R P.838-3 coefficients for alphaH (horizontal polarization)
//     const std::vector<std::vector<double>> alphaH_coeff = {
//         {-0.14318, 1.82442, -0.55187},
//         { 0.29591, 0.77564,  0.19822},
//         { 0.32177, 0.63773,  0.13164},
//         {-5.37610, -0.96230,  1.47828},
//         {16.1721,  -3.29980,  3.43990}
//     };

//     // ITU-R P.838-3 coefficients for alphaV (vertical polarization)
//     const std::vector<std::vector<double>> alphaV_coeff = {
//         {-0.07771, 2.33840, -0.76284},
//         { 0.56727, 0.95545,  0.54039},
//         {-0.20238, 1.14520,  0.26809},
//         {-48.2991, 0.791669,  0.116226},
//         {48.5833, 0.791459,  0.116479}
//     };

//     //  coefficients from ITU-R Table
//     double m_k = (polarization == "horizontal") ? -0.18961 : -0.16398;
//     double c_k = (polarization == "horizontal") ? 0.71147 : 0.63297;
//     double m_alpha = (polarization == "horizontal") ? 0.67849 : -0.053739;
//     double c_alpha = (polarization == "horizontal") ? -1.95537 : 0.83433;

//     // Select appropriate coefficients based on polarization
//     const std::vector<std::vector<double>>& k_coeff = (polarization == "horizontal") ? kH_coeff : kV_coeff;
//     const std::vector<std::vector<double>>& alpha_coeff = (polarization == "horizontal") ? alphaH_coeff : alphaV_coeff;

//     double logF = std::log10(frequency);
//     double sum_k = 0.0;
//     double sum_alpha = 0.0;

//     // Compute k (4 terms for k)
//     for (const auto& coeff : k_coeff) {
//         sum_k += coeff[0] * std::exp(-std::pow((logF - coeff[1]) / coeff[2], 2));
//     }

//     // Compute alpha (5 terms for alpha) -- FIXED FORMULA!
//     for (const auto& coeff : alpha_coeff) {
//         sum_alpha += coeff[0] * std::exp(-std::pow((logF - coeff[1]) / coeff[2], 2));  
//     }

//     // Apply correction coefficients
//     sum_k += m_k * logF + c_k;
//     sum_alpha += m_alpha * logF + c_alpha;

//     // Final values
//     k = std::pow(10, sum_k);  // ✅ k should be exponentiated
//     alpha = sum_alpha;        // ✅ alpha is summed directly (not exponentiated)

// }



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
        double rainAttenuationFactor = 1.0 - (0.02 * rainRate);  // Example formula, adjust as needed
        effectiveRange *= std::max(0.5, rainAttenuationFactor);  // Limit minimum LOS to 50%
    }

    // Check for fog impact on LOS
    if (fogDensity > 0.0) {
        double fogAttenuationFactor = 1.0 - (0.7 * fogSeverity);  // Example formula, adjust as needed
        effectiveRange *= std::max(0.3, fogAttenuationFactor);  // Limit minimum LOS to 30%
    }

    // ✅ Log to CSV whenever LOS is affected by weather
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
        std::cerr << "⚠️ [WeatherManager] Could not open LOS_changes.csv for writing!" << std::endl;
    }

    return effectiveRange;
}




double WeatherManager::CalculateFogAttenuation(double fogDensity, double pathLength, double frequency, double temperature) const {
    // std::cout << "\n===== Debugging Fog Attenuation Calculation =====\n";
    // std::cout << "Frequency (GHz): " << frequency << "\n";
    // std::cout << "Temperature (Celsius): " << temperature << "\n";

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

    // std::cout << "Epsilon_0: " << epsilon_0 << "\n";
    // std::cout << "Epsilon_1: " << epsilon_1 << "\n";
    // std::cout << "Epsilon_2: " << epsilon_2 << "\n";

    //  Relaxation Frequencies (GHz)
    double d = theta - 1.0;
    double f_p = 20.20 - 146.0 * d + 316.0 * d * d;

    //double f_p = 20.20 - (146.0 * (theta - 1)) + (316.0 * pow((theta - 1), 2));
    double f_s = 39.8 * f_p;

    // std::cout << "f_p (GHz): " << f_p << "\n";
    // std::cout << "f_s (GHz): " << f_s << "\n";

    //  Real Permittivity Calculation (ε')
    const double den_p = 1.0 + (frequency / f_p) * (frequency / f_p);
    const double den_s = 1.0 + (frequency / f_s) * (frequency / f_s);

    double epsilon_real =
        (epsilon_0 - epsilon_1) / den_p +
        (epsilon_1 - epsilon_2) / den_s +
        epsilon_2;

    // double epsilon_real = epsilon_0 - (epsilon_1 / (1 + pow(frequency / f_p, 2))) -
    //                       (epsilon_2 / (1 + pow(frequency / f_s, 2)));

    //  the Imaginary Permittivity Calculation (ε'')

    double epsilon_imag =
    (frequency * (epsilon_0 - epsilon_1)) / (f_p * den_p) +
    (frequency * (epsilon_1 - epsilon_2)) / (f_s * den_s);

    // double epsilon_imag = ((epsilon_1 * (frequency / f_p)) / (1 + pow(frequency / f_p, 2))) +
    //                       ((epsilon_2 * (frequency / f_s)) / (1 + pow(frequency / f_s, 2)));

    // std::cout << "Epsilon_real (fixed): " << epsilon_real << "\n";
    // std::cout << "Epsilon_imag (fixed): " << epsilon_imag << "\n";

    // Ensure epsilon_imag is not too small
    if (epsilon_imag < 1e-6) {
      //  std::cout << "⚠️ Warning: ε''(f) is too small! Adjusting...\n";
        epsilon_imag = std::max(epsilon_imag, 1e-6);
    }

    // Compute η = ε'(f) / ε''(f)
    double eta = (2.0 + epsilon_real) / epsilon_imag;

    //double eta = fabs(epsilon_real) / fabs(epsilon_imag);
   // std::cout << "Eta: " << eta << "\n";

    //  the Attenuation Coefficient Calculation
    double K_l = (0.819 * frequency) / (epsilon_imag * (1.0 + eta * eta));

    //double K_l = (0.819 * frequency) / (epsilon_imag * (1 + pow(eta, 2)));

   // std::cout << "K_l (Specific attenuation coefficient): " << K_l << "\n";

    // // ✅ Ensure K_l is in expected range
    // if (K_l < 0.01) {
    //     std::cout << "⚠️ Warning: K_l is too small! Adjusting...\n";
    //     K_l *= 10;
    // }

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

// double WeatherManager::CalculateSnowAttenuation(double snowRate,
//                                                 double pathLengthKm,
//                                                 double frequency,   // GHz
//                                                 bool isWetSnow) const
// {
//     // Guard against invalid inputs
//     if (snowRate <= 0.0 || pathLengthKm <= 0.0 || frequency <= 0.0)
//         return 0.0;

//     // --- Rain-equivalent abstraction parameters ---
//     // For sub-6 GHz MANET, keep wet snow weaker than heavy rain.
//     // These are "model knobs" you can justify as calibration controls.
//     const double c   = isWetSnow ? 0.5  : 0.2;  // snow -> rain equivalent rate factor
//     const double eta = isWetSnow ? 0.7  : 0.4;  // additional scaling (optional)

//     const double R_eq = c * snowRate; // mm/h rain-equivalent

//     // Use the SAME ITU-R rain model for consistency
//     // (Choose a consistent polarization; horizontal is common in examples.
//     // If you already use vertical elsewhere, match it.)
//     const std::string pol = "horizontal";

//     const double rainDb = CalculateRainAttenuation(R_eq, pathLengthKm, frequency, pol);
//     double snowDb = eta * rainDb;

//     // Optional safety cap (keeps snow "secondary" at sub-6 GHz)
//     // Cap total dB over the link (not per-km), so short links stay realistic.
//     // const double MAX_SNOW_DB = 10.0; // adjust based on scenario; 5–10 dB is often enough for stress tests
//     // if (snowDb > MAX_SNOW_DB) snowDb = MAX_SNOW_DB;

//     return snowDb;
// }

// double WeatherManager::CalculateSnowAttenuation(double snowRate,
//                                                 double pathLengthKm,
//                                                 double frequency,   // GHz (kept for interface consistency)
//                                                 bool isWetSnow) const
// {
//     /**
//      * Empirical snow attenuation model for MANET.
//      *
//      * - Computes specific attenuation (dB/km) as a power-law function of snowfall rate
//      * - Applies different coefficients for wet and dry snow
//      * - Scales by link path length (km) to obtain total attenuation (dB)
//      *
//      * NOTE:
//      * - This is NOT a direct ITU-R P.1817 implementation (FSO-only).
//      * - It is a controlled RF degradation abstraction suitable for MANET simulations.
//      */

//     // Guard against invalid inputs
//     if (snowRate <= 0.0 || pathLengthKm <= 0.0)
//         return 0.0;

//     // Coefficients retained EXACTLY as requested
//     double a, b;
//     if (isWetSnow) {
//         a = 0.005;   // wet snow -> stronger attenuation
//         b = 1.6;
//     } else {
//         a = 0.0034;  // dry snow -> weaker attenuation
//         b = 1.3;
//     }

//     // Specific attenuation (dB/km)
//     double specificAttenuation = a * std::pow(snowRate, b);

//     // Safety cap to prevent snow from dominating MANET propagation
//     const double MAX_DB_PER_KM = 1.0;
//     specificAttenuation = std::min(specificAttenuation, MAX_DB_PER_KM);

//     // Total attenuation over the link (dB)
//     return specificAttenuation * pathLengthKm;
// }
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

// double WeatherManager::CalculateSnowAttenuation(double snowRate, double pathLength, double frequency, bool isWetSnow) const {
//     /**
//      * ITU-R P.1817-1 Snow Attenuation Calculation
//      * Computes the specific attenuation (dB/km) caused by snow.
//      * Formula:
//      *      γ_snow = a * S^b
//      * Where:
//      * - γ_snow = Specific attenuation (dB/km)
//      * - S = Snowfall rate (mm/h)
//      * - a, b = Coefficients based on snow type (wet/dry)
//      * - pathLength = Total path length in km
//      */

//     // Define coefficients for wet and dry snow
//     double a, b;
//     if (isWetSnow) {
//         a = 0.005;  // Wet snow coefficient
//         b = 1.6;
//     } else {
//         a = 0.0034; // Dry snow coefficient
//         b = 1.3;
//     }

//     // Compute specific attenuation (dB/km)
//     double specificAttenuation = a * pow(snowRate, b);

//     // Compute total attenuation over the path length (dB)
//     double totalAttenuation = specificAttenuation * pathLength;

// //     if (isWetSnow) {
//     //const double k_eq = 0.5;   // wet-snow → rain-equivalent factor

// //   double R_eq = k_eq * snowRate;   // e.g., k_eq in [0.3, 1.0]
// //   return CalculateRainAttenuation(R_eq, pathLength, frequency, polarization);
// // }

//     return totalAttenuation;
// }

// double WeatherManager::GetSnowAttenuation(Ptr<MobilityModel> a, Ptr<MobilityModel> b, double frequency) const {
//     /**
//      * Retrieves the **snow attenuation** between two nodes based on current weather conditions.
//      * - Fetches **snowfall rate** from `m_weatherConditions`.
//      * - Uses **distance** between nodes to compute **path length** (km).
//      * - Calls `CalculateSnowAttenuation()` to compute final **attenuation in dB**.
//      */

//     // Fetch snowfall rate from weather conditions
//     auto it = m_weatherConditions.find("SnowRate");
//     if (it == m_weatherConditions.end()) {
//         NS_LOG_WARN("Snow rate not set. Returning 0 attenuation.");
//         return 0.0;
//     }

//     double snowRate = it->second; // mm/h (user input)
//     double pathLength = a->GetDistanceFrom(b) / 1000.0; // Convert meters to km


//     // Retrieve the wet/dry condition for snow
//     bool isWetSnow = false;
//     auto itWet = m_weatherConditions.find("WetSnow");
//     if (itWet != m_weatherConditions.end()) {
//         isWetSnow = (itWet->second != 0.0); // nonzero means wet snow
//     }


//     // Compute and return snow attenuation in dB
//     return CalculateSnowAttenuation(snowRate, pathLength, frequency, isWetSnow);
// }



// double WeatherManager::CalculateGasAttenuation(double frequency, double temperature, double humidity) const {
//     /**
//      * ITU-R P.676-12: Atmospheric Gas Attenuation Model
//      *
//      * Computes attenuation (dB/km) from:
//      * - **Oxygen absorption** (resonance at 60 GHz)
//      * - **Water vapor absorption** (resonance at 22.235 GHz)
//      *
//      * Inputs:
//      * - frequency (GHz)
//      * - temperature (°C)
//      * - humidity (g/m³)
//      *
//      * Returns:
//      * - Total attenuation (dB/km)
//      */

//     // Convert temperature to Kelvin
//     double T = temperature + 273.15;

//     // Standard atmospheric pressure (hPa)
//     const double P_standard = 1013.25;

//     // Compute **specific attenuation** (dB/km) due to oxygen
//     double gamma_O2 = (0.1820 * frequency) / (T * (1 + 0.0004 * (P_standard - 1000)));

//     // Compute **specific attenuation** (dB/km) due to water vapor
//     double gamma_H2O = (0.024 * humidity * frequency) / (T * (1 + 0.0003 * (P_standard - 1000)));

//     // Total gas attenuation
//     double totalAttenuation = gamma_O2 + gamma_H2O;

//     return totalAttenuation; // dB/km
// }
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

// double WeatherManager::GetGasAttenuation(double pathLength, double frequency, double temperature, double humidity) const {
//     /**
//      * Retrieves the **gas attenuation** based on atmospheric conditions.
//      * - Calls `CalculateGasAttenuation()` to compute attenuation per km.
//      * - Scales attenuation based on path length.
//      *
//      * Inputs:
//      * - pathLength (km)
//      * - frequency (GHz)
//      * - temperature (°C)
//      * - humidity (g/m³)
//      *
//      * Returns:
//      * - Total attenuation (dB)
//      */

//     // Compute gas attenuation per km
//     double attenuationPerKm = CalculateGasAttenuation(frequency, temperature, humidity);

//     // Compute total attenuation over the path length (dB)
//     double totalAttenuation = attenuationPerKm * pathLength;

//     return totalAttenuation;
// }

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


void WeatherManager::EvaluateAndApplyMobilityReduction(Ptr<MobilityModel> model, const std::string& nodeType)
{
  if (!model) return;

  double rain = GetWeatherCondition("RainRate");           // mm/h
  double fog = GetWeatherCondition("FogDensity");          // g/m³
  double snow = GetWeatherCondition("SnowRate");           // mm/h
  double wind = GetWeatherCondition("WindSpeed");          // m/s

  const WeatherThresholds& t = m_thresholds;

  Ptr<Node> node = model->GetObject<Node>();
  uint32_t nodeId = node->GetId();

  // Store original speed if not already stored
  Vector velocity = model->GetVelocity();
  double speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
  if (speed == 0.0) return;

  if (m_originalSpeed.find(nodeId) == m_originalSpeed.end())
  {
    m_originalSpeed[nodeId] = speed;
    NS_LOG_INFO("Stored original speed for Node " << nodeId << ": " << speed << " m/s");
  }

  double baseSpeed = m_originalSpeed[nodeId];
  double reductionFactor = 1.0;

  // Apply fog logic
 // ---- Fog (affects all) ----
// ===== FOG Impact (all node types) =====
if (fog >= t.fogCritical) {
    if (nodeType == "foot")       reductionFactor *= 0.6;
    else if (nodeType == "vehicle") reductionFactor *= 0.7;
    else if (nodeType == "drone")   reductionFactor *= 0.4;
    else NS_LOG_WARN("Unknown node type in fogCritical.");
} else if (fog >= t.fogDense) {
    if (nodeType == "foot")       reductionFactor *= 0.7;
    else if (nodeType == "vehicle") reductionFactor *= 0.8;
    else if (nodeType == "drone")   reductionFactor *= 0.7;
    else NS_LOG_WARN("Unknown node type in fogDense.");
} else if (fog >= t.fogModerate) {
    if (nodeType == "foot")       reductionFactor *= 0.85;
    else if (nodeType == "vehicle") reductionFactor *= 0.9;
    else if (nodeType == "drone")   reductionFactor *= 0.8;
    else NS_LOG_WARN("Unknown node type in fogModerate.");
} else if (fog >= t.fogLight) {
    if (nodeType == "foot")       reductionFactor *= 0.95;
    else if (nodeType == "vehicle") reductionFactor *= 0.98;
    else if (nodeType == "drone")   reductionFactor *= 0.95;
    else NS_LOG_WARN("Unknown node type in fogLight.");
}

// ===== RAIN Impact =====
if (rain >= t.rainSevere) {
    if (nodeType == "foot")       reductionFactor *= 0.05;
    else if (nodeType == "vehicle") reductionFactor *= 0.2;
    else if (nodeType == "drone")   reductionFactor *= 0.4;
    else NS_LOG_WARN("Unknown node type in rainSevere.");
} else if (rain >= t.rainHeavy) {
    if (nodeType == "foot")       reductionFactor *= 0.2;
    else if (nodeType == "vehicle") reductionFactor *= 0.8;
    else if (nodeType == "drone")   reductionFactor *= 0.8;
    else NS_LOG_WARN("Unknown node type in rainHeavy.");
} else if (rain >= t.rainModerate) {
    if (nodeType == "foot")       reductionFactor *= 0.6;
    else if (nodeType == "vehicle") reductionFactor *= 0.9;
    else if (nodeType == "drone")   reductionFactor *= 0.9;
    else NS_LOG_WARN("Unknown node type in rainModerate.");
} else if (rain >= t.rainLight) {
    if (nodeType == "foot")       reductionFactor *= 0.9;
    else if (nodeType == "vehicle" || nodeType == "drone") reductionFactor *= 1.0;
    else NS_LOG_WARN("Unknown node type in rainLight.");
}

// ===== SNOW Impact =====
if (snow >= t.snowBlizzard) {
    if (nodeType == "foot")       reductionFactor *= 0.1;
    else if (nodeType == "vehicle") reductionFactor *= 0.4;
    else if (nodeType == "drone")   reductionFactor *= 0.2;
    else NS_LOG_WARN("Unknown node type in snowBlizzard.");
} else if (snow >= t.snowHeavy) {
    if (nodeType == "foot")       reductionFactor *= 0.5;
    else if (nodeType == "vehicle") reductionFactor *= 0.6;
    else if (nodeType == "drone")   reductionFactor *= 0.5;
    else NS_LOG_WARN("Unknown node type in snowHeavy.");
} else if (snow >= t.snowModerate) {
    if (nodeType == "foot")       reductionFactor *= 0.7;
    else if (nodeType == "vehicle") reductionFactor *= 0.85;
    else if (nodeType == "drone")   reductionFactor *= 0.7;
    else NS_LOG_WARN("Unknown node type in snowModerate.");
} else if (snow >= t.snowLight) {
    if (nodeType == "foot" || nodeType == "vehicle" || nodeType == "drone")
        reductionFactor *= 0.95;
    else NS_LOG_WARN("Unknown node type in snowLight.");
}

// ===== WIND Impact (all node types) =====
if (wind >= t.windDangerous) {
    if (nodeType == "drone")       reductionFactor *= 0.2;
    else if (nodeType == "vehicle") reductionFactor *= 0.5;
    else if (nodeType == "foot")    reductionFactor *= 0.4;
    else NS_LOG_WARN("Unknown node type in windDangerous.");
} else if (wind >= t.windUnstable) {
    if (nodeType == "drone")       reductionFactor *= 0.4;
    else if (nodeType == "vehicle") reductionFactor *= 0.7;
    else if (nodeType == "foot")    reductionFactor *= 0.7;
    else NS_LOG_WARN("Unknown node type in windUnstable.");
} else if (wind >= t.windDrift) {
    if (nodeType == "drone")       reductionFactor *= 0.7;
    else if (nodeType == "vehicle") reductionFactor *= 0.85;
    else if (nodeType == "foot")    reductionFactor *= 0.85;
    else NS_LOG_WARN("Unknown node type in windDrift.");
} else if (wind >= t.windNoticeable) {
    if (nodeType == "drone")       reductionFactor *= 0.9;
    else if (nodeType == "vehicle") reductionFactor *= 0.95;
    else if (nodeType == "foot")    reductionFactor *= 0.95;
    else NS_LOG_WARN("Unknown node type in windNoticeable.");
}



  //if (reductionFactor >= 0.99) return;

  double newSpeed = baseSpeed * reductionFactor;

  // Apply to mobility model
  if (Ptr<ConstantVelocityMobilityModel> cvm = DynamicCast<ConstantVelocityMobilityModel>(model))
  {
    Vector direction = Vector(velocity.x / speed, velocity.y / speed, velocity.z / speed);
    cvm->SetVelocity(direction * newSpeed);
    NS_LOG_INFO("Node " << nodeId << " (CVMM) new velocity = " << newSpeed);
  }
  else if (Ptr<RandomWaypointMobilityModel> rwm = DynamicCast<RandomWaypointMobilityModel>(model))
  {
    rwm->SetAttribute("Speed", DoubleValue(newSpeed));
    NS_LOG_INFO("Node " << nodeId << " (RWM) speed set to " << newSpeed);
  }
  else if (Ptr<WeatherWaypointMobilityModel> wwm = DynamicCast<WeatherWaypointMobilityModel>(model))
{
  wwm->SetSpeedScale(reductionFactor);
  NS_LOG_INFO("Node " << nodeId << " (WeatherWaypointMobilityModel) speed scale set to " << newSpeed);
}

  
  else if (Ptr<GaussMarkovMobilityModel> gmm = DynamicCast<GaussMarkovMobilityModel>(model))
  {
    gmm->SetAttribute("MeanVelocity", DoubleValue(newSpeed));
    NS_LOG_INFO("Node " << nodeId << " (GMM) mean speed set to " << newSpeed);
  }
  else
  {
    NS_LOG_WARN("Node " << nodeId << " unsupported model for speed change.");
  }

  std::ofstream speedLogFile(m_speedLogFileName, std::ios_base::app);

  if (speedLogFile.is_open()) {
    if (speedLogFile.tellp() == 0) {
      speedLogFile << "routing,Seed, RunNum ,Time,NodeId,NodeType,OriginalSpeed,ReductionFactor,NewSpeed,RainRate,FogDensity,SnowRate,WindSpeed\n";
    }
    uint32_t nodeId = (node->GetObject<Node>() ? node->GetObject<Node>()->GetId() : 0);

    speedLogFile  << m_routingProtocol << ","
                 << m_scenarioId << ","
                 << m_runNumber << ","
                 << Simulator::Now().GetSeconds() << ","
                 << nodeId << ","
                <<nodeType<<","
                 << baseSpeed << ","
                 << reductionFactor << ","
                 << (baseSpeed * reductionFactor) << ","
                 << GetWeatherCondition("RainRate") << ","
                 << GetWeatherCondition("FogDensity") << ","
                 << GetWeatherCondition("SnowRate") << ","
                 << GetWeatherCondition("WindSpeed") << "\n";
    speedLogFile.close();
  } else {
    NS_LOG_ERROR("Unable to open speed_changes.csv for writing.");
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

// void WeatherManager::UpdateWeatherCondition(const std::string &conditionType, double value) {
//   m_weatherConditions[conditionType] = value;
//   NS_LOG_INFO("Weather condition updated: " << conditionType << " = " << value);
// }

void WeatherManager::UpdateWeatherCondition(const std::string &conditionType, double value) {
  double currentTime = Simulator::Now().GetSeconds();

  // ✅ Preserve the first assigned value (if it exists)
  if (m_weatherHistory.find(conditionType) == m_weatherHistory.end() || m_weatherHistory[conditionType].empty()) {
      NS_LOG_WARN("⚠️ WARNING: No initial value recorded for " << conditionType << "! Assigning now.");
      m_weatherHistory[conditionType].push_back({ currentTime, value});
  }

  // ✅ Log the previous duration before updating
  if (!m_weatherHistory[conditionType].empty()) {
      auto &history = m_weatherHistory[conditionType];
      history.back().startTime = currentTime - history.back().startTime;
  }

  // ✅ Add the new weather entry with its update time
  m_weatherHistory[conditionType].push_back({currentTime, value});
  m_weatherConditions[conditionType] = value;

  NS_LOG_INFO("✅ Weather condition updated: " << conditionType << " = " << value << " at time " << currentTime);


}

// void WeatherManager::UpdateWeatherCondition(const std::string &conditionType, double value) {
//   double currentTime = Simulator::Now().GetSeconds();

//   // ✅ Store the latest weather value (for real-time access)
//   m_weatherConditions[conditionType] = value;

//   // ✅ Log the previous duration before updating
//   if (m_weatherHistory.find(conditionType) != m_weatherHistory.end() && 
//       !m_weatherHistory[conditionType].empty()) {
//       auto &history = m_weatherHistory[conditionType];
//       history.back().startTime = currentTime - history.back().startTime;
//   }

//   // ✅ Add the new weather entry with its start time
//   m_weatherHistory[conditionType].push_back({value, currentTime});

//   NS_LOG_INFO("✅ Weather condition updated: " << conditionType << " = " << value);

//   // ✅ Save the latest weather values to a file
//   std::ofstream outFile("weathereee_conditions.txt", std::ios::out);
//   if (outFile.is_open()) {
//       outFile << "===== Current Weather Conditions =====\n";
//       for (const auto& condition : m_weatherConditions) {
//           outFile << condition.first << " = " << condition.second << "\n";
//       }
//       outFile.close();
//   } else {
//       NS_LOG_ERROR("⚠️ Failed to open weather_conditions.txt for writing!");
//   }
// }

//GET AVERG AS A METHOD start to end
std::map<std::string, double>
WeatherManager::GetAverageWeatherConditions() const
{
    double now = Simulator::Now().GetSeconds();
    // Global average from t = 0 to now
    return GetAverageWeatherConditions(0.0, now);
}

// std::map<std::string, double> WeatherManager::GetAverageWeatherConditions() const {
//     std::map<std::string, double> result;
//     double now = Simulator::Now().GetSeconds(); // Simulation end time

//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;
//         if (history.empty()) {
//             NS_LOG_WARN("No entries found for condition: " << conditionType);
//             result[conditionType] = 0.0;
//             continue;
//         }

//         // Defensive copy and sort (in case)
//         std::vector<WeatherLogEntry> sortedHistory = history;
//         std::sort(sortedHistory.begin(), sortedHistory.end(),
//                   [](const WeatherLogEntry& a, const WeatherLogEntry& b) {
//                       return a.startTime < b.startTime;
//                   });

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         // Handle initial period before first update (if needed)
//         if (sortedHistory[0].startTime > 0.0) {
//             double duration = sortedHistory[0].startTime;
//             double intensity = sortedHistory[0].intensity;
//             weightedSum += intensity * duration;
//             totalDuration += duration;
//             NS_LOG_DEBUG("🕒 " << conditionType << " | Initial: " << intensity 
//                             << " × " << duration << " (from 0s to " << sortedHistory[0].startTime << "s)");
//         }

//         // Process all weather intervals
//         for (size_t i = 0; i < sortedHistory.size(); ++i) {
//             double startTime = sortedHistory[i].startTime;
//             double endTime = (i + 1 < sortedHistory.size())
//                              ? sortedHistory[i + 1].startTime
//                              : now;
//             double duration = endTime - startTime;
//             double intensity = sortedHistory[i].intensity;

//             // Only accumulate if interval is positive
//             if (duration > 0.0) {
//                 weightedSum += intensity * duration;
//                 totalDuration += duration;
//                 NS_LOG_DEBUG("🕒 " << conditionType << " | " << intensity
//                                 << " × " << duration << " (from "
//                                 << startTime << "s to " << endTime << "s)");
//             }
//         }

//         // Compute and store average
//         if (totalDuration > 0.0) {
//             double average = weightedSum / totalDuration;
//             result[conditionType] = average;
//             NS_LOG_INFO("✅ Avg " << conditionType << " = " << average
//                          << " over " << totalDuration << "s");
//         } else {
//             NS_LOG_WARN("⚠️ Total duration = 0 for " << conditionType);
//             result[conditionType] = 0.0;
//         }
//     }
//     return result;
// }

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

// std::map<std::string, double> WeatherManager::GetAverageWeatherConditions() const {
//     std::map<std::string, double> result;
//     double now = Simulator::Now().GetSeconds(); // Current simulation time

//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         if (history.empty()) {
//             NS_LOG_WARN("No entries found for condition: " << conditionType);
//             result[conditionType] = 0.0;
//             continue;
//         }

//         // Sort by start time just in case
//         std::vector<WeatherLogEntry> sortedHistory = history;
//         std::sort(sortedHistory.begin(), sortedHistory.end(),
//                   [](const WeatherLogEntry& a, const WeatherLogEntry& b) {
//                       return a.startTime < b.startTime;
//                   });

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         // 🔹 Handle initial period before first update (from 0 to first startTime)
//         if (sortedHistory[0].startTime > 0.0) {
//             double duration = sortedHistory[0].startTime;
//             double intensity = sortedHistory[0].intensity;

//             weightedSum += intensity * duration;
//             totalDuration += duration;

//             NS_LOG_DEBUG("🕒 " << conditionType << " | Initial: " << intensity 
//                             << " × " << duration << " (from 0s to " << sortedHistory[0].startTime << "s)");
//         }

//         // 🔹 Handle all other intervals
//         for (size_t i = 0; i < sortedHistory.size(); ++i) {
//             double startTime = sortedHistory[i].startTime;
//             double endTime = (i + 1 < sortedHistory.size()) 
//                              ? sortedHistory[i + 1].startTime 
//                              : now;

//             double duration = endTime - startTime;
//             double intensity = sortedHistory[i].intensity;

//             if (duration > 0.0) {
//                 weightedSum += intensity * duration;
//                 totalDuration += duration;

//                 NS_LOG_DEBUG("🕒 " << conditionType << " | " << intensity 
//                                 << " × " << duration << " (from " 
//                                 << startTime << "s to " << endTime << "s)");
//             }
//         }

//         // 🔹 Finalize average
//         if (totalDuration > 0.0) {
//             double average = weightedSum / totalDuration;
//             result[conditionType] = average;
//             NS_LOG_INFO("✅ Avg " << conditionType << " = " << average 
//                          << " over " << totalDuration << "s");
//         } else {
//             NS_LOG_WARN("⚠️ Total duration = 0 for " << conditionType);
//             result[conditionType] = 0.0;
//         }
//     }

//     return result;
// }

// std::map<std::string, double> WeatherManager::GetAverageWeatherConditions() const {
//     std::map<std::string, double> result;
//     double now = Simulator::Now().GetSeconds(); // Cache simulation time for final duration

//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         if (history.empty()) {
//             NS_LOG_WARN("No entries found for condition: " << conditionType);
//             result[conditionType] = 0.0;
//             continue;
//         }

//         // Defensive: sort copy (preserves m_weatherHistory integrity)
//         std::vector<WeatherLogEntry> sortedHistory = history;
//         std::sort(sortedHistory.begin(), sortedHistory.end(),
//                   [](const WeatherLogEntry& a, const WeatherLogEntry& b) {
//                       return a.startTime < b.startTime;
//                   });

//         if (sortedHistory.size() == 1) {
//             NS_LOG_WARN("Only one entry for " << conditionType << " — average may not be accurate.");
//         }

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         for (size_t i = 0; i < sortedHistory.size(); ++i) {
//             double startTime = sortedHistory[i].startTime;
//             double endTime = (i + 1 < sortedHistory.size()) ? sortedHistory[i + 1].startTime : now;

//             double duration = endTime - startTime;
//             double intensity = sortedHistory[i].intensity;

//             if (duration > 0.0) {
//                 weightedSum += intensity * duration;
//                 totalDuration += duration;

//                 NS_LOG_DEBUG("🕒 " << conditionType
//                                 << " | " << intensity << " × " << duration
//                                 << " (from " << startTime << "s to " << endTime << "s)");
//             }
//         }

//         if (totalDuration > 0.0) {
//             double average = weightedSum / totalDuration;
//             result[conditionType] = average;
//             NS_LOG_INFO("✅ Avg " << conditionType << " = " << average
//                          << " over " << totalDuration << "s");
//         } else {
//             NS_LOG_WARN("⚠️ Total duration = 0 for " << conditionType);
//             result[conditionType] = 0.0;
//         }
//     }

//     return result;
// }

// std::map<std::string, double> WeatherManager::GetAverageWeatherConditions() const {
//     std::map<std::string, double> result;
//     double now = Simulator::Now().GetSeconds(); // Cache simulation time for final duration

//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         if (history.empty()) {
//             NS_LOG_WARN("No entries found for condition: " << conditionType);
//             result[conditionType] = 0.0;
//             continue;
//         }

//         // Defensive: sort copy (preserves m_weatherHistory integrity)
//         std::vector<WeatherLogEntry> sortedHistory = history;
//         std::sort(sortedHistory.begin(), sortedHistory.end(),
//                   [](const WeatherLogEntry& a, const WeatherLogEntry& b) {
//                       return a.startTime < b.startTime;
//                   });

//         if (sortedHistory.size() == 1) {
//             NS_LOG_WARN("Only one entry for " << conditionType << " — average may not be accurate.");
//         }

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         for (size_t i = 0; i < sortedHistory.size(); ++i) {
//             double startTime = sortedHistory[i].startTime;
//             double endTime = (i + 1 < sortedHistory.size()) ? sortedHistory[i + 1].startTime : now;

//             double duration = endTime - startTime;
//             double intensity = sortedHistory[i].intensity;

//             if (duration > 0.0) {
//                 weightedSum += intensity * duration;
//                 totalDuration += duration;

//                 NS_LOG_DEBUG("🕒 " << conditionType
//                                 << " | " << intensity << " × " << duration
//                                 << " (from " << startTime << "s to " << endTime << "s)");
//             }
//         }

//         if (totalDuration > 0.0) {
//             double average = weightedSum / totalDuration;
//             result[conditionType] = average;
//             NS_LOG_INFO("✅ Avg " << conditionType << " = " << average
//                          << " over " << totalDuration << "s");
//         } else {
//             NS_LOG_WARN("⚠️ Total duration = 0 for " << conditionType);
//             result[conditionType] = 0.0;
//         }
//     }

//     return result;
// }

// std::map<std::string, double> WeatherManager::GetAverageWeatherConditions() const {
//     std::map<std::string, double> result;

//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         std::vector<WeatherLogEntry> history = entry.second;

//         // Defensive: skip empty entries
//         if (history.empty()) {
//             NS_LOG_WARN("No entries found for condition: " << conditionType);
//             result[conditionType] = 0.0;
//             continue;
//         }

//         // Sort by start time (in case not already sorted)
//         std::sort(history.begin(), history.end(),
//                   [](const WeatherLogEntry& a, const WeatherLogEntry& b) {
//                       return a.startTime < b.startTime;
//                   });

//         if (history.size() == 1) {
//             NS_LOG_WARN("Only one entry for " << conditionType
//                          << " — average may not be accurate.");
//         }

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         for (size_t i = 0; i < history.size(); ++i) {
//             double startTime = history[i].startTime;
//             double endTime = (i + 1 < history.size()) ? history[i + 1].startTime : Simulator::Now().GetSeconds();
//             double duration = endTime - startTime;
//             double intensity = history[i].intensity;

//             if (duration > 0) {
//                 weightedSum += intensity * duration;
//                 totalDuration += duration;
//             }
//         }

//         if (totalDuration > 0.0) {
//             result[conditionType] = weightedSum / totalDuration;
//         } else {
//             NS_LOG_WARN("Total duration is zero for condition: " << conditionType);
//             result[conditionType] = 0.0;
//         }
//     }

//     return result;
// }

// std::map<std::string, double> WeatherManager::GetAverageWeatherConditions() const {
//     std::map<std::string, double> result;

//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         for (size_t i = 0; i < history.size(); ++i) {
//             double startTime = history[i].startTime;
//             double endTime = (i + 1 < history.size()) ? history[i + 1].startTime : Simulator::Now().GetSeconds();
//             double duration = endTime - startTime;
//             double intensity = history[i].intensity;

//             weightedSum += intensity * duration;
//             totalDuration += duration;
//         }

//         if (totalDuration > 0.0) {
//             result[conditionType] = weightedSum / totalDuration;
//         } else {
//             result[conditionType] = 0.0;
//         }
//     }

//     return result;
// }

// GENERATE AVERAGE DOC
// void WeatherManager::ComputeAverageWeatherConditions(const std::string& averagesFilename,
//                                                      const std::string& historyFilename) const
// {
//     uint32_t runNumber = ns3::RngSeedManager::GetRun(); // For reproducibility tracking

//     NS_LOG_INFO("\n===== 🔹 Computing Final Weather Averages 🔹 =====");

//     // Open output files
//     std::ofstream outFile(averagesFilename, std::ios::out | std::ios::app);
//     std::ofstream historyFile(historyFilename, std::ios::out | std::ios::app);

//     if (!outFile.is_open() || !historyFile.is_open()) {
//         NS_LOG_ERROR("⚠️ Failed to open output files for writing!");
//         return;
//     }

//     // Headers for this simulation run
//     outFile << "\n===== Final Weather Averages Run " << runNumber << " =====\n";
//     historyFile << "\n===== Full Weather History Run " << runNumber << " =====\n";

//     if (m_weatherHistory.empty()) {
//         NS_LOG_WARN("⚠️ Weather history is EMPTY! No data to save.");
//         outFile << "⚠️ No weather history recorded.\n";
//         historyFile << "⚠️ No weather history recorded.\n";
//         outFile.close();
//         historyFile.close();
//         return;
//     }

//     // === Chronological Full Weather Log ===
//     std::vector<std::tuple<double, std::string, double>> allEntries;
//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         for (const auto& log : history) {
//             allEntries.emplace_back(log.startTime, conditionType, log.intensity);
//         }
//     }

//     // Sort by time
//     std::sort(allEntries.begin(), allEntries.end());

//     for (const auto& event : allEntries) {
//         double time = std::get<0>(event);
//         const std::string& conditionType = std::get<1>(event);
//         double value = std::get<2>(event);

//         historyFile << "⏳ Time: " << time << "s, " << conditionType << " = " << value << "\n";
//         NS_LOG_INFO("⏳ Time: " << time << "s, " << conditionType << " = " << value);
//     }

//     // === Compute Weighted Averages Per Condition ===
//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         std::vector<WeatherLogEntry> history = entry.second;

//         if (history.empty()) {
//             outFile << conditionType << " - No records available.\n";
//             continue;
//         }

//         // Sort condition-specific logs by time
//         std::sort(history.begin(), history.end(), [](const WeatherLogEntry& a, const WeatherLogEntry& b) {
//             return a.startTime < b.startTime;
//         });

//         if (history.size() == 1) {
//             NS_LOG_WARN("⚠️ Only one entry for " << conditionType << " — average may be misleading.");
//         }

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         for (size_t i = 0; i < history.size(); ++i) {
//             double startTime = history[i].startTime;
//             double endTime = (i + 1 < history.size()) ? history[i + 1].startTime : Simulator::Now().GetSeconds();

//             double duration = endTime - startTime;
//             double intensity = history[i].intensity;

//             weightedSum += intensity * duration;
//             totalDuration += duration;

//             NS_LOG_DEBUG("⏱️ [" << startTime << "–" << endTime << "] "
//                           << conditionType << " = " << intensity
//                           << " × " << duration);
//         }

//         if (totalDuration > 0.0) {
//             double average = weightedSum / totalDuration;
//             NS_LOG_INFO("🌦️ " << conditionType << " - Weighted Avg: " << average);
//             outFile << conditionType << " - Average Intensity: " << average << "\n";
//         } else {
//             NS_LOG_WARN("⚠️ No valid duration recorded for " << conditionType);
//             outFile << conditionType << " - No recorded duration.\n";
//         }
//     }

//     outFile.close();
//     historyFile.close();

//     NS_LOG_INFO("✅ Weather averages appended to " << averagesFilename);
//     NS_LOG_INFO("✅ Full weather history appended to " << historyFilename << " (Chronological Order)");
// }


void WeatherManager::WriteWeatherHistoryToFile(const std::string& historyFilename) const
{
    uint32_t runNumber = ns3::RngSeedManager::GetRun();
    NS_LOG_INFO("📜 Dumping full weather history to: " << historyFilename);

    std::ofstream historyFile(historyFilename, std::ios::out | std::ios::app);
    if (!historyFile.is_open()) {
        NS_LOG_ERROR("❌ Unable to open " << historyFilename << " for writing.");
        return;
    }

    // Header for each run
    historyFile << "\n===== Full Weather History - Run " << runNumber << " =====\n";
    historyFile << "Time (s),Condition,Value\n"; // CSV header

    if (m_weatherHistory.empty()) {
        NS_LOG_WARN("⚠️ Weather history is EMPTY!");
        historyFile << "⚠️ No weather history recorded.\n";
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
    NS_LOG_INFO("✅ Weather history successfully written.");
}

// void WeatherManager::WriteWeatherHistoryToFile(const std::string& historyFilename) const
// {
//     uint32_t runNumber = ns3::RngSeedManager::GetRun();
//     NS_LOG_INFO("📜 Dumping full weather history to: " << historyFilename);

//     std::ofstream historyFile(historyFilename, std::ios::out | std::ios::app);
//     if (!historyFile.is_open()) {
//         NS_LOG_ERROR("❌ Unable to open " << historyFilename << " for writing.");
//         return;
//     }

//     // Header for each run
//     historyFile << "\n===== Full Weather History - Run " << runNumber << " =====\n";
//     historyFile << "Time (s),Condition,Value\n"; // CSV header for clarity

//     if (m_weatherHistory.empty()) {
//         NS_LOG_WARN("⚠️ Weather history is EMPTY!");
//         historyFile << "⚠️ No weather history recorded.\n";
//         historyFile.close();
//         return;
//     }

//     // Gather all log entries and sort by time
//     std::vector<std::tuple<double, std::string, double>> allEntries;
//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;
//         for (const auto& log : history) {
//             allEntries.emplace_back(log.startTime, conditionType, log.intensity);
//         }
//     }
//     std::sort(allEntries.begin(), allEntries.end());

//     // Output in CSV style
//     for (const auto& event : allEntries) {
//         double time = std::get<0>(event);
//         const std::string& type = std::get<1>(event);
//         double intensity = std::get<2>(event);

//         historyFile << std::fixed << std::setprecision(2)
//                     << time << "," << type << "," << intensity << "\n";
//         NS_LOG_INFO("⏳ " << type << " changed to " << intensity << " at " << time << "s");
//     }

//     historyFile.close();
//     NS_LOG_INFO("✅ Weather history successfully written.");
// }

// void WeatherManager::WriteWeatherHistoryToFile(const std::string& historyFilename) const
// {
//     uint32_t runNumber = ns3::RngSeedManager::GetRun(); // Simulation run ID
//     NS_LOG_INFO("📜 Dumping full weather history to: " << historyFilename);

//     std::ofstream historyFile(historyFilename, std::ios::out | std::ios::app);
//     if (!historyFile.is_open()) {
//         NS_LOG_ERROR("❌ Unable to open " << historyFilename << " for writing.");
//         return;
//     }

//     // Header for each run
//     historyFile << "\n===== Full Weather History - Run " << runNumber << " =====\n";

//     if (m_weatherHistory.empty()) {
//         NS_LOG_WARN("⚠️ Weather history is EMPTY!");
//         historyFile << "⚠️ No weather history recorded.\n";
//         historyFile.close();
//         return;
//     }

//     // Collect and sort all history entries by timestamp
//     std::vector<std::tuple<double, std::string, double>> allEntries;
//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         for (const auto& log : history) {
//             allEntries.emplace_back(log.startTime, conditionType, log.intensity);
//         }
//     }
//     std::sort(allEntries.begin(), allEntries.end());

//     // Write to file
//     for (const auto& event : allEntries) {
//         double time = std::get<0>(event);
//         const std::string& type = std::get<1>(event);
//         double intensity = std::get<2>(event);

//         historyFile << std::fixed << std::setprecision(2)
//                     << "Time: " << time << "s, " << type << " = " << intensity << "\n";

//         NS_LOG_INFO("⏳ " << type << " changed to " << intensity << " at " << time << "s");
//     }

//     historyFile.close();
//     NS_LOG_INFO("✅ Weather history successfully written.");
// }

// void WeatherManager::ComputeAverageWeatherConditions(const std::string& averagesFilename,
//   const std::string& historyFilename) const
// {

//   uint32_t  runNumber = ns3::RngSeedManager::GetRun(); //  This changes across replicas


// NS_LOG_INFO("\n===== 🔹 Computing Final Weather Averages 🔹 =====");

// // Open the files in append mode
// std::ofstream outFile(averagesFilename, std::ios::out | std::ios::app);
// std::ofstream historyFile(historyFilename, std::ios::out | std::ios::app);

// if (!outFile.is_open() || !historyFile.is_open()) {
// NS_LOG_ERROR("⚠️ Failed to open output files for writing!");
// return;
// }

// // Write some kind of header or separator so that each run’s results are clearly delineated
// outFile << "\n===== Final Weather Averages "<< runNumber <<" =====\n";
// historyFile << "\n===== Full Weather History "<< runNumber <<" =====\n";

// if (m_weatherHistory.empty()) {
// NS_LOG_WARN("⚠️ Weather history is EMPTY! No data to save.");
// outFile << "⚠️ No weather history recorded.\n";
// historyFile << "⚠️ No weather history recorded.\n";
// outFile.close();
// historyFile.close();
// return;
// }

// // Collect and sort all weather history entries
// std::vector<std::tuple<double, std::string, double>> allEntries;
// for (const auto& entry : m_weatherHistory) {
// const std::string& conditionType = entry.first;
// const std::vector<WeatherLogEntry>& history = entry.second;

// for (const auto& log : history) {
// allEntries.push_back(std::make_tuple(log.startTime, conditionType, log.intensity));
// }
// }
// std::sort(allEntries.begin(), allEntries.end());

// // Print sorted history to historyFile
// for (const auto& event : allEntries) {
// double time = std::get<0>(event);
// std::string conditionType = std::get<1>(event);
// double value = std::get<2>(event);

// historyFile << "⏳ Time: " << time << "s, " << conditionType << " = " << value << "\n";
// NS_LOG_INFO("⏳ Time: " << time << "s, " << conditionType << " = " << value);
// }

// // Compute averages for each weather condition
// for (const auto& entry : m_weatherHistory) {
// const std::string& conditionType = entry.first;
// const std::vector<WeatherLogEntry>& history = entry.second;

// double weightedSum = 0.0;
// double totalDuration = 0.0;

// // for (const auto& log : history) {
// // weightedSum += log.intensity * log.startTime;
// // totalDuration += log.startTime;
// // }

// for (size_t i = 0; i < history.size(); ++i) {
//     double start = history[i].startTime;
//     double end = (i + 1 < history.size()) ? history[i + 1].startTime : Simulator::Now().GetSeconds();
//     double duration = end - start;
//     double intensity = history[i].intensity;

//     weightedSum += intensity * duration;
//     totalDuration += duration;
// }


// if (totalDuration > 0) {
// double average = weightedSum / totalDuration;
// NS_LOG_INFO("🌦️ " << conditionType << " - Average Intensity: " << average);
// outFile << conditionType << " - Average Intensity: " << average << "\n";
// } else {
// NS_LOG_INFO("🌦️ " << conditionType << " - No recorded duration.");
// outFile << conditionType << " - No recorded duration.\n";
// }
// }

// outFile.close();
// historyFile.close();

// NS_LOG_INFO("✅ Weather averages appended to " << averagesFilename);
// NS_LOG_INFO("✅ Full weather history appended to " << historyFilename << " (Chronological Order)");
// }
// void WeatherManager::ComputeAverageWeatherConditions(const std::string& averagesFilename,
//                                                      const std::string& historyFilename) const
// {
//     uint32_t runNumber = ns3::RngSeedManager::GetRun(); // Run ID for traceability
//     double currentSimTime = Simulator::Now().GetSeconds();

//     NS_LOG_INFO("\n===== 🔹 Computing Final Weather Averages 🔹 =====");

//     // Open output files
//     std::ofstream outFile(averagesFilename, std::ios::out | std::ios::app);
//     std::ofstream historyFile(historyFilename, std::ios::out | std::ios::app);

//     if (!outFile.is_open() || !historyFile.is_open()) {
//         NS_LOG_ERROR("⚠️ Failed to open output files for writing!");
//         return;
//     }

//     // Headers for this run
//     outFile << "\n===== Final Weather Averages Run " << runNumber << " =====\n";
//     outFile << "Condition,Average,Total Duration(s),Entries\n";
//     historyFile << "\n===== Full Weather History Run " << runNumber << " =====\n";

//     if (m_weatherHistory.empty()) {
//         NS_LOG_WARN("⚠️ Weather history is EMPTY! No data to save.");
//         outFile << "⚠️ No weather history recorded.\n";
//         historyFile << "⚠️ No weather history recorded.\n";
//         outFile.close();
//         historyFile.close();
//         return;
//     }

//     // === Full Chronological Log ===
//     std::vector<std::tuple<double, std::string, double>> allEntries;
//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         for (const auto& log : history) {
//             allEntries.emplace_back(log.startTime, conditionType, log.intensity);
//         }
//     }

//     std::sort(allEntries.begin(), allEntries.end());

//     for (const auto& event : allEntries) {
//         double time = std::get<0>(event);
//         const std::string& conditionType = std::get<1>(event);
//         double value = std::get<2>(event);

//         historyFile << "⏳ Time: " << time << "s, " << conditionType << " = " << value << "\n";
//         NS_LOG_INFO("⏳ Time: " << time << "s, " << conditionType << " = " << value);
//     }

//     // === Per-Condition Averages ===
//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         std::vector<WeatherLogEntry> history = entry.second;

//         if (history.empty()) {
//             outFile << conditionType << ",N/A,0,0\n";
//             continue;
//         }

//         // Sort per condition
//         std::sort(history.begin(), history.end(), [](const WeatherLogEntry& a, const WeatherLogEntry& b) {
//             return a.startTime < b.startTime;
//         });

//         if (history.size() == 1) {
//             NS_LOG_WARN("⚠️ Only one entry for " << conditionType << " — average may not be reliable.");
//         }

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         for (size_t i = 0; i < history.size(); ++i) {
//             double start = history[i].startTime;
//             double end = (i + 1 < history.size()) ? history[i + 1].startTime : currentSimTime;
//             double duration = end - start;
//             double intensity = history[i].intensity;

//             if (duration > 0.0) {
//                 weightedSum += intensity * duration;
//                 totalDuration += duration;
//             } else {
//                 NS_LOG_WARN("Zero duration for " << conditionType << " at index " << i);
//             }
//         }

//         if (totalDuration > 0.0) {
//             double avg = weightedSum / totalDuration;
//             outFile << conditionType << "," << avg << "," << totalDuration << "," << history.size() << "\n";
//             NS_LOG_INFO("🌦️ " << conditionType << " → Avg: " << avg
//                          << " over " << totalDuration << "s (" << history.size() << " entries)");
//         } else {
//             outFile << conditionType << ",0.0,0.0," << history.size() << "\n";
//             NS_LOG_WARN("No valid durations for " << conditionType);
//         }
//     }

//     outFile.close();
//     historyFile.close();

//     NS_LOG_INFO("✅ Weather averages written to: " << averagesFilename);
//     NS_LOG_INFO("✅ Weather history written to: " << historyFilename);
// }

// void WeatherManager::ComputeAverageWeatherConditions(const std::string& averagesFilename,
//                                                      const std::string& historyFilename) const
// {
//     uint32_t runNumber = ns3::RngSeedManager::GetRun(); // Run ID for traceability

//     NS_LOG_INFO("\n===== 🔹 Computing Final Weather Averages 🔹 =====");

//     // Open output files
//     std::ofstream outFile(averagesFilename, std::ios::out | std::ios::app);
//     std::ofstream historyFile(historyFilename, std::ios::out | std::ios::app);

//     if (!outFile.is_open() || !historyFile.is_open()) {
//         NS_LOG_ERROR("⚠️ Failed to open output files for writing!");
//         return;
//     }

//     // Headers for this run
//     outFile << "\n===== Final Weather Averages " << runNumber << " =====\n";
//     historyFile << "\n===== Full Weather History " << runNumber << " =====\n";

//     if (m_weatherHistory.empty()) {
//         NS_LOG_WARN("⚠️ Weather history is EMPTY! No data to save.");
//         outFile << "⚠️ No weather history recorded.\n";
//         historyFile << "⚠️ No weather history recorded.\n";
//         outFile.close();
//         historyFile.close();
//         return;
//     }

//     // Flatten and sort weather history events chronologically
//     std::vector<std::tuple<double, std::string, double>> allEntries;
//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         for (const auto& log : history) {
//             allEntries.emplace_back(log.startTime, conditionType, log.intensity);
//         }
//     }
//     std::sort(allEntries.begin(), allEntries.end());

//     // Write full chronological log
//     for (const auto& event : allEntries) {
//         double time = std::get<0>(event);
//         const std::string& conditionType = std::get<1>(event);
//         double value = std::get<2>(event);

//         historyFile << "⏳ Time: " << time << "s, " << conditionType << " = " << value << "\n";
//         NS_LOG_INFO("⏳ Time: " << time << "s, " << conditionType << " = " << value);
//     }

//     // Compute average for each condition
//     for (const auto& entry : m_weatherHistory) {
//         const std::string& conditionType = entry.first;
//         const std::vector<WeatherLogEntry>& history = entry.second;

//         double weightedSum = 0.0;
//         double totalDuration = 0.0;

//         for (size_t i = 0; i < history.size(); ++i) {
//             double startTime = history[i].startTime;
//             double endTime = (i + 1 < history.size()) ? history[i + 1].startTime : Simulator::Now().GetSeconds();
//             double duration = endTime - startTime;
//             double intensity = history[i].intensity;

//             weightedSum += intensity * duration;
//             totalDuration += duration;
//         }

//         if (totalDuration > 0.0) {
//             double average = weightedSum / totalDuration;
//             NS_LOG_INFO("🌦️ " << conditionType << " - Average Intensity: " << average);
//             outFile << conditionType << " - Average Intensity: " << average << "\n";
//         } else {
//             NS_LOG_INFO("🌦️ " << conditionType << " - No recorded duration.");
//             outFile << conditionType << " - No recorded duration.\n";
//         }
//     }

//     outFile.close();
//     historyFile.close();

//     NS_LOG_INFO("✅ Weather averages appended to " << averagesFilename);
//     NS_LOG_INFO("✅ Full weather history appended to " << historyFilename << " (Chronological Order)");
// }


// void
// WeatherManager::SetAveragesFilename(const std::string &filename)
// {
//   m_averagesFilename = filename;
// }


// void WeatherManager::SetAveragesFilename(const std::string &filename)
// {
//   m_averagesFilename = filename;
// }

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