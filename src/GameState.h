#pragma once

#include "Common.h"
#include "ActionType.h"
#include "Unit.h"
#include "AbilityAction.h"

namespace BOSS
{

class GameState 
{
    std::vector<Unit>	m_units;
    std::vector<size_t>	m_unitsBeingBuilt;  // indices of m_units which are not completed, sorted descending by finish time
    std::vector<size_t>     m_unitsSortedEndFrame;      // indices of m_units which are completed, in order
    std::vector<size_t>     m_armyUnits;                // holds indices of produced army units
    std::vector<AbilityAction> m_chronoBoosts;
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
    AbilityAction       m_lastAbility;

    int			getBuilderID(const ActionType & type) const;
    bool		haveBuilder(const ActionType & type) const;
    bool		havePrerequisites(const ActionType & type) const;

    int			whenSupplyReady(const ActionType & action)          const;
    int			whenPrerequisitesReady(const ActionType & action)   const;
    int			whenResourcesReady(const ActionType & action)       const;
    int			whenBuilderReady(const ActionType & action)         const;

    void        addUnitToSpecialVectors(size_t unitIndex);

    Unit &		getUnit(size_t id);
    void		completeUnit(Unit & Unit);

public: 

    GameState();
    GameState(const GameState & state);

    const double &	                        getMinerals() const;
    const double &	                        getGas() const;
    const int &		                        getCurrentSupply() const;
    const int &		                        getMaxSupply() const;
    const int &		                        getCurrentFrame() const;
    const Unit &	                        getUnit(size_t id) const;
    ActionType                              getUnitType(size_t id);
	bool			                        canBuildNow(const ActionType & action) const;
    int				                        whenCanBuild(const ActionType & action) const;
    int                                     whenCanCast(const ActionType & action, size_t targetID) const;
    int                                     whenEnergyReady(const ActionType & action)          const;
    int			                            getSupplyInProgress() const;
    int                                     getLastActionFinishTime() const;
    int                                     getNextFinishTime(const ActionType & type) const;

    void                                    getSpecialAbilityTargets(ActionSet & actionSet) const;
    void                                    storeChronoBoostTargets(ActionSet & actionSet) const;
    bool                                    chronoBoostableTarget(const Unit & unit) const;
    bool                                    canChronoBoostTarget(const Unit & unit) const;
    bool                                    canChronoBoost()    const;
    size_t                                  getNumberChronoBoostsCast() const;
    const std::vector<AbilityAction> &      getChronoBoostTargets() const;

    size_t			                        getNumMineralWorkers() const;
    size_t			                        getNumGasWorkers() const;
    size_t			                        getNumInProgress(const ActionType & action) const;
    size_t			                        getNumCompleted(const ActionType & action) const;
    size_t			                        getNumTotal(const ActionType & action) const;
    size_t                                  getNumTotalCompleted(const ActionType & action) const;
    void			                        getLegalActions(std::vector<ActionType> & legalActions) const;
    bool			                        isLegal(const ActionType & type) const;
    bool			                        haveType(const ActionType & action) const;
    int				                        getRace() const;
    const ActionType &                      getLastAction() const;
    const AbilityAction &                   getLastAbility() const;
    size_t                                  getNumUnits() const;

    bool                                    doAbility(const ActionType & type, size_t targetID);
    void			                        doAction(const ActionType & type);
    void			                        fastForward(int frames);
    void			                        addUnit(const ActionType & Unit, int builderID = -1);
    void			                        setMinerals(double minerals);
    void			                        setGas(double gas);
    
    const std::vector<size_t> &             getFinishedArmyUnits() const;
    const std::vector<size_t> &             getFinishedUnits() const;
    

    std::string		toString() const;


    void printunitsbeingbuilt() const;
};
}
