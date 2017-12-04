#include "TopTagger/TopTagger/include/TTMTMVA.h"

#include "TopTagger/TopTagger/include/TopTaggerUtilities.h"
#include "TopTagger/TopTagger/include/TopObject.h"
#include "TopTagger/TopTagger/include/TopTaggerResults.h"
#include "TopTagger/CfgParser/include/Context.hh"
#include "TopTagger/CfgParser/include/CfgDocument.hh"
#include "TopTagger/CfgParser/include/TTException.h"

void TTMTMVA::getParameters(const cfg::CfgDocument* cfgDoc, const std::string& localContextName)
{
#ifdef SHOTTOPTAGGER_DO_TMVA
    //Construct contexts
    cfg::Context commonCxt("Common");
    cfg::Context localCxt(localContextName);

    discriminator_ = cfgDoc->get("discCut",       localCxt, -999.9);
    modelFile_     = cfgDoc->get("modelFile",     localCxt, "");
    modelName_     = cfgDoc->get("modelName",     localCxt, "");
    NConstituents_ = cfgDoc->get("NConstituents", localCxt, 3);
    filter_        = cfgDoc->get("filter",        localCxt, false);

    int iVar = 0;
    bool keepLooping;
    do
    {
        keepLooping = false;

        //Get variable name
        std::string varName = cfgDoc->get("mvaVar", iVar, localCxt, "");

        //if it is a non empty string save in vector
        if(varName.size() > 0)
        {
            keepLooping = true;

            vars_.push_back(varName);
        }
        ++iVar;
    }
    while(keepLooping);

    //create TMVA reader
    reader_.reset(new TMVA::Reader( "!Color:!Silent" ));
    if(reader_ == nullptr)
    {
        //Throw if this is an invalid pointer
        THROW_TTEXCEPTION("TMVA reader creation failed!!!");
    }

    //load variables
    varMap_.resize(vars_.size());
    if(NConstituents_ == 1)
    {
        varCalculator_.reset(new ttUtility::BDTMonojetInputCalculator());
    }
    else if(NConstituents_ == 2)
    {
        varCalculator_.reset(new ttUtility::BDTDijetInputCalculator());
    }
    varCalculator_->mapVars(vars_, varMap_.data());

    //load variables into reader
    for(int i = 0; i < vars_.size(); ++i)
    {
        reader_->AddVariable(vars_[i].c_str(), &varMap_[i]);
    }

    //load model file into reader
    auto* imethod = reader_->BookMVA( modelName_.c_str(), modelFile_.c_str() );
    if(imethod == nullptr)
    {
        //Throw if this is an invalid pointer
        THROW_TTEXCEPTION("TMVA reader could not load model named \"" + modelName_ + "\" from file \"" + modelFile_ + "\"!!!");        
    }

#else
    THROW_TTEXCEPTION("Top tagger was not compiled with support for TMVA!!!!"); 
#endif
}

void TTMTMVA::run(TopTaggerResults& ttResults)
{
#ifdef SHOTTOPTAGGER_DO_TMVA
    //Get the list of top candidates as generated by the clustering algo
    std::vector<TopObject>& topCandidates = ttResults.getTopCandidates();
    //Get the list of final tops into which we will stick candidates
    std::vector<TopObject*>& tops = ttResults.getTops();

    for(auto& topCand : topCandidates)
    {
        //Prepare data from top candidate and calculate discriminators 
        if(varCalculator_->calculateVars(topCand))
        {
            //predict value
            double discriminator = reader_->EvaluateMVA(modelName_);
            topCand.setDiscriminator(discriminator);

            //place in final top list if it passes the threshold
            if(filter_)
            {
                if(discriminator <= discriminator_)
                {
                    auto iTop = std::find(tops.begin(), tops.end(), &topCand);
                    if(iTop != tops.end()) tops.erase(iTop);
                }
            }
            else
            {
                if(discriminator > discriminator_)
                {
                    tops.push_back(&topCand);
                }
            }
        }
    }
#endif
}
