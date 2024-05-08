#include <iostream>
using namespace std;

// 在settings.json中添加如下内容,
// "cmake.debugConfig": {
//     "args": [
//         "1",
//         "word",
//         "234"
//     ]
// },
#include <chrono>
#include <ctime>
#include <iomanip>
#include <string>

time_t date_add(time_t t, std::chrono::hours h)
{
    auto ori = std::chrono::system_clock::from_time_t(t);
    auto dst = ori + h;

    time_t dst_time_t = std::chrono::system_clock::to_time_t(dst);
    return dst_time_t;
}

string str_time_t(time_t t)
{
    auto t_tm = std::localtime(&t);
    return std::to_string(t_tm->tm_year + 1900) + "-" + std::to_string(t_tm->tm_mon + 1) + "-" + std::to_string(t_tm->tm_mday);
}

int date_diff_day(time_t start, time_t end)
{
    double diff = difftime( end, start);
    return static_cast<int>((diff / (3600 * 24)));
}

int date_diff_hour(time_t start, time_t end)
{
    double diff = difftime( end, start);
    return static_cast<int>((diff / (3600)));
}

int main(int argc, char* argv[])
{
    tm d1_tm = {0};
    d1_tm.tm_year = 2024 - 1900;
    d1_tm.tm_mon = 4-1;
    d1_tm.tm_mday = 8;
    auto d1_time_t = std::mktime(&d1_tm);

    tm d2_tm = {0};
    d2_tm.tm_year = 2024 - 1900;
    d2_tm.tm_mon = 5-1;
    d2_tm.tm_mday = 16;
    auto d2_time_t = std::mktime(&d2_tm);

    cout<<date_diff_day(d1_time_t, d2_time_t)<<endl;
    cout<<date_diff_hour(d1_time_t, d2_time_t)<<endl;

    auto d3 = date_add(d1_time_t, std::chrono::hours(47));
    cout<<str_time_t(d3)<<endl;

    return 1;

    // 获取当前时间点
    auto now = std::chrono::system_clock::now();
    // 创建一个表示天数的duration对象
    auto days = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::seconds(86400)); // 86400秒等于一天
    auto ont_day_sec = std::chrono::hours(24);
    // 计算未来或过去的日期
    auto new_date = now + /*days*/ ont_day_sec * 10; // 假设我们要加10天


    // 将新的时间点转换为time_t类型
    time_t current_date_time_t = std::chrono::system_clock::to_time_t(now);
    time_t new_date_time_t = std::chrono::system_clock::to_time_t(new_date);
    // 转换为localtime
    std::tm* cur_date_tm = std::localtime(&current_date_time_t);
    std::tm* new_date_tm = std::localtime(&new_date_time_t);

    // 输出新的日期
    std::cout << "Current date: " << std::put_time(cur_date_tm, "%F") << std::endl; // %F是完整日期格式（YYYY-MM-DD）
    std::cout << "New date in 10 days: " << std::put_time(new_date_tm, "%F") << std::endl;
    return 0;

    cout<<"argc: "<<argc<<endl;
    for(int i = 0; i< argc; i++){
        cout<<"  argv["<<i<<"]: "<<argv[i]<<endl;
    }
    system("pause");
}