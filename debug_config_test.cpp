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


int main(int argc, char* argv[])
{
    cout<<"argc: "<<argc<<endl;
    for(int i = 0; i< argc; i++){
        cout<<"  argv["<<i<<"]: "<<argv[i]<<endl;
    }
    system("pause");
}