#pragma once
#include "json.hpp"
#include <curl/curl.h>
#include <string>
#include <iostream>
#include <codecvt>
#include <boost/locale.hpp>




using json = nlohmann::json;

bool check_error_code(long code)
{
    if (code == 200)return true;

    switch (code)
    {
    case 500:
        std::cout << "500 : 서버 오류\n";
        break;

    case 403:
        std::cout << "403 : 권한 없음\n";
        break;

    case 400:
        std::cout << "400 : Bad Request\n";
        break;

    case 429:
        std::cout << "429 : 호출량 과다\n";
        break;

    case 503:
        std::cout << "503 : API 점검 중";
        break;

    default:
        std::cout << code << " : 식별되지 않은 오류코드. 확인 필요\n";
        break;
    }
    return false;
}

std::string ConvertToUTF8(const std::string& str)
{
    // 기존 문자열(ANSI) → Wide Char 변환
    int wide_size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wide_str(wide_size, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wide_str[0], wide_size);

    // Wide Char → UTF-8 변환
    int utf8_size = WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8_str(utf8_size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wide_str.c_str(), -1, &utf8_str[0], utf8_size, nullptr, nullptr);

    return utf8_str;
}

std::string toStr(char* chr)
{
    if (!chr)return std::string();
    if (strlen(chr) >= 3 && strcmp(chr + strlen(chr) - 3, "%00") == 0)
    {
        chr[strlen(chr) - 3] = '\0';
    }
    return std::string(chr, strlen(chr));

}


static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* out)
{
    size_t totalSize = size * nmemb;
    out->append((char*)contents, totalSize);

    //std::cout << "사이즈 확인 : " << size << " " << nmemb << " " << totalSize << " 끝\n\n";
    return totalSize;
}


std::string jsontype(json::value_t type) {
    switch (type) {
    case json::value_t::null: return "null";
    case json::value_t::object: return "object";
    case json::value_t::array: return "array";
    case json::value_t::string: return "string";
    case json::value_t::boolean: return "boolean";
    case json::value_t::number_integer: return "integer";
    case json::value_t::number_unsigned: return "unsigned integer";
    case json::value_t::number_float: return "float";
    case json::value_t::binary: return "binary";
    case json::value_t::discarded: return "discarded";
    default: return "unknown";
    }
}


std::string Utf8ToEuckr(const std::string& euc_str)
{
    // Convert from EUC-KR to UTF-8 using boost::locale::conv::between
    return boost::locale::conv::between(euc_str, "EUC-KR", "UTF-8");
}

class API
{
private:
	std::string key;
	bool init = false;//초기화 여부 확인

    CURL* curl;
    CURLcode res;
    std::string response;
    struct curl_slist* headers = nullptr;

public:
    API()
    {
        if (!init)
        {
            init = true;
            initialize();
        }
    }


    void initialize()
    {
        std::cout << "key 초기화 : ";
        std::cin >> key;

        curl = curl_easy_init();
        headers = curl_slist_append(headers, ("x-nxopen-api-key: " + key).c_str());
        //headers = curl_slist_append(headers, "accept: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    }


    std::string getOCID()//캐릭터 ocid를 얻어온다.
    {
        std::string ocid = "Default";

        std::string name;
        std::cout << "닉네임 입력. 공백없음. 대소문자 구분 : ";
        std::cin >> name;

        name = ConvertToUTF8(name);
        char* encoded_name = curl_easy_escape(curl, name.c_str(), name.length());

        std::string url = "https://open.api.nexon.com/maplestory/v1/id?character_name=" + toStr(encoded_name);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        //요청 실행
        response.clear();
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::cout << "getOCID 에러\n";
            std::cout << "url : " << url << "\n";
            return ocid;
        }

        //응답코드 확인
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (check_error_code(response_code))//true였을 경우
        {
            std::cout << "ocid 응답 성공\n";
            try
            {
                    
                char* contype = nullptr;
                curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &contype);
                json responseData = json::parse(response);
                ocid = responseData["ocid"].get<std::string>();
            }
            catch (const json::exception& e)
            {
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                std::cout << "ocid 파싱 실패\n";
                std::cout << url << "\n";
            }
        }
        else
        {
            std::cout << "에러\n";
            std::cout << "url : " << url << "\n";
            return ocid;
        }

        return ocid;
    }

    std::vector<std::string> getBasic(std::string id)//id에 따른 기본 정보 받아오기
    {

        std::vector<std::string> basic_info;
        //캐릭터명, 월드명, 성별, 직업, 전직 몇 차, 레벨, 길드


        std::string url = "https://open.api.nexon.com/maplestory/v1/character/basic?ocid=" + id;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        //요청 실행
        response.clear();
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::cout << "getOCID 에러\n";
            std::cout << "url : " << url << "\n";
            return basic_info;
        }

        //응답코드 확인
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);



        if (check_error_code(response_code))//true였을 경우
        {
            try
            {
                //response = Utf8ToEuckr(response);//디코딩
                json responseData = json::parse(response);
                std::cout << "응답 json 확인\n";//여기까진 성공


                basic_info.push_back(Utf8ToEuckr(responseData["character_name"].get<std::string>()));
                std::cout << "캐릭터명 확인\n";
                basic_info.push_back(Utf8ToEuckr(responseData["world_name"].get<std::string>()));
                std::cout << "월드명 확인\n";
                basic_info.push_back(Utf8ToEuckr(responseData["character_gender"].get<std::string>()));
                std::cout << "성별 확인\n";
                basic_info.push_back(Utf8ToEuckr(responseData["character_class"].get<std::string>()));
                std::cout << "직업 확인\n";
                basic_info.push_back(Utf8ToEuckr(responseData["character_class_level"].get<std::string>()));
                std::cout << "전직 차수 확인\n";
                basic_info.push_back(Utf8ToEuckr(std::to_string(responseData["character_level"].get<int>())));
                std::cout << "레벨 확인\n";
                basic_info.push_back(Utf8ToEuckr(responseData["character_guild_name"].get<std::string>()));
                std::cout << "길드명 확인\n";
            }
            catch (const json::exception& e)
            {
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                std::cout << "기본 정보 파싱 실패\n";
                std::cout << "raw : " << response << '\n\n';
            }
        }

        return basic_info;
    }

};