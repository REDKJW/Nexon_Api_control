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
        std::cout << "500 : ���� ����\n";
        break;

    case 403:
        std::cout << "403 : ���� ����\n";
        break;

    case 400:
        std::cout << "400 : Bad Request\n";
        break;

    case 429:
        std::cout << "429 : ȣ�ⷮ ����\n";
        break;

    case 503:
        std::cout << "503 : API ���� ��";
        break;

    default:
        std::cout << code << " : �ĺ����� ���� �����ڵ�. Ȯ�� �ʿ�\n";
        break;
    }
    return false;
}

std::string ConvertToUTF8(const std::string& str)
{
    // ���� ���ڿ�(ANSI) �� Wide Char ��ȯ
    int wide_size = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wide_str(wide_size, 0);
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, &wide_str[0], wide_size);

    // Wide Char �� UTF-8 ��ȯ
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

    //std::cout << "������ Ȯ�� : " << size << " " << nmemb << " " << totalSize << " ��\n\n";
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
	bool init = false;//�ʱ�ȭ ���� Ȯ��

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
        std::cout << "key �ʱ�ȭ : ";
        std::cin >> key;

        curl = curl_easy_init();
        headers = curl_slist_append(headers, ("x-nxopen-api-key: " + key).c_str());
        //headers = curl_slist_append(headers, "accept: application/json");

        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    }


    std::string getOCID()//ĳ���� ocid�� ���´�.
    {
        std::string ocid = "Default";

        std::string name;
        std::cout << "�г��� �Է�. �������. ��ҹ��� ���� : ";
        std::cin >> name;

        name = ConvertToUTF8(name);
        char* encoded_name = curl_easy_escape(curl, name.c_str(), name.length());

        std::string url = "https://open.api.nexon.com/maplestory/v1/id?character_name=" + toStr(encoded_name);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        //��û ����
        response.clear();
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::cout << "getOCID ����\n";
            std::cout << "url : " << url << "\n";
            return ocid;
        }

        //�����ڵ� Ȯ��
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        
        if (check_error_code(response_code))//true���� ���
        {
            std::cout << "ocid ���� ����\n";
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
                std::cout << "ocid �Ľ� ����\n";
                std::cout << url << "\n";
            }
        }
        else
        {
            std::cout << "����\n";
            std::cout << "url : " << url << "\n";
            return ocid;
        }

        return ocid;
    }

    std::vector<std::string> getBasic(std::string id)//id�� ���� �⺻ ���� �޾ƿ���
    {

        std::vector<std::string> basic_info;
        //ĳ���͸�, �����, ����, ����, ���� �� ��, ����, ���


        std::string url = "https://open.api.nexon.com/maplestory/v1/character/basic?ocid=" + id;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        //��û ����
        response.clear();
        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            std::cout << "getOCID ����\n";
            std::cout << "url : " << url << "\n";
            return basic_info;
        }

        //�����ڵ� Ȯ��
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);



        if (check_error_code(response_code))//true���� ���
        {
            try
            {
                //response = Utf8ToEuckr(response);//���ڵ�
                json responseData = json::parse(response);
                std::cout << "���� json Ȯ��\n";//������� ����


                basic_info.push_back(Utf8ToEuckr(responseData["character_name"].get<std::string>()));
                std::cout << "ĳ���͸� Ȯ��\n";
                basic_info.push_back(Utf8ToEuckr(responseData["world_name"].get<std::string>()));
                std::cout << "����� Ȯ��\n";
                basic_info.push_back(Utf8ToEuckr(responseData["character_gender"].get<std::string>()));
                std::cout << "���� Ȯ��\n";
                basic_info.push_back(Utf8ToEuckr(responseData["character_class"].get<std::string>()));
                std::cout << "���� Ȯ��\n";
                basic_info.push_back(Utf8ToEuckr(responseData["character_class_level"].get<std::string>()));
                std::cout << "���� ���� Ȯ��\n";
                basic_info.push_back(Utf8ToEuckr(std::to_string(responseData["character_level"].get<int>())));
                std::cout << "���� Ȯ��\n";
                basic_info.push_back(Utf8ToEuckr(responseData["character_guild_name"].get<std::string>()));
                std::cout << "���� Ȯ��\n";
            }
            catch (const json::exception& e)
            {
                curl_slist_free_all(headers);
                curl_easy_cleanup(curl);
                std::cout << "�⺻ ���� �Ľ� ����\n";
                std::cout << "raw : " << response << '\n\n';
            }
        }

        return basic_info;
    }

};