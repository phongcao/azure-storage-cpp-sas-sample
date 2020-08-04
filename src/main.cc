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
    std::cout << "-------------------------------------------" << std::endl;
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

void prepare_test_container() {
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

void cleanup_test_container() {
    if (test_container) {
        test_container->delete_container_if_exists();
    }
}

std::shared_ptr<azure::storage::cloud_blob_client> create_cloud_blob_client(const utility::string_t& account_name, const utility::string_t& sas_token) {
    azure::storage::storage_credentials::sas_credential sas_cred(sas_token);
    azure::storage::storage_credentials storage_cred(account_name, sas_cred);
    auto storage_account = azure::storage::cloud_storage_account(storage_cred, true);
    return std::make_shared<azure::storage::cloud_blob_client>(storage_account.create_cloud_blob_client());
}

bool exists(azure::storage::cloud_blob_client* blob_client, const utility::string_t& container_name, const utility::string_t& blob_name) {
    auto container_ref = blob_client->get_container_reference(container_name);
    auto blob_ref = container_ref.get_block_blob_reference(blob_name);
    return blob_ref.exists();
}

void upload_from_stream(azure::storage::cloud_blob_client* blob_client, const utility::string_t& container_name, const utility::string_t& blob_name, const void *data, size_t len) {
    auto container_ref = blob_client->get_container_reference(container_name);
    auto blob_ref = container_ref.get_block_blob_reference(blob_name);
    azure::storage::blob_request_options blob_request_options;
    blob_request_options.set_use_transactional_crc64(true);
    std::vector<char> buffer((char*)data, (char*)data + len);
    auto buffer_bs = concurrency::streams::bytestream::open_istream(buffer);
    blob_ref.upload_from_stream(buffer_bs, azure::storage::access_condition(), blob_request_options, azure::storage::operation_context());
    buffer_bs.close().wait();
}

void download_range_to_stream(azure::storage::cloud_blob_client* blob_client, const utility::string_t& container_name, const utility::string_t& blob_name, void* data, utility::size64_t offset, utility::size64_t length) {
    auto container_ref = blob_client->get_container_reference(container_name);
    auto blob_ref = container_ref.get_block_blob_reference(blob_name);
    if (!blob_ref.exists()) {
        throw std::runtime_error("Blob does not exist");
    }

    concurrency::streams::container_buffer<std::vector<char>> buffer_container;
    concurrency::streams::ostream buffer_os(buffer_container);
    blob_ref.download_range_to_stream(buffer_os, offset, length);
    auto buffer = buffer_container.collection();
    memcpy(data, buffer.data(), buffer.size());
    buffer_os.close().wait();
}

long long get_blob_size(azure::storage::cloud_blob_client* blob_client, const utility::string_t& container_name, const utility::string_t& blob_name) {
    auto container_ref = blob_client->get_container_reference(container_name);
    auto blob_ref = container_ref.get_block_blob_reference(blob_name);
    if (blob_ref.exists()) {
        auto props = blob_ref.properties();
        return props.size();
    } else {
        throw std::runtime_error("Blob does not exist");
    }
}

void delete_blob(azure::storage::cloud_blob_client* blob_client, const utility::string_t& container_name, const utility::string_t& blob_name) {
    auto container_ref = blob_client->get_container_reference(container_name);
    auto blob_ref = container_ref.get_block_blob_reference(blob_name);
    if (!blob_ref.exists()) {
        throw std::runtime_error("Blob does not exist");
    }

    return blob_ref.delete_blob();
}

void list_blobs_segmented(azure::storage::cloud_blob_client* blob_client, const utility::string_t& container_name, const utility::string_t& prefix, std::string* nextPageToken, std::vector<std::string>* result) {
    const int max_results = 100;
    azure::storage::continuation_token token;

    if (!nextPageToken->empty()) {
        token.set_next_marker(to_string_t(*nextPageToken));
    }

    blob_client->set_directory_delimiter(_XPLATSTR("/"));
    auto container_ref = blob_client->get_container_reference(container_name);
    auto segment = container_ref.list_blobs_segmented(prefix, false, azure::storage::blob_listing_details::none, max_results, token, azure::storage::blob_request_options(), azure::storage::operation_context());
    std::string container_name_str(container_name.begin(), container_name.end());
    for (auto it = segment.results().cbegin(); it != segment.results().cend(); ++it)
    {
        if (it->is_blob())
        {
            auto blob = it->as_blob();
            std::string name(blob.name().begin(), blob.name().end());
            result->emplace_back(name);
        }
    }

    token = segment.continuation_token();
    auto marker = token.next_marker();
    *nextPageToken = std::string(marker.begin(), marker.end());
}

int main(int argc, char* argv[]) {

    // Load environment variables
    const char* storage_name = getenv("STORAGE_NAME");
    const char* storage_connection_string = getenv("STORAGE_CONNECTION_STRING");
    if (storage_name == nullptr || storage_connection_string == nullptr) {
        line();
        std::cout << "- Missing environment variables: " << std::endl;
        std::cout << "- STORAGE_NAME" << std::endl;
        std::cout << "- STORAGE_CONNECTION_STRING" << std::endl;
        line();
        exit(1);
    }

    std::cout << "Start testing..." << std::endl;

    STORAGE_NAME = storage_name;
    STORAGE_CONNECTION_STRING = storage_connection_string;
    prepare_test_container();

    // Sample data
    const int data_size = 10;
    const uint16_t nblob = 10;
    const std::string data = "0123456789";
    std::vector<std::string> blobs(10, "");
    for (auto i = 0; i < nblob; i++) {
        blobs[i] = std::to_string(i);
    }

    // Init blob client
    auto blob_client = create_cloud_blob_client(to_string_t(STORAGE_NAME), sas_container_token);

    // Upload
    for (auto &blob: blobs) {
        std::cout << "Uploading blob: " << blob << std::endl;
        upload_from_stream(blob_client.get(), to_string_t(TEST_CONTAINER_NAME), to_string_t(blob), data.c_str(), data.length());
    }

    line();

    // Exists
    for (auto &blob: blobs) {
        std::cout << "Exist " << blob << ": " << exists(blob_client.get(), to_string_t(TEST_CONTAINER_NAME), to_string_t(blob)) << std::endl;
    }
    std::cout << "Exist a" << ": " << exists(blob_client.get(), to_string_t(TEST_CONTAINER_NAME), _XPLATSTR("a")) << std::endl;

    line();

    // Size
    for (auto &blob: blobs) {
        std::cout << "Size " << blob << ": " << get_blob_size(blob_client.get(), to_string_t(TEST_CONTAINER_NAME), to_string_t(blob)) << std::endl;
    }

    line();

    // Download
    for (auto &blob: blobs) {
        char* buffer = new char[data_size];
        std::cout << "Downloading blob: " << blob << " -> ";
        download_range_to_stream(blob_client.get(), to_string_t(TEST_CONTAINER_NAME), to_string_t(blob), buffer, 0, data_size);
        std::cout << std::string(buffer, data_size) << std::endl;
        delete []buffer;
    }

    line();

    // List
    std::vector<std::string> results;
    std::string nextPageToken;
    list_blobs_segmented(blob_client.get(), to_string_t(TEST_CONTAINER_NAME), _XPLATSTR(""), &nextPageToken, &results);
    std::cout << "Listing blobs: " << std::endl;
    for (auto &result: results) {
        std::cout << result << std::endl;
    }

    line();

    // Delete
    for (auto &blob: blobs) {
        std::cout << "Delete blob " << blob << std::endl;
        delete_blob(blob_client.get(), to_string_t(TEST_CONTAINER_NAME), to_string_t(blob));
    }

    line();

    // List
    results.clear();
    list_blobs_segmented(blob_client.get(), to_string_t(TEST_CONTAINER_NAME), _XPLATSTR(""), &nextPageToken, &results);
    std::cout << "Listing blobs (after deleting): " << std::endl;
    for (auto &result: results) {
        std::cout << result << std::endl;
    }

    cleanup_test_container();

    std::cout << "Finish testing." << std::endl;

    return 0;
}
