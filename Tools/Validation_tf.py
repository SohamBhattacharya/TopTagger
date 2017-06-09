import sys
import pandas as pd
import numpy as np
import math
from MVAcommon_tf import *
import optparse
import matplotlib.pyplot as plt
import pickle

parser = optparse.OptionParser("usage: %prog [options]\n")

#parser.add_option ('-o', "--opencv", dest='opencv', action='store_true', help="Run using opencv RTrees")
#parser.add_option ('-n', "--noRoc", dest='noROC', action='store_true', help="Do not calculate ROC to save time")
parser.add_option ('-d', "--disc", dest='discCut', action='store', default=0.6, help="Discriminator cut")
parser.add_option ('-k', "--sklrf", dest='sklrf', action='store_true', help="Use skl random forest instead of tensorflow")
parser.add_option ('-x', "--xgboost", dest='xgboost', action='store_true', help="Run using xgboost")

options, args = parser.parse_args()

#disc cut
discCut = options.discCut

print "RETRIEVING MODEL FILE"

if options.sklrf:
    from sklearn.ensemble import RandomForestClassifier
    
    fileTraining = open("TrainingOutput.pkl",'r')
    clf1 = pickle.load(fileTraining)
    fileTraining.close()

elif options.xgboost:
    import xgboost as xgb

    bst = xgb.Booster(model_file="./TrainingModel.xgb") # load data

else:
    import tensorflow as tf

    #Get training output
    saver = tf.train.import_meta_graph('models/model.ckpt.meta')
    sess = tf.Session()
    # To initialize values with saved data
    saver.restore(sess, './models/model.ckpt')
    # Restrieve useful variables
    trainInfo = tf.get_collection('TrainInfo')
    x = trainInfo[0]
    y_train = trainInfo[1]


print "PROCESSING TTBAR VALIDATION DATA"

varsname = DataGetter().getList()

dataTTbar = pd.read_pickle("trainingTuple_division_1_TTbarSingleLep_validation.pkl.gz")
numDataTTbar = dataTTbar._get_numeric_data()
numDataTTbar[numDataTTbar < 0.0] = 0.0

#print list(dataTTbar.columns.values)

#Apply baseline cuts
dataTTbar = dataTTbar[dataTTbar.Njet >= 4]

print "CALCULATING TTBAR DISCRIMINATORS"

if options.sklrf:
    dataTTbarAns = clf1.predict_proba(dataTTbar.as_matrix(varsname))[:,1]
elif options.xgboost:
    xgData = xgb.DMatrix(dataTTbar.as_matrix(varsname))
    dataTTbarAns = bst.predict(xgData)
else:
    dataTTbarAns = sess.run(y_train, feed_dict={x: dataTTbar.as_matrix(varsname)})[:,0]

print "CREATING HISTOGRAMS"

#Discriminator plot

inputLabels = dataTTbar.as_matrix(["genConstiuentMatchesVec", "genTopMatchesVec"])
genMatches = (inputLabels[:,0] == 3) & (inputLabels[:,1] == 1)

plt.clf()
plt.hist(dataTTbarAns[genMatches == 1], weights=dataTTbar["sampleWgt"][genMatches == 1], bins=50, normed=True, label="Gen Matched",     fill=False, histtype='step', edgecolor="red")
plt.hist(dataTTbarAns[genMatches != 1], weights=dataTTbar["sampleWgt"][genMatches != 1], bins=50, normed=True, label="Not gen matched", fill=False, histtype='step', edgecolor="blue")
plt.legend(loc='upper right')
plt.xlabel("Discriminator")
plt.ylabel("Normalized events")
plt.savefig("discriminator.png")
plt.close()

#plot efficiency

#ptNum, ptNumBins = numpy.histogram(dataTTbar[]["cand_pt"], bins=numpy.hstack([[0], numpy.linspace(50, 400, 36), numpy.linspace(450, 700, 6), [800, 1000]]), weights=npyInputSampleWgts[:,0])

#input variable histograms

genTopData = dataTTbar[genMatches == 1]
genBGData = dataTTbar[genMatches != 1]
recoTopData = dataTTbar[dataTTbarAns > discCut]
recoBGData = dataTTbar[dataTTbarAns < discCut]
minTTbar = dataTTbar.min()
maxTTbar = dataTTbar.max()

for var in varsname:
    plt.clf()
    bins = numpy.linspace(minTTbar[var], maxTTbar[var], 21)
    ax = recoTopData .hist(column=var, weights=recoTopData["sampleWgt"], bins=bins, grid=False, normed=True, fill=False, histtype='step',                     label="reco top")
    recoBGData       .hist(column=var, weights=recoBGData["sampleWgt"],  bins=bins, grid=False, normed=True, fill=False, histtype='step',                     label="reco bg", ax=ax)
    genTopData       .hist(column=var, weights=genTopData["sampleWgt"],  bins=bins, grid=False, normed=True, fill=False, histtype='step', linestyle="dotted", label="gen top", ax=ax)
    genBGData        .hist(column=var, weights=genBGData["sampleWgt"],   bins=bins, grid=False, normed=True, fill=False, histtype='step', linestyle="dotted", label="gen bkg", ax=ax)
    plt.legend()
    plt.xlabel(var)
    plt.ylabel("Normalized events")
    plt.savefig(var + ".png")
    plt.close()


#purity plots
#dataTTbar.genConstMatchGenPtVec

plt.clf()

ptBins = numpy.hstack([[0], numpy.linspace(50, 400, 15), numpy.linspace(450, 700, 6), [800, 1000]])
purityNum, _ = numpy.histogram(dataTTbar["cand_pt"][genMatches == 1], bins=ptBins, weights=dataTTbar["sampleWgt"][genMatches == 1])
purityDen,_  = numpy.histogram(dataTTbar["cand_pt"],                  bins=ptBins, weights=dataTTbar["sampleWgt"])

purity = purityNum/purityDen

plt.hist(ptBins[:-1], bins=ptBins, weights=purity, fill=False, histtype='step')
#plt.legend(loc='upper right')
plt.xlabel("pT [GeV]")
plt.ylabel("Purity")
plt.savefig("purity.png")
plt.close()

plt.clf()

discBins = numpy.linspace(0, 1, 21)
purityDiscNum, _ = numpy.histogram(dataTTbarAns[genMatches == 1], bins=discBins, weights=dataTTbar["sampleWgt"][genMatches == 1])
purityDiscDen,_  = numpy.histogram(dataTTbarAns,                  bins=discBins, weights=dataTTbar["sampleWgt"])

purityDisc = purityDiscNum/purityDiscDen

plt.hist(discBins[:-1], bins=discBins, weights=purityDisc, fill=False, histtype='step')
#plt.legend(loc='upper right')
plt.xlabel("Discriminator")
plt.ylabel("Purity")
plt.savefig("purity_disc.png")
plt.close()

print "PROCESSING ZNUNU VALIDATION DATA"

dataZnunu = pd.read_pickle("trainingTuple_division_1_ZJetsToNuNu_validation.pkl.gz")
numDataZnunu = dataZnunu._get_numeric_data()
numDataZnunu[numDataZnunu < 0.0] = 0.0

print "CALCULATING ZNUNU DISCRIMINATORS"

if options.sklrf:
    dataZnunuAns = clf1.predict_proba(dataZnunu.as_matrix(varsname))[:,1]
elif options.xgboost:
    xgData = xgb.DMatrix(dataZnunu.as_matrix(varsname))
    dataZnunuAns = bst.predict(xgData)
else:
    dataZnunuAns = sess.run(y_train, feed_dict={x: dataZnunu.as_matrix(varsname)})[:,0]

#calculate fake rate 

plt.clf()

metBins = numpy.linspace(0, 1000, 21)
frMETNum, _ = numpy.histogram(dataZnunu[dataZnunuAns > discCut]["MET"].ix[:,0], bins=metBins, weights=dataZnunu[dataZnunuAns > discCut]["sampleWgt"].ix[:,0])
frMETDen,_  = numpy.histogram(dataZnunu["MET"].ix[:,0],                         bins=metBins, weights=dataZnunu["sampleWgt"].ix[:,0])

frMETNum[frMETDen < 1e-10] = 0.0
frMETDen[frMETDen < 1e-10] = 1.0
frMET = frMETNum/frMETDen

plt.hist(metBins[:-1], bins=metBins, weights=frMET, fill=False, histtype='step')
#plt.legend(loc='upper right')
plt.xlabel("MET [GeV]")
plt.ylabel("Fake rate")
plt.savefig("fakerate_met.png")
plt.close()

plt.clf()

njBins = numpy.linspace(0, 20, 21)
frNjNum, _ = numpy.histogram(dataZnunu[dataZnunuAns > discCut]["Njet"].ix[:,0], bins=njBins, weights=dataZnunu[dataZnunuAns > discCut]["sampleWgt"].ix[:,0])
frNjDen,_  = numpy.histogram(dataZnunu["Njet"].ix[:,0],                         bins=njBins, weights=dataZnunu["sampleWgt"].ix[:,0])

frNjNum[frNjDen < 1e-10] = 0.0
frNjDen[frNjDen < 1e-10] = 1.0
frNj = frNjNum/frNjDen

plt.hist(njBins[:-1], bins=njBins, weights=frNj, fill=False, histtype='step')
#plt.legend(loc='upper right')
plt.xlabel("N jets")
plt.ylabel("Fake rate")
plt.savefig("fakerate_njets.png")
plt.close()

print "CALCULATING ROC CURVES"

cuts = np.hstack([np.arange(0.0, 0.05, 0.005), np.arange(0.05, 0.95, 0.01), np.arange(0.95, 1.00, 0.005)])

FPR = []
TPR = []
FPRZ = []

FPRPtCut = []
TPRPtCut = []
FPRZPtCut = []

evtwgt = dataTTbar["sampleWgt"]
evtwgtZnunu = dataZnunu["sampleWgt"]

NevtTPR = evtwgt[dataTTbar.genConstiuentMatchesVec==3].sum()
NevtFPR = evtwgt[dataTTbar.genConstiuentMatchesVec!=3].sum()
NevtZ = evtwgtZnunu.sum()

ptCut = 200

candPtTTbar = dataTTbar["cand_pt"]
cantPtZnunu = dataZnunu["cand_pt"]

NevtTPRPtCut = evtwgt[(dataTTbar.genConstiuentMatchesVec==3) & (candPtTTbar>ptCut)].sum()
NevtFPRPtCut = evtwgt[(dataTTbar.genConstiuentMatchesVec!=3) & (candPtTTbar>ptCut)].sum()
NevtZPtCut = evtwgtZnunu[cantPtZnunu > ptCut].sum()

for cut in cuts:
    FPR.append(  evtwgt[(dataTTbarAns > cut) & (dataTTbar.genConstiuentMatchesVec!=3)].sum() / NevtFPR    )
    TPR.append(  evtwgt[(dataTTbarAns > cut) & (dataTTbar.genConstiuentMatchesVec==3)].sum() / NevtTPR    )
    FPRZ.append( evtwgtZnunu[(dataZnunuAns > cut)].sum() / NevtZ )

    FPRPtCut.append(  evtwgt[(dataTTbarAns > cut) & (dataTTbar.genConstiuentMatchesVec!=3) & (candPtTTbar > ptCut)].sum() / NevtFPRPtCut    )
    TPRPtCut.append(  evtwgt[(dataTTbarAns > cut) & (dataTTbar.genConstiuentMatchesVec==3) & (candPtTTbar > ptCut)].sum() / NevtTPRPtCut    )
    FPRZPtCut.append( evtwgtZnunu[(dataZnunuAns > cut) & (cantPtZnunu > ptCut)].sum() / NevtZPtCut )

#Dump roc to file
fileObject = open("roc.pkl",'wb')
pickle.dump(TPR, fileObject)
pickle.dump(FPR, fileObject)
pickle.dump(FPRZ, fileObject)
pickle.dump(TPRPtCut, fileObject)
pickle.dump(FPRPtCut, fileObject)
pickle.dump(FPRZPtCut, fileObject)
fileObject.close()

plt.clf()
plt.plot(FPR,TPR)
plt.xlabel("FPR (ttbar)")
plt.ylabel("TPR (ttbar)")
plt.savefig("roc.png")
plt.close()

plt.clf()
plt.plot(FPRZ,TPR)
plt.xlabel("FPR (Znunu)")
plt.ylabel("TPR (ttbar)")
plt.savefig("rocZ.png")
plt.close()



print "VALIDATION DONE!"
