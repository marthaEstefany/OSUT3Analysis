#include "OSUT3Analysis/AnaTools/interface/ObjectSelector.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#if IS_VALID(bxlumis)
typedef ObjectSelector<osu::Bxlumi, TYPE(bxlumis)> BxlumiObjectSelector;
  DEFINE_FWK_MODULE(BxlumiObjectSelector);
#endif
