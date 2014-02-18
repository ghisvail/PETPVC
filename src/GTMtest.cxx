#include "itkImage.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include <metaCommand.h>

#include "petpvcRoussetPVCImageFilter.h"
 
const char * const VERSION_NO = "0.0.2";
const char * const AUTHOR = "Benjamin A. Thomas";
const char * const APP_TITLE = "Geometric Transfer Matrix (GTM) PVC";

typedef itk::Vector<float, 3> VectorType;
typedef itk::Image<float, 4> MaskImageType;
typedef itk::Image<float, 3> PETImageType;

typedef itk::ImageFileReader<MaskImageType> MaskReaderType;
typedef itk::ImageFileReader<PETImageType> PETReaderType;
typedef itk::ImageFileWriter<PETImageType> PETWriterType;

//Produces the text for the acknowledgments dialog in Slicer. 
std::string getAcknowledgments(void);

using namespace petpvc;

int main(int argc, char *argv[])
{
  // Setup types
  typedef itk::Image<float, 3>   ImageType;
	typedef itk::Image<float, 4>   MaskImageType;


  typedef petpvc::RoussetPVCImageFilter<ImageType, MaskImageType>  FilterType;
 
  ImageType::Pointer image = ImageType::New();

//Setting up command line argument list.
    MetaCommand command;

    command.SetVersion(VERSION_NO);
    command.SetAuthor(AUTHOR);
    command.SetName(APP_TITLE);
    command.SetDescription(
            "Performs Geometric Transfer Matrix (GTM) partial volume correction");

    std::string sAcks = getAcknowledgments();
    command.SetAcknowledgments(sAcks.c_str());

    command.SetCategory("PETPVC");

    command.AddField("petfile", "PET filename", MetaCommand::IMAGE, MetaCommand::DATA_IN);
    command.AddField("maskfile", "mask filename", MetaCommand::IMAGE, MetaCommand::DATA_IN);
    command.AddField("outputfile", "output filename", MetaCommand::FILE, MetaCommand::DATA_OUT);

    command.SetOption("FWHMx", "x", true,
            "The full-width at half maximum in mm along x-axis");
    command.AddOptionField("FWHMx", "X", MetaCommand::FLOAT, true, "");

    command.SetOption("FWHMy", "y", true,
            "The full-width at half maximum in mm along y-axis");
    command.AddOptionField("FWHMy", "Y", MetaCommand::FLOAT, true, "");

    command.SetOption("FWHMz", "z", true,
            "The full-width at half maximum in mm along z-axis");
    command.AddOptionField("FWHMz", "Z", MetaCommand::FLOAT, true, "");

    //Parse command line.
    if (!command.Parse(argc, argv)) {
        return EXIT_FAILURE;
    }

    //Get image filenames
    std::string sPETFileName = command.GetValueAsString("petfile");
    std::string sMaskFileName = command.GetValueAsString("maskfile");
    std::string sOutputFileName = command.GetValueAsString("outputfile");

    //Get values for PSF.
    float fFWHM_x = command.GetValueAsFloat("FWHMx", "X");
    float fFWHM_y = command.GetValueAsFloat("FWHMy", "Y");
    float fFWHM_z = command.GetValueAsFloat("FWHMz", "Z");

    //Make vector of FWHM in x,y and z.
    VectorType vFWHM;
    vFWHM[0] = fFWHM_x;
    vFWHM[1] = fFWHM_y;
    vFWHM[2] = fFWHM_z;

    //Create reader for mask image.
    MaskReaderType::Pointer maskReader = MaskReaderType::New();
    maskReader->SetFileName(sMaskFileName);

    //Try to read mask.
    try {
        maskReader->Update();
    } catch (itk::ExceptionObject & err) {
        std::cerr << "[Error]\tCannot read mask input file: " << sMaskFileName
                << std::endl;
        return EXIT_FAILURE;
    }

    //Create reader for PET image.
    PETReaderType::Pointer petReader = PETReaderType::New();
    petReader->SetFileName(sPETFileName);

    //Try to read PET.
    try {
        petReader->Update();
    } catch (itk::ExceptionObject & err) {
        std::cerr << "[Error]\tCannot read PET input file: " << sPETFileName
                << std::endl;
        return EXIT_FAILURE;
    }

    //Calculate the variance for a given FWHM.
    VectorType vVariance;
    vVariance = vFWHM / (2.0 * sqrt(2.0 * log(2.0)));
    //std::cout << vVariance << std::endl;

    VectorType vVoxelSize = petReader->GetOutput()->GetSpacing();
    //std::cout << vVoxelSize << std::endl;

    //Scale for voxel size.
    vVariance[0] = pow((vVariance[0] / vVoxelSize[0]), 2);
    vVariance[1] = pow((vVariance[1] / vVoxelSize[1]), 2);
    vVariance[2] = pow((vVariance[2] / vVoxelSize[2]), 2);

    FilterType::Pointer gtmFilter = FilterType::New();
		gtmFilter->SetInput( petReader->GetOutput() );
    gtmFilter->SetMaskInput( maskReader->GetOutput() );
    gtmFilter->SetPSF( vVariance );
		gtmFilter->Update();

  // Create and the filter
 /* FilterType::Pointer filter = FilterType::New();
  filter->SetInput(image);
  filter->Update();

	std::cout << filter->GetCorrectedMeans() << std::endl;*/
  return EXIT_SUCCESS;
}

std::string getAcknowledgments(void) {
    //Produces acknowledgments string for 3DSlicer.
    std::string sAck = "This program implements the Geometric Transfer Matrix (GTM) partial volume correction (PVC) technique.\n"
            "The method is described in:\n"
            "\tRousset, O. G. and Ma, Y. and Evans, A. C. (1998). \"Correction for\n"
            "\tpartial volume effects in PET: principle and validation\". Journal of\n"
            "\tNuclear Medicine, 39(5):904-11.";

    return sAck;
}
