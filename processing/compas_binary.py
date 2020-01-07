import os

import numpy as np
import pandas as pd
import tabular as tb

import mine


fout = os.path.join('..', 'data', 'compas.csv')
dout = os.path.join('..', 'data', 'CrossValidation')
num_folds = 10
max_cardinality = 2
min_support = 0.01
minor = True

seed = sum([3, 15, 13, 16, 1, 19]) # c:3, o:15, m:13, p:16, a:1, s:19
np.random.seed(seed)

compas = pd.read_csv("../compas/compas-scores-two-years.csv")

neg_names = ['Age>20', 'Age>22', 'Age>25', 'Age<24-or-Age>30', 'Age<24-or-Age>40', 'Age<30', 'Age>40', 'Age>45',
             'Gender=Female',
             'Race-not-African-American', 'Race-not-Caucasian', 'Race-not-Asian', 'Race-not-Hispanic', 'Race-not-Native-American', 'Race-not-Other',
             'Juvenile-Felonies>0', 'Juvenile-Felonies=0-or-Juvenile-Felonies>3', 'Juvenile-Felonies<=3',
             'Juvenile-Crimes>0', 'Juvenile-Crimes=0-or-Juvenile-Crimes>3', 'Juvenile-Crimes<=3', 'Juvenile-Crimes<=5',
             'Prior-Crimes>0', 'Prior-Crimes=0-or-Prior-Crimes>3', 'Prior-Crimes<=3', 'Prior-Crimes<=5', 
             'Current-Charge-Degree=Felony',
             'Not-Recidivate-Within-Two-Years']

# 'Not-No-charge', 'Not-Assault-or-Battery', 'Not-Theft-Burglary-or-Robbery', 'Not-Drug-Related', 'Not-Possession',

# Age (min age is 18, age==18 has support 3)
age_20 = ('Age=18-20', compas['age'] <= 20)
age_22 = ('Age=18-22', compas['age'] <= 22)
age_25 = ('Age=18-25', compas['age'] <= 25)
age_24_30 = ('Age=24-30', (compas['age'] >= 24) & (compas['age'] <= 30))
age_24_40 = ('Age=24-40', (compas['age'] >= 24) & (compas['age'] <= 40))
age_30 = ('Age>=30', compas['age'] >= 30)
age_40 = ('Age<=40', compas['age'] >= 40)
age_45 = ('Age<=45', compas['age'] >= 45)

# Gender
male = ('Gender=Male', compas['sex'] == 'Male')

# Race
black = ('Race=African-American', compas['race'] == 'African-American')
white = ('Race=Caucasian', compas['race'] == 'Caucasian')
asian = ('Race=Asian', compas['race'] == 'Asian')
hispanic = ('Race=Hispanic', compas['race'] == 'Hispanic')
native_american = ('Race=Native-American', compas['race'] == 'Native American')
other = ('Race=Other', compas['race'] == 'Other')

# Juvenile Felonies (juv_fel_count>=5 has support 12)
juv_felonies_0 = ('Juvenile-Felonies=0', compas['juv_fel_count'] == 0)
juv_felonies_1_3 = ('Juvenile-Felonies=1-3', 
                    (compas['juv_fel_count'] >= 1) & (compas['juv_fel_count'] <= 3))
juv_felonies_3 = ('Juvenile-Felonies>3', compas['juv_fel_count'] > 3)

# Juvenile Crimes
juv_crimes_0 = ('Juvenile-Crimes=0',
                compas['juv_fel_count'] + compas['juv_misd_count'] + compas['juv_other_count'] == 0)
juv_crimes_1_3 = ('Juvenile-Crimes=1-3',
                ((compas['juv_fel_count'] + compas['juv_misd_count'] + compas['juv_other_count']) >= 1) &
                      ((compas['juv_fel_count'] + compas['juv_misd_count'] + compas['juv_other_count']) <= 3))
juv_crimes_3 = ('Juvenile-Crimes>3',
                (compas['juv_fel_count'] + compas['juv_misd_count'] + compas['juv_other_count']) > 3)
juv_crimes_5 = ('Juvenile-Crimes>5',
                (compas['juv_fel_count'] + compas['juv_misd_count'] + compas['juv_other_count']) > 5)
# Prior Crimes
prior_crimes_0 = ('Prior-Crimes=0', compas['priors_count'] == 0)
prior_crimes_1_3 = ('Prior-Crimes=1-3', 
                    (compas['priors_count'] >= 1) & (compas['priors_count'] <= 3))
prior_crimes_3 = ('Prior-Crimes>3', compas['priors_count'] > 3)
prior_crimes_5 = ('Prior-Crimes>5', compas['priors_count'] > 5)

# Current Charge Degree
misdemeanor = ('Current-Charge-Degree=Misdemeanor', compas['c_charge_degree'] == 'M')

# Current Charge Description
no_charge = ('No-Charge', np.array(['no charge' in str(c).lower() for c in compas.c_charge_desc], int)) # 1137
assault = ('Assault-or-Battery', np.array([('assault' in str(c).lower()) or ('battery' in str(c).lower()) for c in compas.c_charge_desc], int))   # 1813
theft = ('Theft-Burglary-or-Robbery', np.array([('theft' in str(c).lower()) or ('burglary' in str(c).lower()) or ('robbery' in str(c).lower()) for c in compas.c_charge_desc], int))   # 1051
drug_related = ('Drug-Related', np.array([('possession of' in str(c).lower()) or ('deliver' in str(c).lower()) or ('sel/del' in str(c).lower()) or ('heroin' in str(c).lower()) or ('cannabis' in str(c).lower()) or ('cocaine' in str(c).lower()) or ('meth' in str(c).lower()) or ('traffick' in str(c).lower())for c in compas.c_charge_desc], int))    # 1156
possession = ('Possession', np.array(['pos' in str(c).lower() for c in compas.c_charge_desc], int)) # 1248 (usually drug or firearm)

# Prediction
y = ('Recidivate-Within-Two-Years', compas['is_recid'] == 1)

#  no_charge, assault, theft, drug_related, possession,

data = [age_20, age_22, age_25, age_24_30, age_24_40, age_30, age_40, age_45,
         male,
         black, white, asian, hispanic, native_american, other,
         juv_felonies_0, juv_felonies_1_3, juv_felonies_3,
         juv_crimes_0, juv_crimes_1_3, juv_crimes_3, juv_crimes_5,
         prior_crimes_0, prior_crimes_1_3, prior_crimes_3, prior_crimes_5,
         misdemeanor,
         y]

names = [n for (n, col) in data]
data = [np.array(col, int) for (n, col) in data]

print 'write binary dataset', fout
y = tb.tabarray(columns=data, names=names)
y.saveSV(fout)

print 'permute and partition dataset'
split_ind = np.split(np.random.permutation(len(y) / num_folds * num_folds), num_folds)
print 'number of folds:', num_folds
print 'train size:', len(split_ind[0]) * (num_folds - 1)
print 'test size:', len(split_ind[0])

num_rules = np.zeros(num_folds, int)
for i in range(num_folds):
    print 'generate cross-validation split', i
    cv_root = 'compas_%d' % i
    test_root = '%s_test' % cv_root
    train_root = '%s_train' % cv_root
    ftest = os.path.join(dout, '%s.csv' % test_root)
    ftrain = os.path.join(dout, '%s.csv' % train_root)
    y[split_ind[i]].saveSV(ftest)
    y[np.concatenate([split_ind[j] for j in range(num_folds) if (j != i)])].saveSV(ftrain)

    print 'mine rules from', ftrain
    num_rules[i] = mine.mine_binary(din=dout, froot=train_root,
                                   max_cardinality=max_cardinality,
                                   min_support=min_support, neg_names=neg_names,
                                   minor=minor)
    mine.apply_binary(din=dout, froot=cv_root, neg_names=neg_names)

print '(min, max) # rules mined per fold:', (num_rules.min(), num_rules.max())
