//
// Created by ccornell on 3/8/16.
//

#ifndef IMAGEDATA_H
#define IMAGEDATA_H

struct BmpImage {
  int width;
  int height;
  int rle_data_len;
  unsigned char rle_data[256 * 128 * 4 + 1];
};

class ImageData {
 public:
  static unsigned char* UnpackData(const BmpImage& bmp_image);

  static const BmpImage download_ad_, display_ad_, download_i_ad_,
      display_i_ad_;
};

#endif  // IMAGEDATA_H
