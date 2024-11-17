// Nexon_Api.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "NEXON_API.h"

int main()
{
    API api;

    std::string ocid = api.getOCID();
    if (ocid.compare("Default") == 0)
    {
        std::cout << "ocid 얻어오기 실패하였습니다.\n";
        return 0;
    }
    else std::cout << "ocid를 성공적으로 얻어왔습니다.\n";

    std::vector<std::string> basic_info = api.getBasic(ocid);

    for (auto& i : basic_info)
    {
        std::cout << i << "\n";
    }

    system("pause");
}


