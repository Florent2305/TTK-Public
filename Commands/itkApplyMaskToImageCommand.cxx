/*=========================================================================

  Program:   Tensor ToolKit - TTK
  Module:    $URL$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) INRIA 2010. All rights reserved.
  See LICENSE.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "itkApplyMaskToImageCommand.h"

#include <itkImageFileReader.h>
#include <itkImageFileWriter.h>
#include <itkImage.h>
#include <itkTensorImageIO.h>
#include <itkMaskImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkJoinSeriesImageFilter.h>

#include <iostream>
#include "GetPot.h"


namespace itk
{
    struct argument
    {
        const char *input;
        const char *output;
        const char *mask;
    };

    template <class TInputImage>
    int ApplyMaskToImageCommandImplementation(const argument &arg)
    {
        using ImageType       = TInputImage;
        using MaskType        = itk::Image<unsigned char, ImageType::ImageDimension >;
        using ImageReaderType = itk::ImageFileReader<ImageType>;
        using MaskReaderType  = itk::ImageFileReader<MaskType>;
        using ImageWriterType = itk::ImageFileWriter<ImageType>;
    
        try
        {
            typename ImageReaderType::Pointer reader = ImageReaderType::New();
            reader->SetFileName( arg.input );

            typename MaskReaderType::Pointer maskReader = MaskReaderType::New();
            maskReader->SetFileName(arg.mask);

            using MaskFilterType = itk::MaskImageFilter<ImageType, MaskType, ImageType>;

            typename MaskFilterType::Pointer masker = MaskFilterType::New();
            masker->SetInput1 (reader->GetOutput());
            masker->SetInput2 (maskReader->GetOutput());

            masker->Update();

            typename ImageWriterType::Pointer writer = ImageWriterType::New();
            writer->SetInput (masker->GetOutput());
            writer->SetFileName (arg.output);

            writer->Update();
        }
        catch(itk::ExceptionObject &e)
        {
            std::cerr << e;
            return EXIT_FAILURE;
        }
   
        return EXIT_SUCCESS;
    }


     template <class TInputImage>
    int ApplyMaskTo4DImageCommandImplementation(const argument &arg)
    {
        using ImageType         = TInputImage;
        using MaskType          = itk::Image<unsigned char, 3 >;
        using InternalImageType = itk::Image<typename ImageType::PixelType, 3>;
        using ImageReaderType   = itk::ImageFileReader<ImageType>;
        using MaskReaderType    = itk::ImageFileReader<MaskType>;
        using ImageWriterType   = itk::ImageFileWriter<ImageType>;
    
        try
        {
            typename ImageType::Pointer image = nullptr;

            {
                typename ImageReaderType::Pointer reader = ImageReaderType::New();
                reader->SetFileName( arg.input );
                reader->Update();

                image = reader->GetOutput();
                image->DisconnectPipeline();
            }

            MaskType::Pointer mask = nullptr;
            {
                typename MaskReaderType::Pointer maskReader = MaskReaderType::New();
                maskReader->SetFileName(arg.mask);
                maskReader->Update();

                mask = maskReader->GetOutput();
                mask->DisconnectPipeline();
            }

            using MaskFilterType    = itk::MaskImageFilter<InternalImageType, MaskType, InternalImageType>;
            using ExtractFilterType = itk::ExtractImageFilter<ImageType, InternalImageType>;
            using JoinFilterType    = itk::JoinSeriesImageFilter<InternalImageType, ImageType>;
            typename JoinFilterType::Pointer joiner = JoinFilterType::New();

            unsigned int volumeCount = image->GetLargestPossibleRegion().GetSize()[3];
            for (unsigned int i=0; i<volumeCount; i++)
            {
                typename ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
                size[3] = 0;
                typename ImageType::IndexType index = {{0,0,0,i}};

                typename ImageType::RegionType region;
                region.SetSize(size);
                region.SetIndex(index);

                typename ExtractFilterType::Pointer extractor = ExtractFilterType::New();
                extractor->SetExtractionRegion(region);
                extractor->SetInput(image);
                extractor->SetDirectionCollapseToGuess();

                typename MaskFilterType::Pointer masker = MaskFilterType::New();
                masker->SetInput1 (extractor->GetOutput());
                masker->SetInput2 (mask);

                masker->Update();

                joiner->PushBackInput (masker->GetOutput());
                masker->GetOutput()->DisconnectPipeline();
            }

            joiner->Update();

            typename ImageWriterType::Pointer writer = ImageWriterType::New();
            writer->SetInput (joiner->GetOutput());
            writer->SetFileName (arg.output);

            writer->Update();
        }
        catch(itk::ExceptionObject &e)
        {
            std::cerr << e;
            return EXIT_FAILURE;
        }
   
        return EXIT_SUCCESS;
    }

    template <class TInputTensorImage, class TMaskImage>
    int ApplyMaskToTensorImageCommandImplementation(const argument &arg)
    {
        return EXIT_FAILURE;
    }

  ApplyMaskToImageCommand::ApplyMaskToImageCommand()
  {
    m_ShortDescription = "Apply a mask to a (tensor) image";
    m_LongDescription = "Usage:\n";
    m_LongDescription += "<-i input> <-m mask image> <-o output> <-t type (0: image / 1: tensors)>\n\n";
    m_LongDescription += m_ShortDescription;
  }

  ApplyMaskToImageCommand::~ApplyMaskToImageCommand()
  {}

  int ApplyMaskToImageCommand::Execute (int argc, const char* argv[])
  {

    itk::Object::GlobalWarningDisplayOff();
    
    GetPot cl(argc, const_cast<char**>(argv)); // argument parser
    if( cl.size() == 1 || cl.search(2, "--help", "-h") )
    {
      std::cout << this->GetLongDescription() << std::endl;
      return EXIT_FAILURE;
    }

    argument arg;

    arg.input = cl.follow("",2,"-I","-i");
    arg.output = cl.follow("",2,"-O","-o");
    arg.mask = cl.follow( "", 2, "-m", "-M");
    
    int type = cl.follow(0, 2, "-t", "-T");

    if (type==0)
    {
    
        itk::ImageIOBase::Pointer io = itk::ImageIOFactory::CreateImageIO(arg.input, itk::ImageIOFactory::FileModeEnum::ReadMode);
        if (io.IsNull())
        {
            return EXIT_FAILURE;
        }
        io->SetFileName(arg.input);
        try
        {
            io->ReadImageInformation();
        }
        catch(itk::ExceptionObject &e)
        {
            std::cerr << e;
            return EXIT_FAILURE;
        }

        if (io->GetNumberOfDimensions()==3) 
        {
        switch( io->GetComponentType())
        {
            case itk::ImageIOBase::UCHAR:
            return ApplyMaskToImageCommandImplementation< itk::Image<unsigned char, 3> >(arg);
            break;

            case itk::ImageIOBase::CHAR:
            return ApplyMaskToImageCommandImplementation< itk::Image<char, 3> >(arg);
            break;

            case itk::ImageIOBase::USHORT:
            return ApplyMaskToImageCommandImplementation< itk::Image<unsigned short, 3> >(arg);
            break;

            case itk::ImageIOBase::SHORT:
            return ApplyMaskToImageCommandImplementation< itk::Image<short, 3> >(arg);
            break;

            case itk::ImageIOBase::UINT:
            return ApplyMaskToImageCommandImplementation< itk::Image<unsigned int, 3> >(arg);
            break;

            case itk::ImageIOBase::INT:
            return ApplyMaskToImageCommandImplementation< itk::Image<int, 3> >(arg);
            break;

            case itk::ImageIOBase::ULONG:
            return ApplyMaskToImageCommandImplementation< itk::Image<unsigned long, 3> >(arg);
            break;

            case itk::ImageIOBase::LONG:
            return ApplyMaskToImageCommandImplementation< itk::Image<long, 3> >(arg);
            break;

            case itk::ImageIOBase::FLOAT:
            return ApplyMaskToImageCommandImplementation< itk::Image<float, 3> >(arg);
            break;

            case itk::ImageIOBase::DOUBLE:
            return ApplyMaskToImageCommandImplementation< itk::Image<double, 3> >(arg);
            break;

            default:
            std::cerr << "unsupported component type: " << io->GetComponentTypeAsString( io->GetComponentType() );
            return EXIT_FAILURE;
            break;
        }
        }
        else if (io->GetNumberOfDimensions()==4)
        {
            switch( io->GetComponentType())
            {
            case itk::ImageIOBase::UCHAR:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<unsigned char, 4> >(arg);
            break;

            case itk::ImageIOBase::CHAR:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<char, 4> >(arg);
            break;

            case itk::ImageIOBase::USHORT:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<unsigned short, 4> >(arg);
            break;

            case itk::ImageIOBase::SHORT:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<short, 4> >(arg);
            break;

            case itk::ImageIOBase::UINT:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<unsigned int, 4> >(arg);
            break;

            case itk::ImageIOBase::INT:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<int, 4> >(arg);
            break;

            case itk::ImageIOBase::ULONG:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<unsigned long, 4> >(arg);
            break;

            case itk::ImageIOBase::LONG:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<long, 4> >(arg);
            break;

            case itk::ImageIOBase::FLOAT:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<float, 4> >(arg);
            break;

            case itk::ImageIOBase::DOUBLE:
            return ApplyMaskTo4DImageCommandImplementation< itk::Image<double, 4> >(arg);
            break;

            default:
            std::cerr << "unsupported component type: " << io->GetComponentTypeAsString( io->GetComponentType() );
            return EXIT_FAILURE;
            break;
            }
        }
        else
            std::cerr << "images of dimension " << io->GetNumberOfDimensions() << " not supported";
    }
    else
    {
        std::cerr << "Tensors are not supported yet";
        return EXIT_FAILURE;
    }

    return EXIT_FAILURE;

    }
}
