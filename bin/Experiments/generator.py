import json

total_runs = 100
cValues = [0.01, 0.05, 0.10, 0.15, 0.20, 0.25, 0.30, 0.35, 0.40, 0.45, 0.50, 1.0, 1.5, 2.0
, 2.5, 3.0, 3.5, 4.0, 4.5, 5.0, 10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0,
200.0, 300.0, 400.0, 500.0, 600.0, 700.0, 800.0, 900.0, 1000.0]

MCTS_experiment = dict()
for exp_num,cValue in enumerate(cValues):
    
    exp_name = "MaximizeValue" + str(exp_num)
    MCTS_experiment[exp_name] = dict()

    run_list = []
    run_list.append(True)
    run_list.append(total_runs)
    MCTS_experiment[exp_name]["Run"] = run_list

    MCTS_experiment[exp_name]["MaximizeValue"] = True

    searchType_list = []
    searchType_list.append("IntegralMCTS")
    MCTS_experiment[exp_name]["SearchType"] = searchType_list

    SearchParameters_list = dict()
    SearchParameters_list["ExplorationConstant"] = cValue
    SearchParameters_list["Simulations"] = 2000000
    SearchParameters_list["UseMax"] = False
    MCTS_experiment[exp_name]["SearchParameters"] = SearchParameters_list

    SimulationsPerStep_list = []
    SimulationsPerStep_list.append(True)
    SimulationsPerStep_list.append(2000)
    MCTS_experiment[exp_name]["SimulationsPerStep"] = SimulationsPerStep_list

    MCTS_experiment[exp_name]["Threads"] = 2

    MCTS_experiment[exp_name]["Race"] = "Protoss"

    MCTS_experiment[exp_name]["StartingState"] = "Protoss 0"

    MCTS_experiment[exp_name]["PrintNewBest"] = False

    MCTS_experiment[exp_name]["OutputDir"] = "../bin/ValueParameterSweep/" + str(cValue)

    MCTS_experiment[exp_name]["SearchTimeLimitMS"] = 1000000000

    MCTS_experiment[exp_name]["FrameTimeLimit"] = 6720

    MCTS_experiment[exp_name]["RelevantActions"] = ["ChronoBoost", "Probe", "Pylon", "Nexus", "Assimilator", "Gateway", "CyberneticsCore", "Stalker",
                                    "Zealot", "Colossus", "Forge", "FleetBeacon", "TwilightCouncil", "Stargate", "TemplarArchive",
                                    "DarkShrine", "RoboticsBay", "RoboticsFacility", "ZealotWarped", "Zealot", "StalkerWarped", "DarkTemplarWarped", "DarkTemplar", "Carrier", "VoidRay", "Immortal", "AdeptWarped",
                                    "Adept", "Tempest"]

    MCTS_experiment[exp_name]["MaxActions"] = [ ["CyberneticsCore", 1], ["FleetBeacon", 1], ["TwilightCouncil", 1], ["TemplarArchive", 1], ["DarkShrine", 1], ["RoboticsBay", 1] ]

    MCTS_experiment[exp_name]["AlwaysMakeWorkers"] = True

    MCTS_experiment[exp_name]["OpeningBuildOrder"] = "Empty"

    MCTS_experiment[exp_name]["SortActions"] = False

    MCTS_experiment[exp_name]["SaveStates"] = False 

    MCTS_experiment[exp_name]["UseNetwork"] = False

with open("Experiments.txt", 'w') as outputFile:
    json.dump(MCTS_experiment, outputFile)