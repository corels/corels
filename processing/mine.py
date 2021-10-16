import itertools
import os

import numpy as np
import tabular as tb


def mine_binary(din='../data/compas', froot='compas', max_cardinality=2,
               min_support=0.005, neg_names=None, minor=True,
               verbose=False, prefix=''):
    """
    This computes negations of features
    """
    fin = os.path.join(din, '%s.csv' % froot)
    fout = os.path.join(din, '%s%s.out' % (prefix, froot))
    flabel = os.path.join(din, '%s%s.label' % (prefix, froot))

    x = tb.tabarray(SVfile=fin)

    if (neg_names is None):
        neg_names = [n.replace(':', ':not-') for n in x.dtype.names]

    y = tb.tabarray(array=(1 - x.extract()), names=neg_names)

    features = list(x.dtype.names)[:-1] + list(y.dtype.names)[:-1]
    positive_label_name = x.dtype.names[-1]
    negative_label_name = y.dtype.names[-1]

    x = x.colstack(y)

    ndata = len(x)
    min_threshold = ndata * min_support
    max_threshold = ndata * (1. - min_support)

    records = []
    names = []
    udict = {}
    for n in features:
        ulist = tb.utils.uniqify(x[n])
        #assert (set(ulist) == set([0, 1]))
        udict[n] = []
        bvec = (x[n] == 1)
        supp = bvec.sum()
        if ((supp > min_threshold) and (supp < max_threshold)):
            names += ['{%s}' % n]
            udict[n] += [1]
            records += [bvec]

    for cardinality in range(2, max_cardinality + 1):
        for dlist in list(itertools.combinations(features, cardinality)):
            if (dlist[0][:3] == dlist[1][:3]):  # this is a hack
                continue
            bvec = np.array([(x[d] == 1) for d in dlist]).all(axis=0)
            supp = bvec.sum()
            if ((supp > min_threshold) and (supp < max_threshold)):
                descr = '{%s}' % ','.join(['%s' % d for d in dlist])
                names.append(descr)
                records.append(bvec)
                if (verbose):
                    print descr

    rr = [' '.join(np.cast[str](np.cast[int](r))) for r in records]
    print len(rr), len(set(rr))

    print len(names), 'rules mined'
    print 'writing', fout
    f = open(fout, 'w')
    f.write('\n'.join(['%s %s' % (n, ' '.join(np.cast[str](np.cast[int](r))))
                        for (n, r) in zip(names, records)]) + '\n')
    f.close()

    print 'writing', flabel
    recs = [x[negative_label_name], x[positive_label_name]]
    label_names = [negative_label_name, positive_label_name]
    f = open(flabel, 'w')
    f.write('\n'.join(['{%s} %s' % (l, ' '.join(np.cast[str](np.cast[int](r))))
                       for (l, r) in zip(label_names, recs)]) + '\n')
    f.close()

    if (minor):
        import minority
        fminor = os.path.join(din, '%s%s.minor' % (prefix, froot))
        print 'computing', fminor
        minority.compute_minority(froot='%s%s' % (prefix, froot), dir=din)

    return len(names)

def mine_rules(din='../data/adult', froot='adult', max_cardinality=2,
               min_support=0.01, labels=['<=50K', '>50K'], minor=True,
               verbose=False, prefix='', exclude_not=False):
    """
    This doesn't do negations of features so it's not complete.
    """
    fin = os.path.join(din, '%s.csv' % froot)
    fout = os.path.join(din, '%s%s.out' % (prefix, froot))
    flabel = os.path.join(din, '%s%s.label' % (prefix, froot))

    x = tb.tabarray(SVfile=fin)
    features = list(x.dtype.names)[:-1]
    label_name = x.dtype.names[-1]
    ndata = len(x)
    min_threshold = ndata * min_support
    max_threshold = ndata * (1. - min_support)

    records = []
    names = []
    udict = {}
    for n in features:
        ulist = tb.utils.uniqify(x[n])
        udict[n] = []
        for u in ulist:
            bvec = (x[n] == u)
            supp = bvec.sum()
            if ((supp > min_threshold) and (supp < max_threshold)):
                names += ['{%s:%s}' % (n, u)]
                udict[n] += [u]
                records += [bvec]

    for cardinality in range(2, max_cardinality + 1):
        for dlist in list(itertools.combinations(features, cardinality)):
            for values in list(itertools.product(*[udict[d] for d in dlist])):
                bvec = np.array([(x[d] == v) for (d, v) in zip(dlist, values)]).all(axis=0)
                supp = bvec.sum()
                if ((supp > min_threshold) and (supp < max_threshold)):
                    descr = '{%s}' % ','.join(['%s:%s' % (d, v) for (d, v) in zip(dlist, values)])
                    names.append(descr)
                    records.append(bvec)
                    if (verbose):
                        print descr

    rr = [' '.join(np.cast[str](np.cast[int](r))) for r in records]
    print len(rr), len(set(rr))

    print len(names), 'rules mined'
    print 'writing', fout
    f = open(fout, 'w')
    if (exclude_not):
        print 'excluding "not" rules'
        f.write('\n'.join(['%s %s' % (n, ' '.join(np.cast[str](np.cast[int](r))))
                            for (n, r) in zip(names, records) if ('not' not in n)]) + '\n')
        print len(names), 'rules'
    else:
        f.write('\n'.join(['%s %s' % (n, ' '.join(np.cast[str](np.cast[int](r))))
                        for (n, r) in zip(names, records)]) + '\n')
    f.close()

    print 'writing', flabel
    recs = [(x[label_name] == 0), (x[label_name] == 1)]
    f = open(flabel, 'w')
    f.write('\n'.join(['{%s:%s} %s' % (label_name, l, ' '.join(np.cast[str](np.cast[int](r))))
                       for (l, r) in zip(labels, recs)]) + '\n')
    f.close()

    if (minor):
        import minority
        fminor = os.path.join(din, '%s%s.minor' % (prefix, froot))
        print 'computing', fminor
        minority.compute_minority(froot='%s%s' % (prefix, froot), dir=din)

    return len(names)

def apply_binary(din='../data/compas', froot='compas', neg_names=None, prefix=''):

    ftrain = os.path.join(din, '%s%s_train.out' % (prefix, froot))
    ftrain_label = os.path.join(din, '%s%s_train.label' % (prefix, froot))
    ftest = os.path.join(din, '%s_test.csv' % froot)
    fout = os.path.join(din, '%s%s_test.out' % (prefix, froot))
    flabel = os.path.join(din, '%s%s_test.label' % (prefix, froot))

    x = tb.tabarray(SVfile=ftest)

    if (neg_names is None):
        neg_names = [n.replace(':', ':not-') for n in x.dtype.names]

    y = tb.tabarray(array=(1 - x.extract()), names=neg_names)

    names = list(x.dtype.names) + list(y.dtype.names)
    positive_label_name = x.dtype.names[-1]
    negative_label_name = y.dtype.names[-1]

    x = x.colstack(y)
    recs = [x[negative_label_name], x[positive_label_name]]

    x = x.extract()
    d = dict(zip(names, [x[:,i] for i in range(len(names))]))

    print 'reading rules from', ftrain
    rule_descr = [line.strip().split()[0] for line in open(ftrain, 'rU').read().strip().split('\n')]

    print 'extracting these rules from', ftest
    out = []
    for descr in rule_descr:
        rule = [clause for clause in descr.strip('{}').split(',')]
        bv = np.cast[str](np.cast[int](np.array([(d[name] == 1) for name in rule]).all(axis=0)))
        out.append('%s %s' % (descr, ' '.join(bv)))

    print 'writing', fout
    f = open(fout, 'w')
    f.write('\n'.join(out) + '\n')
    f.close()

    print 'writing', flabel
    labels = [line.split()[0] for line in open(ftrain_label, 'rU').read().strip().split('\n')]
    f = open(flabel, 'w')
    f.write('\n'.join(['{%s} %s' % (l, ' '.join(np.cast[str](np.cast[int](r))))
                       for (l, r) in zip(labels, recs)]) + '\n')
    f.close()

def apply_rules(din='../data/adult', froot='adult', labels=['<=50K', '>50K'], prefix='', numerical=False):

    ftrain = os.path.join(din, '%s%s_train.out' % (prefix, froot))
    ftest = os.path.join(din, '%s_test.csv' % froot)
    fout = os.path.join(din, '%s%s_test.out' % (prefix, froot))
    flabel = os.path.join(din, '%s%s_test.label' % (prefix, froot))

    x = tb.tabarray(SVfile=ftest)
    names = x.dtype.names
    label_name = names[-1]
    y = x[label_name]
    x = x.extract()
    d = dict(zip(names, [x[:,i] for i in range(len(names))]))

    print 'reading rules from', ftrain
    rule_descr = [line.strip().split()[0] for line in open(ftrain, 'rU').read().strip().split('\n')]

    print 'extracting these rules from', ftest
    out = []
    for descr in rule_descr:
        rule = [clause.split(':') for clause in descr.strip('{}').split(',')]
        if numerical:
            rule = [(name, int(value)) for (name, value) in rule]
        bv = np.cast[str](np.cast[int](np.array([(d[name] == value) for (name, value) in rule]).all(axis=0)))
        out.append('%s %s' % (descr, ' '.join(bv)))

    print 'writing', fout
    f = open(fout, 'w')
    f.write('\n'.join(out) + '\n')
    f.close()

    print 'writing', flabel
    recs = [(y == 0), (y == 1)]
    f = open(flabel, 'w')
    f.write('\n'.join(['{%s:%s} %s' % (label_name, l, ' '.join(np.cast[str](np.cast[int](r))))
                       for (l, r) in zip(labels, recs)]) + '\n')
    f.close()
