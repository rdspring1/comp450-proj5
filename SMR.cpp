/*********************************************************************
* Software License Agreement (BSD License)
*
*  Copyright (c) 2008, Willow Garage, Inc.
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*   * Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   * Redistributions in binary form must reproduce the above
*     copyright notice, this list of conditions and the following
*     disclaimer in the documentation and/or other materials provided
*     with the distribution.
*   * Neither the name of the Willow Garage nor the names of its
*     contributors may be used to endorse or promote products derived
*     from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
*  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
*  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
*  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*********************************************************************/

/* Author: Ioan Sucan */

#include "SMR.h"
#include "ompl/base/goals/GoalSampleableRegion.h"
#include "ompl/base/ValidStateSampler.h"
#include "ompl/tools/config/SelfConfig.h"
#include "ompl/control/spaces/DiscreteControlSpace.h"
#include <limits>

ompl::control::SMR::SMR(const SpaceInformationPtr &si) : base::Planner(si, "SMR")
{
    siC_ = si.get();
}

ompl::control::SMR::~SMR(void)
{
    freeMemory();
}

void ompl::control::SMR::setup(void)
{
    base::Planner::setup();
    if (!nn_)
        nn_.reset(tools::SelfConfig::getDefaultNearestNeighbors<Motion*>(si_->getStateSpace()));
    nn_->setDistanceFunction(boost::bind(&SMR::distanceFunction, this, _1, _2));

    //Create SMR
    ompl::base::ValidStateSamplerPtr vsp = si_->allocValidStateSampler(); 
    obstacle = new Motion(siC_, 0);
    nn_->add(obstacle);
    for(int i = 0; i < nodes_; ++i)
    {
        Motion* m = new Motion(siC_, (i+1));
        while(!vsp->sample(m->state))
        {
            si_->freeState(m->state);
        }
        nn_->add(m); 
    }

    std::vector<Motion*> motions;
    nn_->list(motions);
    for(int i = 0; i < nodes_; ++i)
    {
        setupTransitions(motions[i]);
    }
}

void ompl::control::SMR::setupTransitions(Motion* m)
{
    for(int i = 0; i < actions; ++i)
    {
        m->t[i];
        Control* control = siC_->allocControl();
        control->as<DiscreteControlSpace::ControlType>()->value = i;

        for(int j = 0; j < trans_; ++j)
        {
            Motion* newstate = new Motion();
            int steps = siC_->propagateWhileValid(m->state, control, siC_->getMinControlDuration(), newstate->state);
            Motion* nearest = obstacle; 
            if(steps == siC_->getMinControlDuration())
            {
                nearest = nn_->nearest(newstate);
            }
            m->t[i][j] += (1/trans_); 
        }
    }
}

void ompl::control::SMR::clear(void)
{
    Planner::clear();
    sampler_.reset();
    controlSampler_.reset();
    freeMemory();
    if (nn_)
        nn_->clear();
}

void ompl::control::SMR::freeMemory(void)
{
    if (nn_)
    {
        std::vector<Motion*> motions;
        nn_->list(motions);
        for (unsigned int i = 0 ; i < motions.size() ; ++i)
        {
            if (motions[i]->state)
                si_->freeState(motions[i]->state);
            delete motions[i];
        }
    }
}

ompl::base::PlannerStatus ompl::control::SMR::solve(const base::PlannerTerminationCondition &ptc)
{
    //TODO Generate Policy using SMR if new start/goal
    //TODO Use Policy to create a path between start/goal; Return automatically if an obstacle is encountered
}

void ompl::control::SMR::getPlannerData(base::PlannerData &data) const
{
    Planner::getPlannerData(data);
    //TODO Return policy for SMR and start/goal objectives
}
