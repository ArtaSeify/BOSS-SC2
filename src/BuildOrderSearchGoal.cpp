/* -*- c-basic-offset: 4 -*- */

#include "BuildOrderSearchGoal.h"

using namespace BOSS;

BuildOrderSearchGoal::BuildOrderSearchGoal()
  : m_goalUnits(ActionTypes::GetAllActionTypes().size()),
    m_goalUnitsMax(ActionTypes::GetAllActionTypes().size()),
    m_supplyRequiredVal(0)
{
}

void BuildOrderSearchGoal::calculateSupplyRequired()
{
    m_supplyRequiredVal = 0;
    for (ActionID a(0); a<m_goalUnits.size(); ++a)
    {
        m_supplyRequiredVal += m_goalUnits[a] * ActionType(a).supplyCost();
    }
}

bool BuildOrderSearchGoal::operator == (const BuildOrderSearchGoal & g)
{
    for (ActionID a(0); a<m_goalUnits.size(); ++a)
    {
        if ((m_goalUnits[a] != g.m_goalUnits[a]) || (m_goalUnitsMax[a] != g.m_goalUnitsMax[a]))
        {
            return false;
        }
    }

    return true;
}

void BuildOrderSearchGoal::setGoal(ActionType a, int num)
{
    BOSS_ASSERT(a.getID() >= 0 && a.getID() < m_goalUnits.size(), "Action type not valid");

    m_goalUnits[a.getID()] = num;

    calculateSupplyRequired();
}

bool BuildOrderSearchGoal::hasGoal() const
{
    for (ActionID a(0); a<m_goalUnits.size(); ++a)
    {
        if (m_goalUnits[a] > 0)
        {
            return true;
        }
    }

    return false;
}

void BuildOrderSearchGoal::setGoalMax(ActionType a, int num)
{
    BOSS_ASSERT(a.getID() >= 0 && a.getID() < m_goalUnitsMax.size(), "Action type not valid");

    m_goalUnitsMax[a.getID()] = num;
}

int BuildOrderSearchGoal::getGoal(ActionType a) const
{
    BOSS_ASSERT(a.getID() >= 0 && a.getID() < m_goalUnits.size(), "Action type not valid");

    return m_goalUnits[a.getID()];
}

int BuildOrderSearchGoal::getGoalMax(ActionType a) const
{
    BOSS_ASSERT(a.getID() >= 0 && a.getID() < m_goalUnitsMax.size(), "Action type not valid");

    return m_goalUnitsMax[a.getID()];
}

int BuildOrderSearchGoal::supplyRequired() const
{
    return m_supplyRequiredVal;
}

std::string BuildOrderSearchGoal::toString() const
{
    std::stringstream ss;
    ss << "\nSearch Goal Information\n\n";

    for (ActionID a(0); a<m_goalUnits.size(); ++a)
    {
        if (m_goalUnits[a] > 0)
        {
            ss << "        REQ " << m_goalUnits[a] << " " <<  ActionType(a).getName() << "\n";
        }
    }

    for (ActionID a(0); a<m_goalUnitsMax.size(); ++a)
    {
        if (m_goalUnitsMax[a] > 0)
        {
            ss << "        MAX " << m_goalUnitsMax[a]  << " " << ActionType(a).getName() << "\n";
        }
    }

    return ss.str();
}

bool BuildOrderSearchGoal::isAchievedBy(const GameState & state)
{
  static ActionType Hatchery      = ActionTypes::GetActionType("Zerg_Hatchery");
  static ActionType Lair          = ActionTypes::GetActionType("Zerg_Lair");
  static ActionType Hive          = ActionTypes::GetActionType("Zerg_Hive");
  static ActionType Spire         = ActionTypes::GetActionType("Zerg_Spire");
  static ActionType GreaterSpire  = ActionTypes::GetActionType("Zerg_Greater_Spire");

  for (auto & actionType : ActionTypes::GetAllActionTypes())
  {
    int have = state.getNumTotal(actionType);

    if (state.getRace() == Races::Zerg)
    {
      if (actionType == Hatchery)
      {
        have += state.getNumTotal(Lair);
        have += state.getNumTotal(Hive);
      }
      else if (actionType == Lair)
      {
        have += state.getNumTotal(Hive);
      }
      else if (actionType == Spire)
      {
        have += state.getNumTotal(GreaterSpire);
      }
    }

    if (have < getGoal(actionType))
    {
      return false;
    }
  }

  return true;
}
