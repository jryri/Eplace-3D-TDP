#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <tuple>

#include <iostream>

#include "fft.h"

#define REPLACE_FFT_PI 3.141592653589793238462L 

namespace replace {


FFT_2D::FFT_2D()
  : binCntX_(0), binCntY_(0), binSizeX_(0.0), binSizeY_(0.0) {}

FFT_2D::FFT_2D(int binCntX, int binCntY, float binSizeX, float binSizeY)
  : binCntX_(binCntX), binCntY_(binCntY), 
  binSizeX_(binSizeX), binSizeY_(binSizeY) {
  init();   
}

FFT_2D::~FFT_2D() {
  using std::vector;
  for(int i=0; i<binCntX_; i++) {
    delete(binDensity_[i]);
    delete(electroPhi_[i]);
    delete(electroForceX_[i]);
    delete(electroForceY_[i]);
  }
  delete(binDensity_);
  delete(electroPhi_);
  delete(electroForceX_);
  delete(electroForceY_);


  csTable_.clear();
  wx_.clear();
  wxSquare_.clear();
  wy_.clear();
  wySquare_.clear();
  
  workArea_.clear();
}


void
FFT_2D::init() {
  binDensity_ = new float*[binCntX_];
  electroPhi_ = new float*[binCntX_];
  electroForceX_ = new float*[binCntX_];
  electroForceY_ = new float*[binCntX_];

  for(int i=0; i<binCntX_; i++) {
    binDensity_[i] = new float[binCntY_];
    electroPhi_[i] = new float[binCntY_];
    electroForceX_[i] = new float[binCntY_];
    electroForceY_[i] = new float[binCntY_];

    for(int j=0; j<binCntY_; j++) {
      binDensity_[i][j] 
        = electroPhi_[i][j] 
        = electroForceX_[i][j] 
        = electroForceY_[i][j] 
        = 0.0f;  
    }
  }

  csTable_.resize( std::max(binCntX_, binCntY_) * 3 / 2, 0 );

  wx_.resize( binCntX_, 0 );
  wxSquare_.resize( binCntX_, 0);
  wy_.resize( binCntY_, 0 );
  wySquare_.resize( binCntY_, 0 );

  workArea_.resize( round(sqrt(std::max(binCntX_, binCntY_))) + 2, 0 );
 
  for(int i=0; i<binCntX_; i++) {
    wx_[i] = REPLACE_FFT_PI * static_cast<float>(i) 
      / static_cast<float>(binCntX_);
    wxSquare_[i] = wx_[i] * wx_[i]; 
  }

  for(int i=0; i<binCntY_; i++) {
    wy_[i] = REPLACE_FFT_PI * static_cast<float>(i)
      / static_cast<float>(binCntY_) 
      * static_cast<float>(binSizeY_) 
      / static_cast<float>(binSizeX_);
    wySquare_[i] = wy_[i] * wy_[i];
  }
}

void
FFT_2D::updateDensity(int x, int y, float density) {
  binDensity_[x][y] = density;
}

std::pair<float, float> 
FFT_2D::getElectroForce(int x, int y) const {
  return std::make_pair(
      electroForceX_[x][y],
      electroForceY_[x][y]);
}

float
FFT_2D::getElectroPhi(int x, int y) const {
  return electroPhi_[x][y]; 
}

using namespace std;

void
FFT_2D::doFFT() {
  
  ddct2d(binCntX_, binCntY_, -1, binDensity_, 
      NULL, (int*) &workArea_[0], (float*)&csTable_[0]);

  for(int i = 0; i < binCntX_; i++) {
    binDensity_[i][0] *= 0.5;
  }

  for(int i = 0; i < binCntY_; i++) {
    binDensity_[0][i] *= 0.5;
  }

  for(int i = 0; i < binCntX_; i++) {
    for(int j = 0; j < binCntY_; j++) {
      binDensity_[i][j] *= 4.0 / binCntX_ / binCntY_;
    }
  }

  for(int i = 0; i < binCntX_; i++) {
    float wx = wx_[i];
    float wx2 = wxSquare_[i];

    for(int j = 0; j < binCntY_; j++) {
      float wy = wy_[j];
      float wy2 = wySquare_[j];

      float density = binDensity_[i][j];
      float phi = 0;
      float electroX = 0, electroY = 0;

      if(i == 0 && j == 0) {
        phi = electroX = electroY = 0.0f;
      }
      else {
        //////////// lutong
        //  denom =
        //  wx2 / 4.0 +
        //  wy2 / 4.0 ;
        // a_phi = a_den / denom ;
        ////b_phi = 0 ; // -1.0 * b / denom ;
        ////a_ex = 0 ; // b_phi * wx ;
        // a_ex = a_phi * wx / 2.0 ;
        ////a_ey = 0 ; // b_phi * wy ;
        // a_ey = a_phi * wy / 2.0 ;
        ///////////
        phi = density / (wx2 + wy2);
        electroX = phi * wx;
        electroY = phi * wy;
      }
      electroPhi_[i][j] = phi;
      electroForceX_[i][j] = electroX;
      electroForceY_[i][j] = electroY;
    }
  }
  // Inverse DCT
  ddct2d(binCntX_, binCntY_, 1, 
      electroPhi_, NULL, 
      (int*) &workArea_[0], (float*) &csTable_[0]);
  ddsct2d(binCntX_, binCntY_, 1, 
      electroForceX_, NULL, 
      (int*) &workArea_[0], (float*) &csTable_[0]);
  ddcst2d(binCntX_, binCntY_, 1, 
      electroForceY_, NULL, 
      (int*) &workArea_[0], (float*) &csTable_[0]);

}


//========================================================================
// FFT_3D Implementation
//========================================================================

FFT_3D::FFT_3D()
  : binCntX_(0), binCntY_(0), binCntZ_(0), 
    binSizeX_(0.0), binSizeY_(0.0), binSizeZ_(0.0) {}

FFT_3D::FFT_3D(int binCntX, int binCntY, int binCntZ, 
               float binSizeX, float binSizeY, float binSizeZ)
  : binCntX_(binCntX), binCntY_(binCntY), binCntZ_(binCntZ),
    binSizeX_(binSizeX), binSizeY_(binSizeY), binSizeZ_(binSizeZ) {
  init();   
}

FFT_3D::~FFT_3D() {
  // Clean up 3D arrays
  for(int i = 0; i < binCntX_; i++) {
    for(int j = 0; j < binCntY_; j++) {
      delete[] binDensity_[i][j];
      delete[] electroPhi_[i][j];
      delete[] electroForceX_[i][j];
      delete[] electroForceY_[i][j];
      delete[] electroForceZ_[i][j];
    }
    delete[] binDensity_[i];
    delete[] electroPhi_[i];
    delete[] electroForceX_[i];
    delete[] electroForceY_[i];
    delete[] electroForceZ_[i];
  }
  delete[] binDensity_;
  delete[] electroPhi_;
  delete[] electroForceX_;
  delete[] electroForceY_;
  delete[] electroForceZ_;

  csTable_.clear();
  wx_.clear();
  wxSquare_.clear();
  wy_.clear();
  wySquare_.clear();
  wz_.clear();
  wzSquare_.clear();
  workArea_.clear();
  tempWorkArray_.clear();
}

void FFT_3D::init() {
  // Allocate 3D arrays
  binDensity_ = new float**[binCntX_];
  electroPhi_ = new float**[binCntX_];
  electroForceX_ = new float**[binCntX_];
  electroForceY_ = new float**[binCntX_];
  electroForceZ_ = new float**[binCntX_];

  for(int i = 0; i < binCntX_; i++) {
    binDensity_[i] = new float*[binCntY_];
    electroPhi_[i] = new float*[binCntY_];
    electroForceX_[i] = new float*[binCntY_];
    electroForceY_[i] = new float*[binCntY_];
    electroForceZ_[i] = new float*[binCntY_];

    for(int j = 0; j < binCntY_; j++) {
      binDensity_[i][j] = new float[binCntZ_];
      electroPhi_[i][j] = new float[binCntZ_];
      electroForceX_[i][j] = new float[binCntZ_];
      electroForceY_[i][j] = new float[binCntZ_];
      electroForceZ_[i][j] = new float[binCntZ_];

      for(int k = 0; k < binCntZ_; k++) {
        binDensity_[i][j][k] = 0.0f;
        electroPhi_[i][j][k] = 0.0f;
        electroForceX_[i][j][k] = 0.0f;
        electroForceY_[i][j][k] = 0.0f;
        electroForceZ_[i][j][k] = 0.0f;
      }
    }
  }

  // Initialize cos/sin table for 3D
  int maxDim = std::max({binCntX_, binCntY_, binCntZ_});
  
  // Calculate proper sizes for 3D FFT based on fftsg3d requirements
  int nw = maxDim >> 2;  // For makewt
  int nc = maxDim;       // For makect
  int totalTableSize = nw + nc; // w array needs nw + nc elements
  csTable_.resize(totalTableSize + 10, 0); // Add some safety margin
  
  // Calculate proper work area size for 3D FFT
  int maxXY = std::max(binCntX_, binCntY_);
  int workAreaSize = maxXY * 4; // Based on ddct3d requirements: nt = max(n1,n2) * 4
  #ifdef USE_FFT3D_THREADS
  workAreaSize *= FFT3D_MAX_THREADS; // Account for threading if enabled
  #endif
  
  // Allocate work area with proper initialization for FFT
  workArea_.resize(std::max(workAreaSize + 10, maxDim + 10), 0); // Ensure minimum size
  
  // Initialize the first few elements as required by fftsg3d
  if (workArea_.size() >= 2) {
    workArea_[0] = 0; // nw will be set by makewt
    workArea_[1] = 0; // nc will be set by makect
  }
  
  // Initialize temporary work array for ddxt3da_sub functions
  // Based on the function requirements: needs 4 * max(n1, n2) elements
  int tempWorkSize = 4 * maxXY;
  tempWorkArray_.resize(tempWorkSize, 0.0f);

  // Initialize wx, wy, wz arrays
  wx_.resize(binCntX_, 0);
  wxSquare_.resize(binCntX_, 0);
  wy_.resize(binCntY_, 0);
  wySquare_.resize(binCntY_, 0);
  wz_.resize(binCntZ_, 0);
  wzSquare_.resize(binCntZ_, 0);

  // Initialize frequency domain arrays
  for(int i = 0; i < binCntX_; i++) {
    wx_[i] = REPLACE_FFT_PI * static_cast<float>(i) / static_cast<float>(binCntX_);
    wxSquare_[i] = wx_[i] * wx_[i];
  }

  for(int i = 0; i < binCntY_; i++) {
    wy_[i] = REPLACE_FFT_PI * static_cast<float>(i) / static_cast<float>(binCntY_) 
             * static_cast<float>(binSizeY_) / static_cast<float>(binSizeX_);
    wySquare_[i] = wy_[i] * wy_[i];
  }

  for(int i = 0; i < binCntZ_; i++) {
    wz_[i] = REPLACE_FFT_PI * static_cast<float>(i) / static_cast<float>(binCntZ_) 
             * static_cast<float>(binSizeZ_) / static_cast<float>(binSizeX_);
    wzSquare_[i] = wz_[i] * wz_[i];
  }
}

void FFT_3D::updateDensity(int x, int y, int z, float density) {
  // Add boundary checks to prevent segmentation fault
  if (x >= 0 && x < binCntX_ && y >= 0 && y < binCntY_ && z >= 0 && z < binCntZ_) {
    binDensity_[x][y][z] = density;
  }
}

std::tuple<float, float, float> FFT_3D::getElectroForce(int x, int y, int z) const {
  // Add boundary checks to prevent segmentation fault
  if (x >= 0 && x < binCntX_ && y >= 0 && y < binCntY_ && z >= 0 && z < binCntZ_) {
    return std::make_tuple(
        electroForceX_[x][y][z],
        electroForceY_[x][y][z],
        electroForceZ_[x][y][z]);
  }
  return std::make_tuple(0.0f, 0.0f, 0.0f); // Return zero if out of bounds
}

float FFT_3D::getElectroPhi(int x, int y, int z) const {
  // Add boundary checks to prevent segmentation fault
  if (x >= 0 && x < binCntX_ && y >= 0 && y < binCntY_ && z >= 0 && z < binCntZ_) {
    return electroPhi_[x][y][z];
  }
  return 0.0f; // Return zero if out of bounds
}

void FFT_3D::doFFT() {
  // Apply 3D DCT to density
  ddct3d(binCntX_, binCntY_, binCntZ_, -1, binDensity_,
         NULL, (int*) &workArea_[0], (float*)&csTable_[0]);

  // Scale the DC and boundary terms (similar to 2D version)
  for(int i = 0; i < binCntX_; i++) {
    binDensity_[i][0][0] *= 0.5;
  }
  for(int j = 0; j < binCntY_; j++) {
    binDensity_[0][j][0] *= 0.5;
  }
  for(int k = 0; k < binCntZ_; k++) {
    binDensity_[0][0][k] *= 0.5;
  }

  // Normalize
  float normFactor = 8.0 / (binCntX_ * binCntY_ * binCntZ_);
  for(int i = 0; i < binCntX_; i++) {
    for(int j = 0; j < binCntY_; j++) {
      for(int k = 0; k < binCntZ_; k++) {
        binDensity_[i][j][k] *= normFactor;
      }
    }
  }

  // Compute electric potential and forces in frequency domain
  for(int i = 0; i < binCntX_; i++) {
    float wx = wx_[i];
    float wx2 = wxSquare_[i];

    for(int j = 0; j < binCntY_; j++) {
      float wy = wy_[j];
      float wy2 = wySquare_[j];

      for(int k = 0; k < binCntZ_; k++) {
        float wz = wz_[k];
        float wz2 = wzSquare_[k];

        float density = binDensity_[i][j][k];
        float phi = 0;
        float electroX = 0, electroY = 0, electroZ = 0;

        if(i == 0 && j == 0 && k == 0) {
          phi = electroX = electroY = electroZ = 0.0f;
        } else {
          float denom = wx2 + wy2 + wz2;
          phi = density / denom;
          electroX = phi * wx;
          electroY = phi * wy;
          electroZ = phi * wz;
        }

        electroPhi_[i][j][k] = phi;
        electroForceX_[i][j][k] = electroX;
        electroForceY_[i][j][k] = electroY;
        electroForceZ_[i][j][k] = electroZ;
      }
    }
  }

  // Inverse 3D DCT to get spatial domain results
  ddct3d(binCntX_, binCntY_, binCntZ_, 1, 
         electroPhi_, NULL, 
         (int*) &workArea_[0], (float*) &csTable_[0]);

  // For forces, we need different transforms (similar to 2D version)
  // Use tempWorkArray_ instead of NULL to avoid segmentation fault
  float* tempWork = &tempWorkArray_[0];
  
  // X force: DCT-DST-DCT
  ddxt3da_sub(binCntX_, binCntY_, binCntZ_, 0, 0, 1, 
              electroForceX_, tempWork, 
              (int*) &workArea_[0], (float*) &csTable_[0]);
  ddxt3db_sub(binCntX_, binCntY_, binCntZ_, 1, 1, 
              electroForceX_, tempWork, 
              (int*) &workArea_[0], (float*) &csTable_[0]);

  // Y force: DST-DCT-DCT  
  ddxt3da_sub(binCntX_, binCntY_, binCntZ_, 1, 0, 1, 
              electroForceY_, tempWork, 
              (int*) &workArea_[0], (float*) &csTable_[0]);
  ddxt3db_sub(binCntX_, binCntY_, binCntZ_, 0, 1, 
              electroForceY_, tempWork, 
              (int*) &workArea_[0], (float*) &csTable_[0]);

  // Z force: DCT-DCT-DST
  ddxt3da_sub(binCntX_, binCntY_, binCntZ_, 0, 1, 1, 
              electroForceZ_, tempWork, 
              (int*) &workArea_[0], (float*) &csTable_[0]);
  ddxt3db_sub(binCntX_, binCntY_, binCntZ_, 0, 1, 
              electroForceZ_, tempWork, 
              (int*) &workArea_[0], (float*) &csTable_[0]);
}

}
