#include <cmath>
#include <algorithm>
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/string.h"
#include "obstacle-gauss-markov-mobility-model.h"
#include "position-allocator.h"

namespace ns3 {

// Revised ClampToBounds: ensures position is strictly inside by adding a small epsilon.
Vector
ObstacleGaussMarkovMobilityModel::ClampToBounds (Vector pos, const Box& bounds)
{
  // First clamp to the limits.
  pos.x = std::max(bounds.xMin, std::min(pos.x, bounds.xMax));
  pos.y = std::max(bounds.yMin, std::min(pos.y, bounds.yMax));
  pos.z = std::max(bounds.zMin, std::min(pos.z, bounds.zMax));

  // Always push inside if exactly on a boundary.
  const double offset = 0.1;  // Adjust this value as needed

  if (pos.x == bounds.xMin) {
    pos.x = bounds.xMin + offset;
  } else if (pos.x == bounds.xMax) {
    pos.x = bounds.xMax - offset;
  }

  if (pos.y == bounds.yMin) {
    pos.y = bounds.yMin + offset;
  } else if (pos.y == bounds.yMax) {
    pos.y = bounds.yMax - offset;
  }

  if (pos.z == bounds.zMin) {
    pos.z = bounds.zMin + offset;
  } else if (pos.z == bounds.zMax) {
    pos.z = bounds.zMax - offset;
  }

  return pos;
}

NS_OBJECT_ENSURE_REGISTERED (ObstacleGaussMarkovMobilityModel);

TypeId
ObstacleGaussMarkovMobilityModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::ObstacleGaussMarkovMobilityModel")
    .SetParent<MobilityModel> ()
    .SetGroupName ("Mobility")
    .AddConstructor<ObstacleGaussMarkovMobilityModel> ()
    .AddAttribute ("Bounds", "Bounds of the area to cruise.",
                   BoxValue (Box (-100.0, 100.0, -100.0, 100.0, 2.0, 100.0)),
                   MakeBoxAccessor (&ObstacleGaussMarkovMobilityModel::m_bounds),
                   MakeBoxChecker ())
    .AddAttribute ("TimeStep", "Change current direction and speed after moving for this time.",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&ObstacleGaussMarkovMobilityModel::m_timeStep),
                   MakeTimeChecker ())
    .AddAttribute ("Alpha", "Tunable parameter in Gauss-Markov model.",
                   DoubleValue (0.9),
                   MakeDoubleAccessor (&ObstacleGaussMarkovMobilityModel::m_alpha),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MeanVelocity", "Average velocity random variable.",
                   StringValue ("ns3::UniformRandomVariable[Min=10.0|Max=20.0]"),
                   MakePointerAccessor (&ObstacleGaussMarkovMobilityModel::m_rndMeanVelocity),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("MeanDirection", "Average direction random variable.",
                   StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=6.283185307]"),
                   MakePointerAccessor (&ObstacleGaussMarkovMobilityModel::m_rndMeanDirection),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("MeanPitch", "Average pitch random variable (forced to 0 for constant z).",
                   StringValue ("ns3::ConstantRandomVariable[Constant=0.0]"),
                   MakePointerAccessor (&ObstacleGaussMarkovMobilityModel::m_rndMeanPitch),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("NormalVelocity", "Gaussian RV for velocity changes.",
                   StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=2.0|Bound=5.0]"),
                   MakePointerAccessor (&ObstacleGaussMarkovMobilityModel::m_normalVelocity),
                   MakePointerChecker<NormalRandomVariable> ())
    .AddAttribute ("NormalDirection", "Gaussian RV for direction changes.",
                   StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=1.0|Bound=10.0]"),
                   MakePointerAccessor (&ObstacleGaussMarkovMobilityModel::m_normalDirection),
                   MakePointerChecker<RandomVariableStream> ())
    .AddAttribute ("NormalPitch", "Gaussian RV for pitch changes (unused since z is fixed).",
                   StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=1.0|Bound=10.0]"),
                   MakePointerAccessor (&ObstacleGaussMarkovMobilityModel::m_normalPitch),
                   MakePointerChecker<RandomVariableStream> ());
  return tid;
}

ObstacleGaussMarkovMobilityModel::ObstacleGaussMarkovMobilityModel ()
{
  m_meanVelocity = 0.0;
  m_meanDirection = 0.0;
  m_meanPitch = 0.0; // Force no pitch change
  m_event = Simulator::ScheduleNow (&ObstacleGaussMarkovMobilityModel::Start, this);
  m_helper.Unpause ();
  m_closestObstacle = -1;
}

void
ObstacleGaussMarkovMobilityModel::AddObstacle (const Box &obstacle)
{
  m_obstacles.push_back(obstacle);
}

void
ObstacleGaussMarkovMobilityModel::Start (void)
{
  if (m_meanVelocity == 0.0)
    {
      // Initialize mean velocity, direction, and pitch (pitch is forced to 0)
      m_meanVelocity = m_rndMeanVelocity->GetValue ();
      m_meanDirection = m_rndMeanDirection->GetValue ();
      m_meanPitch = 0.0;
      m_Pitch = 0.0;
      m_Velocity = m_meanVelocity;
      m_Direction = m_meanDirection;
      m_Pitch = 0.0;

      double vx = m_Velocity * std::cos (m_Direction);
      double vy = m_Velocity * std::sin (m_Direction);
      double vz = 0.0;
      m_helper.SetVelocity (Vector (vx, vy, vz));
    }

  m_helper.Update ();

  double rv = m_normalVelocity->GetValue ();
  double rd = m_normalDirection->GetValue ();
  //double rp = m_normalPitch->GetValue (); // Unused as pitch remains 0

  double one_minus_alpha = 1 - m_alpha;
  double sqrt_alpha = std::sqrt (1 - m_alpha * m_alpha);
  m_Velocity  = m_alpha * m_Velocity  + one_minus_alpha * m_meanVelocity  + sqrt_alpha * rv;
  m_Direction = m_alpha * m_Direction + one_minus_alpha * m_meanDirection + sqrt_alpha * rd;
  m_Pitch     = 0.0; // Force pitch to 0

  double vx = m_Velocity * std::cos (m_Direction);
  double vy = m_Velocity * std::sin (m_Direction);
  double vz = 0.0;
  m_helper.SetVelocity (Vector (vx, vy, vz));

  // Clamp the current position to ensure it's within bounds.
  Vector position = ClampToBounds(m_helper.GetCurrentPosition(), m_bounds);
  m_helper.SetPosition(position);

  DoWalk(m_timeStep);
}

void
ObstacleGaussMarkovMobilityModel::DoWalk (Time delayLeft)
{
  m_helper.UpdateWithBounds(m_bounds);
  Vector position = m_helper.GetCurrentPosition();
  Vector velocity = m_helper.GetVelocity();

  velocity.z = 0.0; // Ensure constant z for 2D motion.
  m_helper.SetVelocity(velocity);

  double speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);

  Vector nextPosition = position;
  double dt = delayLeft.GetSeconds();
  nextPosition.x += velocity.x * dt;
  nextPosition.y += velocity.y * dt;
  nextPosition.z = m_bounds.zMin;  // We force a fixed z (will be adjusted by ClampToBounds)

  double distance = CalculateDistance(position, nextPosition);
  m_closestObstacle = -1;
  int obstacle_id = 0;
  for (auto it = m_obstacles.begin(); it != m_obstacles.end(); ++it)
    {
      Vector candidateCollision = it->CalculateIntersection(position, velocity);
      if (it->IsInside(candidateCollision))
        {
          double candDist = CalculateDistance(position, candidateCollision);
          if (candDist < distance)
            {
              Vector avoidanceDir = position - candidateCollision;
              double norm = std::sqrt(avoidanceDir.x * avoidanceDir.x +
                                      avoidanceDir.y * avoidanceDir.y +
                                      avoidanceDir.z * avoidanceDir.z);
              if (norm > 0)
                {
                  avoidanceDir.x /= norm;
                  avoidanceDir.y /= norm;
                  avoidanceDir.z = 0.0;
                }
              velocity.x += avoidanceDir.x * m_Velocity * 0.3;
              velocity.y += avoidanceDir.y * m_Velocity * 0.3;
              m_Direction = std::atan2(velocity.y, velocity.x);
              m_meanDirection = m_Direction;
              distance = candDist;
              m_closestObstacle = obstacle_id;
            }
        }
      obstacle_id++;
    }
  velocity.z = 0.0;
  m_helper.SetVelocity(velocity);

  if (m_bounds.IsInside(nextPosition))
    {
      Time delay_tmp = Seconds(distance / speed);
      m_event = Simulator::Schedule(delay_tmp, &ObstacleGaussMarkovMobilityModel::Start, this);
    }
  else
    {
      // Use the bounds' intersection function and clamp the result.
      nextPosition = m_bounds.CalculateIntersection(position, velocity);
      nextPosition = ClampToBounds(nextPosition, m_bounds);
      nextPosition.z = m_bounds.zMin;
      m_helper.SetPosition(nextPosition);
      double dist = CalculateDistance(position, nextPosition);
      Time delay_tmp = Seconds(dist / speed);
      m_event = Simulator::Schedule(delay_tmp, &ObstacleGaussMarkovMobilityModel::Rebound, this,
                                     delayLeft - delay_tmp);
    }
  NotifyCourseChange();
}

void ObstacleGaussMarkovMobilityModel::Rebound(Time delayLeft)
{
  m_helper.UpdateWithBounds(m_bounds);
  Vector position = m_helper.GetCurrentPosition();
  Vector velocity = m_helper.GetVelocity();

  if (m_closestObstacle == -1)
    {
      Box boundary = m_bounds;
      switch (boundary.GetClosestSide(position))
      {
        case Box::RIGHT:
        case Box::LEFT:
          velocity.x = -velocity.x;
          m_meanDirection = M_PI - m_meanDirection;
          break;
        case Box::TOP:
        case Box::BOTTOM:
          velocity.y = -velocity.y;
          m_meanDirection = -m_meanDirection;
          break;
        case Box::UP:
        case Box::DOWN:
          velocity.z = 0.0;
          m_meanPitch = 0.0;
          break;
      }
      Vector newPosition = boundary.CalculateIntersection(position, velocity);
      newPosition = ClampToBounds(newPosition, m_bounds);
      newPosition.z = m_bounds.zMin;
      m_helper.SetPosition(newPosition);
    }
  else
    {
      velocity.x = -velocity.x;
      velocity.y = -velocity.y;
      position.x += velocity.x * 0.1;
      position.y += velocity.y * 0.1;
      position = ClampToBounds(position, m_bounds);
      m_helper.SetPosition(position);
    }

  velocity.z = 0.0;
  m_helper.SetVelocity(velocity);
  m_helper.Unpause();
  DoWalk(delayLeft);
}

void
ObstacleGaussMarkovMobilityModel::DoDispose (void)
{
  MobilityModel::DoDispose();
}

Vector
ObstacleGaussMarkovMobilityModel::DoGetPosition (void) const
{
  m_helper.Update();
  return m_helper.GetCurrentPosition();
}

void
ObstacleGaussMarkovMobilityModel::DoSetPosition (const Vector &position)
{
  Vector safePosition = ClampToBounds(position, m_bounds);
  safePosition.z = m_bounds.zMin;
  m_helper.SetPosition(safePosition);
  Simulator::Remove(m_event);
  m_event = Simulator::ScheduleNow(&ObstacleGaussMarkovMobilityModel::Start, this);
}

Vector
ObstacleGaussMarkovMobilityModel::DoGetVelocity (void) const
{
  return m_helper.GetVelocity();
}

int64_t
ObstacleGaussMarkovMobilityModel::DoAssignStreams (int64_t stream)
{
  m_rndMeanVelocity->SetStream(stream);
  m_normalVelocity->SetStream(stream + 1);
  m_rndMeanDirection->SetStream(stream + 2);
  m_normalDirection->SetStream(stream + 3);
  m_rndMeanPitch->SetStream(stream + 4);
  m_normalPitch->SetStream(stream + 5);
  return 6;
}

} // namespace ns3
