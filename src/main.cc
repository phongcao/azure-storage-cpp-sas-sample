#include <cstring>
#include <string>
#include <locale>
#include <codecvt>
#include <iostream>
#include <fstream>
#include <thread>
#include <was/storage_account.h>
#include <was/blob.h>

const std::string TEST_CONTAINER_NAME = "testcontainer";
std::string STORAGE_NAME("");
std::string STORAGE_CONNECTION_STRING("");
utility::string_t sas_container_token;
std::shared_ptr<azure::storage::cloud_blob_container> test_container;

inline void line() {
    std::cout<<"-------------------------------------------\n";
}

inline utility::string_t to_string_t(const std::string& str) {
#ifdef _WIN32
    // Windows uses wstring
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wstr = converter.from_bytes(str);
    return wstr;
#else
    return str;
#endif
}

void prepare_test_data() {
    // Create test container
    auto storage_account = azure::storage::cloud_storage_account::parse(to_string_t(STORAGE_CONNECTION_STRING));
    auto blob_client = storage_account.create_cloud_blob_client();
    test_container = std::make_shared<azure::storage::cloud_blob_container>(blob_client.get_container_reference(to_string_t(TEST_CONTAINER_NAME)));
    test_container->create_if_not_exists();

    // Get shared access signature token
    azure::storage::blob_shared_access_policy policy;
    policy.set_permissions(
        azure::storage::blob_shared_access_policy::permissions::add |
        azure::storage::blob_shared_access_policy::permissions::create |
        azure::storage::blob_shared_access_policy::permissions::del |
        azure::storage::blob_shared_access_policy::permissions::list |
        azure::storage::blob_shared_access_policy::permissions::read |
        azure::storage::blob_shared_access_policy::permissions::write);

    policy.set_start(utility::datetime::utc_now());
    policy.set_expiry(utility::datetime::utc_now() + utility::datetime::from_hours(1));

    sas_container_token = test_container->get_shared_access_signature(policy);
}

void cleanup_test_data() {
    if (test_container) {
        test_container->delete_container_if_exists();
    }
}

int main(int argc, char* argv[]) {

    std::cout << "Hello World!!\n";
    return 0;
}
