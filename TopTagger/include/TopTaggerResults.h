#ifndef TOPTAGGERRESULTS_H
#define TOPTAGGERRESULTS_H

#include "TopTagger/TopTagger/include/TopObject.h"
#include "TopTagger/TopTagger/include/Constituent.h"

#include <vector>
#include <set>

class TopTaggerResults
{
private:
    //List of input objects which can be included in a resolved top
    //Will never be modified or changed by modules 
    const std::vector<Constituent> * constituents_;

    //List of jets used to construct final tops, needed for Rsys
    std::set<Constituent const *> usedConstituents_;

    //List of top candidates, will be manipulated by modules
    std::vector<TopObject> topCandidates_;

    //List of final top objects, will be filled out by the modules
    std::vector<TopObject*> tops_;

    //The remaining system container
    TopObject rsys_;

public:

    TopTaggerResults(const std::vector<Constituent>& constituents) : constituents_(&constituents) {}

    ~TopTaggerResults() {}

    //Setters
    void setConstituents(const std::vector<Constituent>& constituents) {constituents_ = &constituents;}

    //non-const getters (for modules)
    decltype(topCandidates_)& getTopCandidates() { return topCandidates_; }
    decltype(usedConstituents_)& getUsedConstituents() { return usedConstituents_; }
    decltype(tops_)& getTops() { return tops_; }
    decltype(rsys_)& getRsys() { return rsys_; }
    
    //const getters for public consumption
    const std::vector<Constituent>& getConstituents() const { return *constituents_; }
    const decltype(usedConstituents_)& getUsedConstituents() const { return usedConstituents_; }
    const decltype(topCandidates_)& getTopCandidates() const { return topCandidates_; }
    const decltype(tops_)& getTops() const { return tops_; }
    const decltype(rsys_)& getRsys() const { return rsys_; }
};

#endif
