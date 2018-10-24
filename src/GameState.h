#pragma once

#include "Common.h"
#include "ActionType.h"
#include "Unit.h"

namespace BOSS
{

class GameState 
{
    std::vector<Unit>	m_units;
    std::vector<size_t>	m_unitsBeingBuilt;  // indices of m_units which are not completed, sorted descending by finish time
    std::vector<size_t> m_chronoBoostTargets;
    int					m_race;
    double				m_minerals;
    double				m_gas;
    int					m_currentSupply;
    int					m_maxSupply;
    int					m_currentFrame;
    int					m_previousFrame;
    int					m_mineralWorkers;
    int					m_gasWorkers;
    int					m_buildingWorkers;
    int					m_numRefineries;
    int					m_numDepots;
    ActionType			m_lastAction;

    std::vector< std::pair<size_t, std::pair<size_t,size_t> > >   m_unitsStartAndEndFrames;           // <m_units_index, <start_frame, end_frame>>
    std::vector<size_t>     m_unitsSortedEndFrame;      // holds indices of finished units. Indices point to m_unitsStartAndEndFrames, in the order they were finished in
    std::vector<size_t>     m_armyUnits;                // holds indices of produced army units

    int			getBuilderID(const ActionType & type) const;
    bool		haveBuilder(const ActionType & type) const;
    bool		havePrerequisites(const ActionType & type) const;

    int			whenSupplyReady(const ActionType & action)          const;
    int			whenPrerequisitesReady(const ActionType & action)   const;
    int			whenResourcesReady(const ActionType & action)       const;
    int			whenBuilderReady(const ActionType & action)         const;

    void        addUnitToSpecialVectors(const size_t & unitIndex);

    Unit &		getUnit(const size_t & id);
    void		completeUnit(Unit & Unit);

public: 

    GameState();

    const double &	getMinerals() const;
    const double &	getGas() const;
    const int &		getCurrentSupply() const;
    const int &		getMaxSupply() const;
    const int &		getCurrentFrame() const;
    const Unit &	getUnit(const size_t & id) const;
	bool			canBuildNow(const ActionType & action) const;
    int				whenCanBuild(const ActionType & action) const;
    int             whenCanCast(const ActionType & action, const size_t & targetID) const;
    int             whenEnergyReady(const ActionType & action)          const;
    int			    getSupplyInProgress() const;
    int             getLastActionFinishTime() const;
    int             getNextFinishTime(const ActionType & type) const;

    
    void            getSpecialAbilityTargets(ActionSet & actionSet) const;
    void            storeChronoBoostTargets(ActionSet & actionSet) const;
    bool            chronoBoostableTarget(const Unit & unit) const;
    bool            canChronoBoostTarget(const Unit & unit) const;
    
    bool            canChronoBoost()    const;
    size_t			getNumMineralWorkers() const;
    size_t			getNumGasWorkers() const;
    size_t			getNumInProgress(const ActionType & action) const;
    size_t			getNumCompleted(const ActionType & action) const;
    size_t			getNumTotal(const ActionType & action) const;
    void			getLegalActions(std::vector<ActionType> & legalActions) const;
    bool			isLegal(const ActionType & type) const;
    bool			haveType(const ActionType & action) const;
    int				getRace() const;
    size_t          getChronoBoostsCast() const;
    const ActionType & getLastAction() const;

    bool            doAbility(const ActionType & type, const size_t & targetID);
    void			doAction(const ActionType & type);
    void			fastForward(const int & frames);
    void			addUnit(const ActionType & Unit, int builderID = -1);
    void			setMinerals(const double & minerals);
    void			setGas(const double & gas);
    
    const std::vector<size_t> &                                             getFinishedArmyUnits() const;
    const std::vector< std::pair<size_t, std::pair<size_t, size_t> > > &    getUnitTimes() const;
    const std::vector<size_t> &                                             getFinishedUnits() const;
    const std::vector<size_t> &                                             getChronoBoostTargets() const;

    std::string		toString() const;


    void printunitsbeingbuilt() const;
};
}
