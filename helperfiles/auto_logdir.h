#pragma once
#include <cstdlib>
#include <string>
#include <fstream>

#if __has_include(<filesystem>)
  #include <filesystem>
  namespace fs = std::filesystem;
#else
  #error "C++17 <filesystem> is required. Build with -std=c++17."
#endif

namespace autologdir_min {

// Read env var or default
inline std::string getenv_str(const char* k, const char* defv = "") {
  if (const char* v = std::getenv(k)) return std::string(v);
  return std::string(defv);
}

// Make folder name safe
inline std::string sanitize(const std::string& s) {
  std::string out; out.reserve(s.size());
  for (unsigned char c : s) {
    if (std::isalnum(c) || c=='-' || c=='_') out.push_back(char(c));
    else if (c==' ' || c=='/') out.push_back('_');
  }
  return out;
}

// Create results/<scenarioId> and chdir into it
inline void init_log_dir() {
  const std::string base = getenv_str("NS3_LOG_BASE", "results_csv");   // parent folder
  const std::string scen = getenv_str("NS3_SCENARIO", "default");   // subfolder
  fs::path dir = fs::path(base) / sanitize(scen);
  fs::create_directories(dir);
  fs::current_path(dir);

  // Optional: write a tiny meta file
  std::ofstream meta("_run_meta.txt", std::ios::app);
  if (meta) {
    meta << "cwd=" << fs::current_path().string() << "\n";
    meta << "NS3_LOG_BASE=" << base << "\n";
    meta << "NS3_SCENARIO=" << scen << "\n";
  }
}

// Runs before main()
struct AutoChdir {
  AutoChdir() { init_log_dir(); }
};

// One global instance triggers the behavior at program start
static AutoChdir _auto_chdir_;

} // namespace autologdir_min
