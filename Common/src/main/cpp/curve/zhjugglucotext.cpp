#include "jugglucotext.hpp"
#ifndef WEAROS
constexpr static std::string_view labels[] = {"快速胰岛素", "碳水化合物", "右旋糖酐", "长效胰岛素", "骑自行车", "步行", "血液"};
constexpr static Shortcut_t  shortinit[] = { {"面包", .48}, {"醋酸红", .56f}, {"醋酸红2", .595f}, {"葡萄", .165f}, {"水果酸", .058f}, {"米饭", .75f}, {"通心粉", .65f}, {"番茄", .03f}, {"墨西哥", .078f}, {"橙汁", .109f}, {"运动粉", .873f}, {"混合胡", .07f}, {"混合蘑", .073f}};
#endif


jugglucotext chiText {
    .daylabel = {"周日", "周一", "周二", "周三", "周四", "周五", "周六"},
    .speakdaylabel = {"星期日", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六"},
    .monthlabel = {
        "一月", "二月", "三月", "四月", "五月", "六月",
        "七月", "八月", "九月", "十月", "十一月", "十二月"
    },

    .scanned = "已扫描",
    .readysecEnable = "传感器在 %d 分钟内准备好。再次扫描以启用蓝牙传输。",
    .readysec = "传感器在 %d 分钟内准备好。",
    .networkproblem = "镜像中没有血糖数据",
    .enablebluetooth = "启用蓝牙",
    .useBluetoothOff = "'使用蓝牙' 关闭",
    .noconnectionerror = ": 无连接",
    .stsensorerror = ": 传感器错误",
    .streplacesensor = ": 替换传感器？",
    .endedformat = "%s 停止工作。状态=%d",
    .notreadyformat = "%s 未准备好。状态=%d",

#ifndef WEAROS
    .median = "中位数",
    .middle = "中间",
#endif

    .history = "历史记录",
    .sensorstarted = "已启动:",
    .lastscanned = "最后扫描:",
    .laststream = "最后流式传输:",
    .sensorends = "结束于: ",
	.sensorexpectedend="Expected to end: ",

#ifndef WEAROS
    .newamount = "新标签",
    .averageglucose = "平均葡萄糖: ",
    .duration = "持续时间: %.1f 天",
    .timeactive = "%.1f%% 的时间处于活动状态",
    .nrmeasurement = "测量次数: %d",
    .EstimatedA1C = "估算 A1C: %.1f%% (%d mmol/mol)",
    .GMI = "葡萄糖管理指标: %.1f%% (%d mmol/mol)",
    .SD = "SD: %.2f",
    .glucose_variability = "葡萄糖变异性: %.1f%%",
    .menustr0 = {
        "系统 UI        ",
        "主菜单",
        "手表",
        "传感器",
        "设置",
#if defined(SIBIONICS)
"硅基扫码",
#else
        "关于",
#endif
        "关闭应用",
        "停止警报"
    },
    .menustr1 = {
        "导出",
        "镜像",
        chiText.newamount,
        "列表", 
        "统计",
        "播报",
        "悬浮窗        "
    },
    .menustr2 = {"最后一次扫描",
        "扫描数据",
        "蓝牙数据",
        "历史记录",
        "标签数据",
        "餐饮",
        "黑暗模式           "},
    .menustr3 = {hourminstr,
        "搜索",
        "日期",
        "向前一天",
        "向后一天",
        "向前一周",
        "向后一周"},
#else
    .amount = "标签",
    .menustr0 = {
        "镜像",
        "传感器",
        "黑暗模式",
        "设置",
        "停止警报" },
    .menustr2 = {"日期  ",
        hourminstr,
        "向后一天                ",
        chiText.amount},
#endif

    .scanerrors = {
        {"扫描错误 (%d)",
            "再试一次"},
        {"安装错误",
            "?"},
        {"数据处理错误",
            "再试一次"},
        {"激活传感器",
            ""},
        {"传感器已经结束",
            ""},

        {"传感器在",
            "%d 分钟内准备好"},
        {"传感器错误 (373)",
            "稍后再试"},
        {"新传感器已初始化",
            "再次扫描以使用它"},
        {"",
            "扫描期间阻止触摸"},
        {"",
            ""},
        {"库初始化错误",
            "缺少共享库？"},
        {"类初始化错误",
            "做一些事情"},
        {"程序运行时间过长",
            "我杀死程序"},
        {"更换传感器 (365)",
            "您的传感器不工作。请移除您的传感器并启动一个新的。"},
        {"更换传感器 (368)",
            "您的传感器不工作。请移除您的传感器并启动一个新的。"},
        {"",
            ""},
        {"扫描错误",
            "再试一次"}},

    .libre3scanerror = {"FreeStyle Libre 3，扫描错误", 
        "再试一次"},
    .libre3wrongID = {"错误，账户 ID 错误？",
        "在设置->Libreview 中指定与激活传感器相同的账户"},
    .libre3scansuccess = {"FreeStyle Libre 3 传感器", 
        "葡萄糖值现在将由 Juggluco 接收"},
    .unknownNFC = {"未知 NFC 扫描错误", 
        "再试一次"},
    .nolibre3 = {"FreeStyle Libre 3 传感器",
        "从 https://www.juggluco.nl/download.html 下载正确的版本"},

#ifndef WEAROS
    .advancedstart = R"(<h1>修改过的设备</h1>
<p>此应用程序使用的库之存在BUG，会检测root或者框架什么的，作者已经尝试绕过，但最好以其他方式使这些文件不可检测。例如，Magisk 有选项可以
为某些应用程序隐藏 root（Magiskhide 或 Denylist）并更改自己的名称，两者都需要。在您的情况下，它对以下文件有问题)",
    .add_s = true,
    .shortinit = shortinit,
    .labels = labels,
    .checked = "已选中",
    .unchecked = "未选中",
    .Undetermined = "",
    .FallingQuickly = "快速下降",
    .Falling = "下降",
    .Stable = "缓慢变化",
    .Rising = "上升",
    .RisingQuickly = "快速上升",
#endif
.receivingpastvalues="Receiving old values"
};
#include "logs.hpp"
void setusezh() {
LOGAR("usedtext= &chiText");
 usedtext= &chiText;
 }
