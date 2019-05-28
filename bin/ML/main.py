import tensorflow as tf
import os
import subprocess
import argparse
import model
import json

BIN_PATH = "C:\\School Work\\BOSS\\bin"
MODELS_PATH = BIN_PATH + "\\ML\\models"
DATA_PATH = BIN_PATH + "\\SavedStates"

class Driver:
    def __init__(self, args):
        self.model_name = args.save_name
        self.load_name = args.load_model
        self.iterations = int(args.iterations)
        self.experiment_file_name = "Experiments"

    def start(self):
        with open(BIN_PATH + "\\" + self.experiment_file_name + ".txt", 'r') as experiment_file:
            json_data = experiment_file.read()
            self.experiment_data = json.loads(json_data)
            self.experiment_name = list(self.experiment_data["Experiments"].keys())[0]

            with open(BIN_PATH + "\\" + self.experiment_file_name + "_modified.txt", 'w') as modified_exp_file:
                if self.load_name is None:
                    self.experiment_data["Experiments"][self.experiment_name]["UsePolicyNetwork"] = False
                self.experiment_data["Experiments"][self.experiment_name]["OutputDir"] = "../" + self.experiment_name

                json.dump(self.experiment_data, modified_exp_file)
                self.experiment_file_name = "Experiments_modified"

        for run in range(self.iterations):
            if run == 0 and self.load_name is None:
                print("calling command: ", BIN_PATH + "\\BOSS_main.exe", self.experiment_file_name)
                subprocess.call([BIN_PATH + "\\BOSS_main.exe", self.experiment_file_name])
                
            elif run == 0 and self.load_name is not None:
                print("calling command: ", BIN_PATH + "\\BOSS_main.exe", self.experiment_file_name, self.load_name)
                subprocess.call([BIN_PATH + "\\BOSS_main.exe", self.experiment_file_name, self.load_name])

            else:
                print("calling command: ", BIN_PATH + "\\BOSS_main.exe", self.experiment_file_name, self.model_name)
                subprocess.call([BIN_PATH + "\\BOSS_main.exe", self.experiment_file_name, self.model_name])

            # rewrite the experiment file
            if run == 0:
                self.experiment_data["Experiments"][self.experiment_name]["UsePolicyNetwork"] = True
                with open(BIN_PATH + "\\" + self.experiment_file_name + ".txt", 'w') as experiment_file:
                    json.dump(self.experiment_data, experiment_file)

                if self.load_name is not None:
                    print("calling command: ", "python", BIN_PATH + "\\ML\\train.py", self.model_name, DATA_PATH + "\\" + self.experiment_name + ".csv", "--load_model=" + self.load_name)
                    subprocess.call(["python", BIN_PATH + "\\ML\\train.py", self.model_name, DATA_PATH + "\\" + self.experiment_name + ".csv", "--load_model=" + self.load_name])
                else:
                    print("calling command: ", "python", BIN_PATH + "\\ML\\train.py", self.model_name, DATA_PATH + "\\" + self.experiment_name + ".csv")
                    subprocess.call(["python", BIN_PATH + "\\ML\\train.py", self.model_name, DATA_PATH + "\\" + self.experiment_name + ".csv"])
            else:
                print("calling command: ", "python", BIN_PATH + "\\ML\\train.py", self.model_name, DATA_PATH + "\\" + self.experiment_name + ".csv", "--load_model=" + self.model_name)
                subprocess.call(["python", BIN_PATH + "\\ML\\train.py", self.model_name, DATA_PATH + "\\" + self.experiment_name + ".csv", "--load_model=" + self.model_name])

def main():
    global driver

    parser = argparse.ArgumentParser()
    parser.add_argument("save_name", help="Name of model to save")
    parser.add_argument("iterations", help="Number of times to iterate search and train")
    parser.add_argument("--load_model", help="Name of model to load")
    args = parser.parse_args()

    driver = Driver(args)
    driver.start()

if __name__ == "__main__":
    main()