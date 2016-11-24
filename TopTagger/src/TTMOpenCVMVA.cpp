#include "TopTagger/TopTagger/include/TTMOpenCVMVA.h"

#include "TopTagger/TopTagger/include/TopTaggerUtilities.h"
#include "TopTagger/TopTagger/include/TopObject.h"
#include "TopTagger/TopTagger/include/TopTaggerResults.h"
#include "TopTagger/CfgParser/include/Context.hh"
#include "TopTagger/CfgParser/include/CfgDocument.hh"

#include <iostream>

void TTMOpenCVMVA::getParameters(const cfg::CfgDocument* cfgDoc, const std::string& localContextName)
{
    //Construct contexts
    cfg::Context commonCxt("Common");
    cfg::Context localCxt(localContextName);

    discriminator_ = cfgDoc->get("discCut", localCxt, -999.9);
    modelFile_ = cfgDoc->get("modelFile", localCxt, "");

    treePtr_ = cv::ml::RTrees::load<cv::ml::RTrees>(modelFile_);

    vars_ = ttUtility::getMVAVars();
}

void TTMOpenCVMVA::run(TopTaggerResults& ttResults)
{
    //Get the list of top candidates as generated by the clustering algo
    std::vector<TopObject>& topCandidates = ttResults.getTopCandidates();
    //Get the list of final tops into which we will stick candidates
    std::vector<TopObject*>& tops = ttResults.getTops();

    for(auto& topCand : topCandidates)
    {
        //We only want to apply the MVA algorithm to triplet tops
        if(topCand.getNConstituents() == 3)
        {
            //Construct data matrix for prediction
            std::map<std::string, double> varMap = ttUtility::createMVAInputs(topCand);

            cv::Mat inputData(vars_.size(), 1, 5); //the last 5 is for CV_32F var type
            for(unsigned int i = 0; i < vars_.size(); ++i)
            {
                inputData.at<float>(i, 0) = varMap[vars_[i]];
            }

            //predict value
            double discriminator = treePtr_->predict(inputData);
            topCand.setDiscriminator(discriminator);

            //place in final top list if it passes the threshold
            if(discriminator > discriminator_)
            {
                tops.push_back(&topCand);
            }
        }
    }
}
