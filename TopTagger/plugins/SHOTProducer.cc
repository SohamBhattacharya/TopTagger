// -*- C++ -*-
//
// Package:    TopTagger/TopTagger
// Class:      SHOTProducer
// 
/**\class SHOTProducer SHOTProducer.cc TopTagger/TopTagger/plugins/SHOTProducer.cc

   Description: [one line class summary]

   Implementation:
   [Notes on implementation]
*/
//
// Original Author:  Nathaniel Pastika
//         Created:  Thu, 09 Nov 2017 21:29:56 GMT
//
//


// system include files
#include <memory>
#include <vector>
#include <string>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/stream/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/StreamID.h"

#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/Common/interface/Handle.h"

#include "FWCore/Utilities/interface/Exception.h"

#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"

//#include "FWCore/Framework/interface/EventSetup.h"
//#include "FWCore/Framework/interface/ESHandle.h"

#include "TLorentzVector.h"

#include "TopTagger/TopTagger/include/TopTagger.h"
#include "TopTagger/TopTagger/include/TopTaggerResults.h"
#include "TopTagger/TopTagger/include/TopObject.h"
#include "TopTagger/TopTagger/include/Constituent.h"
#include "TopTagger/TopTagger/include/TopObjLite.h"

//this include is necessary to handle exceptions thrown by the top tagger code                                                                                                                                                               
#include "TopTagger/CfgParser/include/TTException.h"

class SHOTProducer : public edm::stream::EDProducer<> 
{
public:
    explicit SHOTProducer(const edm::ParameterSet&);
    ~SHOTProducer();

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
    template<typename T>
    int findDrMatch(const T& lepton, const edm::Handle<std::vector<pat::Jet> >& jets)
    {
        double mindR = 999.9;
        int iJetMindR = -1;
        for(int i = 0; i < static_cast<int>(jets->size()); ++i)
        {
            const pat::Jet& jet = (*jets)[i];
            double dR = deltaR(lepton.p4(), jet.p4());
            if(dR < mindR)
            {
                mindR = dR;
                iJetMindR = i;
            }
        }
        if(mindR < leptonJetDr_) return iJetMindR;
        else                     return -1;
    }


    virtual void beginStream(edm::StreamID) override;
    virtual void produce(edm::Event&, const edm::EventSetup&) override;
    virtual void endStream() override;

    // helper function to calculate subjet qg input vars 
    void compute(const reco::Jet * jet, bool isReco, double& totalMult_, double& ptD_, double& axis1_, double& axis2_);

    // ----------member data ---------------------------
    edm::EDGetTokenT<std::vector<pat::Jet> > JetTok_;
    edm::EDGetTokenT<std::vector<pat::Muon> > muonTok_;
    edm::EDGetTokenT<std::vector<pat::Electron> > elecTok_;

    std::string elecIDFlag_, qgTaggerKey_, deepCSVBJetTags_, bTagKeyString_, taggerCfgFile_;
    double ak4ptCut_, leptonJetDr_;
    bool doLeptonCleaning_;
    reco::Muon::Selector muonIDFlag_;

    TopTagger tt;
};


SHOTProducer::SHOTProducer(const edm::ParameterSet& iConfig)
{
    //register vector of top objects 
    produces<std::vector<TopObjLite>>();
 
    //now do what ever other initialization is needed
    edm::InputTag jetSrc = iConfig.getParameter<edm::InputTag>("ak4JetSrc");

    edm::InputTag muonSrc = iConfig.getParameter<edm::InputTag>("muonSrc");
    edm::InputTag elecSrc = iConfig.getParameter<edm::InputTag>("elecSrc");

    ak4ptCut_ = iConfig.getParameter<double>("ak4ptCut");

    std::string muonIDFlagName = iConfig.getParameter<std::string>("muonIDFlag");
    if     (muonIDFlagName.compare("CutBasedIdLoose")  == 0) muonIDFlag_ = reco::Muon::CutBasedIdLoose;
    else if(muonIDFlagName.compare("CutBasedIdMedium") == 0) muonIDFlag_ = reco::Muon::CutBasedIdMedium;
    else if(muonIDFlagName.compare("CutBasedIdTight")  == 0) muonIDFlag_ = reco::Muon::CutBasedIdTight;

    elecIDFlag_  = iConfig.getParameter<std::string>("elecIDFlag");

    leptonJetDr_ = iConfig.getParameter<double>("leptonJetDr");

    doLeptonCleaning_  = iConfig.getParameter<bool>("doLeptonCleaning");

    qgTaggerKey_ = iConfig.getParameter<std::string>("qgTaggerKey");
    deepCSVBJetTags_ = iConfig.getParameter<std::string>("deepCSVBJetTags");
    bTagKeyString_ = iConfig.getParameter<std::string>("bTagKeyString");

    taggerCfgFile_ = iConfig.getParameter<std::string>("taggerCfgFile");

    JetTok_ = consumes<std::vector<pat::Jet> >(jetSrc);

    muonTok_ = consumes<std::vector<pat::Muon>>(muonSrc);
    elecTok_ = consumes<std::vector<pat::Electron>>(elecSrc);

    //configure the top tagger 
    try
    {
        tt.setCfgFile(taggerCfgFile_);
    }
    catch(const TTException& e)
    {
        //Convert the TTException into a cms::Exception
        throw cms::Exception(e.getFileName() + ":" + std::to_string(e.getLineNumber()) + ", in function \"" + e.getFunctionName() + "\" -- " + e.getMessage());
    }
}


SHOTProducer::~SHOTProducer()
{
 
}


//
// member functions
//

// ------------ method called to produce the data  ------------
void SHOTProducer::produce(edm::Event& iEvent, const edm::EventSetup& iSetup)
{
    using namespace edm;

    //Get jet collection 
    edm::Handle<std::vector<pat::Jet> > jets;
    iEvent.getByToken(JetTok_, jets);

    std::set<int> jetToClean;
    if(doLeptonCleaning_)
    {

        //Get lepton collections 
        edm::Handle<std::vector<pat::Muon> > muons;
        iEvent.getByToken(muonTok_, muons);
        edm::Handle<std::vector<pat::Electron> > elecs;
        iEvent.getByToken(elecTok_, elecs);

        //remove leptons that match to jets with a dR cone 
        for(const pat::Muon& muon : *muons)
        {
            if(muon.passed(muonIDFlag_))
            {
                int matchIndex = findDrMatch(muon, jets);
                if(matchIndex >= 0) jetToClean.insert(matchIndex);
            }
        }
        for(const pat::Electron& elec : *elecs)
        {
            if(elec.userInt(elecIDFlag_))
            {
                int matchIndex = findDrMatch(elec, jets);
                if(matchIndex >= 0) jetToClean.insert(matchIndex);
            }
        }
    }

    //container holding input jet info for top tagger
    std::vector<Constituent> constituents;

    //initialize iJet such that it is incrememted to 0 upon start of loop
    for(int iJet = 0; iJet < static_cast<int>(jets->size()); ++iJet)
    {
        const pat::Jet& jet = (*jets)[iJet];

        //Apply pt cut on jets 
        if(jet.pt() < ak4ptCut_) continue;

        //Apply lepton cleaning
        if(doLeptonCleaning_ && jetToClean.count(iJet)) continue;

        TLorentzVector perJetLVec(jet.p4().X(), jet.p4().Y(), jet.p4().Z(), jet.p4().T());

        double qgPtD = jet.userFloat(qgTaggerKey_+":ptD");
        double qgAxis1 = jet.userFloat(qgTaggerKey_+":axis1");
        double qgAxis2 = jet.userFloat(qgTaggerKey_+":axis2");
        double qgMult = static_cast<double>(jet.userInt(qgTaggerKey_+":mult"));
        double deepCSVb = jet.bDiscriminator((deepCSVBJetTags_+":probb").c_str());
        double deepCSVc = jet.bDiscriminator((deepCSVBJetTags_+":probc").c_str());
        double deepCSVl = jet.bDiscriminator((deepCSVBJetTags_+":probudsg").c_str());
        double deepCSVbb = jet.bDiscriminator((deepCSVBJetTags_+":probbb").c_str());
        double deepCSVcc = jet.bDiscriminator((deepCSVBJetTags_+":probcc").c_str());
        double btag = jet.bDiscriminator(bTagKeyString_.c_str());
        double chargedHadronEnergyFraction = jet.chargedHadronEnergyFraction();
        double neutralHadronEnergyFraction = jet.neutralHadronEnergyFraction();
        double chargedEmEnergyFraction = jet.chargedEmEnergyFraction();
        double neutralEmEnergyFraction = jet.neutralEmEnergyFraction();
        double muonEnergyFraction = jet.muonEnergyFraction();
        double photonEnergyFraction = jet.photonEnergyFraction();
        double electronEnergyFraction = jet.electronEnergyFraction();
        double recoJetsHFHadronEnergyFraction = jet.HFHadronEnergyFraction();
        double recoJetsHFEMEnergyFraction = jet.HFEMEnergyFraction();
        double chargedHadronMultiplicity = jet.chargedHadronMultiplicity();
        double neutralHadronMultiplicity = jet.neutralHadronMultiplicity();
        double photonMultiplicity = jet.photonMultiplicity();
        double electronMultiplicity = jet.electronMultiplicity();
        double muonMultiplicity = jet.muonMultiplicity();

        constituents.emplace_back(perJetLVec, btag, 0.0);
        constituents.back().setIndex(iJet);
        constituents.back().setExtraVar("qgMult"                              , qgMult);
        constituents.back().setExtraVar("qgPtD"                               , qgPtD);
        constituents.back().setExtraVar("qgAxis1"                             , qgAxis1);
        constituents.back().setExtraVar("qgAxis2"                             , qgAxis2);
        constituents.back().setExtraVar("recoJetschargedHadronEnergyFraction" , chargedHadronEnergyFraction);
        constituents.back().setExtraVar("recoJetschargedEmEnergyFraction"     , chargedEmEnergyFraction);
        constituents.back().setExtraVar("recoJetsneutralEmEnergyFraction"     , neutralEmEnergyFraction);
        constituents.back().setExtraVar("recoJetsmuonEnergyFraction"          , muonEnergyFraction);
        constituents.back().setExtraVar("recoJetsHFHadronEnergyFraction"      , recoJetsHFHadronEnergyFraction);
        constituents.back().setExtraVar("recoJetsHFEMEnergyFraction"          , recoJetsHFEMEnergyFraction);
        constituents.back().setExtraVar("recoJetsneutralEnergyFraction"       , neutralHadronEnergyFraction);
        constituents.back().setExtraVar("PhotonEnergyFraction"                , photonEnergyFraction);
        constituents.back().setExtraVar("ElectronEnergyFraction"              , electronEnergyFraction);
        constituents.back().setExtraVar("ChargedHadronMultiplicity"           , chargedHadronMultiplicity);
        constituents.back().setExtraVar("NeutralHadronMultiplicity"           , neutralHadronMultiplicity);
        constituents.back().setExtraVar("PhotonMultiplicity"                  , photonMultiplicity);
        constituents.back().setExtraVar("ElectronMultiplicity"                , electronMultiplicity);
        constituents.back().setExtraVar("MuonMultiplicity"                    , muonMultiplicity);
        constituents.back().setExtraVar("DeepCSVb"                            , deepCSVb);
        constituents.back().setExtraVar("DeepCSVc"                            , deepCSVc);
        constituents.back().setExtraVar("DeepCSVl"                            , deepCSVl);
        constituents.back().setExtraVar("DeepCSVbb"                           , deepCSVbb);
        constituents.back().setExtraVar("DeepCSVcc"                           , deepCSVcc);
    }

    //run top tagger
    try
    {
        tt.runTagger(std::move(constituents));
    }
    catch(const TTException& e)
    {
        //Convert the TTException into a cms::Exception
        throw cms::Exception(e.getFileName() + ":" + std::to_string(e.getLineNumber()) + ", in function \"" + e.getFunctionName() + "\" -- " + e.getMessage());
    }

    //retrieve the top tagger results object
    const TopTaggerResults& ttr = tt.getResults();
    
    //get reconstructed top
    const std::vector<TopObject*>& tops = ttr.getTops();

    //Translate TopObject to TopObjLite and save to event
    std::unique_ptr<std::vector<TopObjLite>> liteTops(new std::vector<TopObjLite>());
    for(auto const * const top : tops) liteTops->emplace_back(*top);
    iEvent.put(std::move(liteTops));
}

// ------------ method called once each stream before processing any runs, lumis or events  ------------
void SHOTProducer::beginStream(edm::StreamID)
{
}

// ------------ method called once each stream after processing all runs, lumis and events  ------------
void SHOTProducer::endStream() 
{
}
 
// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void SHOTProducer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    //The following says we do not know what parameters are allowed so do no validation
    // Please change this to state exactly what you do use, even if it is no parameters
    edm::ParameterSetDescription desc;
    desc.setUnknown();
    descriptions.addDefault(desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(SHOTProducer);
