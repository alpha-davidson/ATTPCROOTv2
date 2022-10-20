#include "AtHDF5ReadTask.h"

#include "AtEvent.h"
#include "AtHit.h"

#include <FairLogger.h>      // for LOG
#include <FairRootManager.h> // for FairRootManager

#include <TObject.h> // for TObject

#include <H5Cpp.h>

#include <utility> // for move

AtHDF5ReadTask::AtHDF5ReadTask(TString fileName, TString outputBranchName)
   : fInputFileName(std::move(fileName)), fOutputBranchName(std::move(outputBranchName)), fEventArray("AtEvent", 1)
{
}

InitStatus AtHDF5ReadTask::Init()
{

   FairRootManager *ioMan = FairRootManager::Instance();
   if (ioMan == nullptr) {
      LOG(ERROR) << "Cannot find RootManager!";
      return kERROR;
   }

   ioMan->Register(fOutputBranchName, "AtTPC", &fEventArray, fIsPersistence);

   // TODO: Here is where we will need to open the HDF5 file
   fFile = std::make_unique<H5::H5File>(fInputFileName, H5F_ACC_RDONLY);
   return kSUCCESS;
}

void AtHDF5ReadTask::Exec(Option_t *opt)
{
   // You will want to do something like this I think:
   // http://davis.lbl.gov/Manuals/HDF5-1.6.5/cpplus_RM/h5group_8cpp-example.html starting from the comment "Now reopen
   // the group in the file"

   // Because events are indexted by their event number the opening line will be something like
   
    auto hitArray = fFile->openDataSet(TString::Format("Event_[%d]/HitArray", fEventNum));
   //auto hitArray = H5Dopen(fFile, TString::Format("Event_[%d]/HitArray", fEventNum), H5P_DEFAULT);

   auto trace = fFile->openDataSet(TString::Format("Event_[%d]/Trace", fEventNum));
   hid_t dspace = H5Dget_space(hitArray.getId());
   const int ndims = H5Sget_simple_extent_ndims(dspace);
   hsize_t dims[ndims];
   H5Sget_simple_extent_dims(dspace, dims, NULL);
   fEventNum++;

   // Then we create the event to fill

   auto *event = dynamic_cast<AtEvent *>(fEventArray.ConstructedAt(0, "C")); // Get and clear old event
   std::vector<AtHit_t> eventArray;
   eventArray.resize(dims[0]);
   auto hdf5Type = AtHit().GetHDF5Type();
   auto status = H5Dread(hitArray.getId(), hdf5Type.getId(), H5S_ALL, H5S_ALL, H5P_DEFAULT, eventArray.data());

   for(auto & hit : eventArray){
      event->AddHit(-1, AtHit::XYZPoint(hit.x, hit.y, hit.z), hit.A);
   }
}

ClassImp(AtHDF5ReadTask);
