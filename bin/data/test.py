import os

evaluations = [1.2, 1.5, 20.1]

print(evaluations)
if not os.path.isdir(os.path.join(os.getcwd(), "logs\\evaluations\\")):
	os.makedirs(os.path.join(os.getcwd(), "logs\\evaluations\\"))
with open(os.path.join(os.getcwd(), "logs\\evaluations\\" + "dfs"), 'w') as outputFile:
	for val in evaluations:
		outputFile.write(str(val))