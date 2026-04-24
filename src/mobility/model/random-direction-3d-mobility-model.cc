// // // // // /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// // // // // /*
// // // // //  * Copyright (c) 2015 University of Nevada, Reno
// // // // //  *
// // // // //  * This program is free software; you can redistribute it and/or modify
// // // // //  * it under the terms of the GNU General Public License version 2 as
// // // // //  * published by the Free Software Foundation;
// // // // //  *
// // // // //  * This program is distributed in the hope that it will be useful,
// // // // //  * but WITHOUT ANY WARRANTY; without even the implied warranty of
// // // // //  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// // // // //  * GNU General Public License for more details.
// // // // //  *
// // // // //  * You should have received a copy of the GNU General Public License
// // // // //  * along with this program; if not, write to the Free Software
// // // // //  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// // // // //  *
// // // // //  * Author: Paulo Regis <pregis@nevada.unr.edu>
// // // // //  */
// // // // //  #include "ns3/simulator.h"
// // // // //  #include <algorithm>
// // // // //  #include <cmath>
// // // // //  #include "ns3/log.h"
// // // // //  #include "ns3/string.h"
// // // // //  #include "ns3/pointer.h"
// // // // //  #include "random-direction-3d-mobility-model.h"
 
// // // // //  namespace ns3 {
 
// // // // //  NS_LOG_COMPONENT_DEFINE ("RandomDirection3dMobilityModel");
 
// // // // //  NS_OBJECT_ENSURE_REGISTERED (RandomDirection3dMobilityModel);
 
// // // // //  TypeId
// // // // //  RandomDirection3dMobilityModel::GetTypeId (void)
// // // // //  {
// // // // //    static TypeId tid = TypeId ("ns3::RandomDirection3dMobilityModel")
// // // // //        .SetParent<MobilityModel> ()
// // // // //        .SetGroupName ("Mobility")
// // // // //        .AddConstructor<RandomDirection3dMobilityModel> ()
// // // // //        .AddAttribute ("Bounds", "The 3d bounding box",
// // // // //                             BoxValue (Box (-100, 100, -100, 100, 0, 100)),
// // // // //                             MakeBoxAccessor (&RandomDirection3dMobilityModel::m_bounds),
// // // // //                             MakeBoxChecker ())
// // // // //        .AddAttribute ("Speed", "A random variable to control the speed (m/s).",
// // // // //                             StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
// // // // //                             MakePointerAccessor (&RandomDirection3dMobilityModel::m_speed),
// // // // //                             MakePointerChecker<RandomVariableStream> ())
// // // // //        .AddAttribute ("Pause", "A random variable to control the pause (s).",
// // // // //                             StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
// // // // //                             MakePointerAccessor (&RandomDirection3dMobilityModel::m_pause),
// // // // //                             MakePointerChecker<RandomVariableStream> ())
// // // // //        ;
// // // // //    return tid;
// // // // //  }
 
// // // // //  RandomDirection3dMobilityModel::RandomDirection3dMobilityModel ()
// // // // //  {
// // // // //    m_direction = CreateObject<UniformRandomVariable> ();
// // // // //    m_pitch = CreateObject<UniformRandomVariable> ();
// // // // //    m_closestObstacle = -1;
// // // // //  }
 
// // // // //  void
// // // // //  RandomDirection3dMobilityModel::AddObstacle (const Box &obstacle)
// // // // //  {
// // // // //    m_obstacles.push_back(obstacle);
// // // // //  }
 
// // // // //  void
// // // // //  RandomDirection3dMobilityModel::DoDispose (void)
// // // // //  {
// // // // //    // chain up.
// // // // //    MobilityModel::DoDispose ();
// // // // //  }
 
// // // // //  void
// // // // //  RandomDirection3dMobilityModel::DoInitialize (void)
// // // // //  {
// // // // //    DoInitializePrivate ();
// // // // //    MobilityModel::DoInitialize ();
// // // // //  }
 
// // // // // //  void
// // // // // //  RandomDirection3dMobilityModel::DoInitializePrivate (void)
// // // // // //  {
// // // // // //    double direction = m_direction->GetValue (0, 2 * M_PI);
// // // // // //    double pitch = m_direction->GetValue (0, M_PI);
// // // // // //    SetDirectionAndPitchAndSpeed (direction, pitch);
// // // // // //  }

// // // // // void
// // // // // RandomDirection3dMobilityModel::DoInitializePrivate (void)
// // // // // {
// // // // //   // Get current position
// // // // //   Vector pos = m_helper.GetCurrentPosition ();
// // // // //   // If pos is not inside m_bounds, set it to the center of the bounds.
// // // // //   if (!m_bounds.IsInside(pos))
// // // // //     {
// // // // //       pos = Vector((m_bounds.xMin + m_bounds.xMax) / 2.0,
// // // // //                    (m_bounds.yMin + m_bounds.yMax) / 2.0,
// // // // //                    (m_bounds.zMin + m_bounds.zMax) / 2.0);
// // // // //       m_helper.SetPosition(pos);
// // // // //     }
// // // // //   // Now choose a random direction and pitch and set velocity.
// // // // //   double direction = m_direction->GetValue (0, 2 * M_PI);
// // // // //   double pitch = m_direction->GetValue (0, M_PI);
// // // // //   SetDirectionAndPitchAndSpeed (direction, pitch);
// // // // // }

 
// // // // //  void
// // // // //  RandomDirection3dMobilityModel::BeginPause (void)
// // // // //  {
// // // // //    m_helper.Update ();
// // // // //    m_helper.Pause ();
// // // // //    Time pause = Seconds (m_pause->GetValue ());
// // // // //    m_event.Cancel ();
// // // // //    m_event = Simulator::Schedule (pause, &RandomDirection3dMobilityModel::ResetDirectionAndSpeed, this);
// // // // //    NotifyCourseChange ();
// // // // //  }
// // // // //  void
// // // // // RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
// // // // // {
// // // // //   NS_LOG_FUNCTION_NOARGS ();
// // // // //   // Update helper with current bounds.
// // // // //   m_helper.UpdateWithBounds (m_bounds);
  
// // // // //   // Retrieve the current position.
// // // // //   Vector position = m_helper.GetCurrentPosition ();

// // // // //   // Check if the position is inside the bounds.
// // // // //   if (!m_bounds.IsInside (position))
// // // // //     {
// // // // //       // Log detailed error messages.
// // // // //       NS_LOG_ERROR ("SetDirectionAndPitchAndSpeed(): Current position is out-of-bounds.");
// // // // //       NS_LOG_ERROR ("Position = " << position);
// // // // //       NS_LOG_ERROR ("Bounds = " << m_bounds);
// // // // //       NS_FATAL_ERROR ("Position out-of-bounds. Please check initial placement or adjust bounds.");
// // // // //     }
  
// // // // //   double speed = m_speed->GetValue ();
// // // // //   const Vector vel (std::cos (direction) * std::sin (pitch) * speed,
// // // // //                     std::sin (direction) * std::sin (pitch) * speed,
// // // // //                     std::cos (pitch) * speed);
// // // // //   m_helper.SetVelocity (vel);
// // // // //   m_helper.Unpause ();
  
// // // // //   // Now it is safe to call CalculateIntersection.
// // // // //   Vector next = m_bounds.CalculateIntersection (position, vel);
  
// // // // //   double distance = CalculateDistance (position, next);
// // // // //   m_closestObstacle = -1;
// // // // //   int obstacle_id = 0;
// // // // //   for (std::vector<Box>::iterator it = m_obstacles.begin(); it != m_obstacles.end(); ++it)
// // // // //     {
// // // // //       Vector candidateCollision = it->CalculateIntersection (position, vel);
// // // // //       if (it->IsInside (candidateCollision) && CalculateDistance (position, candidateCollision) < distance)
// // // // //         {
// // // // //           distance = CalculateDistance (position, candidateCollision);
// // // // //           m_closestObstacle = obstacle_id;
// // // // //         }
// // // // //       obstacle_id++;
// // // // //     }
  
// // // // //   Time delay = Seconds (distance / speed);
// // // // //   m_event.Cancel ();
// // // // //   m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
// // // // //   NotifyCourseChange ();
// // // // // }

// // // // // //  void
// // // // // //  RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
// // // // // //  {
// // // // // //    NS_LOG_FUNCTION_NOARGS ();
// // // // // //    m_helper.UpdateWithBounds (m_bounds);
// // // // // //    Vector position = m_helper.GetCurrentPosition ();
// // // // // //    double speed = m_speed->GetValue ();
// // // // // //    const Vector vel (std::cos (direction) * std::sin (pitch) * speed,
// // // // // //          std::sin (direction) * std::sin (pitch) * speed,
// // // // // //          std::cos (pitch) * speed);
// // // // // //    m_helper.SetVelocity (vel);
// // // // // //    m_helper.Unpause ();
// // // // // //    Vector next = m_bounds.CalculateIntersection (position, vel);
 
// // // // // //    double distance = CalculateDistance (position, next);
// // // // // //    m_closestObstacle = -1;
// // // // // //    int obstacle_id = 0;
// // // // // //    for (std::vector<Box>::iterator it = m_obstacles.begin() ; it != m_obstacles.end(); ++it)
// // // // // //      {
// // // // // //        // Compute candidate collision point using CalculateIntersection.
// // // // // //        Vector candidateCollision = it->CalculateIntersection (position, vel);
// // // // // //        if (it->IsInside (candidateCollision) && CalculateDistance (position, candidateCollision) < distance)
// // // // // //    {
// // // // // //      distance = CalculateDistance (position, candidateCollision);
// // // // // //      m_closestObstacle = obstacle_id;
// // // // // //    }
// // // // // //        obstacle_id++;
// // // // // //      }
 
// // // // // //    Time delay = Seconds (distance / speed);
// // // // // //    m_event.Cancel ();
// // // // // //    m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
// // // // // //    NotifyCourseChange ();
// // // // // //  }
 
// // // // //  void
// // // // //  RandomDirection3dMobilityModel::ResetDirectionAndSpeed (void)
// // // // //  {
// // // // //    double direction = m_direction->GetValue (0, 2*M_PI);
// // // // //    double pitch = m_pitch->GetValue (0, M_PI);
 
// // // // //    m_helper.UpdateWithBounds (m_bounds);
// // // // //    Vector position = m_helper.GetCurrentPosition ();
 
// // // // //    if (m_closestObstacle > -1) {
// // // // //      switch (m_obstacles[m_closestObstacle].GetClosestSide (position))
// // // // //        {
// // // // //        case Box::RIGHT:
// // // // //          direction += -M_PI / 2;
// // // // //          break;
// // // // //        case Box::LEFT:
// // // // //          direction += M_PI / 2;
// // // // //          break;
// // // // //        case Box::TOP:
// // // // //          direction += -M_PI;
// // // // //          break;
// // // // //        case Box::BOTTOM:
// // // // //          direction += M_PI;
// // // // //          break;
// // // // //        case Box::UP:
// // // // //          pitch += -M_PI / 2;
// // // // //          break;
// // // // //        case Box::DOWN:
// // // // //          pitch += M_PI / 2;
// // // // //          break;
// // // // //        }
// // // // //    } else {
// // // // //      switch (m_bounds.GetClosestSide (position))
// // // // //        {
// // // // //        case Box::RIGHT:
// // // // //          direction += M_PI / 2;
// // // // //          break;
// // // // //        case Box::LEFT:
// // // // //          direction += -M_PI / 2;
// // // // //          break;
// // // // //        case Box::TOP:
// // // // //          direction += M_PI;
// // // // //          break;
// // // // //        case Box::BOTTOM:
// // // // //          direction += -M_PI;
// // // // //          break;
// // // // //        case Box::UP:
// // // // //          pitch += M_PI / 2;
// // // // //          break;
// // // // //        case Box::DOWN:
// // // // //          pitch += -M_PI / 2;
// // // // //          break;
// // // // //        }
// // // // //    }
 
// // // // //    SetDirectionAndPitchAndSpeed (direction, pitch);
// // // // //  }
 
// // // // //  Vector
// // // // //  RandomDirection3dMobilityModel::DoGetPosition (void) const
// // // // //  {
// // // // //    m_helper.UpdateWithBounds (m_bounds);
// // // // //    return m_helper.GetCurrentPosition ();
// // // // //  }
 
// // // // //  void
// // // // //  RandomDirection3dMobilityModel::DoSetPosition (const Vector &position)
// // // // //  {
// // // // //    m_helper.SetPosition (position);
// // // // //    Simulator::Remove (m_event);
// // // // //    m_event.Cancel ();
// // // // //    m_event = Simulator::ScheduleNow (&RandomDirection3dMobilityModel::DoInitializePrivate, this);
// // // // //  }
 
// // // // //  Vector
// // // // //  RandomDirection3dMobilityModel::DoGetVelocity (void) const
// // // // //  {
// // // // //    return m_helper.GetVelocity ();
// // // // //  }
 
// // // // //  int64_t
// // // // //  RandomDirection3dMobilityModel::DoAssignStreams (int64_t stream)
// // // // //  {
// // // // //    m_direction->SetStream (stream);
// // // // //    m_speed->SetStream (stream + 1);
// // // // //    m_pause->SetStream (stream + 2);
// // // // //    m_pitch->SetStream (stream + 3);
// // // // //    return 3;
// // // // //  }
 
// // // // //  } // namespace ns3
// // // //  /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// // // // /*
// // // //  * Copyright (c) 2015 University of Nevada, Reno
// // // //  *
// // // //  * This program is free software; you can redistribute it and/or modify
// // // //  * it under the terms of the GNU General Public License version 2 as
// // // //  * published by the Free Software Foundation;
// // // //  *
// // // //  * This program is distributed in the hope that it will be useful,
// // // //  * but WITHOUT ANY WARRANTY; without even the implied warranty of
// // // //  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// // // //  * GNU General Public License for more details.
// // // //  *
// // // //  * You should have received a copy of the GNU General Public License
// // // //  * along with this program; if not, write to the Free Software
// // // //  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// // // //  *
// // // //  * Author: Paulo Regis <pregis@nevada.unr.edu>
// // // //  */
// // // // #include "ns3/simulator.h"
// // // // #include <algorithm>
// // // // #include <cmath>
// // // // #include "ns3/log.h"
// // // // #include "ns3/string.h"
// // // // #include "ns3/pointer.h"
// // // // #include "random-direction-3d-mobility-model.h"

// // // // namespace ns3 {

// // // // NS_LOG_COMPONENT_DEFINE ("RandomDirection3dMobilityModel");

// // // // NS_OBJECT_ENSURE_REGISTERED (RandomDirection3dMobilityModel);

// // // // TypeId
// // // // RandomDirection3dMobilityModel::GetTypeId (void)
// // // // {
// // // //   static TypeId tid = TypeId ("ns3::RandomDirection3dMobilityModel")
// // // //       .SetParent<MobilityModel> ()
// // // //       .SetGroupName ("Mobility")
// // // //       .AddConstructor<RandomDirection3dMobilityModel> ()
// // // //       .AddAttribute ("Bounds", "The 3d bounding box",
// // // //                            BoxValue (Box (-100, 100, -100, 100, 0, 100)),
// // // //                            MakeBoxAccessor (&RandomDirection3dMobilityModel::m_bounds),
// // // //                            MakeBoxChecker ())
// // // //       .AddAttribute ("Speed", "A random variable to control the speed (m/s).",
// // // //                            StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
// // // //                            MakePointerAccessor (&RandomDirection3dMobilityModel::m_speed),
// // // //                            MakePointerChecker<RandomVariableStream> ())
// // // //       .AddAttribute ("Pause", "A random variable to control the pause (s).",
// // // //                            StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
// // // //                            MakePointerAccessor (&RandomDirection3dMobilityModel::m_pause),
// // // //                            MakePointerChecker<RandomVariableStream> ())
// // // //       ;
// // // //   return tid;
// // // // }

// // // // RandomDirection3dMobilityModel::RandomDirection3dMobilityModel ()
// // // // {
// // // //   m_direction = CreateObject<UniformRandomVariable> ();
// // // //   m_pitch = CreateObject<UniformRandomVariable> ();
// // // //   m_closestObstacle = -1;
// // // // }

// // // // void
// // // // RandomDirection3dMobilityModel::AddObstacle (const Box &obstacle)
// // // // {
// // // //   m_obstacles.push_back(obstacle);
// // // // }

// // // // void
// // // // RandomDirection3dMobilityModel::DoDispose (void)
// // // // {
// // // //   MobilityModel::DoDispose ();
// // // // }

// // // // void
// // // // RandomDirection3dMobilityModel::DoInitialize (void)
// // // // {
// // // //   DoInitializePrivate ();
// // // //   MobilityModel::DoInitialize ();
// // // // }

// // // // void
// // // // RandomDirection3dMobilityModel::DoInitializePrivate (void)
// // // // {
// // // //   Vector pos = m_helper.GetCurrentPosition ();
// // // //   if (!m_bounds.IsInside(pos))
// // // //     {
// // // //       pos = Vector((m_bounds.xMin + m_bounds.xMax) / 2.0,
// // // //                    (m_bounds.yMin + m_bounds.yMax) / 2.0,
// // // //                    (m_bounds.zMin + m_bounds.zMax) / 2.0);
// // // //       m_helper.SetPosition(pos);
// // // //     }
// // // //   double direction = m_direction->GetValue (0, 2 * M_PI);
// // // //   double pitch = m_pitch->GetValue (0, M_PI);
// // // //   SetDirectionAndPitchAndSpeed (direction, pitch);
// // // // }

// // // // void
// // // // RandomDirection3dMobilityModel::BeginPause (void)
// // // // {
// // // //   m_helper.Update ();
// // // //   m_helper.Pause ();
// // // //   Time pause = Seconds (m_pause->GetValue ());
// // // //   m_event.Cancel ();
// // // //   m_event = Simulator::Schedule (pause, &RandomDirection3dMobilityModel::ResetDirectionAndSpeed, this);
// // // //   NotifyCourseChange ();
// // // // }

// // // // void
// // // // RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
// // // // {
// // // //   NS_LOG_FUNCTION_NOARGS ();
// // // //   m_helper.UpdateWithBounds (m_bounds);
// // // //   Vector position = m_helper.GetCurrentPosition ();

// // // //   if (!m_bounds.IsInside (position))
// // // //     {
// // // //       NS_LOG_ERROR ("Current position out-of-bounds: " << position << ". Adjusting to bounds center.");
// // // //       position = Vector((m_bounds.xMin + m_bounds.xMax)/2, 
// // // //                         (m_bounds.yMin + m_bounds.yMax)/2,
// // // //                         (m_bounds.zMin + m_bounds.zMax)/2);
// // // //       m_helper.SetPosition(position);
// // // //     }

// // // //   double speed = m_speed->GetValue ();
// // // //   Vector vel (std::cos(direction) * std::sin(pitch) * speed,
// // // //               std::sin(direction) * std::sin(pitch) * speed,
// // // //               std::cos(pitch) * speed);
// // // //   m_helper.SetVelocity (vel);
// // // //   m_helper.Unpause ();

// // // //   Vector next = m_bounds.CalculateIntersection (position, vel);
// // // //   double distance = CalculateDistance (position, next);
// // // //   m_closestObstacle = -1;
// // // //   int obstacle_id = 0;

// // // //   for (auto& obstacle : m_obstacles)
// // // //     {
// // // //       Vector candidate = obstacle.CalculateIntersection (position, vel);
// // // //       if (obstacle.IsInside (candidate))
// // // //         {
// // // //           double obstacleDist = CalculateDistance (position, candidate);
// // // //           if (obstacleDist < distance)
// // // //             {
// // // //               distance = obstacleDist;
// // // //               m_closestObstacle = obstacle_id;
// // // //             }
// // // //         }
// // // //       obstacle_id++;
// // // //     }

// // // //   Time delay = Seconds (distance / speed);
// // // //   m_event.Cancel ();
// // // //   m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
// // // //   NotifyCourseChange ();
// // // // }

// // // // void
// // // // RandomDirection3dMobilityModel::ResetDirectionAndSpeed (void)
// // // // {
// // // //   Vector normal;
// // // //   if (m_closestObstacle >= 0)
// // // //     {
// // // //       Box obstacle = m_obstacles[m_closestObstacle];
// // // //       Vector pos = m_helper.GetCurrentPosition ();
// // // //       Box::Side side = obstacle.GetClosestSide (pos);
// // // //       switch (side)
// // // //         {
// // // //           case Box::RIGHT:  normal = Vector(-1, 0, 0); break;
// // // //           case Box::LEFT:   normal = Vector(1, 0, 0); break;
// // // //           case Box::TOP:    normal = Vector(0, -1, 0); break;
// // // //           case Box::BOTTOM: normal = Vector(0, 1, 0); break;
// // // //           case Box::UP:     normal = Vector(0, 0, -1); break;
// // // //           case Box::DOWN:   normal = Vector(0, 0, 1); break;
// // // //         }
// // // //     }
// // // //   else
// // // //     {
// // // //       Vector pos = m_helper.GetCurrentPosition ();
// // // //       Box::Side side = m_bounds.GetClosestSide (pos);
// // // //       switch (side)
// // // //         {
// // // //           case Box::RIGHT:  normal = Vector(-1, 0, 0); break;
// // // //           case Box::LEFT:   normal = Vector(1, 0, 0); break;
// // // //           case Box::TOP:    normal = Vector(0, -1, 0); break;
// // // //           case Box::BOTTOM: normal = Vector(0, 1, 0); break;
// // // //           case Box::UP:     normal = Vector(0, 0, -1); break;
// // // //           case Box::DOWN:   normal = Vector(0, 0, 1); break;
// // // //         }
// // // //     }

// // // //   Vector vel = m_helper.GetVelocity ();
// // // //   double dot = vel.x * normal.x + vel.y * normal.y + vel.z * normal.z;
// // // //   Vector reflected = vel - 2 * dot * normal;

// // // //   if (reflected.GetLength () == 0)
// // // //     {
// // // //       // Fallback to random direction if reflection is zero
// // // //       double dir = m_direction->GetValue (0, 2*M_PI);
// // // //       double pch = m_pitch->GetValue (0, M_PI);
// // // //       SetDirectionAndPitchAndSpeed (dir, pch);
// // // //       return;
// // // //     }

// // // //   double direction = std::atan2(reflected.y, reflected.x);
// // // //   double xyLen = std::sqrt(reflected.x*reflected.x + reflected.y*reflected.y);
// // // //   double pitch = std::atan2(xyLen, reflected.z);

// // // //   if (direction < 0) direction += 2*M_PI;

// // // //   SetDirectionAndPitchAndSpeed (direction, pitch);
// // // // }

// // // // Vector
// // // // RandomDirection3dMobilityModel::DoGetPosition (void) const
// // // // {
// // // //   m_helper.UpdateWithBounds (m_bounds);
// // // //   return m_helper.GetCurrentPosition ();
// // // // }

// // // // void
// // // // RandomDirection3dMobilityModel::DoSetPosition (const Vector &position)
// // // // {
// // // //   m_helper.SetPosition (position);
// // // //   Simulator::Remove (m_event);
// // // //   m_event = Simulator::ScheduleNow (&RandomDirection3dMobilityModel::DoInitializePrivate, this);
// // // // }

// // // // Vector
// // // // RandomDirection3dMobilityModel::DoGetVelocity (void) const
// // // // {
// // // //   return m_helper.GetVelocity ();
// // // // }

// // // // int64_t
// // // // RandomDirection3dMobilityModel::DoAssignStreams (int64_t stream)
// // // // {
// // // //   m_direction->SetStream (stream);
// // // //   m_speed->SetStream (stream + 1);
// // // //   m_pause->SetStream (stream + 2);
// // // //   m_pitch->SetStream (stream + 3);
// // // //   return 4;
// // // // }

// // // // } // namespace ns3

// // // #include "ns3/simulator.h"
// // // #include <algorithm>
// // // #include <cmath>
// // // #include <limits>
// // // #include "ns3/log.h"
// // // #include "ns3/string.h"
// // // #include "ns3/pointer.h"
// // // #include "random-direction-3d-mobility-model.h"

// // // namespace ns3 {

// // // // Helper: returns true if the ray (origin + t * direction) intersects the box.
// // // // t will be set to the distance (in scalar units) along the ray.
// // // bool
// // // RayIntersectsBox (const Vector &origin, const Vector &direction, const Box &box, double &t)
// // // {
// // //   double tmin = -std::numeric_limits<double>::infinity ();
// // //   double tmax = std::numeric_limits<double>::infinity ();

// // //   // X slabs
// // //   if (std::fabs(direction.x) < 1e-6)
// // //     {
// // //       if (origin.x < box.xMin || origin.x > box.xMax)
// // //         return false;
// // //     }
// // //   else
// // //     {
// // //       double tx1 = (box.xMin - origin.x) / direction.x;
// // //       double tx2 = (box.xMax - origin.x) / direction.x;
// // //       if (tx1 > tx2) std::swap(tx1, tx2);
// // //       tmin = std::max(tmin, tx1);
// // //       tmax = std::min(tmax, tx2);
// // //       if (tmin > tmax)
// // //         return false;
// // //     }

// // //   // Y slabs
// // //   if (std::fabs(direction.y) < 1e-6)
// // //     {
// // //       if (origin.y < box.yMin || origin.y > box.yMax)
// // //         return false;
// // //     }
// // //   else
// // //     {
// // //       double ty1 = (box.yMin - origin.y) / direction.y;
// // //       double ty2 = (box.yMax - origin.y) / direction.y;
// // //       if (ty1 > ty2) std::swap(ty1, ty2);
// // //       tmin = std::max(tmin, ty1);
// // //       tmax = std::min(tmax, ty2);
// // //       if (tmin > tmax)
// // //         return false;
// // //     }

// // //   // Z slabs
// // //   if (std::fabs(direction.z) < 1e-6)
// // //     {
// // //       if (origin.z < box.zMin || origin.z > box.zMax)
// // //         return false;
// // //     }
// // //   else
// // //     {
// // //       double tz1 = (box.zMin - origin.z) / direction.z;
// // //       double tz2 = (box.zMax - origin.z) / direction.z;
// // //       if (tz1 > tz2) std::swap(tz1, tz2);
// // //       tmin = std::max(tmin, tz1);
// // //       tmax = std::min(tmax, tz2);
// // //       if (tmin > tmax)
// // //         return false;
// // //     }

// // //   if (tmax < 0)
// // //     return false;
// // //   t = (tmin >= 0) ? tmin : tmax;
// // //   return true;
// // // }

// // // NS_LOG_COMPONENT_DEFINE ("RandomDirection3dMobilityModel");

// // // NS_OBJECT_ENSURE_REGISTERED (RandomDirection3dMobilityModel);

// // // TypeId
// // // RandomDirection3dMobilityModel::GetTypeId (void)
// // // {
// // //   static TypeId tid = TypeId ("ns3::RandomDirection3dMobilityModel")
// // //       .SetParent<MobilityModel> ()
// // //       .SetGroupName ("Mobility")
// // //       .AddConstructor<RandomDirection3dMobilityModel> ()
// // //       .AddAttribute ("Bounds", "The 3d bounding box",
// // //                            BoxValue (Box (-100, 100, -100, 100, 0, 100)),
// // //                            MakeBoxAccessor (&RandomDirection3dMobilityModel::m_bounds),
// // //                            MakeBoxChecker ())
// // //       .AddAttribute ("Speed", "A random variable to control the speed (m/s).",
// // //                            StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
// // //                            MakePointerAccessor (&RandomDirection3dMobilityModel::m_speed),
// // //                            MakePointerChecker<RandomVariableStream> ())
// // //       .AddAttribute ("Pause", "A random variable to control the pause (s).",
// // //                            StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
// // //                            MakePointerAccessor (&RandomDirection3dMobilityModel::m_pause),
// // //                            MakePointerChecker<RandomVariableStream> ())
// // //       ;
// // //   return tid;
// // // }

// // // RandomDirection3dMobilityModel::RandomDirection3dMobilityModel ()
// // // {
// // //   m_direction = CreateObject<UniformRandomVariable> ();
// // //   m_pitch = CreateObject<UniformRandomVariable> ();
// // //   m_closestObstacle = -1;
// // // }

// // // void
// // // RandomDirection3dMobilityModel::AddObstacle (const Box &obstacle)
// // // {
// // //   m_obstacles.push_back(obstacle);
// // // }

// // // void
// // // RandomDirection3dMobilityModel::DoDispose (void)
// // // {
// // //   MobilityModel::DoDispose ();
// // // }

// // // void
// // // RandomDirection3dMobilityModel::DoInitialize (void)
// // // {
// // //   DoInitializePrivate ();
// // //   MobilityModel::DoInitialize ();
// // // }

// // // void
// // // RandomDirection3dMobilityModel::DoInitializePrivate (void)
// // // {
// // //   Vector pos = m_helper.GetCurrentPosition ();
// // //   if (!m_bounds.IsInside(pos))
// // //     {
// // //       pos = Vector((m_bounds.xMin + m_bounds.xMax) / 2.0,
// // //                    (m_bounds.yMin + m_bounds.yMax) / 2.0,
// // //                    (m_bounds.zMin + m_bounds.zMax) / 2.0);
// // //       m_helper.SetPosition(pos);
// // //     }
// // //   double direction = m_direction->GetValue (0, 2 * M_PI);
// // //   double pitch = m_pitch->GetValue (0, M_PI);
// // //   SetDirectionAndPitchAndSpeed (direction, pitch);
// // // }

// // // void
// // // RandomDirection3dMobilityModel::BeginPause (void)
// // // {
// // //   m_helper.Update ();
// // //   m_helper.Pause ();
// // //   Time pause = Seconds (m_pause->GetValue ());
// // //   m_event.Cancel ();
// // //   m_event = Simulator::Schedule (pause, &RandomDirection3dMobilityModel::ResetDirectionAndSpeed, this);
// // //   NotifyCourseChange ();
// // // }

// // // void
// // // RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
// // // {
// // //   NS_LOG_FUNCTION_NOARGS ();
// // //   m_helper.UpdateWithBounds (m_bounds);
// // //   Vector position = m_helper.GetCurrentPosition ();

// // //   if (!m_bounds.IsInside (position))
// // //     {
// // //       NS_LOG_ERROR ("Current position out-of-bounds: " << position << ". Adjusting to bounds center.");
// // //       position = Vector((m_bounds.xMin + m_bounds.xMax)/2, 
// // //                         (m_bounds.yMin + m_bounds.yMax)/2,
// // //                         (m_bounds.zMin + m_bounds.zMax)/2);
// // //       m_helper.SetPosition(position);
// // //     }

// // //   double speed = m_speed->GetValue ();
// // //   Vector vel (std::cos(direction) * std::sin(pitch) * speed,
// // //               std::sin(direction) * std::sin(pitch) * speed,
// // //               std::cos(pitch) * speed);
// // //   m_helper.SetVelocity (vel);
// // //   m_helper.Unpause ();

// // //   // Use CalculateIntersection on the mobility bounds.
// // //   Vector next = m_bounds.CalculateIntersection (position, vel);
// // //   double distance = CalculateDistance (position, next);
// // //   m_closestObstacle = -1;
// // //   int obstacle_id = 0;

// // //   // Replace the problematic loop with a ray-box intersection test.
// // //   for (auto& obstacle : m_obstacles)
// // //     {
// // //       double t;
// // //       if (RayIntersectsBox(position, vel, obstacle, t))
// // //         {
// // //           Vector candidate = position + t * vel;
// // //           double obstacleDist = CalculateDistance(position, candidate);
// // //           if (obstacleDist < distance)
// // //             {
// // //               distance = obstacleDist;
// // //               m_closestObstacle = obstacle_id;
// // //             }
// // //         }
// // //       obstacle_id++;
// // //     }

// // //   Time delay = Seconds (distance / speed);
// // //   m_event.Cancel ();
// // //   m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
// // //   NotifyCourseChange ();
// // // }

// // // void
// // // RandomDirection3dMobilityModel::ResetDirectionAndSpeed (void)
// // // {
// // //   Vector normal;
// // //   if (m_closestObstacle >= 0)
// // //     {
// // //       Box obstacle = m_obstacles[m_closestObstacle];
// // //       Vector pos = m_helper.GetCurrentPosition ();
// // //       Box::Side side = obstacle.GetClosestSide (pos);
// // //       switch (side)
// // //         {
// // //           case Box::RIGHT:  normal = Vector(-1, 0, 0); break;
// // //           case Box::LEFT:   normal = Vector(1, 0, 0); break;
// // //           case Box::TOP:    normal = Vector(0, -1, 0); break;
// // //           case Box::BOTTOM: normal = Vector(0, 1, 0); break;
// // //           case Box::UP:     normal = Vector(0, 0, -1); break;
// // //           case Box::DOWN:   normal = Vector(0, 0, 1); break;
// // //         }
// // //     }
// // //   else
// // //     {
// // //       Vector pos = m_helper.GetCurrentPosition ();
// // //       Box::Side side = m_bounds.GetClosestSide (pos);
// // //       switch (side)
// // //         {
// // //           case Box::RIGHT:  normal = Vector(-1, 0, 0); break;
// // //           case Box::LEFT:   normal = Vector(1, 0, 0); break;
// // //           case Box::TOP:    normal = Vector(0, -1, 0); break;
// // //           case Box::BOTTOM: normal = Vector(0, 1, 0); break;
// // //           case Box::UP:     normal = Vector(0, 0, -1); break;
// // //           case Box::DOWN:   normal = Vector(0, 0, 1); break;
// // //         }
// // //     }

// // //   Vector vel = m_helper.GetVelocity ();
// // //   double dot = vel.x * normal.x + vel.y * normal.y + vel.z * normal.z;
// // //   Vector reflected = vel - 2 * dot * normal;

// // //   if (reflected.GetLength () == 0)
// // //     {
// // //       // Fallback to random direction if reflection is zero
// // //       double dir = m_direction->GetValue (0, 2*M_PI);
// // //       double pch = m_pitch->GetValue (0, M_PI);
// // //       SetDirectionAndPitchAndSpeed (dir, pch);
// // //       return;
// // //     }

// // //   double direction = std::atan2(reflected.y, reflected.x);
// // //   double xyLen = std::sqrt(reflected.x*reflected.x + reflected.y*reflected.y);
// // //   double pitch = std::atan2(xyLen, reflected.z);

// // //   if (direction < 0) direction += 2*M_PI;

// // //   SetDirectionAndPitchAndSpeed (direction, pitch);
// // // }

// // // Vector
// // // RandomDirection3dMobilityModel::DoGetPosition (void) const
// // // {
// // //   m_helper.UpdateWithBounds (m_bounds);
// // //   return m_helper.GetCurrentPosition ();
// // // }

// // // void
// // // RandomDirection3dMobilityModel::DoSetPosition (const Vector &position)
// // // {
// // //   m_helper.SetPosition (position);
// // //   Simulator::Remove (m_event);
// // //   m_event = Simulator::ScheduleNow (&RandomDirection3dMobilityModel::DoInitializePrivate, this);
// // // }

// // // Vector
// // // RandomDirection3dMobilityModel::DoGetVelocity (void) const
// // // {
// // //   return m_helper.GetVelocity ();
// // // }

// // // int64_t
// // // RandomDirection3dMobilityModel::DoAssignStreams (int64_t stream)
// // // {
// // //   m_direction->SetStream (stream);
// // //   m_speed->SetStream (stream + 1);
// // //   m_pause->SetStream (stream + 2);
// // //   m_pitch->SetStream (stream + 3);
// // //   return 4;
// // // }

// // // } // namespace ns3
// // #include "ns3/simulator.h"
// // #include <algorithm>
// // #include <cmath>
// // #include <limits>
// // #include "ns3/log.h"
// // #include "ns3/string.h"
// // #include "ns3/pointer.h"
// // #include "random-direction-3d-mobility-model.h"

// // namespace ns3 {

// // // Helper: returns true if the ray (origin + t * direction) intersects the box.
// // // t will be set to the distance (in scalar units) along the ray.
// // bool
// // RayIntersectsBox (const Vector &origin, const Vector &direction, const Box &box, double &t)
// // {
// //   double tmin = -std::numeric_limits<double>::infinity ();
// //   double tmax = std::numeric_limits<double>::infinity ();

// //   // X slabs
// //   if (std::fabs(direction.x) < 1e-6)
// //     {
// //       if (origin.x < box.xMin || origin.x > box.xMax)
// //         return false;
// //     }
// //   else
// //     {
// //       double tx1 = (box.xMin - origin.x) / direction.x;
// //       double tx2 = (box.xMax - origin.x) / direction.x;
// //       if (tx1 > tx2) std::swap(tx1, tx2);
// //       tmin = std::max(tmin, tx1);
// //       tmax = std::min(tmax, tx2);
// //       if (tmin > tmax)
// //         return false;
// //     }

// //   // Y slabs
// //   if (std::fabs(direction.y) < 1e-6)
// //     {
// //       if (origin.y < box.yMin || origin.y > box.yMax)
// //         return false;
// //     }
// //   else
// //     {
// //       double ty1 = (box.yMin - origin.y) / direction.y;
// //       double ty2 = (box.yMax - origin.y) / direction.y;
// //       if (ty1 > ty2) std::swap(ty1, ty2);
// //       tmin = std::max(tmin, ty1);
// //       tmax = std::min(tmax, ty2);
// //       if (tmin > tmax)
// //         return false;
// //     }

// //   // Z slabs
// //   if (std::fabs(direction.z) < 1e-6)
// //     {
// //       if (origin.z < box.zMin || origin.z > box.zMax)
// //         return false;
// //     }
// //   else
// //     {
// //       double tz1 = (box.zMin - origin.z) / direction.z;
// //       double tz2 = (box.zMax - origin.z) / direction.z;
// //       if (tz1 > tz2) std::swap(tz1, tz2);
// //       tmin = std::max(tmin, tz1);
// //       tmax = std::min(tmax, tz2);
// //       if (tmin > tmax)
// //         return false;
// //     }

// //   if (tmax < 0)
// //     return false;
// //   t = (tmin >= 0) ? tmin : tmax;
// //   return true;
// // }

// // NS_LOG_COMPONENT_DEFINE ("RandomDirection3dMobilityModel");

// // NS_OBJECT_ENSURE_REGISTERED (RandomDirection3dMobilityModel);

// // TypeId
// // RandomDirection3dMobilityModel::GetTypeId (void)
// // {
// //   static TypeId tid = TypeId ("ns3::RandomDirection3dMobilityModel")
// //       .SetParent<MobilityModel> ()
// //       .SetGroupName ("Mobility")
// //       .AddConstructor<RandomDirection3dMobilityModel> ()
// //       .AddAttribute ("Bounds", "The 3d bounding box",
// //                            BoxValue (Box (-100, 100, -100, 100, 0, 100)),
// //                            MakeBoxAccessor (&RandomDirection3dMobilityModel::m_bounds),
// //                            MakeBoxChecker ())
// //       .AddAttribute ("Speed", "A random variable to control the speed (m/s).",
// //                            StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
// //                            MakePointerAccessor (&RandomDirection3dMobilityModel::m_speed),
// //                            MakePointerChecker<RandomVariableStream> ())
// //       .AddAttribute ("Pause", "A random variable to control the pause (s).",
// //                            StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
// //                            MakePointerAccessor (&RandomDirection3dMobilityModel::m_pause),
// //                            MakePointerChecker<RandomVariableStream> ())
// //       ;
// //   return tid;
// // }

// // RandomDirection3dMobilityModel::RandomDirection3dMobilityModel ()
// // {
// //   m_direction = CreateObject<UniformRandomVariable> ();
// //   m_pitch = CreateObject<UniformRandomVariable> ();
// //   m_closestObstacle = -1;
// // }

// // void
// // RandomDirection3dMobilityModel::AddObstacle (const Box &obstacle)
// // {
// //   m_obstacles.push_back(obstacle);
// // }

// // void
// // RandomDirection3dMobilityModel::DoDispose (void)
// // {
// //   MobilityModel::DoDispose ();
// // }

// // void
// // RandomDirection3dMobilityModel::DoInitialize (void)
// // {
// //   DoInitializePrivate ();
// //   MobilityModel::DoInitialize ();
// // }

// // void
// // RandomDirection3dMobilityModel::DoInitializePrivate (void)
// // {
// //   Vector pos = m_helper.GetCurrentPosition ();
// //   if (!m_bounds.IsInside(pos))
// //     {
// //       pos = Vector((m_bounds.xMin + m_bounds.xMax) / 2.0,
// //                    (m_bounds.yMin + m_bounds.yMax) / 2.0,
// //                    (m_bounds.zMin + m_bounds.zMax) / 2.0);
// //       m_helper.SetPosition(pos);
// //     }
// //   double direction = m_direction->GetValue (0, 2 * M_PI);
// //   double pitch = m_pitch->GetValue (0, M_PI);
// //   SetDirectionAndPitchAndSpeed (direction, pitch);
// // }

// // void
// // RandomDirection3dMobilityModel::BeginPause (void)
// // {
// //   m_helper.Update ();
// //   m_helper.Pause ();
// //   Time pause = Seconds (m_pause->GetValue ());
// //   m_event.Cancel ();
// //   m_event = Simulator::Schedule (pause, &RandomDirection3dMobilityModel::ResetDirectionAndSpeed, this);
// //   NotifyCourseChange ();
// // }

// // void
// // RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
// // {
// //   NS_LOG_FUNCTION_NOARGS ();
// //   m_helper.UpdateWithBounds (m_bounds);
// //   Vector position = m_helper.GetCurrentPosition ();

// //   if (!m_bounds.IsInside (position))
// //     {
// //       NS_LOG_ERROR ("Current position out-of-bounds: " << position << ". Adjusting to bounds center.");
// //       position = Vector((m_bounds.xMin + m_bounds.xMax)/2, 
// //                         (m_bounds.yMin + m_bounds.yMax)/2,
// //                         (m_bounds.zMin + m_bounds.zMax)/2);
// //       m_helper.SetPosition(position);
// //     }

// //   double speed = m_speed->GetValue ();
// //   Vector vel (std::cos(direction) * std::sin(pitch) * speed,
// //               std::sin(direction) * std::sin(pitch) * speed,
// //               std::cos(pitch) * speed);
// //   m_helper.SetVelocity (vel);
// //   m_helper.Unpause ();

// //   // Use CalculateIntersection on the mobility bounds.
// //   Vector next = m_bounds.CalculateIntersection (position, vel);
// //   double distance = CalculateDistance (position, next);
// //   m_closestObstacle = -1;
// //   int obstacle_id = 0;

// //   // Safety factor: reduce effective obstacle distance to trigger avoidance earlier.
// //   const double safetyFactor = 0.8;

// //   // Replace the problematic loop with a ray-box intersection test.
// //   for (auto& obstacle : m_obstacles)
// //     {
// //       double t;
// //       if (RayIntersectsBox(position, vel, obstacle, t))
// //         {
// //           Vector candidate = position + t * vel;
// //           double obstacleDist = CalculateDistance(position, candidate);
// //           double effectiveDist = obstacleDist * safetyFactor; // reduce distance
// //           if (effectiveDist < distance)
// //             {
// //               distance = effectiveDist;
// //               m_closestObstacle = obstacle_id;
// //             }
// //         }
// //       obstacle_id++;
// //     }

// //   Time delay = Seconds (distance / speed);
// //   m_event.Cancel ();
// //   m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
// //   NotifyCourseChange ();
// // }

// // void
// // RandomDirection3dMobilityModel::ResetDirectionAndSpeed (void)
// // {
// //   Vector normal;
// //   if (m_closestObstacle >= 0)
// //     {
// //       Box obstacle = m_obstacles[m_closestObstacle];
// //       Vector pos = m_helper.GetCurrentPosition ();
// //       Box::Side side = obstacle.GetClosestSide (pos);
// //       switch (side)
// //         {
// //           case Box::RIGHT:  normal = Vector(-1, 0, 0); break;
// //           case Box::LEFT:   normal = Vector(1, 0, 0); break;
// //           case Box::TOP:    normal = Vector(0, -1, 0); break;
// //           case Box::BOTTOM: normal = Vector(0, 1, 0); break;
// //           case Box::UP:     normal = Vector(0, 0, -1); break;
// //           case Box::DOWN:   normal = Vector(0, 0, 1); break;
// //         }
// //     }
// //   else
// //     {
// //       Vector pos = m_helper.GetCurrentPosition ();
// //       Box::Side side = m_bounds.GetClosestSide (pos);
// //       switch (side)
// //         {
// //           case Box::RIGHT:  normal = Vector(-1, 0, 0); break;
// //           case Box::LEFT:   normal = Vector(1, 0, 0); break;
// //           case Box::TOP:    normal = Vector(0, -1, 0); break;
// //           case Box::BOTTOM: normal = Vector(0, 1, 0); break;
// //           case Box::UP:     normal = Vector(0, 0, -1); break;
// //           case Box::DOWN:   normal = Vector(0, 0, 1); break;
// //         }
// //     }

// //   Vector vel = m_helper.GetVelocity ();
// //   double dot = vel.x * normal.x + vel.y * normal.y + vel.z * normal.z;
// //   Vector reflected = vel - 2 * dot * normal;

// //   if (reflected.GetLength () == 0)
// //     {
// //       // Fallback to random direction if reflection is zero
// //       double dir = m_direction->GetValue (0, 2*M_PI);
// //       double pch = m_pitch->GetValue (0, M_PI);
// //       SetDirectionAndPitchAndSpeed (dir, pch);
// //       return;
// //     }

// //   double direction = std::atan2(reflected.y, reflected.x);
// //   double xyLen = std::sqrt(reflected.x*reflected.x + reflected.y*reflected.y);
// //   double pitch = std::atan2(xyLen, reflected.z);

// //   if (direction < 0) direction += 2*M_PI;

// //   SetDirectionAndPitchAndSpeed (direction, pitch);
// // }

// // Vector
// // RandomDirection3dMobilityModel::DoGetPosition (void) const
// // {
// //   m_helper.UpdateWithBounds (m_bounds);
// //   return m_helper.GetCurrentPosition ();
// // }

// // void
// // RandomDirection3dMobilityModel::DoSetPosition (const Vector &position)
// // {
// //   m_helper.SetPosition (position);
// //   Simulator::Remove (m_event);
// //   m_event = Simulator::ScheduleNow (&RandomDirection3dMobilityModel::DoInitializePrivate, this);
// // }

// // Vector
// // RandomDirection3dMobilityModel::DoGetVelocity (void) const
// // {
// //   return m_helper.GetVelocity ();
// // }

// // int64_t
// // RandomDirection3dMobilityModel::DoAssignStreams (int64_t stream)
// // {
// //   m_direction->SetStream (stream);
// //   m_speed->SetStream (stream + 1);
// //   m_pause->SetStream (stream + 2);
// //   m_pitch->SetStream (stream + 3);
// //   return 4;
// // }

// // } // namespace ns3


// /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// /*
//  * Copyright (c) 2015 University of Nevada, Reno
//  *
//  * This program is free software; you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License version 2 as
//  * published by the Free Software Foundation;
//  *
//  * This program is distributed in the hope that it will be useful,
//  * but WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  * GNU General Public License for more details.
//  *
//  * You should have received a copy of the GNU General Public License
//  * along with this program; if not, write to the Free Software
//  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  *
//  * Author: Paulo Regis <pregis@nevada.unr.edu>
//  */
// #include "ns3/simulator.h"
// #include <algorithm>
// #include <cmath>
// #include "ns3/log.h"
// #include "ns3/string.h"
// #include "ns3/pointer.h"
// #include "random-direction-3d-mobility-model.h"

// namespace ns3 {

//   NS_LOG_COMPONENT_DEFINE ("RandomDirection3dMobilityModel");

//   NS_OBJECT_ENSURE_REGISTERED (RandomDirection3dMobilityModel);


//   TypeId
//   RandomDirection3dMobilityModel::GetTypeId (void)
//   {
//     static TypeId tid = TypeId ("ns3::RandomDirection3dMobilityModel")
// 	    .SetParent<MobilityModel> ()
// 	    .SetGroupName ("Mobility")
// 	    .AddConstructor<RandomDirection3dMobilityModel> ()
// 	    .AddAttribute ("Bounds", "The 3d bounding box",BoxValue (Box (-100, 100, -100, 100, 0, 100)),MakeBoxAccessor (&RandomDirection3dMobilityModel::m_bounds),MakeBoxChecker ())
// 	    .AddAttribute ("Speed", "A random variable to control the speed (m/s).",StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),MakePointerAccessor (&RandomDirection3dMobilityModel::m_speed),MakePointerChecker<RandomVariableStream> ())
// 	    .AddAttribute ("Pause", "A random variable to control the pause (s).",StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),MakePointerAccessor (&RandomDirection3dMobilityModel::m_pause),MakePointerChecker<RandomVariableStream> ())
// 	    ;
//     return tid;
//   }

//   RandomDirection3dMobilityModel::RandomDirection3dMobilityModel ()
//   {
//     m_direction = CreateObject <UniformRandomVariable> ();
//     m_pitch = CreateObject <UniformRandomVariable> ();
//     m_closestObstacle = -1;
//   }

//   void
//   RandomDirection3dMobilityModel::AddObstacle (const Box &obstacle)
//   {
//     m_obstacles.push_back(obstacle);
//   }

//   void
//   RandomDirection3dMobilityModel::DoDispose (void)
//   {
//     // chain up.
//     MobilityModel::DoDispose ();
//   }
//   void
//   RandomDirection3dMobilityModel::DoInitialize (void)
//   {
//     DoInitializePrivate ();
//     MobilityModel::DoInitialize ();
//   }

//   void
//   RandomDirection3dMobilityModel::DoInitializePrivate (void)
//   {
//     double direction = m_direction->GetValue (0, 2 * M_PI);
//     double pitch = m_direction->GetValue (0, M_PI);
//     SetDirectionAndPitchAndSpeed (direction, pitch);
//   }

//   void
//   RandomDirection3dMobilityModel::BeginPause (void)
//   {
//     m_helper.Update ();
//     m_helper.Pause ();
//     Time pause = Seconds (m_pause->GetValue ());
//     m_event.Cancel ();
//     m_event = Simulator::Schedule (pause, &RandomDirection3dMobilityModel::ResetDirectionAndSpeed, this);
//     NotifyCourseChange ();
//   }

//   // void
//   // RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
//   // {
//   //   NS_LOG_FUNCTION_NOARGS ();
//   //   m_helper.UpdateWithBounds (m_bounds);
//   //   Vector position = m_helper.GetCurrentPosition ();
//   //   double speed = m_speed->GetValue ();
//   //   const Vector vel (std::cos (direction) * std::sin (pitch) * speed,
// 	// 	      std::sin (direction) * std::sin (pitch) * speed,
// 	// 	      std::cos (pitch) * speed);
//   //   m_helper.SetVelocity (vel);
//   //   m_helper.Unpause ();
//   //   Vector next = m_bounds.CalculateIntersection (position, vel);

//   //   double distance = CalculateDistance (position, next);
//   //   m_closestObstacle = -1;
//   //   int obstacle_id = 0;
//   //   for (std::vector<Box>::iterator it = m_obstacles.begin() ; it != m_obstacles.end(); ++it)
//   //     {
// 	// Vector collision;
// 	// if (it->WillCollide(position,vel,collision) && CalculateDistance(position,collision) < distance)
// 	//   {
// 	//     distance = CalculateDistance(position,collision);
// 	//     m_closestObstacle = obstacle_id;
// 	//   }
// 	// obstacle_id++;
//   //     }

//   //   Time delay = Seconds (distance / speed);
//   //   m_event.Cancel ();
//   //   m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
//   //   NotifyCourseChange ();
//   // }

//   void
// RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
// {
//     NS_LOG_FUNCTION_NOARGS ();
//     m_helper.UpdateWithBounds (m_bounds);
//     Vector position = m_helper.GetCurrentPosition ();
//     double speed = m_speed->GetValue ();

//     // Compute the new velocity based on direction and pitch
//     Vector velocity (std::cos (direction) * std::sin (pitch) * speed,
//                      std::sin (direction) * std::sin (pitch) * speed,
//                      std::cos (pitch) * speed);
    
//     Vector nextPosition = position + velocity; // Predict next step

//     m_closestObstacle = -1;
//     int obstacle_id = 0;
//     Vector collisionPoint;

//     for (std::vector<Box>::iterator it = m_obstacles.begin(); it != m_obstacles.end(); ++it)
//     {
//         if (it->WillCollide(position, velocity, collisionPoint)) // Check for future collision
//         {
//             NS_LOG_WARN ("Collision predicted! Changing direction...");
//             m_closestObstacle = obstacle_id;
//             // Adjust direction to avoid collision
//             direction += M_PI / 2;  // Example: Turn 90 degrees
//             velocity = Vector(std::cos(direction) * std::sin(pitch) * speed,
//                               std::sin(direction) * std::sin(pitch) * speed,
//                               std::cos(pitch) * speed);
//             break;  // Only adjust for the first detected collision
//         }
//         obstacle_id++;
//     }

//     m_helper.SetVelocity(velocity);
//     m_helper.Unpause();
//     NotifyCourseChange();

//     // Compute time to next event (movement duration before pausing)
//     double distance = CalculateDistance(position, position + velocity);
//     Time delay = Seconds(distance / speed);

//     m_event.Cancel();
//     m_event = Simulator::Schedule(delay, &RandomDirection3dMobilityModel::BeginPause, this);
// }

//   // void
//   // RandomDirection3dMobilityModel::ResetDirectionAndSpeed (void)
//   // {
//   //   double direction = m_direction->GetValue (0, 2*M_PI);
//   //   double pitch = m_pitch->GetValue (0, M_PI);

//   //   m_helper.UpdateWithBounds (m_bounds);
//   //   Vector position = m_helper.GetCurrentPosition ();

//   //   if (m_closestObstacle > -1) {
// 	// switch (m_obstacles[m_closestObstacle].GetClosestSide(position))
// 	// {
// 	//   case Box::RIGHT:
// 	//     direction += -M_PI / 2;
// 	//     break;
// 	//   case Box::LEFT:
// 	//     direction += M_PI / 2;
// 	//     break;
// 	//   case Box::TOP:
// 	//     direction += -M_PI;
// 	//     break;
// 	//   case Box::BOTTOM:
// 	//     direction += M_PI;
// 	//     break;
// 	//   case Box::UP:
// 	//     pitch += -M_PI / 2;
// 	//     break;
// 	//   case Box::DOWN:
// 	//     pitch += M_PI / 2;
// 	//     break;
// 	// }
//   //   }else{
// 	// switch (m_bounds.GetClosestSide (position))
// 	// {
// 	//   case Box::RIGHT:
// 	//     direction += M_PI / 2;
// 	//     break;
// 	//   case Box::LEFT:
// 	//     direction += -M_PI / 2;
// 	//     break;
// 	//   case Box::TOP:
// 	//     direction += M_PI;
// 	//     break;
// 	//   case Box::BOTTOM:
// 	//     direction += -M_PI;
// 	//     break;
// 	//   case Box::UP:
// 	//     pitch += M_PI / 2;
// 	//     break;
// 	//   case Box::DOWN:
// 	//     pitch += -M_PI / 2;
// 	//     break;
// 	// }
//   //   }

//   //   SetDirectionAndPitchAndSpeed (direction, pitch);
//   // }

//   void
// RandomDirection3dMobilityModel::ResetDirectionAndSpeed (void)
// {
//     double direction = m_direction->GetValue (0, 2*M_PI);
//     double pitch = m_pitch->GetValue (0, M_PI);

//     m_helper.UpdateWithBounds (m_bounds);
//     Vector position = m_helper.GetCurrentPosition ();
//     Vector newVelocity;

//     if (m_closestObstacle > -1) { // If there was an obstacle in the last move
//         Vector collisionPoint;
//         Box obstacle = m_obstacles[m_closestObstacle];

//         switch (obstacle.GetClosestSide(position))
//         {
//             case Box::RIGHT:
//                 direction += M_PI / 2;
//                 break;
//             case Box::LEFT:
//                 direction -= M_PI / 2;
//                 break;
//             case Box::TOP:
//                 direction += M_PI;
//                 break;
//             case Box::BOTTOM:
//                 direction -= M_PI;
//                 break;
//             case Box::UP:
//                 pitch += M_PI / 2;
//                 break;
//             case Box::DOWN:
//                 pitch -= M_PI / 2;
//                 break;
//         }
//     }

//     // Compute new velocity
//     newVelocity = Vector(std::cos(direction) * std::sin(pitch) * m_speed->GetValue(),
//                          std::sin(direction) * std::sin(pitch) * m_speed->GetValue(),
//                          std::cos(pitch) * m_speed->GetValue());

//     m_helper.SetVelocity(newVelocity);
//     m_helper.Unpause();
//     NotifyCourseChange();
// }

//   Vector
//   RandomDirection3dMobilityModel::DoGetPosition (void) const
//   {
//     m_helper.UpdateWithBounds (m_bounds);
//     return m_helper.GetCurrentPosition ();
//   }
//   void
//   RandomDirection3dMobilityModel::DoSetPosition (const Vector &position)
//   {
//     m_helper.SetPosition (position);
//     Simulator::Remove (m_event);
//     m_event.Cancel ();
//     m_event = Simulator::ScheduleNow (&RandomDirection3dMobilityModel::DoInitializePrivate, this);
//   }
//   Vector
//   RandomDirection3dMobilityModel::DoGetVelocity (void) const
//   {
//     return m_helper.GetVelocity ();
//   }
//   int64_t
//   RandomDirection3dMobilityModel::DoAssignStreams (int64_t stream)
//   {
//     m_direction->SetStream (stream);
//     m_speed->SetStream (stream + 1);
//     m_pause->SetStream (stream + 2);
//     m_pitch->SetStream (stream + 3);
//     return 3;
//   }

// } // namespace ns3
#include "ns3/simulator.h"
#include <algorithm>
#include <cmath>
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "random-direction-3d-mobility-model.h"
#include "ns3/log.h"        // For NS_LOG
#include "ns3/core-module.h" // For Simulator::Now() and logging functions
#include <fstream>           // For file logging (ofstream)
#include "ns3/node.h"



namespace ns3 {

  NS_LOG_COMPONENT_DEFINE ("RandomDirection3dMobilityModel");
  NS_OBJECT_ENSURE_REGISTERED (RandomDirection3dMobilityModel);

  TypeId
  RandomDirection3dMobilityModel::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::RandomDirection3dMobilityModel")
        .SetParent<MobilityModel> ()
        .SetGroupName ("Mobility")
        .AddConstructor<RandomDirection3dMobilityModel> ()
        .AddAttribute ("Bounds", "The 3d bounding box", BoxValue (Box (-100, 100, -100, 100, 0, 100)),
                        MakeBoxAccessor (&RandomDirection3dMobilityModel::m_bounds), MakeBoxChecker ())
        .AddAttribute ("Speed", "A random variable to control the speed (m/s).",
                        StringValue ("ns3::UniformRandomVariable[Min=1.0|Max=2.0]"),
                        MakePointerAccessor (&RandomDirection3dMobilityModel::m_speed), 
                        MakePointerChecker<RandomVariableStream> ())
        .AddAttribute ("Pause", "A random variable to control the pause (s).",
                        StringValue ("ns3::ConstantRandomVariable[Constant=2.0]"),
                        MakePointerAccessor (&RandomDirection3dMobilityModel::m_pause), 
                        MakePointerChecker<RandomVariableStream> ());
    return tid;
  }

  RandomDirection3dMobilityModel::RandomDirection3dMobilityModel ()
  {
    m_direction = CreateObject <UniformRandomVariable> ();
    m_pitch = CreateObject <UniformRandomVariable> ();
    m_closestObstacle = -1;
  }

  void
  RandomDirection3dMobilityModel::AddObstacle (const Box &obstacle)
  {
    m_obstacles.push_back(obstacle);
  }

  void
  RandomDirection3dMobilityModel::DoDispose (void)
  {
    MobilityModel::DoDispose ();
  }

  void
  RandomDirection3dMobilityModel::DoInitialize (void)
  {
    DoInitializePrivate ();
    MobilityModel::DoInitialize ();
  }

  void
  RandomDirection3dMobilityModel::DoInitializePrivate (void)
  {
    double direction = m_direction->GetValue (0, 2 * M_PI);
    double pitch = m_pitch->GetValue (0, M_PI);
    SetDirectionAndPitchAndSpeed (direction, pitch);
  }

  void
  RandomDirection3dMobilityModel::BeginPause (void)
  {
    m_helper.Update ();
    m_helper.Pause ();
    Time pause = Seconds (m_pause->GetValue ());
    m_event.Cancel ();
    m_event = Simulator::Schedule (pause, &RandomDirection3dMobilityModel::ResetDirectionAndSpeed, this);
    NotifyCourseChange ();
  }
  Vector NormalizeVector(const Vector &v)
{
    double magnitude = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (magnitude == 0.0) {
        return Vector(0, 0, 0);  // Prevent division by zero
    }
    return Vector(v.x / magnitude, v.y / magnitude, v.z / magnitude);
}
void RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed(double direction, double pitch)
{std::ofstream debugLog("collision_debug.txt", std::ios::app);  // Append to log file

  NS_LOG_FUNCTION_NOARGS();
  m_helper.UpdateWithBounds(m_bounds);
  Vector position = m_helper.GetCurrentPosition();
  double speed = m_speed->GetValue();

  Vector velocity(
      std::cos(direction) * std::sin(pitch) * speed,
      std::sin(direction) * std::sin(pitch) * speed,
      std::cos(pitch) * speed
  );

  Vector nextPosition = position + velocity;
  double safetyMargin = 150.0;  // Increase safety buffer to prevent missed obstacles

  // Additional checks at multiple distances to avoid missing small obstacles
  Vector checkPosition1 = nextPosition + velocity * 0.25;  
  Vector checkPosition2 = nextPosition + velocity * 0.5;   
  Vector checkPosition3 = nextPosition + velocity * 0.75;  
  Vector checkPosition4 = nextPosition + velocity * safetyMargin;  

  m_closestObstacle = -1;
  int obstacle_id = 0;
  Vector collisionPoint;

  debugLog << "🔍 Checking " << m_obstacles.size() << " obstacles for collisions...\n";
  std::ofstream buildingdebugLog("buildingasobstaclesdebug.txt", std::ios::app);

  for (auto &obstacle : m_obstacles)
  {

    buildingdebugLog << "🏢 Registered Obstacle: x[" << obstacle.xMin << ", " << obstacle.xMax 
    << "] y[" << obstacle.yMin << ", " << obstacle.yMax 
    << "] z[" << obstacle.zMin << ", " << obstacle.zMax << "]\n";


      if (obstacle.IsInside(nextPosition) || 
          obstacle.IsInside(checkPosition1) ||
          obstacle.IsInside(checkPosition2) ||
          obstacle.IsInside(checkPosition3) ||
          obstacle.IsInside(checkPosition4))  
      {
          debugLog << "🚨 COLLISION DETECTED at " << nextPosition << "\n";
          m_closestObstacle = obstacle_id;

          // Determine escape direction (push away from obstacle)
          Vector boxCenter(
              (obstacle.xMin + obstacle.xMax) / 2,
              (obstacle.yMin + obstacle.yMax) / 2,
              (obstacle.zMin + obstacle.zMax) / 2
          );

          Vector escapeDirection = NormalizeVector(position - boxCenter);

          // ✅ Introduce a small random variation to avoid repetitive escape patterns
          Ptr<UniformRandomVariable> randomFactor = CreateObject<UniformRandomVariable>();
          double randomAngle = randomFactor->GetValue(-M_PI / 6, M_PI / 6);  // ±30 degree randomness

          // Apply a slight rotation to the escape vector
          escapeDirection.x = escapeDirection.x * std::cos(randomAngle) - escapeDirection.y * std::sin(randomAngle);
          escapeDirection.y = escapeDirection.x * std::sin(randomAngle) + escapeDirection.y * std::cos(randomAngle);

          // ✅ Scale escape velocity to prevent excessive jumps
          velocity = escapeDirection * std::min(speed, 5.0);  // Limit speed to 5.0

          // ✅ Ensure the Z value stays within a reasonable range
          velocity.z = std::max(-2.0, std::min(2.0, velocity.z));

          debugLog << "✅ Adjusting velocity to avoid obstacle: " << velocity << "\n";
          break;
      }
      obstacle_id++;
  }

  debugLog << "🔄 Old velocity: " << m_helper.GetVelocity() << "\n";

  // ✅ Prevent nodes from stopping completely
  if (velocity.x == 0 && velocity.y == 0 && velocity.z == 0) {
      NS_LOG_WARN("⚠️ Avoidance failed, setting default velocity to prevent getting stuck.");
      
      // Instead of a fixed (2.0, 2.0, 0), add slight variation
      Ptr<UniformRandomVariable> randomFactor = CreateObject<UniformRandomVariable>();
      velocity = Vector(
          randomFactor->GetValue(1.5, 2.5),  // Random x
          randomFactor->GetValue(1.5, 2.5),  // Random y
          0  // Keep z stable
      );
  }

  m_helper.SetVelocity(velocity);
  debugLog << "✅ New velocity after avoiding obstacle: " << velocity << "\n";

  m_helper.Unpause();
  NotifyCourseChange();

  double distance = CalculateDistance(position, position + velocity);
  Time delay = Seconds(distance / speed);

  m_event.Cancel();
  m_event = Simulator::Schedule(delay, &RandomDirection3dMobilityModel::BeginPause, this);

  debugLog.close();
    
    }

    

// void
// RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
// {
//     NS_LOG_FUNCTION_NOARGS ();
//     m_helper.UpdateWithBounds (m_bounds);
//     Vector position = m_helper.GetCurrentPosition ();
//     double speed = m_speed->GetValue ();

    

//     Vector velocity (std::cos (direction) * std::sin (pitch) * speed,
//                      std::sin (direction) * std::sin (pitch) * speed,
//                      std::cos (pitch) * speed);
    
//     Vector nextPosition = position + velocity;
//     double safetyMargin = 10.0;

//     m_closestObstacle = -1;
//     int obstacle_id = 0;
//     Vector collisionPoint;

//     for (auto &obstacle : m_obstacles)
//     {
//         if (obstacle.IsInside(nextPosition) || obstacle.IsInside(nextPosition + velocity * safetyMargin)) 
//         {
//             NS_LOG_WARN ("🚨 Collision predicted at " << nextPosition << "! Adjusting direction...");
//             m_closestObstacle = obstacle_id;

//             // Identify which side of the building the node is hitting
//             Box bounds = obstacle; // Get building boundary
//             Vector escapeDirection;

//             if (nextPosition.x < bounds.xMin) { // Hitting LEFT side
//                 escapeDirection = Vector(-1, 0, 0); // Move LEFT
//             } else if (nextPosition.x > bounds.xMax) { // Hitting RIGHT side
//                 escapeDirection = Vector(1, 0, 0); // Move RIGHT
//             } else if (nextPosition.y < bounds.yMin) { // Hitting BOTTOM
//                 escapeDirection = Vector(0, -1, 0); // Move DOWN
//             } else if (nextPosition.y > bounds.yMax) { // Hitting TOP
//                 escapeDirection = Vector(0, 1, 0); // Move UP
//             } else if (nextPosition.z < bounds.zMin) { // Hitting FLOOR
//                 escapeDirection = Vector(0, 0, -1); // Move DOWN in Z
//             } else if (nextPosition.z > bounds.zMax) { // Hitting CEILING
//                 escapeDirection = Vector(0, 0, 1); // Move UP in Z
//             } else {
//                 // Default: Move away from center (failsafe)
//                 Vector boxCenter ((bounds.xMin + bounds.xMax) / 2,
//                                   (bounds.yMin + bounds.yMax) / 2,
//                                   (bounds.zMin + bounds.zMax) / 2);
//                 escapeDirection = NormalizeVector(position - boxCenter);
//             }

//             velocity = escapeDirection * speed; // Set new velocity to avoid building
//             NS_LOG_WARN ("✅ Adjusting velocity to avoid obstacle: " << velocity);
//             break;
//         }
//         obstacle_id++;
//     }

//     m_helper.SetVelocity (velocity);
//     m_helper.Unpause ();
//     NotifyCourseChange ();

//     double distance = CalculateDistance (position, position + velocity);
//     Time delay = Seconds (distance / speed);
    
//     m_event.Cancel ();
//     m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
// }

// void
// RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
// {
//     NS_LOG_FUNCTION_NOARGS ();
//     m_helper.UpdateWithBounds (m_bounds);
//     Vector position = m_helper.GetCurrentPosition ();
//     double speed = m_speed->GetValue ();

//     Vector velocity (std::cos (direction) * std::sin (pitch) * speed,
//                      std::sin (direction) * std::sin (pitch) * speed,
//                      std::cos (pitch) * speed);
    
//     Vector nextPosition = position + velocity;
//     double safetyMargin = 2.0;

//     m_closestObstacle = -1;
//     int obstacle_id = 0;
//     Vector collisionPoint;

//     for (auto &obstacle : m_obstacles)
//     {
//         if (obstacle.IsInside(nextPosition) || obstacle.IsInside(nextPosition + velocity * safetyMargin)) 
//         {
//             NS_LOG_WARN ("🚨 Collision predicted at " << nextPosition << "! Adjusting direction...");
//             m_closestObstacle = obstacle_id;

//             Vector boxCenter ((obstacle.xMin + obstacle.xMax) / 2,
//                               (obstacle.yMin + obstacle.yMax) / 2,
//                               (obstacle.zMin + obstacle.zMax) / 2);

//             Vector escapeDirection = NormalizeVector(position - boxCenter);
//             velocity = escapeDirection * speed;

//             NS_LOG_WARN ("✅ New velocity adjusted to avoid obstacle: " << velocity);
//             break;
//         }
//         obstacle_id++;
//     }

//     m_helper.SetVelocity (velocity);
//     m_helper.Unpause ();
//     NotifyCourseChange ();

//     double distance = CalculateDistance (position, position + velocity);
//     Time delay = Seconds (distance / speed);
    
//     m_event.Cancel ();
//     m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
// }

  // void
  // RandomDirection3dMobilityModel::SetDirectionAndPitchAndSpeed (double direction, double pitch)
  // {
  //   NS_LOG_FUNCTION_NOARGS ();
  //   m_helper.UpdateWithBounds (m_bounds);
  //   Vector position = m_helper.GetCurrentPosition ();
  //   double speed = m_speed->GetValue ();
    
  //   Vector velocity (std::cos (direction) * std::sin (pitch) * speed,
  //                    std::sin (direction) * std::sin (pitch) * speed,
  //                    std::cos (pitch) * speed);
    
  //   Vector nextPosition = position + velocity;
  //   double safetyMargin = 2.0; 

  //   m_closestObstacle = -1;
  //   int obstacle_id = 0;
  //   Vector collisionPoint;

  //   for (auto &obstacle : m_obstacles)
  //   {
  //       if (obstacle.IsInside(nextPosition) || obstacle.IsInside(nextPosition + velocity * safetyMargin)) 
  //       {
  //           NS_LOG_WARN ("Collision predicted! Adjusting direction...");
  //           m_closestObstacle = obstacle_id;
  //           Vector boxCenter ((obstacle.xMin + obstacle.xMax) / 2,
  //                 (obstacle.yMin + obstacle.yMax) / 2,
  //                 (obstacle.zMin + obstacle.zMax) / 2);

  //                 Vector escapeDirection = NormalizeVector(position - boxCenter);

  //          // Vector escapeDirection = (position - obstacle.GetCenter()).Normalize();
  //           velocity = escapeDirection * speed; 
  //           break;
  //       }
  //       obstacle_id++;
  //   }

  //   m_helper.SetVelocity (velocity);
  //   m_helper.Unpause ();
  //   NotifyCourseChange ();

  //   double distance = CalculateDistance (position, position + velocity);
  //   Time delay = Seconds (distance / speed);
    
  //   m_event.Cancel ();
  //   m_event = Simulator::Schedule (delay, &RandomDirection3dMobilityModel::BeginPause, this);
  // }

  // void
  // RandomDirection3dMobilityModel::ResetDirectionAndSpeed (void)
  // {
  //   double direction = m_direction->GetValue (0, 2*M_PI);
  //   double pitch = m_pitch->GetValue (0, M_PI);

  //   m_helper.UpdateWithBounds (m_bounds);
  //   Vector position = m_helper.GetCurrentPosition ();

  //   if (m_closestObstacle > -1) { 
  //       Box obstacle = m_obstacles[m_closestObstacle];

  //       switch (obstacle.GetClosestSide(position))
  //       {
  //           case Box::RIGHT:
  //               direction += M_PI / 2;
  //               break;
  //           case Box::LEFT:
  //               direction -= M_PI / 2;
  //               break;
  //           case Box::TOP:
  //               direction += M_PI;
  //               break;
  //           case Box::BOTTOM:
  //               direction -= M_PI;
  //               break;
  //           case Box::UP:
  //               pitch += M_PI / 2;
  //               break;
  //           case Box::DOWN:
  //               pitch -= M_PI / 2;
  //               break;
  //       }
  //   }

  //   Vector newVelocity(std::cos(direction) * std::sin(pitch) * m_speed->GetValue(),
  //                      std::sin(direction) * std::sin(pitch) * m_speed->GetValue(),
  //                      std::cos(pitch) * m_speed->GetValue());

  //   Vector newNextPosition = position + newVelocity;
  //   for (auto &obstacle : m_obstacles) {
  //       if (obstacle.IsInside(newNextPosition)) { 
  //           direction += M_PI / 2; 
  //           newVelocity = Vector(std::cos(direction) * std::sin(pitch) * m_speed->GetValue(),
  //                                std::sin(direction) * std::sin(pitch) * m_speed->GetValue(),
  //                                std::cos(pitch) * m_speed->GetValue());
  //           break;
  //       }
  //   }

  //   m_helper.SetVelocity(newVelocity);
  //   m_helper.Unpause();
  //   NotifyCourseChange();
  // }
  void RandomDirection3dMobilityModel::ResetDirectionAndSpeed(void)
  {
      std::ofstream debugLog("collision_debug.txt", std::ios::app);  // Append to log file
  
      double direction = m_direction->GetValue(0, 2 * M_PI);
      double pitch = m_pitch->GetValue(0, M_PI);
  
      m_helper.UpdateWithBounds(m_bounds);
      Vector position = m_helper.GetCurrentPosition();
  
      if (m_closestObstacle > -1)
      {
          Box obstacle = m_obstacles[m_closestObstacle];
  
          debugLog << "🚨 Node is near obstacle at " << position << ". Adjusting direction.\n";
  
          // Introduce randomness to avoid predictable bouncing
          Ptr<UniformRandomVariable> randomFactor = CreateObject<UniformRandomVariable>();
          double randomAdjustment = randomFactor->GetValue(-M_PI / 6, M_PI / 6);  // ±30 degrees
  
          // switch (obstacle.GetClosestSide(position))
          // {
          // case Box::RIGHT:
          //     direction += (M_PI / 1.2) + randomAdjustment;  // Larger turn (150° + randomness)
          //     debugLog << "🔄 Moving left sharply\n";
          //     break;
          // case Box::LEFT:
          //     direction -= (M_PI / 1.2) + randomAdjustment;
          //     debugLog << "🔄 Moving right sharply\n";
          //     break;
          // case Box::TOP:
          //     direction += M_PI + randomAdjustment;  // Full reversal
          //     debugLog << "🔄 Moving downward\n";
          //     break;
          // case Box::BOTTOM:
          //     direction -= M_PI + randomAdjustment;
          //     debugLog << "🔄 Moving upward\n";
          //     break;
          // case Box::UP:
          //     pitch += (M_PI / 3) + randomAdjustment;  // Smaller Z movement
          //     debugLog << "🔄 Adjusting upward movement\n";
          //     break;
          // case Box::DOWN:
          //     pitch -= (M_PI / 3) + randomAdjustment;
          //     debugLog << "🔄 Adjusting downward movement\n";
          //     break;
          // default:
          //     debugLog << "⚠️ Unknown obstacle side detected.\n";
          // }

          switch (obstacle.GetClosestSide(position))
{
    case Box::RIGHT:
        direction += (M_PI / 1.1) + randomAdjustment;  // 165° Turn
        debugLog << "🔄 Moving left strongly\n";
        break;
    case Box::LEFT:
        direction -= (M_PI / 1.1) + randomAdjustment;
        debugLog << "🔄 Moving right strongly\n";
        break;
    case Box::TOP:
        direction += M_PI + randomAdjustment;  // Full 180° Reversal
        debugLog << "🔄 Moving downward sharply\n";
        break;
    case Box::BOTTOM:
        direction -= M_PI + randomAdjustment;
        debugLog << "🔄 Moving upward sharply\n";
        break;
    case Box::UP:
        pitch += (M_PI / 2) + randomAdjustment;
        debugLog << "🔄 Adjusting upward movement strongly\n";
        break;
    case Box::DOWN:
        pitch -= (M_PI / 2) + randomAdjustment;
        debugLog << "🔄 Adjusting downward movement strongly\n";
        break;
}

      }
  
      Vector newVelocity(
          std::max(-5.0, std::min(5.0, std::cos(direction) * std::sin(pitch) * m_speed->GetValue())),
          std::max(-5.0, std::min(5.0, std::sin(direction) * std::sin(pitch) * m_speed->GetValue())),
          std::max(-2.0, std::min(2.0, std::cos(pitch) * m_speed->GetValue()))  // Limit extreme Z changes
      );
  
      debugLog << "🔄 Adjusted velocity after collision: " << newVelocity << "\n";
  
      m_helper.SetVelocity(newVelocity);
      m_helper.Unpause();
      NotifyCourseChange();
  
      debugLog.close();
  }
  
//   void RandomDirection3dMobilityModel::ResetDirectionAndSpeed(void)
// {
//     std::ofstream debugLog("collision_debug.txt", std::ios::app);  // Open file in append mode

//     double direction = m_direction->GetValue(0, 2 * M_PI);
//     double pitch = m_pitch->GetValue(0, M_PI);

//     m_helper.UpdateWithBounds(m_bounds);
//     Vector position = m_helper.GetCurrentPosition();

//     if (m_closestObstacle > -1)
//     {
//         Box obstacle = m_obstacles[m_closestObstacle];

//         debugLog << "🚨 Node is near obstacle at " << position << ". Adjusting direction.\n";
// //         switch (obstacle.GetClosestSide(position))
// // {
// //     case Box::RIGHT:
// //         direction += M_PI / 1.5;  // Larger turn (120 degrees)
// //         break;
// //     case Box::LEFT:
// //         direction -= M_PI / 1.5;
// //         break;
// //     case Box::TOP:
// //         direction += M_PI;
// //         break;
// //     case Box::BOTTOM:
// //         direction -= M_PI;
// //         break;
// //     case Box::UP:
// //         pitch += M_PI / 3;  // Less aggressive turn to prevent flying
// //         break;
// //     case Box::DOWN:
// //         pitch -= M_PI / 3;
// //         break;
// // }
// switch (obstacle.GetClosestSide(position))
// {
//     case Box::RIGHT:
//         direction += M_PI / 1.2;  // Larger turn (150 degrees)
//         break;
//     case Box::LEFT:
//         direction -= M_PI / 1.2;
//         break;
//     case Box::TOP:
//         direction += M_PI;
//         break;
//     case Box::BOTTOM:
//         direction -= M_PI;
//         break;
//     case Box::UP:
//         pitch += M_PI / 2;  // Less aggressive turn to prevent flying
//         break;
//     case Box::DOWN:
//         pitch -= M_PI / 2;
//         break;
// }


//         // switch (obstacle.GetClosestSide(position))
//         // {
//         // case Box::RIGHT:
//         //     direction += M_PI / 2;  // Move left
//         //     debugLog << "🔄 Moving left\n";
//         //     break;
//         // case Box::LEFT:
//         //     direction -= M_PI / 2;  // Move right
//         //     debugLog << "🔄 Moving right\n";
//         //     break;
//         // case Box::TOP:
//         //     direction += M_PI;  // Move downward
//         //     debugLog << "🔄 Moving downward\n";
//         //     break;
//         // case Box::BOTTOM:
//         //     direction -= M_PI;  // Move upward
//         //     debugLog << "🔄 Moving upward\n";
//         //     break;
//         // case Box::UP:
//         //     pitch += M_PI / 2;  // Move downward in Z
//         //     debugLog << "🔄 Moving downward in Z\n";
//         //     break;
//         // case Box::DOWN:
//         //     pitch -= M_PI / 2;  // Move upward in Z
//         //     debugLog << "🔄 Moving upward in Z\n";
//         //     break;
//         // default:
//         //     debugLog << "⚠️ Unknown obstacle side detected.\n";
//         // }
//     }

//     Vector newVelocity(
//         std::cos(direction) * std::sin(pitch) * m_speed->GetValue(),
//         std::sin(direction) * std::sin(pitch) * m_speed->GetValue(),
//         std::cos(pitch) * m_speed->GetValue());

//     debugLog << "🔄 Adjusted velocity after collision: " << newVelocity << "\n";

//     m_helper.SetVelocity(newVelocity);
//     m_helper.Unpause();
//     NotifyCourseChange();

//     debugLog.close();
// }

      Vector
      RandomDirection3dMobilityModel::DoGetPosition (void) const
      {
          m_helper.UpdateWithBounds (m_bounds);
          Vector pos = m_helper.GetCurrentPosition ();
          // Get the Node ID
          uint32_t nodeId = GetObject<Node>()->GetId();
          std::ofstream posFile("positions.csv", std::ios::app);
          posFile << "Node-" << nodeId << "," << Simulator::Now().GetSeconds() 
                  << "," << pos.x << "," << pos.y << "," << pos.z << "\n";
          posFile.close();
      
          return pos;
      }
      
  // Vector
  // RandomDirection3dMobilityModel::DoGetPosition (void) const
  // {
  //   m_helper.UpdateWithBounds (m_bounds);
  //   return m_helper.GetCurrentPosition ();
  // }

  void
  RandomDirection3dMobilityModel::DoSetPosition (const Vector &position)
  {
    m_helper.SetPosition (position);
    Simulator::Remove (m_event);
    m_event.Cancel ();
    m_event = Simulator::ScheduleNow (&RandomDirection3dMobilityModel::DoInitializePrivate, this);
  }

  Vector
  RandomDirection3dMobilityModel::DoGetVelocity (void) const
  {
    return m_helper.GetVelocity ();
  }

  int64_t
  RandomDirection3dMobilityModel::DoAssignStreams (int64_t stream)
  {
    m_direction->SetStream (stream);
    m_speed->SetStream (stream + 1);
    m_pause->SetStream (stream + 2);
    m_pitch->SetStream (stream + 3);
    return 3;
  }

} // namespace ns3
