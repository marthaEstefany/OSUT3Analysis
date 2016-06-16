#include "OSUT3Analysis/Collections/interface/Tau.h"

#if IS_VALID(taus)

osu::Tau::Tau ()
{
}

osu::Tau::Tau (const TYPE(taus) &tau) :
  GenMatchable         (tau),
  metMinusOnePt_       (INVALID_VALUE),
  metMinusOnePx_       (INVALID_VALUE),
  metMinusOnePy_       (INVALID_VALUE),
  metMinusOnePhi_      (INVALID_VALUE),
  metNoMuMinusOnePt_   (INVALID_VALUE),
  metNoMuMinusOnePx_   (INVALID_VALUE),
  metNoMuMinusOnePy_   (INVALID_VALUE),
  metNoMuMinusOnePhi_  (INVALID_VALUE)
{
}

osu::Tau::Tau (const TYPE(taus) &tau, const edm::Handle<vector<osu::Mcparticle> > &particles) :
  GenMatchable         (tau,             particles),
  metMinusOnePt_       (INVALID_VALUE),
  metMinusOnePx_       (INVALID_VALUE),
  metMinusOnePy_       (INVALID_VALUE),
  metMinusOnePhi_      (INVALID_VALUE),
  metNoMuMinusOnePt_   (INVALID_VALUE),
  metNoMuMinusOnePx_   (INVALID_VALUE),
  metNoMuMinusOnePy_   (INVALID_VALUE),
  metNoMuMinusOnePhi_  (INVALID_VALUE)
{
}

osu::Tau::Tau (const TYPE(taus) &tau, const edm::Handle<vector<osu::Mcparticle> > &particles, const edm::ParameterSet &cfg) :
  GenMatchable         (tau,             particles,  cfg),
  metMinusOnePt_       (INVALID_VALUE),
  metMinusOnePx_       (INVALID_VALUE),
  metMinusOnePy_       (INVALID_VALUE),
  metMinusOnePhi_      (INVALID_VALUE),
  metNoMuMinusOnePt_   (INVALID_VALUE),
  metNoMuMinusOnePx_   (INVALID_VALUE),
  metNoMuMinusOnePy_   (INVALID_VALUE),
  metNoMuMinusOnePhi_  (INVALID_VALUE)
{
}

osu::Tau::Tau (const TYPE(taus) &tau, const edm::Handle<vector<osu::Mcparticle> > &particles, const edm::ParameterSet &cfg, const osu::Met &met) :
  GenMatchable         (tau,             particles,  cfg),
  metMinusOnePt_       (INVALID_VALUE),
  metMinusOnePx_       (INVALID_VALUE),
  metMinusOnePy_       (INVALID_VALUE),
  metMinusOnePhi_      (INVALID_VALUE),
  metNoMuMinusOnePt_   (INVALID_VALUE),
  metNoMuMinusOnePx_   (INVALID_VALUE),
  metNoMuMinusOnePy_   (INVALID_VALUE),
  metNoMuMinusOnePhi_  (INVALID_VALUE)
{
  TVector2 p (met.px () + this->px (), met.py () + this->py ()),
           pNoMu (met.noMuPx () + this->px (), met.noMuPy () + this->py ());

  metMinusOnePt_ = p.Mod ();
  metMinusOnePx_ = p.Px ();
  metMinusOnePy_ = p.Py ();
  metMinusOnePhi_ = p.Phi ();

  metNoMuMinusOnePt_ = pNoMu.Mod ();
  metNoMuMinusOnePx_ = pNoMu.Px ();
  metNoMuMinusOnePy_ = pNoMu.Py ();
  metNoMuMinusOnePhi_ = pNoMu.Phi ();
}

osu::Tau::~Tau ()
{
}

// Tau ID and isolation discriminators are documented here:
// https://twiki.cern.ch/twiki/bin/view/CMS/TauIDRecommendation13TeV
// The names of the discriminators are out-of-date though, with the current
// names for 76X samples found here:
// https://github.com/cms-sw/cmssw/blob/CMSSW_7_6_X/PhysicsTools/PatAlgos/python/producersLayer1/tauProducer_cfi.py#L62-L106

const bool
osu::Tau::passesDecayModeReconstruction () const
{
  return (this->tauID ("decayModeFinding") > 0.5);
}

const bool
osu::Tau::passesLightFlavorRejection () const
{
  return (this->tauID ("againstElectronLooseMVA5") > 0.5 && this->tauID ("againstMuonLoose3") > 0.5);
}

const bool
osu::Tau::passesLooseCombinedIsolation () const
{
  return (this->tauID ("byLooseCombinedIsolationDeltaBetaCorr3Hits") > 0.5);
}

const bool
osu::Tau::passesMediumCombinedIsolation () const
{
  return (this->tauID ("byMediumCombinedIsolationDeltaBetaCorr3Hits") > 0.5);
}

const bool
osu::Tau::passesTightCombinedIsolation () const
{
  return (this->tauID ("byTightCombinedIsolationDeltaBetaCorr3Hits") > 0.5);
}

const double
osu::Tau::metMinusOnePt () const
{
  return metMinusOnePt_;
}

const double
osu::Tau::metMinusOnePx () const
{
  return metMinusOnePx_;
}

const double
osu::Tau::metMinusOnePy () const
{
  return metMinusOnePy_;
}

const double
osu::Tau::metMinusOnePhi () const
{
  return metMinusOnePhi_;
}

const double
osu::Tau::metNoMuMinusOnePt () const
{
  return metNoMuMinusOnePt_;
}

const double
osu::Tau::metNoMuMinusOnePx () const
{
  return metNoMuMinusOnePx_;
}

const double
osu::Tau::metNoMuMinusOnePy () const
{
  return metNoMuMinusOnePy_;
}

const double
osu::Tau::metNoMuMinusOnePhi () const
{
  return metNoMuMinusOnePhi_;
}

#endif
