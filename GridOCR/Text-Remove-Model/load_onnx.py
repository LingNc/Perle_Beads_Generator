import numpy as np
import cv2
import sys
import re

# === 环境检测：CUDA 与 ONNXRuntime ===
print("=== 环境检测 ===")
# 检查 OpenCV 是否编译了 CUDA 支持
build_info = cv2.getBuildInformation()
print("OpenCV WITH CUDA:", "CUDA" in build_info)
try:
    cuda_count = cv2.cuda.getCudaEnabledDeviceCount()
    print("CUDA 设备数量:", cuda_count)
except Exception:
    print("cv2.cuda.getCudaEnabledDeviceCount 不可用或无 CUDA 设备")
# 检查 onnxruntime GPU provider
try:
    import onnxruntime as ort
    print("ONNXRuntime 可用提供者:", ort.get_available_providers())
except ImportError:
    print("未安装 onnxruntime")
print("================")

def inspect_onnx_model(onnx_path):
    """检查 ONNX 模型的输入输出信息"""
    try:
        import onnxruntime as ort
        session = ort.InferenceSession(onnx_path, providers=["CPUExecutionProvider"])

        print(f"\n=== 模型信息：{onnx_path} ===")

        # 输入信息
        for i, input_info in enumerate(session.get_inputs()):
            print(f"输入 {i}:")
            print(f"  名称: {input_info.name}")
            print(f"  形状: {input_info.shape}")
            print(f"  类型: {input_info.type}")

            # 检查是否有动态维度
            if any(isinstance(dim, str) or dim == -1 for dim in input_info.shape):
                print(f"  ✓ 支持动态尺寸")
            else:
                print(f"  ⚠️ 固定尺寸，必须严格匹配")

        # 输出信息
        for i, output_info in enumerate(session.get_outputs()):
            print(f"输出 {i}:")
            print(f"  名称: {output_info.name}")
            print(f"  形状: {output_info.shape}")
            print(f"  类型: {output_info.type}")

        print("=" * 50)
        return session.get_inputs()[0].shape

    except Exception as e:
        print(f"⚠️ 无法检查模型信息: {e}")
        return None

def preprocess_image(img, target_size=1024):
    """读光OCR的预处理方式 - 保持原始比例"""
    h, w = img.shape[:2]

    # 如果target_size是"auto"，根据图片大小自动调整
    if target_size == "auto":
        # 设置最大尺寸限制
        max_size = 2048
        # 选择合适的尺寸，保持32的倍数（对模型友好）
        max_dim = max(h, w)
        if max_dim <= 512:
            target_size = 512
        elif max_dim <= 1024:
            target_size = 1024
        elif max_dim <= 1600:
            target_size = 1600
        elif max_dim <= 2048:
            target_size = 2048
        else:
            target_size = max_size
        print(f"自动选择目标尺寸: {target_size}x{target_size}")

    # 确保target_size是32的倍数
    target_size = ((target_size + 31) // 32) * 32
    print(f"调整为32的倍数: {target_size}x{target_size}")

    # 计算缩放比例，保持长宽比
    scale = target_size / max(h, w)
    new_h, new_w = int(h * scale), int(w * scale)

    # resize图像
    img_resized = cv2.resize(img, (new_w, new_h))

    # 创建目标尺寸的画布（填充黑色）
    canvas = np.zeros((target_size, target_size, 3), dtype=np.uint8)

    # 将resize后的图像放到画布中央
    start_h = (target_size - new_h) // 2
    start_w = (target_size - new_w) // 2
    canvas[start_h:start_h+new_h, start_w:start_w+new_w] = img_resized

    return canvas, scale, (start_w, start_h)

def postprocess_boxes(boxes, scale, offset, original_size):
    """将检测框坐标转换回原图尺寸"""
    processed_boxes = []
    start_w, start_h = offset
    orig_w, orig_h = original_size

    for box in boxes:
        # 转换回原图坐标
        new_box = []
        for point in box:
            x = (point[0] - start_w) / scale
            y = (point[1] - start_h) / scale
            # 限制在原图范围内
            x = max(0, min(x, orig_w))
            y = max(0, min(y, orig_h))
            new_box.append([x, y])
        processed_boxes.append(np.array(new_box, dtype=np.int32))

    return processed_boxes

def generate_original_binary_map(binary_map, scale, offset, original_size, target_size):
    """正确地将二值化图缩放回原图尺寸"""
    original_w, original_h = original_size
    start_w, start_h = offset

    # 计算原图在预处理图像中的有效区域尺寸
    scaled_w = int(original_w * scale)
    scaled_h = int(original_h * scale)

    # 从二值化图中提取有效区域（去除padding）
    end_h = min(start_h + scaled_h, target_size)
    end_w = min(start_w + scaled_w, target_size)

    # 提取有效区域
    valid_binary = binary_map[start_h:end_h, start_w:end_w]

    # 缩放回原图尺寸
    if valid_binary.size > 0:
        binary_resized = cv2.resize(valid_binary, (original_w, original_h))
        return binary_resized
    else:
        return np.zeros((original_h, original_w), dtype=np.uint8)

def generate_original_prob_map(prob_map, scale, offset, original_size, target_size):
    """将概率图缩放回原图尺寸"""
    original_w, original_h = original_size
    start_w, start_h = offset

    # 计算原图在预处理图像中的有效区域尺寸
    scaled_w = int(original_w * scale)
    scaled_h = int(original_h * scale)

    # 从概率图中提取有效区域（去除padding）
    end_h = min(start_h + scaled_h, target_size)
    end_w = min(start_w + scaled_w, target_size)

    # 提取有效区域
    valid_prob = prob_map[start_h:end_h, start_w:end_w]

    # 缩放回原图尺寸
    if valid_prob.size > 0:
        prob_resized = cv2.resize(valid_prob, (original_w, original_h))
        # 归一化到0-255用于保存
        prob_normalized = (prob_resized * 255).astype(np.uint8)
        return prob_normalized
    else:
        return np.zeros((original_h, original_w), dtype=np.uint8)

def detect_text_duguang(onnx_path, image_path, target_size=1024, threshold=0.3, use_gpu=False, use_ort=False):
    """使用读光OCR的方式检测文本"""

    # 读取图像
    img = cv2.imread(image_path)
    if img is None:
        print(f"无法读取图像: {image_path}")
        return [], None, None

    original_h, original_w = img.shape[:2]
    print(f"原始图像尺寸: {original_w}x{original_h}")

    # 预处理
    processed_img, scale, offset = preprocess_image(img, target_size)
    print(f"预处理后尺寸: {processed_img.shape}")
    print(f"缩放比例: {scale:.4f}, 偏移: {offset}")

    # 检查模型期望的通道数
    expected_channels = 3  # 默认RGB
    if use_ort:
        try:
            import onnxruntime as ort
            session_check = ort.InferenceSession(onnx_path, providers=["CPUExecutionProvider"])
            input_shape = session_check.get_inputs()[0].shape
            if len(input_shape) >= 2 and isinstance(input_shape[1], int):
                expected_channels = input_shape[1]
                print(f"检测到模型期望通道数: {expected_channels}")
        except:
            pass

    # 创建blob
    if expected_channels == 1:
        # 单通道模型：转换为灰度图
        gray_img = cv2.cvtColor(processed_img, cv2.COLOR_BGR2GRAY)
        blob = cv2.dnn.blobFromImage(gray_img,
                                    scalefactor=1.0/255.0,
                                    size=(target_size, target_size),
                                    mean=(0.485,),  # 单通道均值
                                    swapRB=False,
                                    crop=False)
        print(f"单通道模型，使用灰度图输入")
    else:
        # 多通道模型：RGB
        blob = cv2.dnn.blobFromImage(processed_img,
                                    scalefactor=1.0/255.0,
                                    size=(target_size, target_size),
                                    mean=(0.485, 0.456, 0.406),  # ImageNet均值
                                    swapRB=True,  # BGR->RGB
                                    crop=False)
        print(f"多通道模型，使用RGB输入")

    print(f"输入 blob 形状: {blob.shape}")

    # 推理
    if use_ort:
        # 使用 ONNXRuntime 推理
        try:
            import onnxruntime as ort
            providers = ["CUDAExecutionProvider", "CPUExecutionProvider"] if use_gpu else ["CPUExecutionProvider"]
            session = ort.InferenceSession(onnx_path, providers=providers)
            print(f"✓ 使用 ONNXRuntime，提供者: {session.get_providers()}")

            input_name = session.get_inputs()[0].name
            outputs = session.run(None, {input_name: blob.astype(np.float32)})
            print(f"✓ ONNXRuntime 推理成功")
        except Exception as e:
            print(f"⚠️ ONNXRuntime 推理失败: {e}")
            return [], None, None
    else:
        # 使用 OpenCV DNN
        net = cv2.dnn.readNetFromONNX(onnx_path)

        # 设置GPU或CPU后端
        if use_gpu:
            try:
                net.setPreferableBackend(cv2.dnn.DNN_BACKEND_CUDA)
                net.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA)
                print("✓ 使用 OpenCV DNN GPU加速")
            except Exception as e:
                print(f"⚠️ GPU设置失败，使用CPU: {e}")
                net.setPreferableBackend(cv2.dnn.DNN_BACKEND_DEFAULT)
                net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)
        else:
            net.setPreferableBackend(cv2.dnn.DNN_BACKEND_DEFAULT)
            net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)
            print("使用 OpenCV DNN CPU推理")

        # OpenCV DNN 推理
        net.setInput(blob)
        try:
            outputs = net.forward()
        except cv2.error as e:
            print(f"⚠️ GPU推理失败，回退到CPU: {e}")
            # 回退到CPU
            net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
            net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)
            outputs = net.forward()

    print(f"模型输出数量: {len(outputs)}")
    for i, output in enumerate(outputs):
        print(f"输出 {i} 形状: {output.shape}, 数值范围: [{np.min(output):.4f}, {np.max(output):.4f}]")

    # 处理输出（通常是概率图）
    if len(outputs) == 0:
        return [], None, None

    # 取第一个输出作为概率图
    prob_map = outputs[0]

    # 处理不同的输出格式
    if len(prob_map.shape) == 4:  # [N, C, H, W]
        prob_map = prob_map[0, 0]  # 取第一个batch和channel
    elif len(prob_map.shape) == 3:  # [N, H, W]
        prob_map = prob_map[0]

    print(f"概率图形状: {prob_map.shape}")

    # 调整到预处理图像尺寸
    if prob_map.shape != (target_size, target_size):
        prob_map = cv2.resize(prob_map, (target_size, target_size))

    # 二值化
    binary_map = (prob_map > threshold).astype(np.uint8) * 255

    # 形态学操作清理噪声
    kernel = cv2.getStructuringElement(cv2.MORPH_RECT, (3, 3))
    binary_map = cv2.morphologyEx(binary_map, cv2.MORPH_CLOSE, kernel)
    binary_map = cv2.morphologyEx(binary_map, cv2.MORPH_OPEN, kernel)

    # 寻找轮廓
    contours, _ = cv2.findContours(binary_map, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    # 提取文本框
    boxes = []
    for contour in contours:
        # 过滤太小的区域
        # area = cv2.contourArea(contour)
        # if area < 100:
            # continue

        # 获取最小外接矩形
        rect = cv2.minAreaRect(contour)
        box_points = cv2.boxPoints(rect)

        # 过滤太小或太细长的框
        # w, h = rect[1]
        # if min(w, h) < 10 or max(w, h) / min(w, h) > 20:
        #     continue

        boxes.append(box_points)

    print(f"预处理图像中检测到 {len(boxes)} 个候选区域")

    # 将检测框坐标转换回原图
    final_boxes = postprocess_boxes(boxes, scale, offset, (original_w, original_h))

    # 正确生成原图尺寸的二值化图和概率图
    binary_original = generate_original_binary_map(binary_map, scale, offset, (original_w, original_h), target_size)
    prob_original = generate_original_prob_map(prob_map, scale, offset, (original_w, original_h), target_size)

    return final_boxes, binary_original, prob_original

def visualize_results(image_path, boxes, binary_map=None, prob_map=None):
    """可视化检测结果"""
    img = cv2.imread(image_path)

    if img is None:
        print(f"无法读取图像: {image_path}")
        return

    # 绘制检测框
    for i, box in enumerate(boxes):
        cv2.drawContours(img, [box], 0, (0, 255, 0), 2)
        # 不添加编号标注

    # 保存结果
    output_path = image_path.replace('.jpg', '_detected.jpg')
    cv2.imwrite(output_path, img)
    print(f"检测结果已保存到: {output_path}")

    # 保存二值化图
    if binary_map is not None:
        binary_path = image_path.replace('.jpg', '_binary.jpg')
        cv2.imwrite(binary_path, binary_map)
        print(f"二值化结果已保存到: {binary_path}")

    # 保存概率图
    if prob_map is not None:
        prob_path = image_path.replace('.jpg', '_probability.jpg')
        cv2.imwrite(prob_path, prob_map)
        print(f"概率图已保存到: {prob_path}")

    # 给概率图清理噪声
    if prob_map is not None:
        prob_map_cleaned = cv2.morphologyEx(prob_map, cv2.MORPH_CLOSE, np.ones((3, 3), np.uint8))
        prob_path_cleaned = image_path.replace('.jpg', '_probability_cleaned.jpg')
        cv2.imwrite(prob_path_cleaned, prob_map_cleaned)
        print(f"清理后的概率图已保存到: {prob_path_cleaned}")

    # 保留大于阈值0.3的部分，低于阈值的设为0，但不进行二值化
    if prob_map is not None:
        prob_map_filtered = np.where(prob_map > 77, prob_map, 0)  # 0.3 * 255 ≈ 77
        prob_path_filtered = image_path.replace('.jpg', '_probability_filtered.jpg')
        cv2.imwrite(prob_path_filtered, prob_map_filtered)
        print(f"过滤后的概率图已保存到: {prob_path_filtered}")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("用法: python load_onnx.py <model.onnx> <test.jpg> [--gpu] [--ort]")
        print("示例: python load_onnx.py model_1024x1024.onnx test_image.jpg")
        print("GPU加速 (OpenCV): python load_onnx.py model_1024x1024.onnx test_image.jpg --gpu")
        print("ONNXRuntime CPU: python load_onnx.py model_1024x1024.onnx test_image.jpg --ort")
        print("ONNXRuntime GPU: python load_onnx.py model_1024x1024.onnx test_image.jpg --ort --gpu")
        sys.exit(1)

    onnx_path, img_path = sys.argv[1], sys.argv[2]

    # 首先检查模型的输入要求
    model_input_shape = inspect_onnx_model(onnx_path)

    # 检查推理方式
    use_gpu = "--gpu" in sys.argv or "-g" in sys.argv
    use_ort = "--ort" in sys.argv or "--onnxruntime" in sys.argv

    # 智能推断输入尺寸
    target_size = "auto"  # 默认自动

    # 最优先：从模型本身获取固定尺寸
    if model_input_shape and len(model_input_shape) >= 4:
        # 检查是否是固定尺寸 (非动态维度)
        h_dim, w_dim = model_input_shape[2], model_input_shape[3]
        if isinstance(h_dim, int) and isinstance(w_dim, int) and h_dim == w_dim:
            target_size = h_dim
            print(f"从模型获取固定输入尺寸: {target_size}x{target_size}")

    # 次优先：从文件名推断
    if target_size == "auto":
        # 使用正则表达式从文件名中提取尺寸信息
        size_pattern = r'(\d+)x\1'  # 匹配如 1024x1024, 2400x2400 等格式
        size_match = re.search(size_pattern, onnx_path)

        if size_match:
            target_size = int(size_match.group(1))
            print(f"从文件名检测到固定尺寸: {target_size}x{target_size}")
        else:
            # 如果没有找到标准格式，尝试匹配 NxN 格式（如 600x600, 800x800 等）
            general_size_pattern = r'(\d+)x(\d+)'  # 匹配任意 NxM 格式
            general_match = re.search(general_size_pattern, onnx_path)
            if general_match:
                width, height = int(general_match.group(1)), int(general_match.group(2))
                if width == height:  # 只接受正方形输入
                    target_size = width
                    print(f"从文件名推断尺寸: {target_size}x{target_size}")
                else:
                    print(f"从文件名检测到非正方形尺寸 {width}x{height}，将使用 {max(width, height)}x{max(width, height)}")
                    target_size = max(width, height)

    # 如果没有从文件名中找到尺寸，读取图像来自动判断
    if target_size == "auto":
        temp_img = cv2.imread(img_path)
        if temp_img is not None:
            h, w = temp_img.shape[:2]
            max_dim = max(h, w)

            # 根据图像实际尺寸选择最适合的模型输入尺寸
            # if max_dim <= 512:
            #     target_size = 512
            # elif max_dim <= 1024:
            #     target_size = 1024
            # elif max_dim <= 1600:
            #     target_size = 1600
            # elif max_dim <= 2048:
            #     target_size = 2048
            # else:
            target_size = min(4096,max_dim)  # 最大限制

            # 确保是32的倍数
            target_size = ((target_size + 31) // 32) * 32
            print(f"图像尺寸: {w}x{h}, 自动选择模型输入尺寸: {target_size}x{target_size}")
        else:
            target_size = 1024  # 默认值

    # 对于从文件名中获取的尺寸，也确保是32的倍数
    if isinstance(target_size, int):
        original_size = target_size
        target_size = ((target_size + 31) // 32) * 32
        if original_size != target_size:
            print(f"输入尺寸从 {original_size} 调整为 {target_size} (32的倍数)")

    print(f"加载模型: {onnx_path}")
    print(f"处理图像: {img_path}")
    print(f"使用输入尺寸: {target_size}x{target_size}")
    print(f"推理引擎: {'ONNXRuntime' if use_ort else 'OpenCV DNN'}")
    print(f"GPU加速: {'开启' if use_gpu else '关闭'}")

    try:
        # 检测文本
        boxes, binary_map, prob_map = detect_text_duguang(onnx_path, img_path, target_size, use_gpu=use_gpu, use_ort=use_ort)
        print(f"\n最终结果：检测到 {len(boxes)} 个文本区域")

        # 可视化结果
        visualize_results(img_path, boxes, binary_map, prob_map)

    except Exception as e:
        print(f"处理过程中出现错误: {e}")
        import traceback
        traceback.print_exc()