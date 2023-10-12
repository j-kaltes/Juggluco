/*      This file is part of Juggluco, an Android app to receive and display         */
/*      glucose values from Freestyle Libre 2 and 3 sensors.                         */
/*                                                                                   */
/*      Copyright (C) 2021 Jaap Korthals Altes <jaapkorthalsaltes@gmail.com>         */
/*                                                                                   */
/*      Juggluco is free software: you can redistribute it and/or modify             */
/*      it under the terms of the GNU General Public License as published            */
/*      by the Free Software Foundation, either version 3 of the License, or         */
/*      (at your option) any later version.                                          */
/*                                                                                   */
/*      Juggluco is distributed in the hope that it will be useful, but              */
/*      WITHOUT ANY WARRANTY; without even the implied warranty of                   */
/*      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                         */
/*      See the GNU General Public License for more details.                         */
/*                                                                                   */
/*      You should have received a copy of the GNU General Public License            */
/*      along with Juggluco. If not, see <https://www.gnu.org/licenses/>.            */
/*                                                                                   */
/*      Fri Jan 27 15:20:04 CET 2023                                                 */


#include "jugglucotext.h"
#ifndef WEAROS
constexpr static  char zhlabelsbuf[][12]={"快速 ins","糖类","葡萄糖","长效 ins","自行车","步行","血液"};
constexpr static std::string_view zhlabels[]={"快速 ins","糖类","葡萄糖","长效 ins","自行车","步行","血液"};
constexpr static Shortcut_t zhshortinit[]= {
	{"面包",.48}, 
	{"醋栗面",.56f},
        {"葡萄",.165f},
        {"水果瑜",.058f},
        {"米",.75f},
        {"通心粉",.65f},
        {"番茄",.03f},
        {"墨西哥",.078f},
        {"橙汁",.109f},
        {"运动粉",.873f},
        {"混合",.07f},
        {"混合蘑",.07300000f}};
#endif

static jugglucotext zhtext {
	.daylabel={"星期日","星期一","星期二","星期三","星期四","星期五","星期六"},
	.monthlabel={
      "一月","二月","三月","四月","五月","六月",
       "七月","八月","九月",
      "十月","十一月","十二月"},

	.scanned="已扫描",
	.readysecEnable="传感器在 %d 分钟内准备就绪。再次扫描以启用流式传输。",
	.readysec="传感器在 %d 分钟内准备就绪。",
.networkproblem="网络问题？",
.enablebluetooth="启用蓝牙",
.useBluetoothOff="'使用蓝牙'关闭",
.noconnectionerror ="：无连接",
.stsensorerror ="：传感器错误",
.streplacesensor=": 更换传感器？",
.endedformat="%s 停止工作。state=%d",
.notreadyformat="%s 未就绪。状态=%d",
#ifndef WEAROS
	.median="中位数",
	.middle="中",
#endif
	.history="历史",
	.historyinfo="每 15 分钟一次。\n在传感器上记住 8 小时。\n扫描会将它们传输到此程序。\n传感器: ",
	.history3info="每 5 分钟一次。\n在传感器上记住 14 天。\n通过蓝牙传输到此程序。\n传感器：",
	.sensorstarted= "传感器启动：",
	.lastscanned="上次扫描：",
	.laststream="最后一个流：",
	.sensorends="传感器结束：",
#ifndef WEAROS
	.newamount="新金额",
	.averageglucose="平均血糖：",
	.duration="持续时间：%.1f 天",
	.timeactive="%.1f%% 的活动时间",
	.nrmeasurement="测量次数：%d",
	.EstimatedA1C="估计 A1C: %.1f%% (%d mmol/mol)",
	.GMI="血糖管理指标：%.1f%% (%d mmol/mol)",
	.SD="SD: %.2f",
	.glucose_variability="血糖变异性：%.1f%%",
     .menustr0={
		"系统用户界面       ",
		"Menus",
		"手表",
		"传感器",
		"设置",
		"关于",
		"关闭",
		"停止警报"
		},
	.menustr1={
		"出口",
		"镜子",
		zhtext.newamount,
		"列表",
		"统计数据",
		"讲话",
		"漂浮 "
		},
	.menustr2= {"上次扫描","扫描","流","历史记录","金额","餐食","深色模式"},
	.menustr3= {hourminstr,"搜索","日期","前一天","后一天","前一周","后一周"},
#else
 .amount="金额",
 .menustr0= {
	"镜子",
	"传感器",
	"深色模式",
        "设置",
	"停止闹钟"},
.menustr2= {"日期",hourminstr,"日后",zhtext.amount},
#endif

	.scanerrors={
		{"扫描错误 (%d)","重试"},
		{"安装错误","?"},
		{"数据处理错误","重试"},
		{"激活传感器",""},
		{"传感器肯定已经结束",""},

		{"传感器准备就绪","%d 分钟"},
		{"373: 传感器错误","稍后重试"},
		{"新传感器已初始化","再次扫描以使用它"},
		{"","扫描期间阻止触摸"},
		{"",""},
		{"库初始化错误","共享库是否丢失？"},
		{"类初始化错误","做某事"},
		{"程序耗时太长","我杀死程序"},
		{"365: 更换传感器","您的传感器无法工作。请移除您的传感器并启动一个新的。"},
		{"368: 更换传感器","您的传感器无法工作。请移除您的传感器并启动一个新的。"},
		{"",""},
		{"扫描错误","重试"}},

.libre3scanerror={"FreeStyle Libre 3,扫描错误", "重试"},
.libre3wrongID={"错误,帐户 ID 错误？","在设置->Libreview 中指定用于激活传感器的同一帐户"},
.libre3scansuccess= {"FreeStyle Libre 3 传感器", "Juggluco 现在将接收血糖值"},
.unknownNFC={"无法识别的 NFC 扫描错误", "重试"},
.nolibre3={"FreeStyle Libre 3 传感器","此版本的 Juggluco 不支持"},
#ifndef WEAROS
	.advancedstart= R"(<h1>修改设备</h1>
<p>此应用程序使用的库之一有一个 BUG,使得
如果检测到某些文件,它就会崩溃。您的设备包含一些
这些文件。这个程序包含一个 hack 来规避这个 BUG,但是
最好让这些文件在其他地方无法检测到
方式。例如,Magisk 可以选择隐藏某些根目录
应用程序（Magiskhide 或 Denylist）并更改其自己的名称,两者
需要。在您的情况下,以下文件有问题)",
	.add_s=true,
.shortinit=zhshortinit,
.labels=zhlabels
#endif
		}


		;

extern void setusezh(); 
void setusezh() {
	usedtext= &zhtext;
	}

