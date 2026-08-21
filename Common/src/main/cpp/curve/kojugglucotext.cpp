#include "jugglucotext.hpp"
#ifdef INJUGGLUCO
#ifndef WEAROS
constexpr static std::string_view kolabels[]={"탄수",
"포도당",
"속효성",
"지속형",
"자전거",
"걷기",
"혈당"};
constexpr static Shortcut_t koshortinit[]= { {"빵",
        .48},
        {"건포빵1",
        .56f},
        {"건포빵2",
        .595f},
        {"포도",
        .165f},
        {"과일요",
        .058f},
        {"쌀밥",
        .75f},
        {"마카로",
        .65f},
        {"토마토",
        .03f},
        {"멕시칸",
        .078f},
        {"오렌지",
        .109f},
        {"스포츠",
        .873f},
        {"당근믹",
        .07f},
        {"버섯믹",
        .07300000f}};
#endif
#endif //INJUGGLUCO

jugglucotext kotext {
    .daylabel={"일", "월", "화", "수", "목", "금", "토"},
#ifdef INJUGGLUCO
    .speakdaylabel={"일요일","월요일","화요일","수요일","목요일","금요일","토요일"},
    .monthlabel={"1월","2월","3월","4월","5월","6월","7월","8월","9월","10월","11월","12월"},

    .scanned="스캔됨",
    .readysecEnable="센서가 %d분 후 준비됩니다. 스트리밍을 사용하려면 다시 스캔하십시오.",
    .readysec="센서가 %d분 후 준비됩니다.",
    .networkproblem="미러에서 혈당이 오지 않음",
    .enablebluetooth="Bluetooth 켜기",
    .useBluetoothOff="'Bluetooth 사용' 꺼짐",
    .noconnectionerror=": 연결 없음",
    .stsensorerror=": 센서 오류",
    .streplacesensor=": 센서를 교체할까요?",
    .endedformat="%s이(가) 작동을 멈췄습니다. state=%d",
    .notreadyformat="%s이(가) 준비되지 않았습니다. state=%d",
#ifndef WEAROS
    .median="중앙값",
    .middle="중간",
#endif //WEAROS
    .history="기록",
#ifndef WEAROS
    .sensorstarted= "시작:\t\t\t\t\t\t",
#else
    .sensorstarted= "시작:",
#endif
    .lastscanned="마지막 스캔:",
#ifndef WEAROS
    .laststream="마지막 스트림:\t\t",
#else
    .laststream="마지막 스트림:",
#endif
    .sensorends="공식 종료: ",
    .sensorexpectedend="예상 종료: ",
#endif //INJUGGLUCO
#ifndef WEAROS

#ifdef INJUGGLUCO
    .newamount="새 입력값",
    .averageglucose="평균 혈당: ",
    .duration="기간: %.1f일",
    .timeactive="전체 시간의 %.1f%% 활성",
    .nrmeasurement="측정 횟수: %d",
    .EstimatedA1C="추정 A1C: %.1f%% (%d mmol/mol)",
    .GMI="혈당 관리 지표(GMI): %.1f%% (%d mmol/mol)",
    .SD="SD: %.2f",
    .glucose_variability="혈당 변동성: %.1f%%",
    .menustr0={
        "시스템 UI",
        "메뉴",
        "워치",
        "센서",
        "설정",
#if defined(SIBIONICS)
        "사진",
#else
        "정보",
#endif
        "닫기",
        "알람 중지"
        },
    .menustr1={
        "내보내기",
        "미러",
        kotext.newamount,
        "목록",
#else
       .statistics=
#endif //INJUGGLUCO
        "통계",
#ifdef INJUGGLUCO
        "말하기",
        "플로팅"
        },
    .menustr2= {"마지막 스캔",
        "스캔",
        "스트림",
        "기록",
        "입력값",
        "식사",
        "다크 모드"},
    .menustr3= {hourminstr,
        "검색",
        "날짜",
        "하루 전",
        "하루 후",
        "일주일 전",
        "일주일 후"},
#endif //INJUGGLUCO
#else
    .amount="입력값",
    .menustr0= {
        "미러",
        "센서",
        "표시",
        "설정",
        "알람 중지" },
    .menustr2= {"날짜",
        hourminstr,
        "하루 전",
        kotext.amount},
#endif //WEAROS
#ifdef INJUGGLUCO
    .scanerrors={
        {"스캔 오류 (%d)", "다시 시도하십시오"},
        {"설치 오류", "?"},
        {"데이터 처리 오류", "다시 시도하십시오"},
        {"센서 활성화 중", ""},
        {"센서가 완전히 종료됨", ""},
        {"센서 준비까지", "%d분"},
        {"센서 오류 (373)", "나중에 다시 시도하십시오"},
        {"새 센서 초기화됨", "사용하려면 다시 스캔하십시오"},
        {"", "스캔 중 터치를 차단합니다"},
        {"", ""},
        {"라이브러리 초기화 오류", "공유 라이브러리가 없습니까?"},
        {"클래스 초기화 오류", "조치가 필요합니다"},
        {"처리 시간이 너무 깁니다", "프로그램을 종료합니다"},
        {"센서 교체 (365)", "센서가 작동하지 않습니다. 센서를 제거하고 새 센서를 시작하십시오."},
        {"센서 교체 (368)", "센서가 작동하지 않습니다. 센서를 제거하고 새 센서를 시작하십시오."},
        {"", ""},
        {"스캔 오류", "다시 시도하십시오"}},

    .libre3scanerror={"FreeStyle Libre 3, 스캔 오류",
        "다시 시도하십시오"},
    .libre3wrongID={"오류, 계정 ID가 잘못되었습니까?",
        R"(센서를 활성화할 때 사용한 것과 같은 ID여야 합니다. LibreView에서 가져오려면 왼쪽 메뉴→설정→데이터 교환→LibreView→"계정 ID 가져오기"로 이동하십시오.)"},
    .libre3scansuccess= {"FreeStyle Libre 3 센서",
        "이제 Juggluco가 혈당값을 수신합니다"},
    .unknownNFC={"인식할 수 없는 NFC 스캔 오류",
        "다시 시도하십시오"},
    .nolibre3={"FreeStyle Libre 3 센서",
        "https://www.juggluco.nl/download.html 에서 올바른 버전을 받으십시오"},
    .libre3zeroID={"오류, 계정 ID가 0입니까?",
        R"(0이 아닌 계정 ID를 설정하려면 왼쪽 메뉴→설정→데이터 교환→LibreView→"계정 ID 가져오기"를 사용하십시오.)"},
    .needsandroid8="최소 Android 8 필요"sv,
#ifndef WEAROS
    .advancedstart= R"(<h1>수정된 기기</h1>
<p>이 애플리케이션이 사용하는 라이브러리 중 하나에는 특정 파일을 발견하면 충돌하는 버그가 있습니다. 기기에 이러한 파일 중 일부가 있습니다. 이 프로그램에는 이 버그를 우회하는 방법이 포함되어 있지만, 가능하면 다른 방법으로 이 파일들이 감지되지 않게 하는 편이 좋습니다. 예를 들어 Magisk에는 특정 애플리케이션에서 루트를 숨기는 옵션(MagiskHide 또는 DenyList)과 자체 이름을 변경하는 기능이 있으며 둘 다 필요합니다. 현재 문제가 되는 파일은 다음과 같습니다.)",
    .add_s=true,
    .shortinit=koshortinit,
    .labels=kolabels,
#endif
#ifndef DONTTALK
    .checked="선택됨",
    .unchecked="선택 안 됨",
    .Undetermined=""sv,
    .FallingQuickly="빠르게 하강"sv,
    .Falling="하강"sv,
    .Stable="천천히 변화"sv,
    .Rising="상승"sv,
    .RisingQuickly="빠르게 상승"sv,
#endif
    .receivingpastvalues="이전 값 수신 중"sv,
    .receivingdata="데이터 수신 중"sv,
    .unsupportedSibionics="지원되지 않는 Sibionics 센서"sv,
    .waitingforconnection="연결 대기 중"sv,
    .deleted="삭제됨"sv,
    .nolocationpermission="위치 권한 필요"sv,
    .nonearbydevicespermission="주변 기기 권한 필요"sv,
#endif //INJUGGLUCO
    .summarygraph="요약 그래프"sv,
    .logdays="일별 로그"sv,
    .unhide="표시"sv
    };

addlang(ko);
