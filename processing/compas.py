from datetime import datetime
import os
import time

import numpy as np
import tabular as tb

import mine
import utils

def get_age(dob, cd):
    dob = datetime.strptime(dob, "%Y-%m-%d")
    cd = datetime.strptime(cd, "%Y-%m-%d")
    return ((cd - dob) + datetime(1, 1, 1)).year

def age_func(dob, cd):
    a = get_age(dob, cd)
    if (a <= 20):       # minimum age is 18
        return '18-20'  # support = 220
    elif (a <= 22):
        return '21-22'  # support = 623
    elif (a <= 25):
        return '23-25'  # support = 1018
    elif (a <= 45):
        return '26-45'  # support = 3890
    else:
        return '>45'    # support = 1463

def priors_count_func(p):
    if (p == 0):
        return '=0'     # support = 2150
    elif (p <= 1):
        return '=1'     # support = 1397
    elif (p <= 3):
        return '2-3'    # support = 1408
    else:
        return '>3'    # support = 2259


fin = os.path.join('..', 'compas', 'compas-scores-two-years.csv')
fout = os.path.join('..', 'data', 'compas.csv')
bout = os.path.join('..', 'data', 'compas-binary.csv')
din = os.path.join('..', 'data')
dout = os.path.join('..', 'data', 'CrossValidation')

if not os.path.exists(dout):
    os.mkdir(dout)

seed = sum([3, 15, 13, 16, 1, 19]) # c:3, o:15, m:13, p:16, a:1, s:19
num_folds = 10
max_cardinality = 2
min_support = 0.005
labels = ['No', 'Yes']
minor = True
race = False

np.random.seed(seed)

# duplicate names in header:  decile_score, priors_count
names = open(fin, 'rU').read().strip().split('\n')[0].split(',')

nlist = []
for n in names:
    if n in nlist:
        print 'duplicate name', n, '->', n + '_'
        nlist.append('%s_' % n)
    else:
        nlist.append(n)

x = tb.tabarray(SVfile=fin, names=nlist)

assert (x['priors_count'] == x['priors_count_']).all()
assert (x['decile_score'] == x['decile_score_']).all()

"""
columns = [(x['sex'] == 'Male'),
           (x['age'] <= 20), (x['age'] <= 22), (x['age'] <= 25),
           (x['age'] < 30), (x['age'] >= 60),
           ((x['age'] >= 30) & (x['age'] <= 44)),
           ((x['age'] >= 45) & (x['age'] <= 59))]
"""

age = np.array([age_func(dob, cd) for dob, cd in zip(x['dob'], x['compas_screening_date'])])

juvenile_felonies = np.array(['>0' if (i > 0) else '=0' for i in x['juv_fel_count']])   # support = 282

juvenile_misdemeanors = np.array(['>0' if (i > 0) else '=0' for i in x['juv_misd_count']])  # support = 415

juvenile_crimes = np.array(['>0' if (i > 0) else '=0' for i in x['juv_fel_count'] + x['juv_misd_count'] + x['juv_other_count']]) # support = 973

priors_count = np.array([priors_count_func(i) for i in x['priors_count']])

assert (set(x['c_charge_degree']) == set(['F', 'M']))

c_charge_degree = np.array(['Misdemeanor' if (i == 'M') else 'Felony' for i in x['c_charge_degree']])

# see `c_jail_in` and `c_jail_out` for time in jail?

columns = [x['sex'], age, juvenile_felonies, juvenile_misdemeanors, juvenile_crimes,
           priors_count, c_charge_degree]

cnames = ['sex', 'age', 'juvenile-felonies', 'juvenile-misdemeanors', 'juvenile-crimes',
          'priors', 'current-charge-degree']

if race:
    race_list = list(set(x['race']))
    columns += [(x['race'] == n) for n in race_list]
    cnames += ['Race=%s' % r for r in race_list]

columns.append(x['two_year_recid'])
cnames.append('recidivate-within-two-years')

print 'write categorical dataset', fout
y = tb.tabarray(columns=columns, names=cnames)
y.saveSV(fout)

print 'write binary dataset', bout
b = utils.to_binary(y)
b.saveSV(bout)

print 'permute and partition dataset'
split_ind = np.array_split(np.random.permutation(len(y)), num_folds)
print 'number of folds:', num_folds
print 'train size:', [sum([len(split_ind[i]) for i in range(num_folds) if i != j]) for j in range(num_folds)]
print 'test size:', [sum([len(split_ind[i]) for i in range(num_folds) if i == j]) for j in range(num_folds)]

num_rules = np.zeros(num_folds, int)
mine_time = np.zeros(num_folds)
for i in range(num_folds):
    print 'generate cross-validation split', i
    cv_root = 'compas_%d' % i
    test_root = '%s_test' % cv_root
    train_root = '%s_train' % cv_root
    ftest = os.path.join(dout, '%s.csv' % test_root)
    ftrain = os.path.join(dout, '%s.csv' % train_root)
    btest = os.path.join(dout, '%s-binary.csv' % test_root)
    btrain = os.path.join(dout, '%s-binary.csv' % train_root)
    train_ind = np.concatenate([split_ind[j] for j in range(num_folds) if (j != i)])
    y[split_ind[i]].saveSV(ftest)
    y[train_ind].saveSV(ftrain)
    b[split_ind[i]].saveSV(btest)
    b[train_ind].saveSV(btrain)

    t0 = time.time()
    print 'mine rules from', ftrain
    num_rules[i] = mine.mine_rules(din=dout, froot=train_root,
                                    max_cardinality=max_cardinality,
                                    min_support=min_support, labels=labels,
                                    minor=minor)
    mine_time[i] = time.time() - t0
    mine.apply_rules(din=dout, froot=cv_root, labels=labels)

print '(min, max) # rules mined per fold:', (num_rules.min(), num_rules.max())
print 'mining times:', mine_time
print 'average, std:', mine_time.mean(), mine_time.std()

# rule mining is fast, about 0.51 seconds
