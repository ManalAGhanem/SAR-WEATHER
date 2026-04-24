// // /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// // /*
// //  * Copyright (c) 2009 Dan Broyles
// //  *
// //  * This program is free software; you can redistribute it and/or modify
// //  * it under the terms of the GNU General Public License version 2 as
// //  * published by the Free Software Foundation;
// //  *
// //  * This program is distributed in the hope that it will be useful,
// //  * but WITHOUT ANY WARRANTY; without even the implied warranty of
// //  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// //  * GNU General Public License for more details.
// //  *
// //  * You should have received a copy of the GNU General Public License
// //  * along with this program; if not, write to the Free Software
// //  * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
// //  *
// //  * Author: Dan Broyles <dbroyl01@ku.edu>
// //  * Thanks to Kevin Peters, faculty advisor James P.G. Sterbenz, and the ResiliNets 
// //  * initiative at The University of Kansas, https://wiki.ittc.ku.edu/resilinets
// //  *
// //  * Modifications made by: Paulo Regis <pregis@nevada.unr.edu>
// //  */
// // #ifndef OBS_GAUSS_MARKOV_MOBILITY_MODEL_H
// // #define OBS_GAUSS_MARKOV_MOBILITY_MODEL_H

// // #include "constant-velocity-helper.h"
// // #include "mobility-model.h"
// // #include "position-allocator.h"
// // #include "ns3/ptr.h"
// // #include "ns3/object.h"
// // #include "ns3/nstime.h"
// // #include "ns3/event-id.h"
// // #include "ns3/box.h"
// // #include "ns3/random-variable-stream.h"

// // namespace ns3 {

// // /**
// //  * \ingroup mobility
// //  * \brief Gauss-Markov mobility model
// //  *
// //  * This is a 3D version of the Gauss-Markov mobility model described in [1]. 
// //  * Unlike the other mobility models in ns-3, which are memoryless, the Gauss
// //  * Markov model has both memory and variability. The tunable alpha parameter
// //  * determines the how much memory and randomness you want to model.
// //  * Each object starts with a specific velocity, direction (radians), and pitch 
// //  * angle (radians) equivalent to the mean velocity, direction, and pitch. 
// //  * At each timestep, a new velocity, direction, and pitch angle are generated 
// //  * based upon the previous value, the mean value, and a gaussian random variable. 
// //  * This version is suited for simple airplane flight, where direction, velocity,
// //  * and pitch are the key variables.
// //  * The motion field is limited by a 3D bounding box (called "box") which is a 3D
// //  * version of the "rectangle" field that is used in 2-dimensional ns-3 mobility models.
// //  * 
// //  * Here is an example of how to implement the model and set the initial node positions:
// //  * \code
// //     MobilityHelper mobility;

// //     mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
// //       "Bounds", BoxValue (Box (0, 150000, 0, 150000, 0, 10000)),
// //       "TimeStep", TimeValue (Seconds (0.5)),
// //       "Alpha", DoubleValue (0.85),
// //       "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=800|Max=1200]"),
// //       "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
// //       "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
// //       "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
// //       "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
// //       "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));

// //     mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
// //       "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150000]"),
// //       "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150000]"),
// //       "Z", StringValue ("ns3::UniformRandomVariable[Min=0|Max=10000]"));
 
// //     mobility.Install (wifiStaNodes);
// //  * \endcode
// //  * [1] Tracy Camp, Jeff Boleng, Vanessa Davies, "A Survey of Mobility Models
// //  * for Ad Hoc Network Research", Wireless Communications and Mobile Computing,
// //  * Wiley, vol.2 iss.5, September 2002, pp.483-502
// //  */
// // class ObstacleGaussMarkovMobilityModel : public MobilityModel
// // {
// // public:
// //   /**
// //    * Register this type with the TypeId system.
// //    * \return the object TypeId
// //    */
// //   static TypeId GetTypeId (void);
// //   ObstacleGaussMarkovMobilityModel ();
// //   /**
// //    * \param obstacle an obstacle to be added
// //    *
// //    * The box must be inside the boundaries,
// //    * this method does not verify it.
// //    * This method assumes the box has dimensions > 0
// //    */
// //   void AddObstacle(const Box &obstacle);
// // private:
// //   /**
// //    * Initialize the model and calculate new velocity, direction, and pitch
// //    */
// //   void Start (void);
// //   /**
// //    * Perform a walk operation
// //    * \param timeLeft time until Start method is called again
// //    */
// //   void DoWalk (Time timeLeft);
// //   /**
// //    * \brief Performs the rebound of the node if it reaches a boundary
// //    * \param timeLeft The remaining time of the walk
// //    */
// //   void Rebound (Time timeLeft);
// //   virtual void DoDispose (void);
// //   virtual Vector DoGetPosition (void) const;
// //   virtual void DoSetPosition (const Vector &position);
// //   virtual Vector DoGetVelocity (void) const;
// //   virtual int64_t DoAssignStreams (int64_t);
// //   ConstantVelocityHelper m_helper; //!< constant velocity helper
// //   Time m_timeStep; //!< duraiton after which direction and speed should change
// //   double m_alpha; //!< tunable constant in the model
// //   double m_meanVelocity; //!< current mean velocity
// //   double m_meanDirection; //!< current mean direction
// //   double m_meanPitch; //!< current mean pitch
// //   double m_Velocity; //!< current velocity
// //   double m_Direction; //!< current direction
// //   double m_Pitch;  //!< current pitch
// //   Ptr<RandomVariableStream> m_rndMeanVelocity; //!< rv used to assign avg velocity
// //   Ptr<NormalRandomVariable> m_normalVelocity; //!< Gaussian rv used to for next velocity
// //   Ptr<RandomVariableStream> m_rndMeanDirection; //!< rv used to assign avg direction
// //   Ptr<NormalRandomVariable> m_normalDirection; //!< Gaussian rv for next direction value
// //   Ptr<RandomVariableStream> m_rndMeanPitch; //!< rv used to assign avg. pitch 
// //   Ptr<NormalRandomVariable> m_normalPitch; //!< Gaussian rv for next pitch
// //   EventId m_event; //!< event id of scheduled start
// //   Box m_bounds; //!< bounding box

// //   mutable std::vector<Box> m_obstacles; // list of obstacles
// //   int m_closestObstacle; // if collision is detected, this wil be set as the id of the obstacle in the array
// // };

// // } // namespace ns3

// // #endif /* GAUSS_MARKOV_MOBILITY_MODEL_H */
// /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// /*
//  * Copyright (c) 2009 Dan Broyles
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
//  * Author: Dan Broyles <dbroyl01@ku.edu>
//  * Thanks to Kevin Peters, faculty advisor James P.G. Sterbenz, and the ResiliNets 
//  * initiative at The University of Kansas, https://wiki.ittc.ku.edu/resilinets
//  *
//  * Modifications made by: Paulo Regis <pregis@nevada.unr.edu>
//  */

// //  #ifndef OBS_GAUSS_MARKOV_MOBILITY_MODEL_H
// //  #define OBS_GAUSS_MARKOV_MOBILITY_MODEL_H
 
// //  #include "constant-velocity-helper.h"
// //  #include "mobility-model.h"
// //  #include "position-allocator.h"
// //  #include "ns3/ptr.h"
// //  #include "ns3/object.h"
// //  #include "ns3/nstime.h"
// //  #include "ns3/event-id.h"
// //  #include "ns3/box.h"
// //  #include "ns3/random-variable-stream.h"
 
// //  namespace ns3 {
 
// //  /**
// //   * \ingroup mobility
// //   * \brief Obstacle Gauss-Markov mobility model
// //   *
// //   * This is an extended 3D version of the Gauss-Markov mobility model described in [1].
// //   * Unlike the standard Gauss-Markov model, this version incorporates obstacles into the simulation,
// //   * and crucially enforces a minimum altitude constraint by ensuring that the z-coordinate is never
// //   * below 2.0. This modification is useful in scenarios where a minimum operational altitude must be maintained.
// //   *
// //   * Each object starts with a specific velocity, direction (radians), and pitch angle (radians)
// //   * equivalent to the mean velocity, direction, and pitch. At each timestep, new values are generated
// //   * based on the previous value, the mean value, and a gaussian random variable. The motion is constrained
// //   * by a 3D bounding box (called "box"), which is a 3D version of the "rectangle" field used in 2D mobility models.
// //   * 
// //   * Example usage:
// //   * \code
// //      MobilityHelper mobility;
 
// //      mobility.SetMobilityModel ("ns3::GaussMarkovMobilityModel",
// //        "Bounds", BoxValue (Box (0, 150000, 0, 150000, 0, 10000)),
// //        "TimeStep", TimeValue (Seconds (0.5)),
// //        "Alpha", DoubleValue (0.85),
// //        "MeanVelocity", StringValue ("ns3::UniformRandomVariable[Min=800|Max=1200]"),
// //        "MeanDirection", StringValue ("ns3::UniformRandomVariable[Min=0|Max=6.283185307]"),
// //        "MeanPitch", StringValue ("ns3::UniformRandomVariable[Min=0.05|Max=0.05]"),
// //        "NormalVelocity", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.0|Bound=0.0]"),
// //        "NormalDirection", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.2|Bound=0.4]"),
// //        "NormalPitch", StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=0.02|Bound=0.04]"));
 
// //      mobility.SetPositionAllocator ("ns3::RandomBoxPositionAllocator",
// //        "X", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150000]"),
// //        "Y", StringValue ("ns3::UniformRandomVariable[Min=0|Max=150000]"),
// //        "Z", StringValue ("ns3::UniformRandomVariable[Min=0|Max=10000]"));
  
// //      mobility.Install (wifiStaNodes);
// //   * \endcode
// //   *
// //   * [1] Tracy Camp, Jeff Boleng, Vanessa Davies, "A Survey of Mobility Models
// //   * for Ad Hoc Network Research", Wireless Communications and Mobile Computing,
// //   * Wiley, vol.2 iss.5, September 2002, pp.483-502
// //   */
// //  class ObstacleGaussMarkovMobilityModel : public MobilityModel
// //  {
// //  public:
// //    /**
// //     * Register this type with the TypeId system.
// //     * \return the object TypeId
// //     */
// //    static TypeId GetTypeId (void);
// //    ObstacleGaussMarkovMobilityModel ();
// //    /**
// //     * \param obstacle an obstacle to be added
// //     *
// //     * The box must be inside the boundaries,
// //     * this method does not verify it.
// //     * This method assumes the box has dimensions > 0
// //     */
// //    void AddObstacle(const Box &obstacle);
// //  private:
// //    /**
// //     * Initialize the model and calculate new velocity, direction, and pitch
// //     */
// //    void Start (void);
// //    /**
// //     * Perform a walk operation
// //     * \param timeLeft time until Start method is called again
// //     */
// //    void DoWalk (Time timeLeft);
// //    /**
// //     * \brief Performs the rebound of the node if it reaches a boundary
// //     * \param timeLeft The remaining time of the walk
// //     */
// //    void Rebound (Time timeLeft);
// //    virtual void DoDispose (void);
// //    virtual Vector DoGetPosition (void) const;
// //    virtual void DoSetPosition (const Vector &position);
// //    virtual Vector DoGetVelocity (void) const;
// //    virtual int64_t DoAssignStreams (int64_t);
// //    ConstantVelocityHelper m_helper; //!< constant velocity helper
// //    Time m_timeStep; //!< duration after which direction and speed should change
// //    double m_alpha; //!< tunable constant in the model
// //    double m_meanVelocity; //!< current mean velocity
// //    double m_meanDirection; //!< current mean direction
// //    double m_meanPitch; //!< current mean pitch
// //    double m_Velocity; //!< current velocity
// //    double m_Direction; //!< current direction
// //    double m_Pitch;  //!< current pitch
// //    Ptr<RandomVariableStream> m_rndMeanVelocity; //!< random variable used to assign average velocity
// //    Ptr<NormalRandomVariable> m_normalVelocity; //!< Gaussian random variable used for next velocity
// //    Ptr<RandomVariableStream> m_rndMeanDirection; //!< random variable used to assign average direction
// //    Ptr<NormalRandomVariable> m_normalDirection; //!< Gaussian random variable for next direction value
// //    Ptr<RandomVariableStream> m_rndMeanPitch; //!< random variable used to assign average pitch 
// //    Ptr<NormalRandomVariable> m_normalPitch; //!< Gaussian random variable for next pitch
// //    EventId m_event; //!< event id of scheduled start
// //    Box m_bounds; //!< bounding box
 
// //    mutable std::vector<Box> m_obstacles; //!< list of obstacles
// //    int m_closestObstacle; //!< if collision is detected, this will be set as the id of the obstacle in the array
// //  };
 
// //  } // namespace ns3
 
// //  #endif /* OBS_GAUSS_MARKOV_MOBILITY_MODEL_H */
//  /* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
// #ifndef OBS_GAUSS_MARKOV_MOBILITY_MODEL_H
// #define OBS_GAUSS_MARKOV_MOBILITY_MODEL_H

// #include "constant-velocity-helper.h"
// #include "mobility-model.h"
// #include "position-allocator.h"
// #include "ns3/ptr.h"
// #include "ns3/object.h"
// #include "ns3/nstime.h"
// #include "ns3/event-id.h"
// #include "ns3/box.h"
// #include "ns3/random-variable-stream.h"

// namespace ns3 {

// /**
//  * \ingroup mobility
//  * \brief 3D Gauss-Markov mobility model with obstacle avoidance and Z≥2 enforcement
//  *
//  * This model extends the basic Gauss-Markov mobility model with:
//  * - Obstacle collision detection and rebound logic
//  * - Mandatory minimum altitude (Z ≥ 2.0) enforcement
//  * - Physical bounce behavior when approaching minimum Z boundary
//  * - Pitch angle correction during boundary collisions
//  */
// class ObstacleGaussMarkovMobilityModel : public MobilityModel
// {
// public:
//   static TypeId GetTypeId (void);
//   ObstacleGaussMarkovMobilityModel();

//   /**
//    * \brief Add obstacle to mobility environment
//    * \param obstacle 3D box representing obstacle bounds
//    * 
//    * Obstacles must be axis-aligned boxes. The model will
//    * automatically detect collisions and adjust trajectories.
//    */
//   void AddObstacle(const Box &obstacle);
//   /**
//  * Ensures the node position stays inside the bounding box.
//  * \param pos The position to check.
//  * \param bounds The bounding box.
//  * \return Adjusted position within the bounding box.
//  */

//  static Vector ClampToBounds(Vector pos, const Box& bounds);

// //  Vector ClampToBounds(Vector pos, const Box& bounds);

// private:
//   // Internal motion handling methods
//   void Start (void);
//   void DoWalk (Time timeLeft);
//   void Rebound (Time timeLeft);
  
//   // Z-axis constraint management
//   Vector ClampPosition(const Vector &position) const;
//   void HandleZConstraints(Vector &position, Vector &velocity);
  
//   // MobilityModel overrides
//   virtual void DoDispose (void);
//   virtual Vector DoGetPosition (void) const;
//   virtual void DoSetPosition (const Vector &position);
//   virtual Vector DoGetVelocity (void) const;
//   virtual int64_t DoAssignStreams (int64_t);

//   // Motion calculation helpers
//   Vector CalculateVelocity() const;
//   void UpdateMotionParameters();

//   // Core model parameters
//   ConstantVelocityHelper m_helper;
//   Time m_timeStep;
//   double m_alpha;
//   double m_meanVelocity;
//   double m_meanDirection;
//   double m_meanPitch;
//   double m_Velocity;
//   double m_Direction;
//   double m_Pitch;

//   // Random variable streams
//   Ptr<RandomVariableStream> m_rndMeanVelocity;
//   Ptr<NormalRandomVariable> m_normalVelocity;
//   Ptr<RandomVariableStream> m_rndMeanDirection;
//   Ptr<NormalRandomVariable> m_normalDirection;
//   Ptr<RandomVariableStream> m_rndMeanPitch;
//   Ptr<NormalRandomVariable> m_normalPitch;

//   // Spatial configuration
//   EventId m_event;
//   Box m_bounds;
//   std::vector<Box> m_obstacles;
//   int m_closestObstacle;

//   // Z-axis enforcement state
//   bool m_zConstraintActive; ///< Flag for minimum Z enforcement
// };

// } // namespace ns3

// #endif /* OBS_GAUSS_MARKOV_MOBILITY_MODEL_H */
/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 Dan Broyles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Dan Broyles <dbroyl01@ku.edu>
 * Modifications made by: Paulo Regis <pregis@nevada.unr.edu>
 * Additional Fixes for Speed, Movement, and Z-Axis by: Assistant
 */

 #ifndef OBSTACLE_GAUSS_MARKOV_MOBILITY_MODEL_H
 #define OBSTACLE_GAUSS_MARKOV_MOBILITY_MODEL_H
 
 #include "constant-velocity-helper.h"
 #include "mobility-model.h"
 #include "position-allocator.h"
 #include "ns3/ptr.h"
 #include "ns3/object.h"
 #include "ns3/nstime.h"
 #include "ns3/event-id.h"
 #include "ns3/box.h"
 #include "ns3/random-variable-stream.h"

 
 namespace ns3 {
 
 /**
  * \ingroup mobility
  * \brief Gauss-Markov Mobility Model with obstacle awareness and Z-axis constraints
  *
  * This is a 3D version of the Gauss-Markov mobility model with constraints
  * ensuring the node remains within defined bounds, avoids obstacles, and prevents
  * its Z-position from falling below a minimum threshold (e.g., `Z >= 2.0`).
  *
  * The motion field is limited by a 3D bounding box, which prevents movement
  * outside the defined simulation space.
  */
 class ObstacleGaussMarkovMobilityModel : public MobilityModel
 {
 public:
   /**
    * \brief Get the TypeId
    * \return the object TypeId
    */
   static TypeId GetTypeId (void);
   ObstacleGaussMarkovMobilityModel ();
 
   /**
    * \brief Add an obstacle box to the simulation
    * \param obstacle The bounding box of the obstacle
    */
   void AddObstacle(const Box &obstacle);
 
 private:
   /**
    * \brief Initialize the model and calculate new velocity, direction, and pitch
    */
   void Start (void);
 
   /**
    * \brief Perform a movement step for the node
    * \param delayLeft Remaining time before the next update
    */
   void DoWalk (Time delayLeft);
 
   /**
    * \brief Handle the rebound effect when a node reaches a boundary or obstacle
    * \param delayLeft The remaining time of the walk
    */
   void Rebound (Time delayLeft);
 
   /**
    * \brief Clean up memory on object destruction
    */
   virtual void DoDispose (void);
 
   /**
    * \brief Get the current position of the node
    * \return The current position vector
    */
   virtual Vector DoGetPosition (void) const;
 
   /**
    * \brief Set the position of the node
    * \param position The new position vector
    */
   virtual void DoSetPosition (const Vector &position);
 
   /**
    * \brief Get the current velocity of the node
    * \return The velocity vector
    */
   virtual Vector DoGetVelocity (void) const;
 
   /**
    * \brief Assign random stream values for reproducibility
    * \param stream The base stream index
    * \return The next stream index
    */
   virtual int64_t DoAssignStreams (int64_t stream);
 
   /**
    * \brief Ensure the node stays within simulation bounds
    * \param pos The current position of the node
    * \param bounds The bounding box of the simulation area
    * \return The adjusted position vector, ensuring it remains inside bounds
    */
   static Vector ClampToBounds(Vector pos, const Box& bounds);
 
   ConstantVelocityHelper m_helper; //!< Constant velocity helper
   Time m_timeStep; //!< Time interval after which direction and speed should change
   double m_alpha; //!< Tunable constant in the Gauss-Markov model
   double m_meanVelocity; //!< Current mean velocity
   double m_meanDirection; //!< Current mean direction
   double m_meanPitch; //!< Current mean pitch
   double m_Velocity; //!< Current velocity
   double m_Direction; //!< Current direction
   double m_Pitch;  //!< Current pitch
   Ptr<RandomVariableStream> m_rndMeanVelocity; //!< Random variable for velocity
   Ptr<NormalRandomVariable> m_normalVelocity; //!< Gaussian RV for velocity fluctuations
   Ptr<RandomVariableStream> m_rndMeanDirection; //!< Random variable for direction
   Ptr<NormalRandomVariable> m_normalDirection; //!< Gaussian RV for direction fluctuations
   Ptr<RandomVariableStream> m_rndMeanPitch; //!< Random variable for pitch
   Ptr<NormalRandomVariable> m_normalPitch; //!< Gaussian RV for pitch fluctuations
   EventId m_event; //!< Event ID for scheduling next movement
   Box m_bounds; //!< Bounding box for movement constraints
   std::vector<Box> m_obstacles; //!< List of obstacles in the simulation
   int m_closestObstacle; //!< Closest obstacle ID, used for collision detection
 };
 
 } // namespace ns3
 
 #endif /* OBSTACLE_GAUSS_MARKOV_MOBILITY_MODEL_H */
 