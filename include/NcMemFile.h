/*--------------------------------------------------------------------------*/
/*--------------------------- File NcMemFile.h -----------------------------*/
/*--------------------------------------------------------------------------*/
/** @file
 * Header file for the class NcMemFile, a netCDF::NcFile whose content lives
 * in memory rather than on disk, i.e., a netCDF "file image" in a buffer of
 * bytes. Since NcMemFile is a netCDF::NcFile, everything that reads or
 * writes an open netCDF::NcFile (e.g., Block::serialize( netCDF::NcFile & ,
 * int ) and Block::deserialize( const netCDF::NcFile & , ... )) works on it
 * unchanged.
 *
 * \author Antonio Frangioni \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \author Donato Meoli \n
 *         Dipartimento di Informatica \n
 *         Universita' di Pisa \n
 *
 * \copyright &copy; by Antonio Frangioni, Donato Meoli
 */
/*--------------------------------------------------------------------------*/
/*----------------------------- DEFINITIONS --------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef __NcMemFile
 #define __NcMemFile
                      /* self-identification: #endif at the end of the file */

/*--------------------------------------------------------------------------*/
/*------------------------------ INCLUDES ----------------------------------*/
/*--------------------------------------------------------------------------*/

#include <netcdf>
#include <span>
#include <vector>

/*--------------------------------------------------------------------------*/
/*----------------------------- NAMESPACE ----------------------------------*/
/*--------------------------------------------------------------------------*/

/// namespace for the Structured Modeling System++ (SMS++)
namespace SMSpp_di_unipi_it
{

/*--------------------------------------------------------------------------*/
/*------------------------------- CLASSES ----------------------------------*/
/*--------------------------------------------------------------------------*/
/** @defgroup NcMemFile_CLASSES Classes in NcMemFile.h
 *  @{ */

/*--------------------------------------------------------------------------*/
/*--------------------------- CLASS NcMemFile ------------------------------*/
/*--------------------------------------------------------------------------*/
/// a netCDF-4 file held in memory
/** NcMemFile is a netCDF::NcFile whose content is a netCDF-4 file image in
 * memory, built on the in-memory support of the netCDF-C library
 * (nc_create_mem(), nc_open_memio() and nc_close_memio()). An NcMemFile is
 * either
 *
 * - created empty and open for writing, by the default constructor;
 *
 * - opened read-only on an existing image, by NcMemFile( std::span ),
 *   working on the bytes of the caller, which are not copied and have to
 *   outlive the NcMemFile.
 *
 * close( Image & ) closes the file and gives its image, a netCDF-4 file in
 * its own right: it can be written to disk and opened from there by any
 * netCDF reader, sent elsewhere, or opened again by an NcMemFile, and the
 * bytes of a netCDF-4 file read from disk can be opened by an NcMemFile.
 * The image need not be byte for byte the file that a netCDF::NcFile on
 * disk with the same content would hold, since the HDF5 layer writes a
 * file in memory with an older version of its format and pads it to the
 * size it has allocated, but the same content written in the same order
 * with the same netCDF and HDF5 libraries gives the same bytes. close() and
 * the destructor close the file discarding the image.
 *
 * An image cannot be opened for writing, since the netCDF-C library does
 * not write in a netCDF-4 image it has not created (nor, on disk, in the
 * file of such an image): what has to go in an image is written in the
 * NcMemFile that creates it, before it is closed.
 *
 * The in-memory files need netCDF-C 4.6.2 or later; with an older library
 * the constructors throw std::logic_error. */

class NcMemFile : public netCDF::NcFile {

/*--------------------------------------------------------------------------*/
/*----------------------- PUBLIC PART OF THE CLASS -------------------------*/
/*--------------------------------------------------------------------------*/

 public:

/*--------------------------------------------------------------------------*/
/*---------------------------- PUBLIC TYPES --------------------------------*/
/*--------------------------------------------------------------------------*/

 /// the image of a netCDF file: its bytes
 using Image = std::vector< unsigned char >;

/*--------------------------------------------------------------------------*/
/*--------------------- CONSTRUCTOR AND DESTRUCTOR -------------------------*/
/*--------------------------------------------------------------------------*/

 /// create an empty netCDF-4 file in memory, open for writing

 NcMemFile( void );

/*--------------------------------------------------------------------------*/
 /// open the netCDF image \p image read-only
 /** The file works on the bytes of \p image themselves, which have to stay
  * there unchanged until the NcMemFile is closed. An \p image that is not a
  * netCDF file throws. */

 explicit NcMemFile( std::span< const unsigned char > image );

/*--------------------------------------------------------------------------*/
 /// destructor: closes the file, if open, discarding its image

 ~NcMemFile() override = default;

/*--------------------------------------------------------------------------*/
/*-------------------------- OTHER METHODS ---------------------------------*/
/*--------------------------------------------------------------------------*/

 /// close the file, discarding its image

 using netCDF::NcFile::close;

/*--------------------------------------------------------------------------*/
 /// close the file and write its image in \p image
 /** Closes the file and replaces the content of \p image with the image of
  * the file as it is when closed. The NcMemFile is then a null object, as
  * after netCDF::NcFile::close(). Throws if the file is not open. */

 void close( Image & image );

/*--------------------------------------------------------------------------*/
/*------------------------ PRIVATE PART OF THE CLASS -----------------------*/
/*--------------------------------------------------------------------------*/

 private:

 bool f_read = false;  ///< true if the bytes are those of the caller

/*--------------------------------------------------------------------------*/

 };  // end( class( NcMemFile ) )

/** @} end( group( NcMemFile_CLASSES ) ) -----------------------------------*/
/*--------------------------------------------------------------------------*/

}  // end( namespace SMSpp_di_unipi_it )

/*--------------------------------------------------------------------------*/

#endif  /* NcMemFile.h included */

/*--------------------------------------------------------------------------*/
/*------------------------ End File NcMemFile.h ----------------------------*/
/*--------------------------------------------------------------------------*/
