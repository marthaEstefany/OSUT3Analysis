#include "TFile.h"
#include "TH2D.h"

#include "FWCore/ParameterSet/interface/FileInPath.h"

#include "OSUT3Analysis/Collections/plugins/OSUTrackProducer.h"

#include "Geometry/Records/interface/CaloGeometryRecord.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"

#if IS_VALID(tracks)

#include "OSUT3Analysis/AnaTools/interface/CommonUtils.h"

OSUTrackProducer::OSUTrackProducer (const edm::ParameterSet &cfg) :
  collections_ (cfg.getParameter<edm::ParameterSet> ("collections")),
  cfg_ (cfg)
{
  collection_ = collections_.getParameter<edm::InputTag> ("tracks");

  produces<vector<osu::Track> > (collection_.instance ());

  token_ = consumes<vector<TYPE(tracks)> > (collection_);
  mcparticleToken_ = consumes<vector<osu::Mcparticle> > (collections_.getParameter<edm::InputTag> ("mcparticles"));
  jetsToken_ = consumes<vector<TYPE(jets)> > (collections_.getParameter<edm::InputTag> ("jets"));

  const edm::ParameterSet &fiducialMaps = cfg.getParameter<edm::ParameterSet> ("fiducialMaps");
  const vector<edm::ParameterSet> &electronFiducialMaps = fiducialMaps.getParameter<vector<edm::ParameterSet> > ("electrons");
  const vector<edm::ParameterSet> &muonFiducialMaps = fiducialMaps.getParameter<vector<edm::ParameterSet> > ("muons");

  maskedEcalChannelStatusThreshold_ = cfg.getParameter<int> ("maskedEcalChannelStatusThreshold");
  outputBadEcalChannels_ = cfg.getParameter<bool> ("outputBadEcalChannels");

  EBRecHitsTag_    =  cfg.getParameter<edm::InputTag>  ("EBRecHits");
  EERecHitsTag_    =  cfg.getParameter<edm::InputTag>  ("EERecHits");
  HBHERecHitsTag_  =  cfg.getParameter<edm::InputTag>  ("HBHERecHits");

  EBRecHitsToken_    =  consumes<EBRecHitCollection>    (EBRecHitsTag_);
  EERecHitsToken_    =  consumes<EERecHitCollection>    (EERecHitsTag_);
  HBHERecHitsToken_  =  consumes<HBHERecHitCollection>  (HBHERecHitsTag_);

  gsfTracksToken_ = consumes<vector<reco::GsfTrack> > (cfg.getParameter<edm::InputTag> ("gsfTracks"));

#ifdef DISAPP_TRKS
  stringstream ss;
  for (const auto &electronFiducialMap : electronFiducialMaps)
    {
      const edm::FileInPath &histFile = electronFiducialMap.getParameter<edm::FileInPath> ("histFile");

      ss << "================================================================================" << endl;
      ss << "calculating electron veto regions in (eta, phi)..." << endl;
      ss << "extracting histograms from \"" << histFile.relativePath () << "\"..." << endl;
      ss << "--------------------------------------------------------------------------------" << endl;

      extractFiducialMap (electronFiducialMap, electronVetoList_, ss);

      ss << "================================================================================" << endl;
    }

  for (const auto &muonFiducialMap : muonFiducialMaps)
    {
      const edm::FileInPath &histFile = muonFiducialMap.getParameter<edm::FileInPath> ("histFile");

      ss << "================================================================================" << endl;
      ss << "calculating muon veto regions in (eta, phi)..." << endl;
      ss << "extracting histograms from \"" << histFile.relativePath () << "\"..." << endl;
      ss << "--------------------------------------------------------------------------------" << endl;

      extractFiducialMap (muonFiducialMap, muonVetoList_, ss);

      ss << "================================================================================" << endl;
    }

  sort (electronVetoList_.begin (), electronVetoList_.end (), [] (EtaPhi a, EtaPhi b) -> bool { return (a.eta < b.eta && a.phi < b.phi); });
  sort (muonVetoList_.begin (), muonVetoList_.end (), [] (EtaPhi a, EtaPhi b) -> bool { return (a.eta < b.eta && a.phi < b.phi); });

  ss << "================================================================================" << endl;
  ss << "electron veto regions in (eta, phi)" << endl;
  ss << "--------------------------------------------------------------------------------" << endl;
  for (const auto &etaPhi : electronVetoList_)
    ss << "(" << setw (10) << etaPhi.eta << "," << setw (10) << etaPhi.phi << ")" << endl;
  ss << "================================================================================" << endl;

  ss << "================================================================================" << endl;
  ss << "muon veto regions in (eta, phi)" << endl;
  ss << "--------------------------------------------------------------------------------" << endl;
  for (const auto &etaPhi : muonVetoList_)
    ss << "(" << setw (10) << etaPhi.eta << "," << setw (10) << etaPhi.phi << ")" << endl;
  ss << "================================================================================";
  edm::LogInfo ("OSUTrackProducer") << ss.str ();
#endif
}

OSUTrackProducer::~OSUTrackProducer ()
{
}

void
OSUTrackProducer::beginRun (const edm::Run &run, const edm::EventSetup& setup)
{
#ifdef DISAPP_TRKS
  envSet (setup);
  getChannelStatusMaps ();
#endif
}

void
OSUTrackProducer::produce (edm::Event &event, const edm::EventSetup &setup)
{
  edm::Handle<vector<TYPE(tracks)> > collection;
  if (!event.getByToken (token_, collection))
    {
      edm::LogWarning ("OSUTrackProducer") << "Track collection not found. Skipping production of osu::Track collection...";
      return;
    }

#ifdef DISAPP_TRKS
  edm::Handle<vector<TYPE(jets)> > jets;
  if (!event.getByToken (jetsToken_, jets))
    edm::LogWarning ("OSUTrackProducer") << "Jet collection not found.";

  edm::Handle<vector<osu::Mcparticle> > particles;
  event.getByToken (mcparticleToken_, particles);

  edm::Handle<EBRecHitCollection> EBRecHits;
  event.getByToken(EBRecHitsToken_, EBRecHits);
  if (!EBRecHits.isValid()) throw cms::Exception("FatalError") << "Unable to find EBRecHitCollection in the event!\n";
  edm::Handle<EERecHitCollection> EERecHits;
  event.getByToken(EERecHitsToken_, EERecHits);
  if (!EERecHits.isValid()) throw cms::Exception("FatalError") << "Unable to find EERecHitCollection in the event!\n";
  edm::Handle<HBHERecHitCollection> HBHERecHits;
  event.getByToken(HBHERecHitsToken_, HBHERecHits);
  if (!HBHERecHits.isValid()) throw cms::Exception("FatalError") << "Unable to find HBHERecHitCollection in the event!\n";

  edm::Handle<vector<reco::GsfTrack> > gsfTracks;
  event.getByToken (gsfTracksToken_, gsfTracks);

#endif

  pl_ = unique_ptr<vector<osu::Track> > (new vector<osu::Track> ());
  for (const auto &object : *collection)
    {
#ifdef DISAPP_TRKS
      pl_->emplace_back (object, particles, cfg_, gsfTracks, electronVetoList_, muonVetoList_, &EcalAllDeadChannelsValMap_, &EcalAllDeadChannelsBitMap_, !event.isRealData ());
      osu::Track &track = pl_->back ();
#else
      pl_->emplace_back (object);
#endif

#ifdef DISAPP_TRKS
      // Calculate the associated calorimeter energy for the disappearing tracks search.

      if (jets.isValid ())
        {
          double dRMinJet = 999;
          for (const auto &jet : *jets) {
            if (!(jet.pt() > 30))         continue;
            if (!(fabs(jet.eta()) < 4.5)) continue;
            if (!anatools::jetPassesTightLepVeto(jet)) continue;
            double dR = deltaR(track, jet);
            if (dR < dRMinJet) {
              dRMinJet = dR;
            }
          }

          track.set_dRMinJet(dRMinJet);
        }

      double eEM = 0;
      double dR = 0.5;
      for (EBRecHitCollection::const_iterator hit=EBRecHits->begin(); hit!=EBRecHits->end(); hit++) {
        if (insideCone(track, (*hit).detid(), dR)) {
          eEM += (*hit).energy();
          // cout << "       Added EB rec hit with (eta, phi) = "
          //      << getPosition((*hit).detid()).eta() << ", "
          //      << getPosition((*hit).detid()).phi() << endl;
        }
      }
      for (EERecHitCollection::const_iterator hit=EERecHits->begin(); hit!=EERecHits->end(); hit++) {
        if (insideCone(track, (*hit).detid(), dR)) {
          eEM += (*hit).energy();
          // cout << "       Added EE rec hit with (eta, phi) = "
          //      << getPosition((*hit).detid()).eta() << ", "
          //      << getPosition((*hit).detid()).phi() << endl;
        }
      }
      double eHad = 0;
      for (HBHERecHitCollection::const_iterator hit = HBHERecHits->begin(); hit != HBHERecHits->end(); hit++) {
        if (insideCone(track, (*hit).detid(), dR)) {
          eHad += (*hit).energy();
        }
      }

      track.set_caloNewEMDRp5(eEM);
      track.set_caloNewHadDRp5(eHad);
#endif
    }

  event.put (std::move (pl_), collection_.instance ());
  pl_.reset ();
}

bool OSUTrackProducer::insideCone(TYPE(tracks)& candTrack, const DetId& id, const double dR)
{
   GlobalPoint idPosition = getPosition(id);
   if (idPosition.mag()<0.01) return false;
   math::XYZVector idPositionRoot( idPosition.x(), idPosition.y(), idPosition.z() );
   return deltaR(candTrack, idPositionRoot) < dR;
}

GlobalPoint OSUTrackProducer::getPosition( const DetId& id)
{
   if ( ! caloGeometry_.isValid() ||
        ! caloGeometry_->getSubdetectorGeometry(id) ||
        ! caloGeometry_->getSubdetectorGeometry(id)->getGeometry(id) ) {
      throw cms::Exception("FatalError") << "Failed to access geometry for DetId: " << id.rawId();
      return GlobalPoint(0,0,0);
   }
   return caloGeometry_->getSubdetectorGeometry(id)->getGeometry(id)->getPosition();
}



void
OSUTrackProducer::extractFiducialMap (const edm::ParameterSet &cfg, EtaPhiList &vetoList, stringstream &ss) const
{
  const edm::FileInPath &histFile = cfg.getParameter<edm::FileInPath> ("histFile");
  const string &beforeVetoHistName = cfg.getParameter<string> ("beforeVetoHistName");
  const string &afterVetoHistName = cfg.getParameter<string> ("afterVetoHistName");
  const double &thresholdForVeto = cfg.getParameter<double> ("thresholdForVeto");

  TFile *fin = TFile::Open (histFile.fullPath ().c_str ());
  if (!fin)
    {
      edm::LogWarning ("OSUTrackProducer") << "No file named \"" << histFile.fullPath () << "\" found. Skipping...";
      return;
    }

  TH2D *beforeVetoHist = (TH2D *) fin->Get (beforeVetoHistName.c_str ());
  beforeVetoHist->SetDirectory (0);
  TH2D *afterVetoHist = (TH2D *) fin->Get (afterVetoHistName.c_str ());
  afterVetoHist->SetDirectory (0);
  fin->Close ();

  //////////////////////////////////////////////////////////////////////////////
  // First calculate the mean efficiency and error on the mean efficiency.
  //////////////////////////////////////////////////////////////////////////////
  double a = 0, b = 0, aErr2 = 0, bErr2 = 0, mean, meanErr;
  for (int i = 1; i <= beforeVetoHist->GetXaxis ()->GetNbins (); i++)
    {
      for (int j = 1; j <= beforeVetoHist->GetYaxis ()->GetNbins (); j++)
        {
          double binRadius = hypot (0.5 * beforeVetoHist->GetXaxis ()->GetBinWidth (i), 0.5 * beforeVetoHist->GetYaxis ()->GetBinWidth (j));
          (vetoList.minDeltaR < binRadius) && (vetoList.minDeltaR = binRadius);

          double contentBeforeVeto = beforeVetoHist->GetBinContent (i, j),
                 errorBeforeVeto = beforeVetoHist->GetBinError (i, j);

          if (!contentBeforeVeto) // skip bins that are empty in the before-veto histogram
            continue;

          double contentAfterVeto = afterVetoHist->GetBinContent (i, j),
                 errorAfterVeto = afterVetoHist->GetBinError (i, j);

          a += contentAfterVeto;
          b += contentBeforeVeto;
          aErr2 += errorAfterVeto * errorAfterVeto;
          bErr2 += errorBeforeVeto * errorBeforeVeto;
        }
    }
  mean = a / b;
  meanErr = mean * hypot (sqrt (aErr2) / a, sqrt (bErr2) / b);
  //////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////
  // Then find the bins which are greater than the mean by more than
  // thresholdForVeto sigma. Add the coordinates for these bins to the veto
  // list.
  //////////////////////////////////////////////////////////////////////////////
  afterVetoHist->Divide (beforeVetoHist);
  for (int i = 1; i <= afterVetoHist->GetXaxis ()->GetNbins (); i++)
    {
      for (int j = 1; j <= afterVetoHist->GetYaxis ()->GetNbins (); j++)
        {
          double content = afterVetoHist->GetBinContent (i, j),
                 error = afterVetoHist->GetBinError (i, j),
                 eta = afterVetoHist->GetXaxis ()->GetBinCenter (i),
                 phi = afterVetoHist->GetYaxis ()->GetBinCenter (j);

          content && ss << "(" << setw (10) << eta << ", " << setw (10) << phi << "): " << setw (10) << (content - mean) / hypot (error, meanErr) << " sigma above mean of " << setw (10) << mean;
          if ((content - mean) > thresholdForVeto * hypot (error, meanErr))
            {
              vetoList.emplace_back (eta, phi, (content - mean) / hypot (error, meanErr));
              ss << " * HOT SPOT *";
            }
          content && ss << endl;
        }
    }
  //////////////////////////////////////////////////////////////////////////////
  delete beforeVetoHist;
  delete afterVetoHist;
}

void
OSUTrackProducer::envSet (const edm::EventSetup& iSetup)
{
  iSetup.get<EcalChannelStatusRcd> ().get(ecalStatus_);
  iSetup.get<CaloGeometryRecord>   ().get(caloGeometry_);

  if( !ecalStatus_.isValid() )  throw "Failed to get ECAL channel status!";
  if( !caloGeometry_.isValid()   )  throw "Failed to get the caloGeometry_!";
}

int
OSUTrackProducer::getChannelStatusMaps ()
{
  EcalAllDeadChannelsValMap_.clear(); EcalAllDeadChannelsBitMap_.clear();
  TH2D *badChannels = (outputBadEcalChannels_ ? new TH2D ("badChannels", ";#eta;#phi", 360, -3.0, 3.0, 360, -3.2, 3.2) : NULL);

// Loop over EB ...
  for( int ieta=-85; ieta<=85; ieta++ ){
     for( int iphi=0; iphi<=360; iphi++ ){
        if(! EBDetId::validDetId( ieta, iphi ) )  continue;

        const EBDetId detid = EBDetId( ieta, iphi, EBDetId::ETAPHIMODE );
        EcalChannelStatus::const_iterator chit = ecalStatus_->find( detid );
// refer https://twiki.cern.ch/twiki/bin/viewauth/CMS/EcalChannelStatus
        int status = ( chit != ecalStatus_->end() ) ? chit->getStatusCode() & 0x1F : -1;

        const CaloSubdetectorGeometry*  subGeom = caloGeometry_->getSubdetectorGeometry (detid);
        const CaloCellGeometry*        cellGeom = subGeom->getGeometry (detid);
        double eta = cellGeom->getPosition ().eta ();
        double phi = cellGeom->getPosition ().phi ();
        double theta = cellGeom->getPosition().theta();

        if(status >= maskedEcalChannelStatusThreshold_){
           std::vector<double> valVec; std::vector<int> bitVec;
           valVec.push_back(eta); valVec.push_back(phi); valVec.push_back(theta);
           bitVec.push_back(1); bitVec.push_back(ieta); bitVec.push_back(iphi); bitVec.push_back(status);
           EcalAllDeadChannelsValMap_.insert( std::make_pair(detid, valVec) );
           EcalAllDeadChannelsBitMap_.insert( std::make_pair(detid, bitVec) );
           if (outputBadEcalChannels_)
             badChannels->Fill (eta, phi);
        }
     } // end loop iphi
  } // end loop ieta

// Loop over EE detid
  for( int ix=0; ix<=100; ix++ ){
     for( int iy=0; iy<=100; iy++ ){
        for( int iz=-1; iz<=1; iz++ ){
           if(iz==0)  continue;
           if(! EEDetId::validDetId( ix, iy, iz ) )  continue;

           const EEDetId detid = EEDetId( ix, iy, iz, EEDetId::XYMODE );
           EcalChannelStatus::const_iterator chit = ecalStatus_->find( detid );
           int status = ( chit != ecalStatus_->end() ) ? chit->getStatusCode() & 0x1F : -1;

           const CaloSubdetectorGeometry*  subGeom = caloGeometry_->getSubdetectorGeometry (detid);
           const CaloCellGeometry*        cellGeom = subGeom->getGeometry (detid);
           double eta = cellGeom->getPosition ().eta () ;
           double phi = cellGeom->getPosition ().phi () ;
           double theta = cellGeom->getPosition().theta();

           if(status >= maskedEcalChannelStatusThreshold_){
              std::vector<double> valVec; std::vector<int> bitVec;
              valVec.push_back(eta); valVec.push_back(phi); valVec.push_back(theta);
              bitVec.push_back(2); bitVec.push_back(ix); bitVec.push_back(iy); bitVec.push_back(iz); bitVec.push_back(status);
              EcalAllDeadChannelsValMap_.insert( std::make_pair(detid, valVec) );
              EcalAllDeadChannelsBitMap_.insert( std::make_pair(detid, bitVec) );
               if (outputBadEcalChannels_)
                 badChannels->Fill (eta, phi);
           }
        } // end loop iz
     } // end loop iy
  } // end loop ix

  if (outputBadEcalChannels_)
    {
      TFile *fout = new TFile ("badEcalChannels.root", "recreate");
      fout->cd ();
      badChannels->Write ();
      fout->Close ();

      delete badChannels;
      delete fout;
    }

  return 1;
}

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(OSUTrackProducer);

#endif
