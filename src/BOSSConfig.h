/* -*- c-basic-offset: 4 -*- */

#pragma once

#include "BOSS.h"
#include "JSONTools.h"
#include "BuildOrderSearchGoal.h"
#include "BuildOrderAbilities.h"

namespace BOSS
{

    class BOSSConfig
    {
        std::string                                 m_configFile;

        std::map<std::string, GameState>            m_stateMap;
        std::map<std::string, BuildOrderAbilities>  m_buildOrderMap;
        std::map<std::string, BuildOrderSearchGoal> m_buildOrderSearchGoalMap;
        
        BOSSConfig();

    public:

        static BOSSConfig & Instance();
        void ParseConfig(const std::string & configFile);

        const GameState &               GetState(const std::string & key);
        const BuildOrderAbilities &     GetBuildOrder(const std::string & key);
        const BuildOrderSearchGoal &    GetBuildOrderSearchGoalMap(const std::string & key);
    };
}
