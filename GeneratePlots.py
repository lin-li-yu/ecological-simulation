import matplotlib.pyplot as plt
 
import string 
import sys
import ast
import os

def parse_row(row):
    row_data = []
    idx_start = 0
    idx_end = 0
    while idx_start < len(row):
        # move to first non-whitespace character
        if row[idx_start] in string.whitespace:
            idx_start += 1
        elif row[idx_start]=='(':  # parse a tuple
            idx_end = idx_start
            while row[idx_end]!=')':
                idx_end += 1
            val = ast.literal_eval(row[idx_start:idx_end+1])
            row_data.append(val)
            while idx_end < len(row) and row[idx_end]!=',':  # find the next comma
                idx_end += 1
            idx_start = idx_end+1
        else:  # parse a float
            idx_end = idx_start
            while idx_end < len(row) and row[idx_end]!=',':
                idx_end += 1
            val = float(row[idx_start:idx_end])
            row_data.append(val)
            idx_start = idx_end + 1
    return row_data

if len(sys.argv) < 2:
    raise RuntimeError("A filename must be specified when running this script, as in the following example:\n python3 GeneratePlots.py example_data.txt")

filename = sys.argv[1]

with open(filename) as file:
    header = file.readline()
    columns = [x.strip() for x in header.split(',')]

    rows = [parse_row(row) for row in file.readlines()]

n_col = len(columns)

data = [[row[i] for row in rows] for i in range(n_col)]

# Create plot directory if it doesn't exist
os.makedirs("plot", exist_ok=True)

for i in range(1, n_col):
    plt.figure()
    if isinstance(data[i][0], tuple):
        means = [x[0] for x in data[i]]
        sigmas = [x[1] for x in data[i]]
        plt.errorbar(data[0], means, sigmas)
    else:
        plt.plot(data[0], data[i])
    plt.xlabel(columns[0])
    plt.ylabel(columns[i])
    plt.savefig(os.path.join("plot", columns[i] + "_plot.png"))
 