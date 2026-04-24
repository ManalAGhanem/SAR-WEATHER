#include "weather-waypoint-mobility-model.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/string.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("WeatherWaypointMobilityModel");
NS_OBJECT_ENSURE_REGISTERED(WeatherWaypointMobilityModel);

TypeId WeatherWaypointMobilityModel::GetTypeId()
{
  static TypeId tid = TypeId("ns3::WeatherWaypointMobilityModel")
    .SetParent<MobilityModel>()
    .SetGroupName("Mobility")
    .AddConstructor<WeatherWaypointMobilityModel>()
    .AddAttribute("Speed",
                  "Speed random variable",
                  StringValue("ns3::UniformRandomVariable[Min=1.0|Max=5.0]"),
                  MakePointerAccessor(&WeatherWaypointMobilityModel::m_speed),
                  MakePointerChecker<RandomVariableStream>())
    .AddAttribute("Pause",
                  "Pause random variable",
                  StringValue("ns3::ConstantRandomVariable[Constant=2.0]"),
                  MakePointerAccessor(&WeatherWaypointMobilityModel::m_pause),
                  MakePointerChecker<RandomVariableStream>())
    .AddAttribute("PositionAllocator",
                  "Waypoint generator",
                  PointerValue(),
                  MakePointerAccessor(&WeatherWaypointMobilityModel::m_positionAlloc),
                  MakePointerChecker<PositionAllocator>());
  return tid;
}

// WeatherWaypointMobilityModel::WeatherWaypointMobilityModel()
//   : m_speedScale(1.0)
// {}

WeatherWaypointMobilityModel::WeatherWaypointMobilityModel()
{
  m_speedScaleTarget  = 1.0;
  m_speedScaleApplied = 1.0;
  m_lastScaleUpdate   = Seconds(0);
  m_scaleTauSeconds   = 2.0;
}


WeatherWaypointMobilityModel::~WeatherWaypointMobilityModel()
{
  m_event.Cancel();
}
  double
  WeatherWaypointMobilityModel::UpdateAppliedScale ()
  {
    Time now = Simulator::Now ();
    if (m_lastScaleUpdate == Seconds (0))
      {
        m_lastScaleUpdate = now;
        m_speedScaleApplied = m_speedScaleTarget;
        return m_speedScaleApplied;
      }

    double dt = (now - m_lastScaleUpdate).GetSeconds ();
    m_lastScaleUpdate = now;

    // Exponential smoothing
    double tau = std::max (0.001, m_scaleTauSeconds);
    double alpha = 1.0 - std::exp (-dt / tau);
    m_speedScaleApplied = m_speedScaleApplied + alpha * (m_speedScaleTarget - m_speedScaleApplied);

    // Clamp to sane bounds
    if (m_speedScaleApplied < 0.05) m_speedScaleApplied = 0.05;
    if (m_speedScaleApplied > 1.50) m_speedScaleApplied = 1.50;

    return m_speedScaleApplied;
  }

// void WeatherWaypointMobilityModel::SetSpeedScale(double scale)
// {
//   NS_LOG_INFO("Adjusting speed scale to " << scale);
//   //m_speedScale = scale;
//    if (m_speedScale == scale) return;
//   m_speedScale = scale;
//   m_event.Cancel();
//   Simulator::Schedule(MilliSeconds(2), &WeatherWaypointMobilityModel::StartWalk, this);
// }
void WeatherWaypointMobilityModel::StartPause ()
{
  m_helper.Update ();
  m_helper.SetVelocity (Vector (0.0, 0.0, 0.0));
  m_helper.Pause ();

  m_isPaused = true;
  m_hasDest = false;

  Time pause = Seconds (m_pause->GetValue ());
  if (m_event.IsPending ()) m_event.Cancel ();
  m_event = Simulator::Schedule (pause, &WeatherWaypointMobilityModel::StartWalk, this);

  NotifyCourseChange ();
}



void
WeatherWaypointMobilityModel::SetSpeedScale (double scale)
{
  // Store target (do NOT resample base speed here)
  m_speedScaleTarget = scale;

  // If paused or no active destination yet, just store it (applies when movement resumes)
  if (m_isPaused || !m_hasDest)
    {
      // Still update applied scale for continuity
      UpdateAppliedScale ();
      return;
    }

  // Update position to 'now' before changing velocity
  m_helper.Update ();

  Vector pos = m_helper.GetCurrentPosition ();
  Vector toDest = m_dest - pos;
  double dist = std::sqrt (toDest.x * toDest.x + toDest.y * toDest.y + toDest.z * toDest.z);

  if (dist < 1e-6)
    {
      return; // already essentially at destination
    }

  // Smoothly update applied scale
  double applied = UpdateAppliedScale ();

  // Effective speed is base speed for this leg * applied scale
  double v = std::max (0.01, m_legBaseSpeed * applied);

  // Unit direction toward same destination
  Vector unit (toDest.x / dist, toDest.y / dist, toDest.z / dist);
  Vector newVel (unit.x * v, unit.y * v, unit.z * v);

  // Apply new velocity without changing destination
  m_helper.SetVelocity (newVel);

  // Reschedule arrival based on remaining distance and new speed
  if (m_event.IsPending ())
    {
      m_event.Cancel ();
    }
  Time tArrive = Seconds (dist / v);
  m_event = Simulator::Schedule (tArrive, &WeatherWaypointMobilityModel::StartPause, this);

  NotifyCourseChange ();
}


void WeatherWaypointMobilityModel::DoInitialize()
{
  //ScheduleNextPause();
    StartPause();

  MobilityModel::DoInitialize();
}

// void WeatherWaypointMobilityModel::ScheduleNextPause()
// {
//   Time pauseTime = Seconds(m_pause->GetValue());
//   m_event = Simulator::Schedule(pauseTime, &WeatherWaypointMobilityModel::StartWalk, this);
//   m_helper.Pause();
//   NotifyCourseChange();
// }

void WeatherWaypointMobilityModel::StartWalk ()
{
  m_isPaused = false;
  m_helper.Update ();

  Vector pos = m_helper.GetCurrentPosition ();

  m_dest = m_positionAlloc->GetNext ();
  m_hasDest = true;

  Vector toDest = m_dest - pos;
  double dist = toDest.GetLength ();

  if (dist < 1e-6)
    {
      StartPause ();
      return;
    }

  m_legBaseSpeed = m_speed->GetValue ();

  double applied = UpdateAppliedScale ();
  double v = std::max (0.01, m_legBaseSpeed * applied);

  Vector vel = toDest * (v / dist);
  m_helper.SetVelocity (vel);
  m_helper.Unpause ();

  if (m_event.IsPending ()) m_event.Cancel ();
  m_event = Simulator::Schedule (Seconds (dist / v),
                                 &WeatherWaypointMobilityModel::StartPause,
                                 this);

  NotifyCourseChange ();
}


// void
// WeatherWaypointMobilityModel::StartWalk ()
// {
//   m_isPaused = false;
//   m_helper.Update ();

//   // choose destination for this leg
//   m_dest = m_position->GetNext ();   // or your allocator/get-next logic
//   m_hasDest = true;

//   Vector pos = m_helper.GetCurrentPosition ();
//   Vector toDest = m_dest - pos;
//   double dist = std::sqrt (toDest.x*toDest.x + toDest.y*toDest.y + toDest.z*toDest.z);

//   if (dist < 1e-6)
//     {
//       StartPause ();
//       return;
//     }

//   // base speed sampled ONCE per leg (your Speed RV: Uniform 2..5)
//   m_legBaseSpeed = m_speed->GetValue ();

//   // applied scale (smoothed)
//   double v = std::max (0.01, m_legBaseSpeed * m_speedScaleApplied);

//   Vector unit (toDest.x / dist, toDest.y / dist, toDest.z / dist);
//   m_helper.SetVelocity (Vector (unit.x*v, unit.y*v, unit.z*v));

//   if (m_moveEvent.IsPending ())
//     {
//       m_moveEvent.Cancel ();
//     }
//   m_moveEvent = Simulator::Schedule (Seconds (dist / v),
//                                      &WeatherWaypointMobilityModel::StartPause,
//                                      this);

//   NotifyCourseChange ();
// }


Vector WeatherWaypointMobilityModel::DoGetPosition() const
{
  m_helper.Update();
  return m_helper.GetCurrentPosition();
}

void WeatherWaypointMobilityModel::DoSetPosition(const Vector& position)
{
  // m_helper.SetPosition(position);
  // m_event.Cancel();
  // ScheduleNextPause();
  m_helper.SetPosition(position);
  m_event.Cancel();
  StartPause();   // restart state machine safely

}

Vector WeatherWaypointMobilityModel::DoGetVelocity() const
{
  return m_helper.GetVelocity();
}

int64_t WeatherWaypointMobilityModel::DoAssignStreams(int64_t stream)
{
  m_speed->SetStream(stream);
  m_pause->SetStream(stream + 1);
  return 2;
}

} // namespace ns3
