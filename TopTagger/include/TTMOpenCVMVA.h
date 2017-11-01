#ifndef TTMOPENCVMVA_H
#define TTMOPENCVMVA_H

#include "TopTagger/TopTagger/include/TTModule.h"

#include <string>
#include <vector>

#include "opencv/cv.h"
#include "opencv/ml.h"

class TTMOpenCVMVA : public TTModule
{
#ifdef SHOTTOPTAGGER_DO_OPENCV
private:
    double discriminator_;
    std::string modelFile_;
    double csvThreshold_;
    double bEtaCut_;
    int maxNbInTop_;

    //cv::Ptr is the opencv implementation of a smart pointer
    cv::Ptr<cv::ml::RTrees> treePtr_;
    std::vector<std::string> vars_;

public:
    void getParameters(const cfg::CfgDocument*, const std::string&);
    void run(TopTaggerResults&);
#endif
};
REGISTER_TTMODULE(TTMOpenCVMVA);

#endif
