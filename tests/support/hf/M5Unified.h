#pragma once
#include <cstdint>
#include <string>
#include <vector>
#define EXT_RAM_BSS_ATTR
constexpr uint16_t TFT_BLACK=0,TFT_WHITE=0xffff,TFT_RED=0xf800,TFT_ORANGE=0xfd20,
 TFT_DARKGREY=0x7bef,TFT_LIGHTGREY=0xc618,TFT_NAVY=0xf;
enum textdatum_t {middle_left,middle_center,middle_right,top_left};
struct HostDisplay {
 struct Line {int x,y,h;uint16_t color;};
 std::vector<std::string> labels;
 std::vector<Line> verticals;
 void drawString(const char* text,int,int){labels.emplace_back(text);}
 void fillScreen(uint16_t){labels.clear();verticals.clear();}
 void drawFastVLine(int x,int y,int h,uint16_t c){verticals.push_back({x,y,h,c});}
 void drawFastHLine(int,int,int,uint16_t){}
 void drawLine(int,int,int,int,uint16_t){}
 void fillRect(int,int,int,int,uint16_t){}
 void drawRect(int,int,int,int,uint16_t){}
 void fillRoundRect(int,int,int,int,int,uint16_t){}
 void drawRoundRect(int,int,int,int,int,uint16_t){}
 void fillCircle(int,int,int,uint16_t){}
 void setTextDatum(textdatum_t){}
 void setTextSize(int){}
 void setTextColor(uint16_t){}
 void clearScrollRect(){}
 void setScrollRect(int,int,int,int,uint16_t){}
 void scroll(int,int){}
 void startWrite(){}
 void endWrite(){}
 void pushImage(int,int,int,int,const uint16_t*){}
 uint16_t color565(uint8_t r,uint8_t g,uint8_t b){return uint16_t((r>>3)<<11|(g>>2)<<5|(b>>3));}
};
struct HostM5 {HostDisplay Display;};
inline HostM5 M5;
