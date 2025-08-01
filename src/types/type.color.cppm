export module type.color;
import type.base;

export struct RgbColor{
    int r,g,b;
    RgbColor(int red,int green,int blue): r(red),g(green),b(blue){}
};