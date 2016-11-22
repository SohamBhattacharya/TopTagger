#include "TopTagger/TopTagger/include/TTMOverlapResolution.h"

#include "TopTagger/TopTagger/include/TopTaggerResults.h"
#include "TopTagger/CfgParser/include/Context.hh"
#include "TopTagger/CfgParser/include/CfgDocument.hh"

#include <set>
#include <algorithm>
#include <vector>
#include <cmath>

#include <iostream>

void TTMOverlapResolution::getParameters(const cfg::CfgDocument* cfgDoc)
{
    //Construct contexts
    cfg::Context commonCxt("Common");
    cfg::Context localCxt("TTMOverlapResolution");

    mt_         = cfgDoc->get("mt",         commonCxt, -999.9);

    maxTopEta_  = cfgDoc->get("maxTopEta",  localCxt,  -999.9);
    sortMethod_ = cfgDoc->get("sortMethod", localCxt,  "topMass");
}

void TTMOverlapResolution::run(TopTaggerResults& ttResults)
{
    //Get vector of final tops to prune
    std::vector<TopObject*>& tops = ttResults.getTops();

    //This container will kep trach of which jets have been included in final tops
    //This is not the ideal method to track this, maybe have candidates?
    std::set<Constituent const *>& usedJets = ttResults.getUsedConstituents();

    //Sort the top vector for overlap resolution
    if(sortMethod_.compare("topMass") == 0)
    {
        std::sort(tops.begin(), tops.end(), [this](TopObject* t1, TopObject* t2){ return fabs(t1->p().M() - this->mt_) < fabs(t2->p().M() - this->mt_); } );
    }
    else if(sortMethod_.compare("topPt") == 0)
    {
        std::sort(tops.begin(), tops.end(), [this](TopObject* t1, TopObject* t2){ return t1->p().Pt() > t2->p().Pt(); } );
    }
    else if(sortMethod_.compare("mvaDisc") == 0)
    {
        std::sort(tops.begin(), tops.end(), [this](TopObject* t1, TopObject* t2){ return t1->getDiscriminator() > t2->getDiscriminator(); } );
    }
    else if(sortMethod_.compare("mvaDiscWithb") == 0)
    {
        auto sortFunc = [this](TopObject* t1, TopObject* t2)
        {
            int nb1 = t1->getNBConstituents(0.800);
            int nb2 = t2->getNBConstituents(0.800);
            if     (nb1 == 1 && nb2 == 0) return true;
            else if(nb1 == 0 && nb2 == 1) return false;
            else if(nb1 < nb2)            return true;
            else if(nb1 > nb2)            return false;

            if(t1->getDiscriminator() > t2->getDiscriminator()) return true; 

            return false;
        };
        std::sort(tops.begin(), tops.end(), sortFunc);
    }

    for(auto iTop = tops.begin(); iTop != tops.end();)
    {
        //Get constituent jets for this top
        const std::vector<Constituent const *>& jets = (*iTop)->getConstituents();

        //Requirement on top eta here
        bool passTopEta = (fabs((*iTop)->p().Eta()) < maxTopEta_);

        //Check if the candidates have been used in another top with better top mass
        bool overlaps = false;
        for(const auto& jet : jets)
        {
            if(usedJets.count(jet))
            {
                overlaps = true;
                break;
            }
        }

        //Prune top from final top collection if it fails the following requirements
        if(overlaps || !passTopEta)
        {
            //This is inefficient and dumb
            iTop = tops.erase(iTop);
            continue;
        }

        //If the candidate survived, it must be a good top!!!

        //Add the good tops constituents to the set tracking which constituents have been used
        for(const auto& jet : jets)
        {
            usedJets.insert(jet);
        }

        ++iTop;
    }
}
