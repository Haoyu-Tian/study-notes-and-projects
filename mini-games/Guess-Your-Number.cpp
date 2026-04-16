/*
一个基于 C++ 实现的数字读心术魔术小游戏，通过经典数学原理 + 控制台交互，精准猜出你心中的最终数字，趣味十足！
*/

#include <iostream>
#include <windows.h>
#include <cstdlib>
#include <ctime>
using namespace std;

// 全局变量：初始值9
int num = 9;

// 设置控制台文字颜色
void setColor(int c) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

// 生成 1~100 随机数
int getRandom100() {
    return 1 + rand() % 100;
}

// 生成 1~3 随机数（用于选择 加/减/乘）
int getRandomOp() {
    return 1 + rand() % 3;
}

// 倒计时函数
void countdown(int seconds) {
    for (int i = seconds; i > 0; --i) {
        setColor(15);
        cout << "\r还有" << i << " 秒开始游戏" << flush;
        Sleep(1000);
    }
    cout << endl;
}

// 加法运算（原版逻辑不变）
int add(int b) {
    system("cls");
    setColor(4);
    cout << "加 " << b << endl;
    return num += b;
}

// 减法运算
int sub(int b) {
    system("cls");
    setColor(4);
    cout << "减 " << b << endl;
    return num -= b;
}

// 乘法运算
int mul(int b) {
    system("cls");
    setColor(4);
    cout << "乘 " << b << endl;
    return num *= b;
}

// 随机执行一次 加/减/乘
void randomOperate() {
    int op = getRandomOp();
    int val = getRandom100();

    switch (op) {
    case 1: add(val);    break;
    case 2: sub(val);    break;
    case 3: mul(val);    break;
    }
}

int main() {
    // 随机数种子只初始化一次
    srand((unsigned int)time(NULL));

    // ==================== 欢迎界面 ====================
    setColor(4);
    cout << "请记下一个 0 -- 100 的整数" << endl;
    cout << "请按照以下提示进行计算，并记录所得到的新数据\n" << endl;
    cout << "建议使用计算器进行计算" << endl;
    cout << "\n每次均需得到新数值后再进行下一步\n" << endl;

    // 倒计时5秒
    countdown(5);

    // ==================== 前3轮迷惑操作（只显示，不修改num） ====================
    int count = 1;
    int choice = 1;
    while (count <= 3 && choice == 1) {
        int op = getRandomOp();
        int val = getRandom100();

        system("cls");
        setColor(4);
        switch (op) {
        case 1: cout << "加 " << val << endl; break;
        case 2: cout << "减 " << val << endl; break;
        case 3: cout << "乘 " << val << endl; break;
        }

        setColor(7);
        cout << "\n计算完毕请按‘1’" << endl;
        cin >> choice;
        count++;
    }

    // ==================== 固定提示：乘 9 ====================
    system("cls");
    setColor(4);
    cout << "乘 9" << endl;
    setColor(7);
    cout << "\n计算完毕请按‘1’" << endl;
    cin >> choice;

    // ==================== 数字各位相加提示 ====================
    system("cls");
    setColor(4);
    cout << "将每个位置上的数字依次 相加求和" << endl;
    cout << "循环上述操作，直到得到一个个位数字" << endl;
    setColor(7);
    cout << "\n计算完毕请按‘1’" << endl;
    cin >> choice;

    // ==================== 后3轮真实随机运算 ====================
    count = 1;
    while (count <= 3 && choice == 1) {
        // 随机 加/减/乘，并真正修改全局变量 num
        randomOperate();

        setColor(7);
        cout << "\n计算完毕请按‘1’" << endl;
        cin >> choice;
        count++;
    }

    // ==================== 输出最终结果 ====================
    system("cls");
    setColor(4);
    cout << "最终这个数字是 " << num << endl;
    setColor(7);

    return 0;
}
