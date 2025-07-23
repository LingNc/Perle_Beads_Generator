#include <cairomm/cairomm.h>
#include <iostream>
#include <stdexcept>

int main() {
    try {
        std::cout << "=== Cairomm 功能测试 ===" << std::endl;

        // 创建一个简单的图像表面
        std::cout << "1. 创建图像表面 (100x100)..." << std::endl;
        auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 100, 100);
        auto context = Cairo::Context::create(surface);

        // 设置白色背景
        std::cout << "2. 设置白色背景..." << std::endl;
        context->set_source_rgb(1.0, 1.0, 1.0);  // 白色
        context->paint();

        // 绘制红色矩形
        std::cout << "3. 绘制红色矩形..." << std::endl;
        context->set_source_rgb(1.0, 0.0, 0.0);  // 红色
        context->rectangle(10, 10, 50, 50);
        context->fill();

        // 绘制蓝色圆形
        std::cout << "4. 绘制蓝色圆形..." << std::endl;
        context->set_source_rgb(0.0, 0.0, 1.0);  // 蓝色
        context->arc(75, 25, 15, 0, 2 * M_PI);
        context->fill();

        // 绘制绿色线条
        std::cout << "5. 绘制绿色线条..." << std::endl;
        context->set_source_rgb(0.0, 1.0, 0.0);  // 绿色
        context->set_line_width(3.0);
        context->move_to(10, 70);
        context->line_to(90, 70);
        context->stroke();

        // 添加一些文本（如果支持）
        std::cout << "6. 添加文本..." << std::endl;
        context->set_source_rgb(0.0, 0.0, 0.0);  // 黑色
        context->select_font_face("Sans", Cairo::ToyFontFace::Slant::NORMAL, Cairo::ToyFontFace::Weight::NORMAL);
        context->set_font_size(12);
        context->move_to(10, 90);
        context->show_text("Test");

        // 保存为PNG
        std::cout << "7. 保存为 test_output.png..." << std::endl;
        surface->write_to_png("test_output.png");

        std::cout << "✅ cairomm 工作正常！生成了 test_output.png" << std::endl;
        std::cout << "   图像包含：红色矩形、蓝色圆形、绿色线条和文本" << std::endl;

        // 额外测试：创建不同格式的表面
        std::cout << "\n=== 额外测试 ===" << std::endl;

        // 测试 RGB24 格式
        std::cout << "8. 测试 RGB24 格式..." << std::endl;
        auto rgb_surface = Cairo::ImageSurface::create(Cairo::Surface::Format::RGB24, 50, 50);
        auto rgb_context = Cairo::Context::create(rgb_surface);
        rgb_context->set_source_rgb(0.5, 0.8, 0.2);
        rgb_context->paint();
        rgb_surface->write_to_png("test_rgb24.png");

        // 获取表面信息
        std::cout << "9. 表面信息：" << std::endl;
        std::cout << "   宽度: " << surface->get_width() << " 像素" << std::endl;
        std::cout << "   高度: " << surface->get_height() << " 像素" << std::endl;
        std::cout << "   格式: " << static_cast<int>(surface->get_format()) << std::endl;

        std::cout << "\n🎉 所有测试通过！" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ 错误: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "❌ 未知错误发生" << std::endl;
        return 1;
    }
}