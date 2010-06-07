#ifndef _itk_TensorImageIO_h_
#define _itk_TensorImageIO_h_

#ifdef _MSC_VER
#pragma warning ( disable : 4786 )
#endif

#include <itkProcessObject.h>
#include <itkImage.h>
#include "itkTensor.h"
#include <itkNrrdImageIOFactory.h>
#include <itkNiftiImageIOFactory.h>
#include <itkMetaImageIO.h>

namespace itk
{
  /**
     \class TensorImageIO
     
     Author: Pierre Fillard
     
  */
  
  template <class T=double, unsigned int TensorDimension=3, unsigned int ImageDimension=3>
    class ITK_EXPORT TensorImageIO : public ProcessObject
    {
      
    public:
    
    typedef TensorImageIO Self;
    typedef ProcessObject Superclass;
    typedef SmartPointer<Self> Pointer;
    typedef SmartPointer<const Self> ConstPointer;

    itkNewMacro (Self);
    itkTypeMacro (TensorImageIO, ProcessObject);
    
    /** specific typedefs */
    typedef T                                     ValueType;
    typedef Tensor<T,TensorDimension>             TensorType;
    typedef Image<TensorType, ImageDimension>     TensorImageType;
    typedef typename TensorImageType::SizeType    SizeType;
    typedef typename TensorImageType::IndexType   IndexType;
    typedef typename TensorImageType::RegionType  RegionType;
    typedef typename TensorImageType::SpacingType SpacingType;
    typedef typename TensorImageType::PointType   PointType;
    typedef typename TensorImageType::DirectionType DirectionType;
    


	static const unsigned int DegreesOfFreedom = TensorDimension*(TensorDimension+1)/2;

    /** Actually read the data */
    void Read(void);

    /** Actually read the data */
    void Write(void);

    
    itkSetStringMacro(FileName);
    itkGetStringMacro(FileName);

    itkSetConstObjectMacro(Input,  TensorImageType);
    itkGetConstObjectMacro(Input,  TensorImageType);
    itkGetObjectMacro(Output, TensorImageType);
    
    
  protected:

  TensorImageIO()
    {
      m_Output = TensorImageType::New();
      itk::NrrdImageIOFactory::RegisterOneFactory();
      itk::NiftiImageIOFactory::RegisterOneFactory();
    }
  ~TensorImageIO(){};
  
  void PrintSelf (std::ostream &os, Indent indent) const
    {
      Superclass::PrintSelf (os,indent);      
    }
  
    void ReadVTK (const char* filename);
    void ReadNrrd (const char* filename);
    void ReadInrimage (const char* filename);
    void ReadNifti (const char* filename);
    void ReadMha (const char* filename);

    void WriteVTK (const char* filename);
    void WriteNrrd (const char* filename);
    void WriteInrimage (const char* filename);
	void WriteNifti (const char* filename);
    void WriteMha (const char* filename);

    
  private:

    TensorImageIO (const Self&);
    void operator=(const Self&);

    std::string m_FileName;
    typename TensorImageType::ConstPointer m_Input; // purposely not implemented
    typename TensorImageType::Pointer m_Output; // purposely not implemented    
        
  };
  
  
  

} // end of namespace itk


#ifndef ITK_MANUAL_INSTANTIATION
#include "itkTensorImageIO.txx"
#endif

#endif