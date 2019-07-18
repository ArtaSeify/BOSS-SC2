import tensorflow as tf
import os
import subprocess
import argparse
import model
import json
import copy
import random

if os.name == "nt":
    WINDOWS = True
else:
    WINDOWS = False
    
if WINDOWS:
    BIN_PATH = "C:\\School Work\\BOSS\\bin"
    MODELS_PATH = BIN_PATH + "\\ML\\models"
    DATA_PATH = BIN_PATH + "\\SavedStates"
    python = "python"
else:
    BIN_PATH = "/home/seify/NEWBOSS/bin"
    MODELS_PATH = BIN_PATH + "/ML/models"
    DATA_PATH = BIN_PATH + "/SavedStates"
    python = "python3"

class Driver:
    def __init__(self, args):
        self.model_name = args.save_name
        self.load_name = args.load_model
        self.start_run = int(args.run) if args.run is not None else 0
        self.iterations = int(args.iterations)
        self.experiment_file_name = "Experiments"
        self.output_type = args.output_type
        self.args = args

        self.numTestRuns = 20

        self.validation_split = 0.10
        self.training_samples = 0
        self.validation_samples = 0
        self.TRAINING_SAMPLES_LIMIT = 100000
        self.VALIDATION_SAMPLES_LIMIT = int(self.validation_split * self.TRAINING_SAMPLES_LIMIT)

        with open(BIN_PATH + "/" + self.experiment_file_name + ".txt", 'r') as experiment_file:
            json_data = experiment_file.read()
            self.experiment_data = json.loads(json_data)
            self.experiment_name = list(self.experiment_data["Experiments"].keys())[0]
            self.data_file_name = self.experiment_name
            self.train_file_name = self.data_file_name + "_train.csv"
            self.validation_file_name = self.data_file_name + "_validation.csv"
            self.strengthtest_file_name = "Experiments_strengthtest"

            # count initial samples
            if os.path.isfile(os.path.join(DATA_PATH, self.train_file_name)):
                with open(os.path.join(DATA_PATH, self.train_file_name), 'r') as train_file:
                    self.training_samples = len(train_file.readlines())
            if os.path.isfile(os.path.join(DATA_PATH, self.validation_file_name)):
                with open(os.path.join(DATA_PATH, self.validation_file_name), 'r') as validation_file:
                    self.validation_samples = len(validation_file.readlines())

            print(self.training_samples)
            print(self.validation_samples)

            with open(BIN_PATH + "/" + self.experiment_file_name + "_modified.txt", 'w') as modified_exp_file:
                if self.load_name is None:
                    self.experiment_data["Experiments"][self.experiment_name]["UsePolicyNetwork"] = False
                    self.experiment_data["Experiments"][self.experiment_name]["UsePolicyValueNetwork"] = False
                else:
                    if self.output_type == "B":
                        self.experiment_data["Experiments"][self.experiment_name]["UsePolicyValueNetwork"] = True
                        self.experiment_data["Experiments"][self.experiment_name]["UsePolicyNetwork"] = False
                    elif self.output_type == "P":
                        self.experiment_data["Experiments"][self.experiment_name]["UsePolicyValueNetwork"] = False
                        self.experiment_data["Experiments"][self.experiment_name]["UsePolicyNetwork"] = True
                self.experiment_data["Experiments"][self.experiment_name]["OutputDir"] = BIN_PATH + "/" + self.experiment_name + "/" + str(self.start_run)

                json.dump(self.experiment_data, modified_exp_file)
                self.experiment_file_name = "Experiments_modified"

    def create_teststrength_file(self, run):
        strength_exp_json = copy.deepcopy(self.experiment_data)
        usePolicy = True if (run != "-1" or self.load_name is not None) else False
        
        with open(BIN_PATH + "/" + self.strengthtest_file_name + ".txt", 'w') as strength_exp_file:
            # Reset after changing root
            strength_exp_json["Experiments"][self.experiment_name]["OutputDir"] = BIN_PATH + "/" + self.experiment_name + "/StrengthTest/WithReset/" + str(run)
            strength_exp_json["Experiments"][self.experiment_name]["Run"][1] = self.numTestRuns
            if self.output_type == "B":
                strength_exp_json["Experiments"][self.experiment_name]["UsePolicyValueNetwork"] = usePolicy
                strength_exp_json["Experiments"][self.experiment_name]["UsePolicyNetwork"] = False
            elif self.output_type == "P":
                strength_exp_json["Experiments"][self.experiment_name]["UsePolicyValueNetwork"] = False
                strength_exp_json["Experiments"][self.experiment_name]["UsePolicyNetwork"] = usePolicy
            strength_exp_json["Experiments"][self.experiment_name]["SaveStates"] = False
            strength_exp_json["Experiments"][self.experiment_name]["ChangingRoot"]["Active"] = True
            strength_exp_json["Experiments"][self.experiment_name]["SearchParameters"]["TemperatureChangeFrame"] = 0

            # No reset after changing root
            strength_exp_json["Experiments"][self.experiment_name + "_2"] = copy.deepcopy(strength_exp_json["Experiments"][self.experiment_name])
            strength_exp_json["Experiments"][self.experiment_name + "_2"]["OutputDir"] = BIN_PATH + "/" + self.experiment_name + "/StrengthTest/WithoutReset/" + str(run)
            strength_exp_json["Experiments"][self.experiment_name + "_2"]["Run"][1] = self.numTestRuns
            if self.output_type == "B":
                strength_exp_json["Experiments"][self.experiment_name + "_2"]["UsePolicyValueNetwork"] = usePolicy
                strength_exp_json["Experiments"][self.experiment_name + "_2"]["UsePolicyNetwork"] = False
            elif self.output_type == "P":
                strength_exp_json["Experiments"][self.experiment_name + "_2"]["UsePolicyValueNetwork"] = False
                strength_exp_json["Experiments"][self.experiment_name + "_2"]["UsePolicyNetwork"] = usePolicy
            strength_exp_json["Experiments"][self.experiment_name + "_2"]["SaveStates"] = False
            strength_exp_json["Experiments"][self.experiment_name + "_2"]["ChangingRoot"]["Active"] = True
            strength_exp_json["Experiments"][self.experiment_name + "_2"]["ChangingRoot"]["Reset"] = False
            strength_exp_json["Experiments"][self.experiment_name + "_2"]["SearchParameters"]["TemperatureChangeFrame"] = 0

            # Test policy of the network
            if run != "-1" or self.load_name is not None:
                strength_exp_json["Experiments"][self.experiment_name + "_3"] = copy.deepcopy(strength_exp_json["Experiments"][self.experiment_name])
                strength_exp_json["Experiments"][self.experiment_name + "_3"]["OutputDir"] = BIN_PATH + "/" + self.experiment_name + "/StrengthTest/Network/" + str(run)
                strength_exp_json["Experiments"][self.experiment_name + "_3"]["Run"][1] = 1
                strength_exp_json["Experiments"][self.experiment_name + "_3"]["UsePolicyNetwork"] = True
                strength_exp_json["Experiments"][self.experiment_name + "_3"]["SearchParameters"]["Simulations"] = -1
            
            json.dump(strength_exp_json, strength_exp_file)

    def split_training_data(self, run):
        with open(os.path.join(DATA_PATH, self.experiment_name + ".csv"), 'r') as data_file:
            all_data = data_file.readlines()
            random.shuffle(all_data)
            samples = len(all_data)
            validation_size = int(samples * self.validation_split)
            validation_data = all_data[:validation_size]
            train_data = all_data[validation_size:]

            self.training_samples += len(train_data)
            self.validation_samples += len(validation_data)

            with open(os.path.join(DATA_PATH, self.validation_file_name), 'a+') as validation_file:
                for sample in validation_data:
                    validation_file.write(sample)
            with open(os.path.join(DATA_PATH, self.train_file_name), 'a+') as train_file:
                for sample in train_data:
                    train_file.write(sample)

            if self.training_samples > self.TRAINING_SAMPLES_LIMIT:
                with open(os.path.join(DATA_PATH, self.train_file_name), "r+") as train_file:
                    all_data = train_file.readlines()
                    with open(os.path.join(DATA_PATH, self.train_file_name), "w") as train_file_overwrite:
                        for sample in all_data[self.training_samples - self.TRAINING_SAMPLES_LIMIT:]:
                            train_file_overwrite.write(sample)
                with open(os.path.join(DATA_PATH, self.validation_file_name), "r+") as validation_file:
                    all_data = validation_file.readlines()
                    with open(os.path.join(DATA_PATH, self.validation_file_name), "w") as validation_file_overwrite:
                        for sample in all_data[self.validation_samples - self.VALIDATION_SAMPLES_LIMIT:]:
                            validation_file_overwrite.write(sample)

                self.training_samples = self.TRAINING_SAMPLES_LIMIT
                self.validation_samples = self.VALIDATION_SAMPLES_LIMIT

        os.rename(os.path.join(DATA_PATH, self.experiment_name + ".csv"), os.path.join(DATA_PATH, self.experiment_name + "_" + str(run) + ".csv"))

    def start(self):
        # initial strength test
        if self.start_run == 0:
            self.create_teststrength_file("-1")
        elif self.args.teststr is not None:
            self.create_teststrength_file(str(self.start_run-1))

        if self.start_run == 0 or self.args.teststr is not None:
            print("calling command: ", BIN_PATH + "/BOSS_main", self.strengthtest_file_name, self.load_name)
            if self.load_name:
                subprocess.call([BIN_PATH + "/BOSS_main", self.strengthtest_file_name, self.load_name])
            else:
                subprocess.call([BIN_PATH + "/BOSS_main", self.strengthtest_file_name])

        for run in range(self.start_run, self.iterations + self.start_run):
            if run == 0 and self.load_name is None:
                print("calling command: ", BIN_PATH + "/BOSS_main.exe", self.experiment_file_name)
                subprocess.call([BIN_PATH + "/BOSS_main", self.experiment_file_name])
                
            elif run == 0 and self.load_name is not None:
                print("calling command: ", BIN_PATH + "/BOSS_main.exe", self.experiment_file_name, self.load_name)
                subprocess.call([BIN_PATH + "/BOSS_main", self.experiment_file_name, self.load_name])

            else:
                print("calling command: ", BIN_PATH + "/BOSS_main.exe", self.experiment_file_name, self.model_name)
                subprocess.call([BIN_PATH + "/BOSS_main", self.experiment_file_name, self.model_name])

            self.split_training_data(run)

            # rewrite the experiment file
            if run == 0:
                if self.output_type == "B":
                    self.experiment_data["Experiments"][self.experiment_name]["UsePolicyValueNetwork"] = True
                    self.experiment_data["Experiments"][self.experiment_name]["UsePolicyNetwork"] = False
                elif self.output_type == "P":
                    self.experiment_data["Experiments"][self.experiment_name]["UsePolicyNetwork"] = True
                    self.experiment_data["Experiments"][self.experiment_name]["UsePolicyValueNetwork"] = False
                with open(BIN_PATH + "/" + self.experiment_file_name + ".txt", 'w') as experiment_file:
                    json.dump(self.experiment_data, experiment_file)

                if self.load_name is not None:
                    print("calling command: ", python, BIN_PATH + "/ML/train.py", self.model_name, DATA_PATH + "/" + self.train_file_name, DATA_PATH + "/" + self.validation_file_name, self.output_type)
                    subprocess.call([python, BIN_PATH + "/ML/train.py", self.model_name, DATA_PATH + "/" + self.train_file_name, DATA_PATH + "/" + self.validation_file_name, self.output_type])
                else:
                    print("calling command: ", python, BIN_PATH + "/ML/train.py", self.model_name, DATA_PATH + "/" + self.train_file_name, DATA_PATH + "/" + self.validation_file_name, self.output_type)
                    subprocess.call([python, BIN_PATH + "/ML/train.py", self.model_name, DATA_PATH + "/" + self.train_file_name, DATA_PATH + "/" + self.validation_file_name, self.output_type])
            else:
                print("calling command: ", python, BIN_PATH + "/ML/train.py", self.model_name, DATA_PATH + "/" + self.train_file_name, DATA_PATH + "/" + self.validation_file_name, self.output_type)
                subprocess.call([python, BIN_PATH + "/ML/train.py", self.model_name, DATA_PATH + "/" + self.train_file_name, DATA_PATH + "/" + self.validation_file_name, self.output_type])
            
            # Test strength
            self.create_teststrength_file(run)
            subprocess.call([BIN_PATH + "/BOSS_main", self.strengthtest_file_name, self.model_name])

            # Put runs for each iteration in a different subfolder
            self.experiment_data["Experiments"][self.experiment_name]["OutputDir"] = BIN_PATH + "/" + self.experiment_name + "/" + str(run + 1)
            with open(BIN_PATH + "/" + self.experiment_file_name + ".txt", 'w') as experiment_file:
                json.dump(self.experiment_data, experiment_file)            

def main():
    global driver

    parser = argparse.ArgumentParser()
    parser.add_argument("save_name", help="Name of model to save")
    parser.add_argument("iterations", help="Number of times to iterate search and train")
    parser.add_argument("output_type", help="One of Policy, Value, or Both")
    parser.add_argument("--load_model", help="Name of model to load")
    parser.add_argument("--run", help="The run to start from")
    parser.add_argument("--teststr", help="Run strength test if loading model and run is above 0")
    args = parser.parse_args()

    driver = Driver(args)
    driver.start()

if __name__ == "__main__":
    main()