import json

total_runs = 20
cValues = [0.50, 0.75, 1.0, 1.25, 1.50]
startingStates = [ "Start State" ]

MCTS_experiment = dict()
MCTS_experiment["Experiments"] = dict()

# SRMV experiments
for cValue in cValues:
      for startingState in startingStates:
            exp_name = "{}_{}_{}_{}".format("Integral", "SRMV", cValue, startingState)
            MCTS_experiment["Experiments"][exp_name] = dict()

            run_list = []
            run_list.append(True)
            run_list.append(total_runs)
            MCTS_experiment["Experiments"][exp_name]["Run"] = run_list

            MCTS_experiment["Experiments"][exp_name]["MaximizeValue"] = False

            MCTS_experiment["Experiments"][exp_name]["UseTotalTimeLimit"] = True

            searchType_list = []
            searchType_list.append("IntegralMCTS")
            MCTS_experiment["Experiments"][exp_name]["SearchType"] = searchType_list

            SearchParameters_list = dict()
            SearchParameters_list["ExplorationConstant"] = cValue
            SearchParameters_list["UseMax"] = True
            SearchParameters_list["ValueNormalization"] = 1
            SearchParameters_list["VisitsBeforeExpand"] = 2
            MCTS_experiment["Experiments"][exp_name]["SearchParameters"] = SearchParameters_list

            ChangingRoot = dict()
            ChangingRoot["Active"] = False
            ChangingRoot["Simulations"] = 0
            MCTS_experiment["Experiments"][exp_name]["ChangingRoot"] = ChangingRoot

            MCTS_experiment["Experiments"][exp_name]["Threads"] = 2

            MCTS_experiment["Experiments"][exp_name]["Race"] = "Protoss"

            MCTS_experiment["Experiments"][exp_name]["StartingState"] = "Protoss " + str(startingState)

            MCTS_experiment["Experiments"][exp_name]["PrintNewBest"] = False

            MCTS_experiment["Experiments"][exp_name]["OutputDir"] = "./HyperParameterTuning/{}/{}/{}/{}".format(startingState, "Integral", "SRMV", cValue)

            MCTS_experiment["Experiments"][exp_name]["SearchTimeLimitMS"] = 75000

            MCTS_experiment["Experiments"][exp_name]["FrameTimeLimit"] = 6720

            MCTS_experiment["Experiments"][exp_name]["RelevantActions"] = ["ChronoBoost", "Probe", "Pylon", "Nexus", "Assimilator", "Gateway", "CyberneticsCore", "Stalker", "Zealot", "Colossus", "FleetBeacon", "TwilightCouncil", "Stargate", "TemplarArchive", "DarkShrine", "RoboticsBay", "RoboticsFacility", "Zealot", "DarkTemplar", "Carrier", "VoidRay", "Immortal", "Adept", "Tempest", "Mothership"]

            MCTS_experiment["Experiments"][exp_name]["MaxActions"] = [ ["CyberneticsCore", 1], ["FleetBeacon", 1], ["TwilightCouncil", 1], ["TemplarArchive", 1], ["DarkShrine", 1], ["RoboticsBay", 1] ]

            MCTS_experiment["Experiments"][exp_name]["AlwaysMakeWorkers"] = True

            MCTS_experiment["Experiments"][exp_name]["OpeningBuildOrder"] = "Empty"

            MCTS_experiment["Experiments"][exp_name]["SortActions"] = False

            MCTS_experiment["Experiments"][exp_name]["SaveStates"] = False 

            MCTS_experiment["Experiments"][exp_name]["UsePolicyNetwork"] = False

            MCTS_experiment["Experiments"][exp_name]["UsePolicyValueNetwork"] = False

print("number of experiments: {}".format(len(MCTS_experiment["Experiments"])))

MCTS_experiment["States"] = dict()
MCTS_experiment["States"]["Protoss Start State"] = { "race" : "Protoss", "minerals" : 50, "gas" : 0, "units" : [ ["Probe", 12], ["Nexus", 1] ] }
MCTS_experiment["States"]["Protoss 0"] = {"race": "Protoss", "units": [["Nexus", 3], ["Probe", 68], ["Pylon", 20], ["Gateway", 9], ["Assimilator", 6], ["CyberneticsCore", 1], ["Stargate", 1], ["RoboticsFacility", 1], ["Stalker", 3], ["Observer", 2], ["Sentry", 1], ["Phoenix", 5], ["Immortal", 5], ["Zealot", 17], ["Forge", 1], ["TwilightCouncil", 1], ["RoboticsBay", 1], ["TemplarArchive", 1], ["PhotonCannon", 3], ["Disruptor", 3]], "minerals": 1065, "gas": 436}
MCTS_experiment["States"]["Protoss 1"] = {"race": "Protoss", "units": [["Nexus", 3], ["Probe", 54], ["Pylon", 17], ["Gateway", 14], ["Assimilator", 6], ["CyberneticsCore", 1], ["Stargate", 1], ["TwilightCouncil", 1], ["RoboticsFacility", 1], ["Immortal", 4], ["Forge", 1], ["Adept", 1], ["TemplarArchive", 1], ["Zealot", 14], ["PhotonCannon", 3], ["Observer", 2]], "minerals": 15, "gas": 592}
MCTS_experiment["States"]["Protoss 2"] = {"race": "Protoss", "units": [["Nexus", 2], ["Probe", 19], ["Pylon", 1], ["Gateway", 1], ["Assimilator", 1]], "minerals": 185, "gas": 36}
MCTS_experiment["States"]["Protoss 3"] = {"race": "Protoss", "units": [["Nexus", 2], ["Probe", 29], ["Pylon", 3], ["Gateway", 3], ["Assimilator", 2], ["CyberneticsCore", 1], ["Stalker", 2]], "minerals": 35, "gas": 62}

MCTS_experiment["Build Orders"] = dict()
MCTS_experiment["Build Orders"]["Empty"] = []

MCTS_experiment["Game Data"] = "SC2Data.json"
MCTS_experiment["ExperimentsInParallel"] = 1

with open("Experiments_SRMV.txt", 'w') as outputFile:
    json.dump(MCTS_experiment, outputFile)