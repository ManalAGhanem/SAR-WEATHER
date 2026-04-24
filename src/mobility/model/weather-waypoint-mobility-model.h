#ifndef WEATHER_WAYPOINT_MOBILITY_MODEL_H
#define WEATHER_WAYPOINT_MOBILITY_MODEL_H

#include "ns3/mobility-model.h"
#include "ns3/random-variable-stream.h"
#include "ns3/position-allocator.h"
#include "ns3/event-id.h"
#include "ns3/vector.h"
#include "ns3/constant-velocity-helper.h"
#include "ns3/nstime.h"





namespace ns3 {

class WeatherWaypointMobilityModel : public MobilityModel
{
public:
  static TypeId GetTypeId();

  WeatherWaypointMobilityModel();
  ~WeatherWaypointMobilityModel() override;

  void SetSpeedScale(double scale); // Public method to control speed

protected:
  void DoInitialize() override;
  Vector DoGetPosition() const override;
  void DoSetPosition(const Vector& position) override;
  Vector DoGetVelocity() const override;
  int64_t DoAssignStreams(int64_t stream) override;

private:
  void StartWalk();
 // void ScheduleNextPause();
  void StartPause();
  double UpdateAppliedScale();


  Ptr<RandomVariableStream> m_speed;
  Ptr<RandomVariableStream> m_pause;
  Ptr<PositionAllocator> m_positionAlloc;

  EventId m_event;
  ConstantVelocityHelper m_helper;

  //double m_speedScale;


  // Speed scaling
  double m_speedScaleTarget {1.0};
  double m_speedScaleApplied {1.0};
  Time   m_lastScaleUpdate {Seconds(0)};
  double m_scaleTauSeconds {2.0}; // smoothing time constant (tune 1–3s)

  // Current leg info (critical to avoid spikes)
  Vector m_dest;
  bool   m_hasDest {false};
  double m_legBaseSpeed {0.0};    // sampled ONCE per leg from m_speed RV

  // State
  bool   m_isPaused {false};

  // Events (names may differ in your implementation)
  //EventId m_pauseEvent;           // pause end event (if you have one)

};

} // namespace ns3

#endif // WEATHER_WAYPOINT_MOBILITY_MODEL_H
